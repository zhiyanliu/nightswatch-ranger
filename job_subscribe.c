//
// Created by Zhi Yan Liu on 2019-05-20.
//

#include "aws_iot_error.h"
#include "aws_iot_jobs_interface.h"
#include "aws_iot_jobs_topics.h"
#include "aws_iot_json_utils.h"
#include "aws_iot_log.h"
#include "aws_iot_mqtt_client.h"
#include "aws_iot_mqtt_client_interface.h"
#include "aws_iot_version.h"

#include "client.h"


IoT_Error_t dmp_dev_client_job_listen(p_dmp_dev_client client, char* thing_name) {
    IoT_Error_t rc = FAILURE;

    char tpc_sub_notify_next[MAX_JOB_TOPIC_LENGTH_BYTES];
    char tpc_sub_get_next[MAX_JOB_TOPIC_LENGTH_BYTES];
    char tpc_sub_upd_accepted[MAX_JOB_TOPIC_LENGTH_BYTES];
    char tpc_sub_upd_rejected[MAX_JOB_TOPIC_LENGTH_BYTES];
//    char tpc_pub_get_pending[MAX_JOB_TOPIC_LENGTH_BYTES];

    if (NULL == client)
        return FAILURE;

    if (NULL == thing_name)
        return FAILURE;

    rc = aws_iot_jobs_subscribe_to_job_messages(
            &client->c, QOS0, thing_name, NULL, JOB_NOTIFY_NEXT_TOPIC, JOB_REQUEST_TYPE,
            _next_job_callback_handler, NULL, tpc_sub_notify_next, MAX_JOB_TOPIC_LENGTH_BYTES);
    if (SUCCESS != rc) {
        IOT_ERROR("failed to subscribe topic $aws/things/{thing-name}/jobs/notify-next: %d", rc);
        return rc;
    }

    rc = aws_iot_jobs_subscribe_to_job_messages(
            &client->c, QOS0, thing_name, JOB_ID_NEXT, JOB_DESCRIBE_TOPIC, JOB_WILDCARD_REPLY_TYPE,
            _next_job_callback_handler, NULL, tpc_sub_get_next, MAX_JOB_TOPIC_LENGTH_BYTES);
    if (SUCCESS != rc) {
        IOT_ERROR("failed to subscribe topic $aws/things/{thing-name}/jobs/$next/get/+: %d", rc);
        return rc;
    }

    rc = aws_iot_jobs_subscribe_to_job_messages(
            &client->c, QOS0, thing_name, JOB_ID_WILDCARD, JOB_UPDATE_TOPIC, JOB_ACCEPTED_REPLY_TYPE,
            _update_accepted_callback_handler, NULL, tpc_sub_upd_accepted, MAX_JOB_TOPIC_LENGTH_BYTES);
    if (SUCCESS != rc) {
        IOT_ERROR("failed to subscribe topic $aws/things/{thing-name}/jobs/+/update/accepted: %d", rc);
        return rc;
    }

    rc = aws_iot_jobs_subscribe_to_job_messages(
            &client->c, QOS0, thing_name, JOB_ID_WILDCARD, JOB_UPDATE_TOPIC, JOB_REJECTED_REPLY_TYPE,
            _update_rejected_callback_handler, NULL, tpc_sub_upd_rejected, MAX_JOB_TOPIC_LENGTH_BYTES);
    if (SUCCESS != rc) {
        IOT_ERROR("failed to subscribe topic $aws/things/{thing-name}/jobs/+/update/rejected: %d", rc);
    }

//    // $aws/things/{thing-name}/jobs/get
//    rc = aws_iot_jobs_send_query(&client->c, QOS0, thing_name, NULL, NULL,
//            client->tpc_pub_get_pending, MAX_JOB_TOPIC_LENGTH_BYTES, NULL, 0, JOB_GET_PENDING_TOPIC);

    return rc;
}

IoT_Error_t dmp_dev_client_job_ask(p_dmp_dev_client client, char *thing_name) {
    IoT_Error_t rc = FAILURE;

    char tpc_pub_get_next[MAX_JOB_TOPIC_LENGTH_BYTES];

    AwsIotDescribeJobExecutionRequest desc_req;
    desc_req.executionNumber = 0;
    desc_req.includeJobDocument = true;
    desc_req.clientToken = NULL;

    // $aws/things/{thing-name}/jobs/$next
    rc = aws_iot_jobs_describe(&client->c, QOS0, thing_name, JOB_ID_NEXT, &desc_req,
            tpc_pub_get_next, MAX_JOB_TOPIC_LENGTH_BYTES, NULL, 0);
    if (SUCCESS != rc) {
        IOT_ERROR("failed to subscribe topic $aws/things/{thing-name}/jobs/+/update/rejected: %d", rc);
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

void _next_job_callback_handler(AWS_IoT_Client *client, char *topic_name, uint16_t topic_name_l,
        IoT_Publish_Message_Params *params, void *data) {

}

void _update_accepted_callback_handler(AWS_IoT_Client *client, char *topic_name, uint16_t topic_name_l,
        IoT_Publish_Message_Params *params, void *data) {

    IOT_UNUSED(client);
    IOT_UNUSED(data);

    IOT_INFO("job update accepted, topic: %.*s:", topic_name_l, topic_name);
    IOT_INFO("payload: %.*s", (int) params->payloadLen, (char *)params->payload);
}

void _update_rejected_callback_handler(AWS_IoT_Client *client, char *topic_name, uint16_t topic_name_l,
        IoT_Publish_Message_Params *params, void *data) {

    IOT_UNUSED(client);
    IOT_UNUSED(data);

    IOT_ERROR("job update rejected, topic: %.*s:", topic_name_l, topic_name);
    IOT_ERROR("payload: %.*s", (int) params->payloadLen, (char *)params->payload);

    // do error handling here for when the update was rejected.
}
