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


static int parse_job_doc(pjob pj, char *app_name, size_t app_name_l) {
    IoT_Error_t rc = FAILURE;
    jsmntok_t *tok_app_name;

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

    rc = parseStringValue(app_name, app_name_l, pj->job_doc, tok_app_name);
    if (SUCCESS != rc) {
        IOT_ERROR("failed to parse job application name: %d", rc);
        return rc;
    }

    return 0;
}

static int step1_check_app_deployed(pjob_dispatch_param pparam, char *app_name, size_t app_name_l) {
    int rc = 0;

    rc = parse_job_doc(pparam->pj, app_name, app_name_l);
    if (0 != rc) {
        IOT_ERROR("failed to parse job doc for destroy application operation: %d", rc);
        return rc;
    }

    rc = app_exists(app_name);
    if (0 == rc) {
        IOT_ERROR("application %s was not deployed on this device yet: %d", app_name, rc);
        return rc;
    }

    return rc;
}

int op_destroy_app_pkg_entry(pjob_dispatch_param pparam) {
    char app_name[PATH_MAX + 1];
    int rc = 0;

    rc = step1_check_app_deployed(pparam, app_name, PATH_MAX + 1);
    if (1 != rc) {
        dmp_dev_client_job_failed(pparam->paws_iot_client, pparam->thing_name, pparam->pj->job_id,
                "{\"detail\":\"Application was not deployed on this device yet.\"}");
        return 1;
    }

    dmp_dev_client_job_wip(pparam->paws_iot_client, pparam->thing_name, pparam->pj->job_id,
            "{\"detail\":\"Destroying application container.\"}");

    rc = app_destroy(app_name);
    if (0 != rc) {
        dmp_dev_client_job_failed(pparam->paws_iot_client, pparam->thing_name, pparam->pj->job_id,
                "{\"detail\":\"Failed to destroy application container.\"}");
        return rc;
    }

    rc = dmp_dev_client_job_done(pparam->paws_iot_client, pparam->thing_name, pparam->pj->job_id,
            "{\"detail\":\"Application destroyed.\"}");
    if (SUCCESS != rc) {
        IOT_ERROR("failed to set application destroy job status to SUCCEEDED, "
                  "will redo this job if it still under IN_PROGRESS status: %d", rc);
    } else {
        IOT_INFO("destroy application %s successfully, job id: %s", app_name, pparam->pj->job_id);
    }

    return rc;
}
