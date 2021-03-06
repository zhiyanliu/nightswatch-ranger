//
// Created by Zhi Yan Liu on 2019-05-20.
//

#include <errno.h>
#include <libgen.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "aws_iot_error.h"
#include "aws_iot_log.h"

#include "apps.h"
#include "apps_store.h"
#include "agent.h"
#include "client.h"
#include "certs.h"
#include "flag.h"
#include "funcs.h"
#include "job_dispatch_bootstrap.h"
#include "utils.h"
#include "s3_http.h"


typedef enum {
    CONN_SUCCESS = 0,
    CONN_FAILED = -1,
    CERTS_FALLBACK_CONN_SUCCESS = -2,
    CERTS_FALLBACK_CONN_FAILED = -3,
    CERTS_ERROR = -4
} client_connect_ret;

int to_update_dev_ca(int argc, char **argv, char **upd_dev_ca_job_id) {
    int rc = 0;
    char *flag_ca_par_name, cur_par_name[PATH_MAX + 1];

    if (NULL == upd_dev_ca_job_id)
        return 0;

    rc = flagged_update_dev_ca(argc, argv, upd_dev_ca_job_id, &flag_ca_par_name);
    if (0 == rc) { // flag not provided
        return 0;
    }

    rc = certs_cur_par_name(cur_par_name, PATH_MAX + 1);
    if (0 != rc) {
        IOT_ERROR("failed to get current certs partition name: %d", rc);
        return 0;
    }

    rc = strncmp(cur_par_name, flag_ca_par_name, PATH_MAX + 1);
    if (0 != rc) {
        IOT_WARN("invalid flag of certs partition name provided, current: %s, flag: %s, skip update device certs",
                 cur_par_name, flag_ca_par_name);
        return 0;
    }

    return 1;
}

int to_ota_agent_pkg(int argc, char **argv, char **ota_agent_pkg_job_id, char **ota_agent_pkg_ver) {
    int rc = 0;
    char *flag_agent_par_name, cur_par_name[PATH_MAX + 1];

    if (NULL == ota_agent_pkg_job_id)
        return 0;

    if (NULL == ota_agent_pkg_ver)
        return 0;

    rc = flagged_ota_agent_pkg(argc, argv, ota_agent_pkg_job_id, &flag_agent_par_name, ota_agent_pkg_ver);
    if (0 == rc) { // flag not provided
        return 0;
    }

    rc = agent_cur_par_name(cur_par_name, PATH_MAX + 1);
    if (0 != rc) {
        IOT_ERROR("failed to get current agent partition name: %d", rc);
        return 0;
    }

    rc = strncmp(cur_par_name, flag_agent_par_name, PATH_MAX + 1);
    if (0 != rc) {
        IOT_WARN("invalid flag of agent partition name provided, current: %s, flag: %s, skip update device agent",
                cur_par_name, flag_agent_par_name);
        return 0;
    }

    return 1;
}

IoT_Error_t _client_connect(pnw_dev_client *ppclient) {
    char cur_par_name[PATH_MAX + 1];
    IoT_Error_t iot_rc = FAILURE;
    int rc = 0;

    *ppclient = nw_dev_client_create();
    if (NULL == *ppclient) {
        IOT_ERROR("failed to allocate client");
        return FAILURE;
    }

    rc = certs_cur_par_name(cur_par_name, PATH_MAX + 1);
    if (0 != rc)
        return FAILURE;

    IOT_INFO("current certs partition: %s", cur_par_name);

    iot_rc = nw_dev_client_init(*ppclient, AWS_IOT_MY_THING_NAME,
            AWS_IOT_ROOT_CA_FILENAME, AWS_IOT_CERTIFICATE_FILENAME,
            AWS_IOT_PRIVATE_KEY_FILENAME, AWS_IOT_MQTT_HOST, AWS_IOT_MQTT_PORT);
    if (SUCCESS != iot_rc) {
        IOT_ERROR("failed to init client: %d", iot_rc);
        nw_dev_client_free(*ppclient);
        return iot_rc;
    }

    iot_rc = nw_dev_client_connect(*ppclient, AWS_IOT_MQTT_CLIENT_ID);
    if (SUCCESS != iot_rc) {
        IOT_ERROR("failed to connect to AWS IoT Core: %d", iot_rc);
        nw_dev_client_free(*ppclient);
        return iot_rc;
    }

    return iot_rc;
}

