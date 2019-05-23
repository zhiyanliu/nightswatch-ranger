//
// Created by Zhi Yan Liu on 2019-05-21.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "aws_iot_json_utils.h"
#include "aws_iot_log.h"

#include "job_dispatch.h"


/*
 * we handle job message one by one, no concurrent process.
 * so reuse follow single parser, token vector and token counter to save resource.
 */
static jsmn_parser _json_parser;
static jsmntok_t _json_tok_v[MAX_JSON_TOKEN_EXPECTED];
static int32_t _token_c;


pjob_dispatcher job_dispatcher_reg_executor(pjob_dispatcher pdispatcher,
        const char *op_type, size_t op_type_l, executor_t executor) {
    pexecutor_record record = NULL, executor_v_new = NULL;

    if (NULL == pdispatcher) {
        return NULL;
    }

    record = (pexecutor_record)malloc(sizeof(executor_record));
    if (NULL == record) {
        IOT_ERROR("unable to allocate executor record");
        return pdispatcher;
    }

    memset(record, 0 , sizeof(executor_record));

    record->op_type = op_type;
    record->op_type_l = op_type_l;
    record->executor = executor;

    executor_v_new = (pexecutor_record)realloc(pdispatcher->executor_v,
            sizeof(executor_record)*(pdispatcher->executor_v_l + 1));
    if (NULL == executor_v_new) {
        IOT_ERROR("unable to increase executor list");
        goto error;
    }

    pdispatcher->executor_v = executor_v_new;
    pdispatcher->executor_v_l++;

    goto ret;

error:
    free(record);

ret:
    return pdispatcher;
}

int job_dispatcher_dispatch(pjob_dispatch_param pparam, executor_t *pfun) {
    size_t i = 0;
    int op_l = 0, equal;
    jsmntok_t *tok_operate, *tok;
    bool executed = false;

    if (NULL == pparam)
        return 1;

    if (NULL == pparam->thing_name)
        return 1;

    if (NULL == pparam->paws_iot_client)
        return 1;

    if (NULL == pparam->pdispatcher)
        return 1;

    if (NULL == pparam->pj)
        return 1;

    jsmn_init(&_json_parser);
    _token_c = jsmn_parse(&_json_parser, pparam->pj->job_doc, (int)pparam->pj->job_doc_l,
            _json_tok_v, MAX_JSON_TOKEN_EXPECTED);
    if(_token_c < 0) {
        IOT_ERROR("failed to parse job document json: %d", _token_c);
        return 1;
    }

    tok_operate = findToken("operate", pparam->pj->job_doc, _json_tok_v);
    if (NULL == tok_operate) {
        IOT_WARN("job operate property not found, nothing to do");
        return 2;
    }

    op_l = tok_operate->end - tok_operate->start;

    IOT_DEBUG("dispatch job operate: %.*s", op_l, (char *)pparam->pj->job_doc + tok_operate->start);

    while (i++ < pparam->pdispatcher->executor_v_l) {
        if (op_l != (pparam->pdispatcher->executor_v + i)->op_type_l)
            continue;

        equal = strncmp((pparam->pdispatcher->executor_v + i)->op_type,
                (char *)pparam->pj->job_doc + tok_operate->start, op_l);
        if (0 != equal)
            continue;

        (*pfun) = (pparam->pdispatcher->executor_v + i)->executor;

        // one executor for one kind of job operation, ignore duplicated executors
        return 0;
    }

    IOT_WARN("no executor found for job operation %.*s, job can NOT be executed",
            op_l, (char *)pparam->pj->job_doc + tok_operate->start);

    return 3;
}