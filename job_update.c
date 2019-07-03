//
// Created by Zhi Yan Liu on 2019-05-21.
//

#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "aws_iot_jobs_interface.h"
#include "aws_iot_log.h"
#include "aws_iot_mqtt_client_interface.h"

#include "client.h"


IoT_Error_t dmp_dev_client_job_update(AWS_IoT_Client *paws_iot_client, char *thing_name, char *job_id,
        const char *job_status, const char *job_status_details) {
    IoT_Error_t rc = FAILURE;
    AwsIotJobExecutionUpdateRequest req;
    char tpc_pub_update[MAX_JOB_TOPIC_LENGTH_BYTES];
    char msg[256];

    if (NULL == job_id)
        return FAILURE;

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

    do {
        rc = aws_iot_jobs_send_update(paws_iot_client, QOS1, thing_name, job_id, &req,
                tpc_pub_update, MAX_JOB_TOPIC_LENGTH_BYTES, msg, sizeof(msg) / sizeof(char));

        if (MQTT_CLIENT_NOT_IDLE_ERROR == rc)
            usleep(500); // same as timeout of yield() in main thread loop
    } while (MQTT_CLIENT_NOT_IDLE_ERROR == rc || NETWORK_ATTEMPTING_RECONNECT == rc || NETWORK_RECONNECTED == rc);

    if (SUCCESS != rc) {
        IOT_ERROR("failed to request updating job %s to %s status: %d", job_id, job_status, rc);
    }

    // FIXME(prod): wait update received by cloud and get response in accepted or rejected topic.

    return rc;
}

IoT_Error_t dmp_dev_client_job_wip(AWS_IoT_Client *paws_iot_client, char *thing_name, char *job_id,
        const char *job_status_details) {
    return dmp_dev_client_job_update(paws_iot_client, thing_name, job_id, "IN_PROGRESS", job_status_details);
}

IoT_Error_t dmp_dev_client_job_reject(AWS_IoT_Client *paws_iot_client, char *thing_name, char *job_id,
        const char *job_status_details) {
    return dmp_dev_client_job_update(paws_iot_client, thing_name, job_id, "REJECTED", job_status_details);
}

IoT_Error_t dmp_dev_client_job_cancel(AWS_IoT_Client *paws_iot_client, char *thing_name, char *job_id,
        const char *job_status_details) {
    return dmp_dev_client_job_update(paws_iot_client, thing_name, job_id, "CANCELED", job_status_details);
}

IoT_Error_t dmp_dev_client_job_done(AWS_IoT_Client *paws_iot_client, char *thing_name, char *job_id,
        const char *job_status_details) {
    return dmp_dev_client_job_update(paws_iot_client, thing_name, job_id, "SUCCEEDED", job_status_details);
}

IoT_Error_t dmp_dev_client_job_failed(AWS_IoT_Client *paws_iot_client, char *thing_name, char *job_id,
        const char *job_status_details) {
    return dmp_dev_client_job_update(paws_iot_client, thing_name, job_id, "FAILED", job_status_details);
}
