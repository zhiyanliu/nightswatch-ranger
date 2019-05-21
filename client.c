//
// Created by Zhi Yan Liu on 2019-05-20.
//

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "aws_iot_error.h"
#include "aws_iot_log.h"
#include "aws_iot_mqtt_client.h"
#include "aws_iot_mqtt_client_interface.h"

#include "client.h"


p_dmp_dev_client dmp_dev_client_alloc() {
    p_dmp_dev_client client = (p_dmp_dev_client)malloc(sizeof(dmp_dev_client));
    if (NULL != client)
        memset(client, 0, sizeof(dmp_dev_client));
    return client;
}

IoT_Error_t dmp_dev_client_init(p_dmp_dev_client client, char *thing_name,
        char *root_ca_file, char *client_ca_file, char *key_file, char *host_url, uint16_t port) {

    if (NULL == client)
        return FAILURE;

    if (NULL == thing_name)
        return FAILURE;

    client->thing_name = thing_name; // outside keep the buffer

    getcwd(client->work_dir_path, PATH_MAX + 1);

    snprintf(client->root_ca_file_path, PATH_MAX + 1, "%s/%s/%s",
            client->work_dir_path, IROOTECH_DMP_RP_AGENT_CERTS_DIR, root_ca_file);
    snprintf(client->client_ca_file_path, PATH_MAX + 1, "%s/%s/%s",
             client->work_dir_path, IROOTECH_DMP_RP_AGENT_CERTS_DIR, client_ca_file);
    snprintf(client->key_file_path, PATH_MAX + 1, "%s/%s/%s",
             client->work_dir_path, IROOTECH_DMP_RP_AGENT_CERTS_DIR, key_file);

    IOT_DEBUG("root ca file: %s", client->root_ca_file_path);
    IOT_DEBUG("client cert file: %s", client->client_ca_file_path);
    IOT_DEBUG("client key file: %s", client->key_file_path);

    client->mqtt_init_params = (IoT_Client_Init_Params*)malloc(sizeof(IoT_Client_Init_Params));
    if (NULL == client->mqtt_init_params) {
        IOT_ERROR("unable to allocate mqtt_init_params")
        return FAILURE;
    }

    memcpy(client->mqtt_init_params, &iotClientInitParamsDefault, sizeof(IoT_Client_Init_Params));

    client->mqtt_init_params->enableAutoReconnect = false; // enable this later below
    client->mqtt_init_params->pHostURL = host_url; // outside keep the buffer
    client->mqtt_init_params->port = port;
    client->mqtt_init_params->pRootCALocation = client->root_ca_file_path;
    client->mqtt_init_params->pDeviceCertLocation = client->client_ca_file_path;
    client->mqtt_init_params->pDevicePrivateKeyLocation = client->key_file_path;
    client->mqtt_init_params->mqttCommandTimeout_ms = 20000;
    client->mqtt_init_params->tlsHandshakeTimeout_ms = 5000;
    client->mqtt_init_params->isSSLHostnameVerify = true;
    client->mqtt_init_params->disconnectHandler = _disconnect_callback_handler;
    client->mqtt_init_params->disconnectHandlerData = NULL;

    return aws_iot_mqtt_init(&client->c, client->mqtt_init_params);
}

void dmp_dev_client_free(p_dmp_dev_client client) {
    if (NULL == client)
        return;

    if (NULL != client->mqtt_init_params)
        free(client->mqtt_init_params);

    if (NULL != client->mqtt_conn_params)
        free(client->mqtt_conn_params);

    free(client);
}

IoT_Error_t dmp_dev_client_connect(p_dmp_dev_client client, char *client_id) {
    IoT_Error_t rc = FAILURE;

    if (NULL == client)
        return FAILURE;

    client->mqtt_conn_params = (IoT_Client_Connect_Params*)malloc(sizeof(IoT_Client_Connect_Params));
    if (NULL == client->mqtt_conn_params) {
        IOT_ERROR("unable to allocate mqtt_conn_params")
        return FAILURE;
    }

    memcpy(client->mqtt_conn_params, &iotClientConnectParamsDefault, sizeof(IoT_Client_Connect_Params));

    client->mqtt_conn_params->keepAliveIntervalInSec = 600;
    client->mqtt_conn_params->isCleanSession = true;
    client->mqtt_conn_params->MQTTVersion = MQTT_3_1_1;
    client->mqtt_conn_params->pClientID = client_id; // outside keep the buffer
    client->mqtt_conn_params->clientIDLen = (uint16_t) strlen(client_id);
    client->mqtt_conn_params->isWillMsgPresent = false;

    rc = aws_iot_mqtt_connect(&client->c, client->mqtt_conn_params);
    if(SUCCESS != rc) {
        IOT_ERROR("unable to connect AWS IoT Core server: %d", rc);
        return rc;
    }

    /*
     * Enable Auto Reconnect functionality. Minimum and Maximum time of Exponential backoff are set in aws_iot_config.h
     *  #AWS_IOT_MQTT_MIN_RECONNECT_WAIT_INTERVAL
     *  #AWS_IOT_MQTT_MAX_RECONNECT_WAIT_INTERVAL
     */
    rc = aws_iot_mqtt_autoreconnect_set_status(&client->c, true);
    if(SUCCESS != rc) {
        IOT_ERROR("unable to enable auto reconnect: %d", rc);
        return rc;
    }

    return rc;
}

void _disconnect_callback_handler(AWS_IoT_Client *client, void *data) {
    IoT_Error_t rc = FAILURE;

    IOT_WARN("mqtt connection disconnected");

    if (NULL == client) {
        return;
    }

    IOT_UNUSED(data);

    if (aws_iot_is_autoreconnect_enabled(client)) {
        IOT_INFO("auto reconnect is enabled, reconnecting attempt will start now");
    } else {
        IOT_WARN("auto reconnect is not enabled, starting manual reconnect...");

        rc = aws_iot_mqtt_attempt_reconnect(client);
        if(NETWORK_RECONNECTED == rc) {
            IOT_WARN("manual reconnect successful");
        } else {
            IOT_WARN("manual reconnect failed: %d", rc);
        }
    }
}

