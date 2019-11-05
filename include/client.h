//
// Created by Zhi Yan Liu on 2019-05-20.
//

#ifndef NIGHTSWATCH_RANGER_CLIENT_H_
#define NIGHTSWATCH_RANGER_CLIENT_H_

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
} nw_dev_client, *pnw_dev_client;

// lifecycle

pnw_dev_client nw_dev_client_create();

IoT_Error_t nw_dev_client_init(pnw_dev_client client, char *thing_name,
        char *root_ca_file, char *client_ca_file,
        char *key_file, char *host_url, uint16_t port);

void nw_dev_client_free(pnw_dev_client client);

// connect

IoT_Error_t nw_dev_client_connect(pnw_dev_client client, char *client_id);

// job subscribe

IoT_Error_t nw_dev_client_job_listen_next(pnw_dev_client client, pjob_dispatcher pdispatcher);

IoT_Error_t nw_dev_client_job_listen_update(pnw_dev_client client, pjob_dispatcher pdispatcher);

IoT_Error_t nw_dev_client_job_ask(pnw_dev_client client);

IoT_Error_t nw_dev_client_job_loop(pnw_dev_client client);

// job update

IoT_Error_t nw_dev_client_job_update(AWS_IoT_Client *paws_iot_client, char *thing_name, char *job_id,
        const char *job_status, const char *job_status_details);

IoT_Error_t nw_dev_client_job_wip(AWS_IoT_Client *paws_iot_client, char *thing_name, char *job_id,
        const char *job_status_details);

IoT_Error_t nw_dev_client_job_reject(AWS_IoT_Client *paws_iot_client, char *thing_name, char *job_id,
        const char *job_status_details);

IoT_Error_t nw_dev_client_job_cancel(AWS_IoT_Client *paws_iot_client, char *thing_name, char *job_id,
        const char *job_status_details);

IoT_Error_t nw_dev_client_job_done(AWS_IoT_Client *paws_iot_client, char *thing_name, char *job_id,
        const char *job_status_details);

IoT_Error_t nw_dev_client_job_failed(AWS_IoT_Client *paws_iot_client, char *thing_name, char *job_id,
        const char *job_status_details);

#endif // NIGHTSWATCH_RANGER_CLIENT_H_
