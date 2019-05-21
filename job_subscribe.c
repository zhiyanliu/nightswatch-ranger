//
// Created by Zhi Yan Liu on 2019-05-20.
//

#include <stdbool.h>

#include "aws_iot_error.h"
#include "aws_iot_jobs_interface.h"
#include "aws_iot_jobs_topics.h"
#include "aws_iot_log.h"
#include "aws_iot_mqtt_client.h"
#include "aws_iot_mqtt_client_interface.h"
#include "aws_iot_version.h"

#include "client.h"


IoT_Error_t dmp_dev_client_job_listen(p_dmp_dev_client client) {
    IoT_Error_t rc = FAILURE;


    if (NULL == client)
        return FAILURE;

    rc = aws_iot_jobs_subscribe_to_job_messages(
            &client->c, QOS0, client->thing_name, NULL, JOB_NOTIFY_NEXT_TOPIC, JOB_REQUEST_TYPE,
            _next_job_callback_handler, client, client->tpc_sub_notify_next, MAX_JOB_TOPIC_LENGTH_BYTES);
    if (SUCCESS != rc) {
        IOT_ERROR("failed to subscribe topic $aws/things/{thing-name}/jobs/notify-next: %d", rc);
        return rc;
    }

    rc = aws_iot_jobs_subscribe_to_job_messages(
            &client->c, QOS0, client->thing_name, JOB_ID_NEXT, JOB_DESCRIBE_TOPIC, JOB_WILDCARD_REPLY_TYPE,
            _next_job_callback_handler, client, client->tpc_sub_get_next, MAX_JOB_TOPIC_LENGTH_BYTES);
    if (SUCCESS != rc) {
        IOT_ERROR("failed to subscribe topic $aws/things/{thing-name}/jobs/$next/get/+: %d", rc);
        return rc;
    }

    rc = aws_iot_jobs_subscribe_to_job_messages(
            &client->c, QOS0, client->thing_name, JOB_ID_WILDCARD, JOB_UPDATE_TOPIC, JOB_ACCEPTED_REPLY_TYPE,
            _update_accepted_callback_handler, client, client->tpc_sub_upd_accepted, MAX_JOB_TOPIC_LENGTH_BYTES);
    if (SUCCESS != rc) {
        IOT_ERROR("failed to subscribe topic $aws/things/{thing-name}/jobs/+/update/accepted: %d", rc);
        return rc;
    }

    rc = aws_iot_jobs_subscribe_to_job_messages(
            &client->c, QOS0, client->thing_name, JOB_ID_WILDCARD, JOB_UPDATE_TOPIC, JOB_REJECTED_REPLY_TYPE,
            _update_rejected_callback_handler, client, client->tpc_sub_upd_rejected, MAX_JOB_TOPIC_LENGTH_BYTES);
    if (SUCCESS != rc) {
        IOT_ERROR("failed to subscribe topic $aws/things/{thing-name}/jobs/+/update/rejected: %d", rc);
    }

    return rc;
}

IoT_Error_t dmp_dev_client_job_ask(p_dmp_dev_client client) {
    IoT_Error_t rc = FAILURE;

    AwsIotDescribeJobExecutionRequest desc_req;
    desc_req.executionNumber = 0;
    desc_req.includeJobDocument = true;
    desc_req.clientToken = NULL;

    // $aws/things/{thing-name}/jobs/$next
    rc = aws_iot_jobs_describe(&client->c, QOS0, client->thing_name, JOB_ID_NEXT, &desc_req,
            client->tpc_pub_get_next, MAX_JOB_TOPIC_LENGTH_BYTES, NULL, 0);
    if (SUCCESS != rc) {
        IOT_ERROR("failed to describe topic $aws/things/{thing-name}/jobs/$next: %d", rc);
    }

    return rc;
}

IoT_Error_t dmp_dev_client_job_loop(p_dmp_dev_client client) {
    IoT_Error_t rc = FAILURE;

    if (NULL == client)
        return FAILURE;

    do {
        // max time the yield function will wait for read messages
        rc = aws_iot_mqtt_yield(&client->c, 50000);
    } while(SUCCESS == rc);
    
    return rc;
}