client_connect_ret client_connect(pnw_dev_client *ppclient, IoT_Error_t *iot_rc, int upd_dev_ca) {
    int rc = 0;

    IOT_DEBUG("connecting to AWS IoT Core: %s:%d", AWS_IOT_MQTT_HOST, AWS_IOT_MQTT_PORT);

    *iot_rc = _client_connect(ppclient);
    if (SUCCESS == *iot_rc)
        return CONN_SUCCESS;

    if (NETWORK_SSL_READ_ERROR != *iot_rc && NETWORK_X509_ROOT_CRT_PARSE_ERROR != *iot_rc &&
            NETWORK_X509_DEVICE_CRT_PARSE_ERROR != *iot_rc && NETWORK_PK_PRIVATE_KEY_PARSE_ERROR != *iot_rc) {
        IOT_ERROR("failed to connect client to the cloud");
        return CONN_FAILED;
    }

    // connection failure, suppose caused by current cert and key are invalid, switch to other one

    if (upd_dev_ca)
        IOT_WARN("failed to connect client to the cloud with new certs, fallback to old certs and reconnect");

    rc = certs_switch_par(NULL, 0, NULL, 0);
    if (0 != rc) {
        IOT_ERROR("[CRITICAL] certs partition symbolic link might be broken, will reset in next start");
        *iot_rc = FAILURE;
        return CERTS_ERROR;
    }

    IOT_INFO("certs partition is switched, reconnect");

    *iot_rc = _client_connect(ppclient);
    if (SUCCESS != *iot_rc) {
        IOT_ERROR("failed to connect client to the cloud");

        if (NETWORK_SSL_READ_ERROR != *iot_rc && NETWORK_X509_ROOT_CRT_PARSE_ERROR != *iot_rc &&
                NETWORK_X509_DEVICE_CRT_PARSE_ERROR != *iot_rc && NETWORK_PK_PRIVATE_KEY_PARSE_ERROR != *iot_rc)
            return CONN_FAILED;
        else
            return CERTS_FALLBACK_CONN_FAILED;
    }

    return CERTS_FALLBACK_CONN_SUCCESS;
}

IoT_Error_t subscribe(pnw_dev_client pclient, int upd_dev_ca, char *upd_dev_ca_job_id, int upd_dev_ca_works) {
    IoT_Error_t rc = FAILURE;
    pjob_dispatcher pdispatcher = job_dispatcher_bootstrap();

    IOT_DEBUG("subscribe job topics")

    rc = nw_dev_client_job_listen_update(pclient, pdispatcher);
    if (SUCCESS != rc) {
        IOT_ERROR("failed to subscribe job status update topics: %d", rc);
        return rc;
    }

    if (upd_dev_ca) { // update wip device ca update job status first
        if (NULL == upd_dev_ca_job_id) {
            IOT_ERROR("[BUG] invalid job id to update job status");
            return rc;
        }

        if (upd_dev_ca_works) {
            rc = nw_dev_client_job_done(&pclient->c, pclient->thing_name, upd_dev_ca_job_id,
                    "{\"detail\":\"Device connected to the client using new certs.\"}");
            if (SUCCESS != rc) {
                IOT_ERROR("failed to set update device certs job status to SUCCEEDED, "
                          "will redo this job if it still under IN_PROGRESS status: %d", rc);
            } else {
                IOT_INFO("update certs successfully, job id: %s", upd_dev_ca_job_id);
            }
        } else {
            rc = nw_dev_client_job_failed(&pclient->c, pclient->thing_name, upd_dev_ca_job_id,
                    "{\"detail\":\"Device connected to the client using old certs.\"}");
            if (SUCCESS != rc) {
                IOT_ERROR("failed to update update device certs job status to FAILED, "
                          "will redo this job if it still under IN_PROGRESS status: %d", rc);
            }

            IOT_ERROR("failed to apply new certs, job id: %s", upd_dev_ca_job_id);
        }
    }

    rc = nw_dev_client_job_listen_next(pclient, pdispatcher);
    if (SUCCESS != rc) {
        IOT_ERROR("failed to subscribe job next topics: %d", rc);
        return rc;
    }

    return rc;
}

IoT_Error_t run(pnw_dev_client pclient) {
    IoT_Error_t rc = FAILURE;

    IOT_DEBUG("ask job to process")

    do {
        rc = nw_dev_client_job_ask(pclient);
        if (MQTT_CLIENT_NOT_IDLE_ERROR == rc)
            usleep(100); // faster timeout than yield() in main thread loop
    } while (MQTT_CLIENT_NOT_IDLE_ERROR == rc);

    if (SUCCESS != rc) {
        IOT_ERROR("failed to ask job to process: %d", rc);
        return rc;
    }

    IOT_DEBUG("start mqtt message handling loop")

    rc = nw_dev_client_job_loop(pclient);
    if (SUCCESS != rc) {
        IOT_ERROR("failed to start mqtt message handling loop: %d", rc);
    }

    return rc;
}

