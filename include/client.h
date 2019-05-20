//
// Created by Zhi Yan Liu on 2019-05-20.
//

#ifndef IROOTECH_DMP_RP_AGENT_CLIENT_H_
#define IROOTECH_DMP_RP_AGENT_CLIENT_H_


#include "aws_iot_error.h"
#include "aws_iot_mqtt_client.h"

#include "aws_iot_config.h"


#define IROOTECH_DMP_RP_AGENT_CERTS_DIR "certs"


typedef struct {
    char root_ca_file_path[PATH_MAX + 1];
    char client_ca_file_path[PATH_MAX + 1];
    char key_file_path[PATH_MAX + 1];
    char work_dir_path[PATH_MAX + 1];

    IoT_Client_Init_Params *mqtt_init_params;
    IoT_Client_Connect_Params *mqtt_conn_params;
    AWS_IoT_Client c;
} dmp_dev_client, *p_dmp_dev_client;

// lifecycle

p_dmp_dev_client dmp_dev_client_alloc();

IoT_Error_t dmp_dev_client_init(p_dmp_dev_client client, char *root_ca_file, char *client_ca_file,
        char *key_file, char *host_url, uint16_t port);

void dmp_dev_client_free(p_dmp_dev_client client);

// connect

IoT_Error_t dmp_dev_client_connect(p_dmp_dev_client client, char *client_id);

// job subscribe

IoT_Error_t dmp_dev_client_job_listen(p_dmp_dev_client client, char* thing_name);

IoT_Error_t dmp_dev_client_job_ask(p_dmp_dev_client client, char *thing_name);

IoT_Error_t dmp_dev_client_job_loop(p_dmp_dev_client client);

// callback handler

void _disconnect_callback_handler(AWS_IoT_Client *client, void *data);

void _next_job_callback_handler(AWS_IoT_Client *client, char *topic_name, uint16_t topic_name_l,
        IoT_Publish_Message_Params *params, void *data);

void _update_accepted_callback_handler(AWS_IoT_Client *client, char *topic_name, uint16_t topic_name_l,
        IoT_Publish_Message_Params *params, void *data);

void _update_rejected_callback_handler(AWS_IoT_Client *client, char *topic_name, uint16_t topic_name_l,
        IoT_Publish_Message_Params *params, void *data);

#endif /* IROOTECH_DMP_RP_AGENT_CLIENT_H_ */
