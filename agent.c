//
// Created by Zhi Yan Liu on 2019-05-20.
//

#include <stdio.h>
#include <stdlib.h>

#include "aws_iot_error.h"
#include "aws_iot_log.h"

#include "client.h"
#include "certs.h"
#include "job_dispatch_bootstrap.h"
#include "s3_http.h"


IoT_Error_t run(pdmp_dev_client pclient, pjob_dispatcher pdispatcher) {
    IoT_Error_t rc = FAILURE;

    IOT_DEBUG("subscribe job topics")

    rc = dmp_dev_client_job_listen(pclient, pdispatcher);
    if (SUCCESS != rc) {
        IOT_ERROR("failed to subscribe job topics: %d", rc);
        return rc;
    }

    IOT_DEBUG("ask job to process")

    rc = dmp_dev_client_job_ask(pclient);
    if (SUCCESS != rc) {
        IOT_ERROR("failed to ask job to process: %d", rc);
        return rc;
    }

    IOT_DEBUG("start mqtt message handling loop")

    rc = dmp_dev_client_job_loop(pclient);
    if (SUCCESS != rc) {
        IOT_ERROR("failed to start mqtt message handling loop: %d", rc);
    }

    return rc;
}

IoT_Error_t client_connect(pdmp_dev_client *ppclient) {
    char cur_par_name[PATH_MAX + 1];
    IoT_Error_t iot_rc = FAILURE;
    int rc = 0;

    *ppclient = dmp_dev_client_create();
    if (NULL == *ppclient) {
        IOT_ERROR("failed to allocate client");
        return FAILURE;
    }

    rc = certs_cur_par_name(cur_par_name, PATH_MAX + 1);
    if (0 != rc)
        return FAILURE;

    IOT_INFO("current certs partition: %s", cur_par_name);

    iot_rc = dmp_dev_client_init(*ppclient, AWS_IOT_MY_THING_NAME,
                                 AWS_IOT_ROOT_CA_FILENAME, AWS_IOT_CERTIFICATE_FILENAME,
                                 AWS_IOT_PRIVATE_KEY_FILENAME, AWS_IOT_MQTT_HOST, AWS_IOT_MQTT_PORT);
    if (SUCCESS != iot_rc) {
        IOT_ERROR("failed to init client: %d", iot_rc);
        dmp_dev_client_free(*ppclient);
        return iot_rc;
    }

    iot_rc = dmp_dev_client_connect(*ppclient, AWS_IOT_MQTT_CLIENT_ID);
    if (SUCCESS != iot_rc) {
        IOT_ERROR("failed to connect to AWS IoT Core: %d", iot_rc);
        dmp_dev_client_free(*ppclient);
        return iot_rc;
    }

    return iot_rc;
}

int main(int argc, char **argv) {
    pdmp_dev_client pclient = NULL;
    IoT_Error_t iot_rc = FAILURE;
    int rc = 0;

    rc = certs_init();
    if (0 != rc) {
        IOT_ERROR("failed to init certs, exit");
        return rc;
    }

    IOT_DEBUG("connecting to AWS IoT Core: %s:%d", AWS_IOT_MQTT_HOST, AWS_IOT_MQTT_PORT);

    iot_rc = client_connect(&pclient);
    if (SUCCESS != iot_rc) {
        if (NETWORK_SSL_READ_ERROR != iot_rc) {
            IOT_ERROR("failed to connect client to the cloud, exit");
            return 1;
        } else { // connection failure, suppose caused by current cert and key are invalid, switch to other one
            rc = certs_switch_par(NULL, 0);
            if (0 != rc) {
                IOT_ERROR("[CRITICAL] certs partition symbolic link might be broken, will reset in next start");
                return 1;
            }

            IOT_INFO("certs partition is switched, reconnect");

            iot_rc = client_connect(&pclient);
            if (SUCCESS != iot_rc) {
                IOT_ERROR("failed to connect client to the cloud, exit");
                return 1;
            }
        }
    }

    IOT_INFO("client connected to the cloud");

    s3_http_init();

    pjob_dispatcher pdispatcher = job_dispatcher_bootstrap();

    iot_rc = run(pclient, pdispatcher);
    if (SUCCESS != iot_rc)
        IOT_ERROR("failed to run client: %d", iot_rc);

    s3_http_free();

    dmp_dev_client_free(pclient);

    return iot_rc;
}
