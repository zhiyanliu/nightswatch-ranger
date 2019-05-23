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
#include "job_dispatch.h"


typedef struct {
    pdmp_dev_client pclient;
    pjob_dispatcher pdispatcher;
}job_callback_param, *pjob_callback_param;

IoT_Error_t dmp_dev_client_job_listen(pdmp_dev_client pclient, pjob_dispatcher pdispatcher) {
    IoT_Error_t rc = FAILURE;
    pjob_callback_param pparam = NULL;

    if (NULL == pclient)
        return FAILURE;

    pparam = (pjob_callback_param)malloc(sizeof(job_callback_param));
    if (NULL == pparam) {
        IOT_ERROR("unable to allocate job execution parameter");
        return FAILURE;
    }

    pparam->pclient = pclient;
    pparam->pdispatcher = pdispatcher;

    rc = aws_iot_jobs_subscribe_to_job_messages(
            &pclient->c, QOS0, pclient->thing_name, NULL, JOB_NOTIFY_NEXT_TOPIC, JOB_REQUEST_TYPE,
            _next_job_callback_handler, pparam, pclient->tpc_sub_notify_next, MAX_JOB_TOPIC_LENGTH_BYTES);
    if (SUCCESS != rc) {
        IOT_ERROR("failed to subscribe topic $aws/things/{thing-name}/jobs/notify-next: %d", rc);
        return rc;
    }

    rc = aws_iot_jobs_subscribe_to_job_messages(
            &pclient->c, QOS0, pclient->thing_name, JOB_ID_NEXT, JOB_DESCRIBE_TOPIC, JOB_WILDCARD_REPLY_TYPE,
            _next_job_callback_handler, pparam, pclient->tpc_sub_get_next, MAX_JOB_TOPIC_LENGTH_BYTES);
    if (SUCCESS != rc) {
        IOT_ERROR("failed to subscribe topic $aws/things/{thing-name}/jobs/$next/get/+: %d", rc);
        return rc;
    }

    rc = aws_iot_jobs_subscribe_to_job_messages(
            &pclient->c, QOS0, pclient->thing_name, JOB_ID_WILDCARD, JOB_UPDATE_TOPIC, JOB_ACCEPTED_REPLY_TYPE,
            _update_accepted_callback_handler, pparam, pclient->tpc_sub_upd_accepted, MAX_JOB_TOPIC_LENGTH_BYTES);
    if (SUCCESS != rc) {
        IOT_ERROR("failed to subscribe topic $aws/things/{thing-name}/jobs/+/update/accepted: %d", rc);
        return rc;
    }

    rc = aws_iot_jobs_subscribe_to_job_messages(
            &pclient->c, QOS0, pclient->thing_name, JOB_ID_WILDCARD, JOB_UPDATE_TOPIC, JOB_REJECTED_REPLY_TYPE,
            _update_rejected_callback_handler, pparam, pclient->tpc_sub_upd_rejected, MAX_JOB_TOPIC_LENGTH_BYTES);
    if (SUCCESS != rc) {
        IOT_ERROR("failed to subscribe topic $aws/things/{thing-name}/jobs/+/update/rejected: %d", rc);
    }

    return rc;
}

IoT_Error_t dmp_dev_client_job_ask(pdmp_dev_client pclient) {
    IoT_Error_t rc = FAILURE;

    AwsIotDescribeJobExecutionRequest desc_req;
    desc_req.executionNumber = 0;
    desc_req.includeJobDocument = true;
    desc_req.clientToken = NULL;

    // $aws/things/{thing-name}/jobs/$next
    rc = aws_iot_jobs_describe(&pclient->c, QOS0, pclient->thing_name, JOB_ID_NEXT, &desc_req,
            pclient->tpc_pub_get_next, MAX_JOB_TOPIC_LENGTH_BYTES, NULL, 0);
    if (SUCCESS != rc) {
        IOT_ERROR("failed to describe topic $aws/things/{thing-name}/jobs/$next: %d", rc);
    }

    return rc;
}

IoT_Error_t dmp_dev_client_job_loop(pdmp_dev_client pclient) {
    IoT_Error_t rc = FAILURE;

    if (NULL == pclient)
        return FAILURE;

    do {
        // max time the yield function will wait for read messages
        rc = aws_iot_mqtt_yield(&pclient->c, 50000);
    } while(SUCCESS == rc);
    
    return rc;
}

