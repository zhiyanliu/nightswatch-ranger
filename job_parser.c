//
// Created by Zhi Yan Liu on 2019-05-21.
//

#include <string.h>

#include "aws_iot_error.h"
#include "aws_iot_jobs_types.h"
#include "aws_iot_json_utils.h"
#include "aws_iot_log.h"
#include "aws_iot_mqtt_client.h"

#include "aws_iot_config.h"
#include "job_parser.h"


/*
 * we handle job message one by one, no concurrent process.
 * so reuse follow single parser, token vector and token counter to save resource.
 */
static jsmn_parser _json_parser;
static jsmntok_t _json_tok_v[MAX_JSON_TOKEN_EXPECTED];
static int32_t _token_c;

Job_Parse_Error_t job_parser_parse(void *payload, size_t payload_l, ppjob ppj,
        char *job_status_cur, size_t job_status_cur_l) {

    IoT_Error_t rc = FAILURE;
    Job_Parse_Error_t prc = JOB_PARSE_FAILURE;

    jsmntok_t *tok_execution, *tok;

    if (NULL == payload)
        return JOB_PARSE_FAILURE;

    if (0 == payload_l)
        return JOB_PARSE_FAILURE;

    if (NULL == ppj)
        return JOB_PARSE_FAILURE;

    *ppj = (pjob)malloc(sizeof(job));
    if (NULL == *ppj) {
        IOT_ERROR("unable to allocate job");
        return JOB_PARSE_FAILURE;
    }

    memset(*ppj, 0, sizeof(job));

    (*ppj)->payload = payload;
    (*ppj)->payload_l = payload_l;

    jsmn_init(&_json_parser);

    _token_c = jsmn_parse(&_json_parser, payload, (int)payload_l, _json_tok_v, MAX_JSON_TOKEN_EXPECTED);
    if(_token_c < 0) {
        IOT_ERROR("failed to parse job json: %d", _token_c);
        prc = JOB_PARSE_FAILURE;
        goto ret;
    }

    /* assume the top-level element is an object */
    if(_token_c < 1 || _json_tok_v[0].type != JSMN_OBJECT) {
        IOT_ERROR("invalid job json, top level is not an object");
        prc = JOB_PARSE_FAILURE;
        goto ret;
    }

    tok_execution = findToken("execution", payload, _json_tok_v);
    if (NULL == tok_execution) {
        IOT_WARN("job execution property not found, nothing to do");
        prc = JOB_EXECUTION_EMPTY;
        goto ret;
    }

    IOT_DEBUG("job execution: %.*s", tok_execution->end - tok_execution->start,
            (char *)payload + tok_execution->start);

    tok = findToken("jobId", payload, tok_execution);
    if (NULL == tok) {
        IOT_ERROR("invalid job json, job id not found");
        prc = JOB_ID_NOT_FOUND_ERROR;
        goto ret;
    }

    rc = parseStringValue((*ppj)->job_id, MAX_SIZE_OF_JOB_ID + 1, payload, tok);
    if (SUCCESS != rc) {
        IOT_ERROR("failed to parse job id: %d", rc);
        prc = JOB_ID_INVALID_ERROR;
        goto ret;
    }

    IOT_DEBUG("job id: %s", (*ppj)->job_id);

    if (NULL != job_status_cur && job_status_cur_l > 0) {
        tok = findToken("status", payload, tok_execution);
        if (NULL == tok) {
            IOT_ERROR("invalid job json, job status not found");
            prc = JOB_STATUS_NOT_FOUND_ERROR;
            goto ret;
        }

        rc = parseStringValue(job_status_cur, job_status_cur_l, payload, tok);
        if (SUCCESS != rc) {
            IOT_ERROR("failed to parse job status: %d", rc);
            prc = JOB_STATUS_INVALID_ERROR;
            goto ret;
        }

        IOT_DEBUG("job status: %s", job_status_cur);
    }

    tok = findToken("jobDocument", payload, tok_execution);
    if (NULL == tok) {
        IOT_ERROR("invalid job json, job document not found");
        prc = JOB_DOCUMENT_NOT_FOUND_ERROR;
        goto ret;
    }

    (*ppj)->job_doc = (char*)payload + tok->start;
    (*ppj)->job_doc_l = tok->end - tok->start;

    IOT_DEBUG("job document: %.*s", (int)(*ppj)->job_doc_l, (*ppj)->job_doc);

    prc = JOB_PARSE_SUCCESS;

ret:
    return prc;
}

void job_parser_job_free(pjob pj) {
    if (NULL == pj)
        return;

    free(pj);
}
