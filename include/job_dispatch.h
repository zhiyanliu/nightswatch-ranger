//
// Created by Zhi Yan Liu on 2019-05-21.
//

#ifndef IROOTECH_DMP_RP_AGENT_JOB_DISPATCH_H_
#define IROOTECH_DMP_RP_AGENT_JOB_DISPATCH_H_

#include <stddef.h>

#include "aws_iot_mqtt_client.h"

#include "job_parser.h"

// declaration
typedef struct job_dispatch_param job_dispatch_param;
typedef struct job_dispatch_param *pjob_dispatch_param;

// definition
typedef int (*executor_t)(pjob_dispatch_param pparam);

typedef struct {
    const char *op_type; // outside keeps the buffer
    size_t op_type_l;
    executor_t executor;
} executor_record, *pexecutor_record;

typedef struct {
    pexecutor_record executor_v;
    size_t executor_v_l;
} job_dispatcher, *pjob_dispatcher;

struct job_dispatch_param {
    char *thing_name; // outside keep the buffer
    AWS_IoT_Client *paws_iot_client;
    pjob_dispatcher pdispatcher;
    pjob pj;
    char *job_status_recv;
    size_t job_status_recv_l;
};

pjob_dispatcher job_dispatcher_reg_executor(pjob_dispatcher pdispatcher,
        const char *op_type, size_t op_type_l, executor_t executor);

int job_dispatcher_dispatch(pjob_dispatch_param pparam, executor_t *pfun);

#endif //IROOTECH_DMP_RP_AGENT_JOB_DISPATCH_H_
