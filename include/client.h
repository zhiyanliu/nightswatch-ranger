//
// Created by Zhi Yan Liu on 2019-05-20.
//

#ifndef IROOTECH_DMP_RP_AGENT_CLIENT_H_
#define IROOTECH_DMP_RP_AGENT_CLIENT_H_

#include "aws_iot_error.h"
#include "aws_iot_mqtt_client.h"

#define IROOTECH_DMP_RP_AGENT_CERTS_DIR "certs"

typedef struct {
    char *root_ca_file_path;
    char *client_ca_file_path;
    char *key_file_path;
    char *work_dir_path;
    IoT_Client_Init_Params *mqtt_init_params;
    IoT_Client_Connect_Params *mqtt_conn_params;
    IoT_Publish_Message_Params pub_msg_param_qos0;
    AWS_IoT_Client c;
} dmp_dev_client, *p_dmp_dev_client;

p_dmp_dev_client dmp_dev_client_alloc();

IoT_Error_t dmp_dev_client_init(p_dmp_dev_client client, char *root_ca_file, char *client_ca_file,
        char *key_file, char *host_url, uint16_t port);

void dmp_dev_client_free(p_dmp_dev_client client);

IoT_Error_t dmp_dev_client_connect(p_dmp_dev_client client, char *client_id);

void disconnect_callback_handler(AWS_IoT_Client *client, void *data);

#endif /* IROOTECH_DMP_RP_AGENT_CLIENT_H_ */
