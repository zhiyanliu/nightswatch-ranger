//
// Created by Zhi Yan Liu on 2019-05-21.
//

#include <stdbool.h>
#include <string.h>

#include "aws_iot_error.h"
#include "aws_iot_jobs_interface.h"
#include "aws_iot_jobs_types.h"
#include "aws_iot_log.h"
#include "aws_iot_mqtt_client.h"

#include "client.h"


IoT_Error_t dmp_dev_client_job_update(AWS_IoT_Client *paws_iot_client, char *thing_name, pjob pj,
        const char *job_status, const char *job_status_details) {
    IoT_Error_t rc = FAILURE;
    AwsIotJobExecutionUpdateRequest req;
    char tpc_pub_update[MAX_JOB_TOPIC_LENGTH_BYTES];
    char msg[256];

    req.status = aws_iot_jobs_map_string_to_job_status(job_status);
    if (JOB_EXECUTION_UNKNOWN_STATUS == req.status) {
        IOT_ERROR("[BUG] invalid job status to update: %s", job_status)
        return FAILURE;
    }

    req.statusDetails = job_status_details;
    req.expectedVersion = 0;
    req.executionNumber = 0;
    req.includeJobExecutionState = false;
    req.includeJobDocument = false;
    req.clientToken = NULL;

    rc = aws_iot_jobs_send_update(paws_iot_client, QOS0, thing_name, pj->job_id, &req,
            tpc_pub_update, MAX_JOB_TOPIC_LENGTH_BYTES, msg, sizeof(msg) / sizeof(char));
    if (SUCCESS != rc) {
        IOT_ERROR("failed to request updating job %s to %s status: %d", pj->job_id, job_status, rc);
    }

    // FIXME(prod): wait update received by cloud and get response in accepted or rejected topic.

    return rc;
}

IoT_Error_t dmp_dev_client_job_wip(AWS_IoT_Client *paws_iot_client, char *thing_name, pjob pj,
        const char *job_status_details) {
    return dmp_dev_client_job_update(paws_iot_client, thing_name, pj, "IN_PROGRESS", job_status_details);
}

IoT_Error_t dmp_dev_client_job_reject(AWS_IoT_Client *paws_iot_client, char *thing_name, pjob pj,
        const char *job_status_details) {
    return dmp_dev_client_job_update(paws_iot_client, thing_name, pj, "REJECTED", job_status_details);
}

IoT_Error_t dmp_dev_client_job_cancel(AWS_IoT_Client *paws_iot_client, char *thing_name, pjob pj,
        const char *job_status_details) {
    return dmp_dev_client_job_update(paws_iot_client, thing_name, pj, "CANCELED", job_status_details);
}

IoT_Error_t dmp_dev_client_job_done(AWS_IoT_Client *paws_iot_client, char *thing_name, pjob pj,
        const char *job_status_details) {
    return dmp_dev_client_job_update(paws_iot_client, thing_name, pj, "SUCCEEDED", job_status_details);
}

IoT_Error_t dmp_dev_client_job_failed(AWS_IoT_Client *paws_iot_client, char *thing_name, pjob pj,
        const char *job_status_details) {
    return dmp_dev_client_job_update(paws_iot_client, thing_name, pj, "FAILED", job_status_details);
}