void _next_job_callback_handler(AWS_IoT_Client *c, char *topic_name, uint16_t topic_name_l,
        IoT_Publish_Message_Params *params, void *data) {

    IoT_Error_t rc = FAILURE;
    Job_Parse_Error_t prc = JOB_PARSE_FAILURE;
    pjob pj;
    pjob_callback_param pparam = (pjob_callback_param)data;
    char job_status_recv[20] = {0};
    char *job_status = NULL, *job_status_details = NULL;
    bool dispatch = true;

    if (NULL == pparam) {
        IOT_ERROR("[BUG] invalid context, job callback parameter not found");
        return;
    }

    if (NULL == pparam->pclient) {
        IOT_ERROR("[BUG] invalid context, dmp device client not found");
        return;
    }

    if (NULL == pparam->pdispatcher) {
        IOT_ERROR("[BUG] invalid context, job operation dispatcher not found");
        return;
    }

    IOT_UNUSED(c);

    IOT_DEBUG("job received, topic: %.*s:", topic_name_l, topic_name);
    IOT_DEBUG("payload: %.*s", (int) params->payloadLen, (char *)params->payload);


    prc = job_parser_parse(params->payload, params->payloadLen, &pj, job_status_recv, 20);
    if (SUCCESS != prc) {
        IOT_ERROR("job_parser_parse returned error: %d", rc);

        switch(prc) {
            case JOB_EXECUTION_EMPTY:
                IOT_WARN("invalid job without execution property, skip");
                goto ret;
            case JOB_ID_NOT_FOUND_ERROR:
                IOT_WARN("invalid job without job id, skip");
                goto ret;
            case JOB_ID_INVALID_ERROR:
                IOT_WARN("invalid job id, skip");
                goto ret;
            case JOB_DOCUMENT_NOT_FOUND_ERROR:
                job_status = "REJECTED";
                job_status_details = "{\"failureDetail\":\"Job document not found.\"}";
                break;
            case JOB_STATUS_NOT_FOUND_ERROR:
                job_status = "REJECTED";
                job_status_details = "{\"failureDetail\":\"Job status not found.\"}";
                break;
            case JOB_STATUS_INVALID_ERROR:
                job_status = "REJECTED";
                job_status_details = "{\"failureDetail\":\"Job status is invalid.\"}";
                break;
            case FAILURE:
                job_status = "FAILED";
                job_status_details = "{\"failureDetail\":\"Unable to process job, check log from the device.\"}";
                break;
            default:
                break;
        }

        if (NULL != job_status) {
            rc = dmp_dev_client_job_update(pparam->pclient, pj, job_status, job_status_details);
            if (SUCCESS != rc)
                IOT_ERROR("dmp_dev_client_job_update returned error: %d", rc);
        }
    } else {
        int dispatch_rc = 0, execute_rc = 0;
        executor_t fun = NULL;

        // dispatch job
        job_dispatch_param dispatch_param;
        dispatch_param.thing_name = pparam->pclient->thing_name;
        dispatch_param.paws_iot_client = &pparam->pclient->c;
        dispatch_param.pdispatcher = pparam->pdispatcher;
        dispatch_param.pj = pj;
        dispatch_param.job_status_recv = job_status_recv;
        dispatch_param.job_status_recv_l = 20;

        dispatch_rc = job_dispatcher_dispatch(&dispatch_param, &fun);
        switch (dispatch_rc) {
            case 1: // invalid input
                IOT_ERROR("[BUG] invalid input for dispatch");

                dmp_dev_client_job_cancel(pparam->pclient, pj,
                        "{\"detail\":\"Job is cancelled due to device client bug.\"}");
                break;
            case 2: // nothing to do
                IOT_WARN("job operate property not found, reject");

                dmp_dev_client_job_reject(pparam->pclient, pj,
                        "{\"failureDetail\":\"Job operate property not found.\"}");
                break;
            case 3: // executor not found, unsupported operate
                IOT_ERROR("unsupported job operator for this device client, reject");

                dmp_dev_client_job_reject(pparam->pclient, pj,
                        "{\"failureDetail\":\"Job operate not support for this device client.\"}");
                break;
            case 0:
                dmp_dev_client_job_wip(pparam->pclient, pj,
                        "{\"detail\":\"Job is accepted and start to execute.\"}");

                // execute job
                IOT_DEBUG("start to execute job %s...", pj->job_id);
                execute_rc = (*fun)(&dispatch_param);
                IOT_INFO("job %s executed successfully (rc=%d)", pj->job_id, execute_rc);

                break;
        }
    }

ret:
    job_parser_job_free(pj);

    return;
}

void _update_accepted_callback_handler(AWS_IoT_Client *client, char *topic_name, uint16_t topic_name_l,
        IoT_Publish_Message_Params *params, void *data) {

    IOT_UNUSED(client);
    IOT_UNUSED(data); // typeof(data) == pjob_callback_param

    IOT_DEBUG("job update accepted, topic: %.*s:", topic_name_l, topic_name);
    IOT_DEBUG("payload: %.*s", (int) params->payloadLen, (char *)params->payload);
}

void _update_rejected_callback_handler(AWS_IoT_Client *client, char *topic_name, uint16_t topic_name_l,
        IoT_Publish_Message_Params *params, void *data) {

    IOT_UNUSED(client);
    IOT_UNUSED(data); // typeof(data) == pjob_callback_param

    IOT_ERROR("job update rejected, topic: %.*s:", topic_name_l, topic_name);
    IOT_ERROR("payload: %.*s", (int) params->payloadLen, (char *)params->payload);

    // do error handling here for when the update was rejected.
}
