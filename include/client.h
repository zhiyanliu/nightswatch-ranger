//
// Created by Zhi Yan Liu on 2019-05-20.
//

#ifndef IROOTECH_DMP_RP_AGENT_CLIENT_H_
#define IROOTECH_DMP_RP_AGENT_CLIENT_H_


#include "aws_iot_error.h"
#include "aws_iot_mqtt_client.h"

#include "aws_iot_config.h"
#include "job_dispatch.h"
#include "job_parser.h"


typedef struct {
    char *thing_name; // outside keep the buffer
    char root_ca_file_path[PATH_MAX + 1];
    char client_ca_file_path[PATH_MAX + 1];
    char key_file_path[PATH_MAX + 1];
    char work_dir_path[PATH_MAX + 1];

    IoT_Client_Init_Params *mqtt_init_params;
    IoT_Client_Connect_Params *mqtt_conn_params;
    AWS_IoT_Client c;

    char tpc_sub_notify_next[MAX_JOB_TOPIC_LENGTH_BYTES];
    char tpc_sub_get_next[MAX_JOB_TOPIC_LENGTH_BYTES];
    char tpc_sub_upd_accepted[MAX_JOB_TOPIC_LENGTH_BYTES];
    char tpc_sub_upd_rejected[MAX_JOB_TOPIC_LENGTH_BYTES];
    char tpc_pub_get_next[MAX_JOB_TOPIC_LENGTH_BYTES];
} dmp_dev_client, *pdmp_dev_client;

// lifecycle

pdmp_dev_client dmp_dev_client_create();

IoT_Error_t dmp_dev_client_init(pdmp_dev_client client, char *thing_name,
        char *root_ca_file, char *client_ca_file,
        char *key_file, char *host_url, uint16_t port);

void dmp_dev_client_free(pdmp_dev_client client);

// connect

IoT_Error_t dmp_dev_client_connect(pdmp_dev_client client, char *client_id);

// job subscribe

IoT_Error_t dmp_dev_client_job_listen(pdmp_dev_client client, pjob_dispatcher pdispatcher);

IoT_Error_t dmp_dev_client_job_ask(pdmp_dev_client client);

IoT_Error_t dmp_dev_client_job_loop(pdmp_dev_client client);

// job update

IoT_Error_t dmp_dev_client_job_update(AWS_IoT_Client *paws_iot_client, char *thing_name, pjob pj,
        const char *job_status, const char *job_status_details);

IoT_Error_t dmp_dev_client_job_wip(AWS_IoT_Client *paws_iot_client, char *thing_name, pjob pj,
        const char *job_status_details);

IoT_Error_t dmp_dev_client_job_reject(AWS_IoT_Client *paws_iot_client, char *thing_name, pjob pj,
        const char *job_status_details);

IoT_Error_t dmp_dev_client_job_cancel(AWS_IoT_Client *paws_iot_client, char *thing_name, pjob pj,
        const char *job_status_details);

IoT_Error_t dmp_dev_client_job_done(AWS_IoT_Client *paws_iot_client, char *thing_name, pjob pj,
        const char *job_status_details);

#endif /* IROOTECH_DMP_RP_AGENT_CLIENT_H_ */