void sig_handler(int signo)
{
    apps_kill();

    IOT_INFO("agent exists");
    exit(0);
}

int main(int argc, char **argv) {
    pnw_dev_client pclient = NULL;
    client_connect_ret conn_ret = CONN_FAILED;
    IoT_Error_t iot_rc = FAILURE;
    int rc = 0, upd_dev_ca = 0, upd_dev_ca_works = 1, ota_agent_pkg = 0;
    char *self_path, *upd_dev_ca_job_id = NULL, *ota_agent_pkg_job_id = NULL, *ota_agent_ver = NULL;

    // disable buffering for logging
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    IOT_INFO("===============(pid: %d)===============", getpid());

    rc = setuid(0) + setgid(0);
    if (0 != rc) {
        IOT_ERROR("need root permission, try sudo");
        return rc;
    }

    if (SIG_ERR == signal(SIGINT, sig_handler)) {
        IOT_ERROR("could not catch signal SIGINT");
        return errno;
    }
    if (SIG_ERR == signal(SIGTERM, sig_handler)) {
        IOT_ERROR("could not catch signal SIGTERM");
        return errno;
    }

    self_path = realpath(argv[0], 0);
    if (NULL == self_path) {
        IOT_ERROR("failed to get current process path");
        return rc;
    }

    rc = chdir(dirname(self_path));
    if (0 != rc) {
        IOT_ERROR("failed to change process working path");
        return rc;
    }

    rc = certs_init();
    if (0 != rc) {
        IOT_ERROR("failed to init certs");
        return rc;
    }

    upd_dev_ca = to_update_dev_ca(argc, argv, &upd_dev_ca_job_id);
    if (upd_dev_ca)
        IOT_DEBUG("application continues to apply new certs, job id: %s", upd_dev_ca_job_id);

    conn_ret = client_connect(&pclient, &iot_rc, upd_dev_ca);
    switch (conn_ret) {
        case CERTS_FALLBACK_CONN_FAILED:
            IOT_ERROR("[CRITICAL] failed to connect client to the cloud, both new and old certs are invalid");
        case CONN_FAILED:
        case CERTS_ERROR:
            return iot_rc;
        case CERTS_FALLBACK_CONN_SUCCESS:
            upd_dev_ca_works = 0;
        case CONN_SUCCESS:
            IOT_INFO("client connected to the cloud successfully");
    }

    ota_agent_pkg = to_ota_agent_pkg(argc, argv, &ota_agent_pkg_job_id, &ota_agent_ver);
    if (ota_agent_pkg) {
        // suppose "agent_ver" property in job document equals NIGHTSWATCH_RANGER_VER in program, maybe need a check
        IOT_DEBUG("application continues to apply new agent, version: %s, job id: %s",
                ota_agent_ver, ota_agent_pkg_job_id);

        rc = nw_dev_client_job_done(&pclient->c, pclient->thing_name, ota_agent_pkg_job_id,
                "{\"detail\":\"Device connected to the client using new agent.\"}");

        IOT_INFO("update agent to version %s successfully, job id: %s", ota_agent_ver, ota_agent_pkg_job_id);
    }

    rc = funcs_bootstrap(&pclient->c);
    if (0 != rc) {
        IOT_ERROR("failed to bootstrap functions: %d", rc);
        goto release;
    }

    s3_http_init();

    iot_rc = subscribe(pclient, upd_dev_ca, upd_dev_ca_job_id, upd_dev_ca_works);
    if (SUCCESS != iot_rc)
        IOT_ERROR("failed to subscribe job event from the cloud: %d", iot_rc);

    rc = apps_store_init();
    if (0 != rc) {
        IOT_ERROR("failed to init application store, skip to launch already deployed application(s): %d", rc);
    } else {
        // launch deployed applications
        rc = apps_traverse_records(&pclient->c, apps_launch);
        if (0 != rc)
            IOT_WARN("failed to launch %d deployed application(s), ignored", rc);
    }

    iot_rc = run(pclient);
    if (SUCCESS != iot_rc)
        IOT_ERROR("failed to run client job event loop: %d", iot_rc);

    apps_kill();

    apps_store_free();

    funcs_shutdown();

    s3_http_free();

release:
    nw_dev_client_free(pclient);

    return iot_rc;
}
