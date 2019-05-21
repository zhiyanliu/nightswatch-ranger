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


IoT_Error_t dmp_dev_client_job_update(p_dmp_dev_client client, pjob pj,
        const char *job_status, const char *job_status_details) {

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

    return aws_iot_jobs_send_update(&client->c, QOS0, client->thing_name, pj->job_id, &req,
            tpc_pub_update, MAX_JOB_TOPIC_LENGTH_BYTES, msg, sizeof(msg) / sizeof(char));
}
