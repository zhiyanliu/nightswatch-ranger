//
// Created by Zhi Yan Liu on 2019/9/9.
//

#include <errno.h>

#ifdef __linux__
#include <linux/limits.h>
#elif defined(__APPLE__)
#include <sys/syslimits.h>
#endif

#include <stddef.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "aws_iot_error.h"
#include "aws_iot_json_utils.h"
#include "aws_iot_log.h"

#include "apps.h"
#include "apps_store.h"
#include "aws_iot_config.h"
#include "client.h"
#include "job_dispatch.h"
#include "job_parser.h"
#include "op_destroy_app_pkg.h"


/*
 * we handle job message one by one, no concurrent process.
 * so reuse follow single parser, token vector and token counter to save resource.
 */
static jsmn_parser _json_parser;
static jsmntok_t _json_tok_v[MAX_JSON_TOKEN_EXPECTED];
static int32_t _token_c;


static int parse_job_doc(pjob pj, char *app_name, size_t app_name_l, bool *use_container) {
    IoT_Error_t rc = FAILURE;
    jsmntok_t *tok_app_name, *tok_use_container;

    int use_container_flag = 0;

    jsmn_init(&_json_parser);
    _token_c = jsmn_parse(&_json_parser, pj->job_doc, (int) pj->job_doc_l,
                          _json_tok_v, MAX_JSON_TOKEN_EXPECTED);
    if (_token_c < 0) {
        IOT_ERROR("failed to parse job document json: %d", _token_c);
        return 1;
    }

    tok_app_name = findToken(JOB_APP_NAME_PROPERTY_NAME, pj->job_doc, _json_tok_v);
    if (NULL == tok_app_name) {
        IOT_WARN("job application name property %s not found, nothing to do", JOB_APP_NAME_PROPERTY_NAME);
        return 2;
    }

    tok_use_container = findToken(JOB_APP_USE_CONTAINER_PROPERTY_NAME, pj->job_doc, _json_tok_v);
    if (NULL != tok_use_container) // optional flag
        use_container_flag = 1;
    else
        *use_container = 1;  // by default True

    rc = parseStringValue(app_name, app_name_l, pj->job_doc, tok_app_name);
    if (SUCCESS != rc) {
        IOT_ERROR("failed to parse job application name: %d", rc);
        return rc;
    }

    if (use_container_flag) {
        rc = parseBooleanValue((bool*)use_container, pj->job_doc, tok_use_container);
        if (SUCCESS != rc) {
            IOT_ERROR("failed to parse application if was destroyed by container flag: %d", rc);
            return rc;
        }
    }

    return 0;
}

static int step1_check_app_deployed(pjob_dispatch_param pparam, char *app_name, size_t app_name_l,
        int *launcher_type) {

    int rc = 0;
    bool use_container;

    rc = parse_job_doc(pparam->pj, app_name, app_name_l, &use_container);
    if (0 != rc) {
        IOT_ERROR("failed to parse job doc for destroy application operation: %d", rc);
        return rc;
    }

    if (use_container)
        // using runc by default
        *launcher_type = NIGHTSWATCH_RANGER_APP_LAUNCHER_TYPE_RUNC;
    else
        *launcher_type = NIGHTSWATCH_RANGER_APP_LAUNCHER_TYPE_RUND;

    rc = app_exists(app_name, *launcher_type);
    if (0 == rc) {
        IOT_ERROR("application %s was not deployed on this device yet: %d", app_name, rc);
        return rc;
    }

    return rc;
}

int op_destroy_app_pkg_entry(pjob_dispatch_param pparam) {
    char app_name[PATH_MAX + 1];
    int launcher_type = 0, rc = 0;

    rc = step1_check_app_deployed(pparam, app_name, PATH_MAX + 1, &launcher_type);
    if (1 != rc) {
        nw_dev_client_job_failed(pparam->paws_iot_client, pparam->thing_name, pparam->pj->job_id,
                "{\"detail\":\"Application was not deployed on this device yet.\"}");
        return 1;
    }

    nw_dev_client_job_wip(pparam->paws_iot_client, pparam->thing_name, pparam->pj->job_id,
            "{\"detail\":\"Destroying application process.\"}");

    rc = app_destroy(app_name, launcher_type);
    if (0 != rc) {
        nw_dev_client_job_failed(pparam->paws_iot_client, pparam->thing_name, pparam->pj->job_id,
                "{\"detail\":\"Failed to destroy application process.\"}");
        return rc;
    }

    rc = app_unregister(app_name, launcher_type);
    if (0 != rc) {
        IOT_ERROR("failed to unregister application %s (launcher-type=%d) from local store, ignored",
                  app_name, launcher_type);
    } else {
        IOT_DEBUG("application %s (launcher-type=%d) is unregistered from local store", app_name, launcher_type);
    }

    rc = nw_dev_client_job_done(pparam->paws_iot_client, pparam->thing_name, pparam->pj->job_id,
            "{\"detail\":\"Application destroyed.\"}");
    if (SUCCESS != rc) {
        IOT_ERROR("failed to set application destroy job status to SUCCEEDED, "
                  "will redo this job if it still under IN_PROGRESS status: %d", rc);
    } else {
        IOT_INFO("destroy application %s successfully, job id: %s", app_name, pparam->pj->job_id);
    }

    return rc;
}