void _next_job_callback_handler(AWS_IoT_Client *c, char *topic_name, uint16_t topic_name_l,
        IoT_Publish_Message_Params *params, void *data) {

    IoT_Error_t rc = FAILURE;
    Job_Parse_Error_t prc = JOB_PARSE_FAILURE;
    pjob pj;
    p_dmp_dev_client client = (p_dmp_dev_client)data;
    char job_status_local[20] = {0};
    char *job_status = NULL, *job_status_details = NULL;
    bool dispatch = true;


    if (NULL == client) {
        IOT_ERROR("[BUG] invalid context, dmp device client not found");
        return;
    }

    IOT_UNUSED(c);

    IOT_DEBUG("job received, topic: %.*s:", topic_name_l, topic_name);
    IOT_DEBUG("payload: %.*s", (int) params->payloadLen, (char *)params->payload);


    prc = job_parser_parse(params->payload, params->payloadLen, &pj, job_status_local, 20);
    if (SUCCESS != prc) {
        IOT_ERROR("job_parser_parse returned error: %d", rc);
        dispatch = false;
    }

    switch(prc) {
        case JOB_EXECUTION_EMPTY:
            IOT_WARN("reject job without execution property");
            job_status = "REJECTED";
            job_status_details = "{\"failureDetail\":\"Job execution property not found.\"}";
        case JOB_ID_NOT_FOUND_ERROR:
            job_status = "FAILED";
            job_status_details = "{\"failureDetail\":\"Job id not found.\"}";
        case JOB_ID_INVALID_ERROR:
            job_status = "FAILED";
            job_status_details = "{\"failureDetail\":\"Job id is invalid.\"}";
        case JOB_DOCUMENT_NOT_FOUND_ERROR:
            job_status = "FAILED";
            job_status_details = "{\"failureDetail\":\"Job document not found.\"}";
        case JOB_STATUS_NOT_FOUND_ERROR:
            job_status = "FAILED";
            job_status_details = "{\"failureDetail\":\"Job status not found.\"}";
        case JOB_STATUS_INVALID_ERROR:
            job_status = "FAILED";
            job_status_details = "{\"failureDetail\":\"Job status is invalid.\"}";
        case FAILURE:
            job_status = "FAILED";
            job_status_details = "{\"failureDetail\":\"Unable to process job, check log from the device.\"}";
        default:
            // in-progress status update can be moved into operation executor, when the job is executed really
            job_status = "IN_PROGRESS";
            job_status_details = "{\"detail\":\"Job is accepted and dispatch to execute now.\"}";
    }

    rc = dmp_dev_client_job_update(client, pj, job_status, job_status_details);
    if (SUCCESS != rc) {
        IOT_ERROR("dmp_dev_client_job_update returned error: %d", rc);
        if (dispatch)
            IOT_ERROR("Do NOT dispatch job, due to failed to update job status");
        return;
    }

    if (!dispatch)
        return;

    // TODO(zhiyan): dispatch job

    return;
}

void _update_accepted_callback_handler(AWS_IoT_Client *client, char *topic_name, uint16_t topic_name_l,
        IoT_Publish_Message_Params *params, void *data) {

    IOT_UNUSED(client);
    IOT_UNUSED(data); // typeof(data) == p_dmp_dev_client

    IOT_DEBUG("job update accepted, topic: %.*s:", topic_name_l, topic_name);
    IOT_DEBUG("payload: %.*s", (int) params->payloadLen, (char *)params->payload);
}

void _update_rejected_callback_handler(AWS_IoT_Client *client, char *topic_name, uint16_t topic_name_l,
        IoT_Publish_Message_Params *params, void *data) {

    IOT_UNUSED(client);
    IOT_UNUSED(data); // typeof(data) == p_dmp_dev_client

    IOT_ERROR("job update rejected, topic: %.*s:", topic_name_l, topic_name);
    IOT_ERROR("payload: %.*s", (int) params->payloadLen, (char *)params->payload);

    // do error handling here for when the update was rejected.
}
