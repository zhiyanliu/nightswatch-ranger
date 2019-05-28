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

#include "certs.h"
#include "client.h"


static void disconnect_callback_handler(AWS_IoT_Client *pclient, void *data);

pdmp_dev_client dmp_dev_client_create() {
    pdmp_dev_client pclient = (pdmp_dev_client)malloc(sizeof(dmp_dev_client));
    if (NULL != pclient)
        memset(pclient, 0, sizeof(dmp_dev_client));
    return pclient;
}

IoT_Error_t dmp_dev_client_init(pdmp_dev_client pclient, char *thing_name,
        char *root_ca_file, char *client_ca_file, char *key_file, char *host_url, uint16_t port) {

    if (NULL == pclient)
        return FAILURE;

    if (NULL == thing_name)
        return FAILURE;

    pclient->thing_name = thing_name;

    getcwd(pclient->work_dir_path, PATH_MAX + 1);

    snprintf(pclient->root_ca_file_path, PATH_MAX + 1, "%s/%s/%s/%s",
            pclient->work_dir_path, IROOTECH_DMP_RP_AGENT_CERTS_DIR, IROOTECH_DMP_RP_AGENT_CERTS_PARTITION_CURRENT,
            root_ca_file);
    snprintf(pclient->client_ca_file_path, PATH_MAX + 1, "%s/%s/%s/%s",
            pclient->work_dir_path, IROOTECH_DMP_RP_AGENT_CERTS_DIR, IROOTECH_DMP_RP_AGENT_CERTS_PARTITION_CURRENT,
            client_ca_file);
    snprintf(pclient->key_file_path, PATH_MAX + 1, "%s/%s/%s/%s",
            pclient->work_dir_path, IROOTECH_DMP_RP_AGENT_CERTS_DIR, IROOTECH_DMP_RP_AGENT_CERTS_PARTITION_CURRENT,
            key_file);

    IOT_DEBUG("root ca file: %s", pclient->root_ca_file_path);
    IOT_DEBUG("client cert file: %s", pclient->client_ca_file_path);
    IOT_DEBUG("client key file: %s", pclient->key_file_path);

    pclient->mqtt_init_params = (IoT_Client_Init_Params*)malloc(sizeof(IoT_Client_Init_Params));
    if (NULL == pclient->mqtt_init_params) {
        IOT_ERROR("unable to allocate mqtt_init_params")
        return FAILURE;
    }

    memcpy(pclient->mqtt_init_params, &iotClientInitParamsDefault, sizeof(IoT_Client_Init_Params));

    pclient->mqtt_init_params->enableAutoReconnect = false; // enable this later below
    pclient->mqtt_init_params->pHostURL = host_url; // outside keep the buffer
    pclient->mqtt_init_params->port = port;
    pclient->mqtt_init_params->pRootCALocation = pclient->root_ca_file_path;
    pclient->mqtt_init_params->pDeviceCertLocation = pclient->client_ca_file_path;
    pclient->mqtt_init_params->pDevicePrivateKeyLocation = pclient->key_file_path;
    pclient->mqtt_init_params->mqttCommandTimeout_ms = 20000;
    pclient->mqtt_init_params->tlsHandshakeTimeout_ms = 5000;
    pclient->mqtt_init_params->isSSLHostnameVerify = true;
    pclient->mqtt_init_params->disconnectHandler = disconnect_callback_handler;
    pclient->mqtt_init_params->disconnectHandlerData = NULL;

    return aws_iot_mqtt_init(&pclient->c, pclient->mqtt_init_params);
}

void dmp_dev_client_free(pdmp_dev_client pclient) {
    if (NULL == pclient)
        return;

    if (NULL != pclient->mqtt_init_params)
        free(pclient->mqtt_init_params);

    if (NULL != pclient->mqtt_conn_params)
        free(pclient->mqtt_conn_params);

    free(pclient);
}

IoT_Error_t dmp_dev_client_connect(pdmp_dev_client pclient, char *client_id) {
    IoT_Error_t rc = FAILURE;

    if (NULL == pclient)
        return FAILURE;

    pclient->mqtt_conn_params = (IoT_Client_Connect_Params*)malloc(sizeof(IoT_Client_Connect_Params));
    if (NULL == pclient->mqtt_conn_params) {
        IOT_ERROR("unable to allocate mqtt_conn_params")
        return FAILURE;
    }

    memcpy(pclient->mqtt_conn_params, &iotClientConnectParamsDefault, sizeof(IoT_Client_Connect_Params));

    pclient->mqtt_conn_params->keepAliveIntervalInSec = 600;
    pclient->mqtt_conn_params->isCleanSession = true;
    pclient->mqtt_conn_params->MQTTVersion = MQTT_3_1_1;
    pclient->mqtt_conn_params->pClientID = client_id; // outside keep the buffer
    pclient->mqtt_conn_params->clientIDLen = (uint16_t) strlen(client_id);
    pclient->mqtt_conn_params->isWillMsgPresent = false;

    rc = aws_iot_mqtt_connect(&pclient->c, pclient->mqtt_conn_params);
    if(SUCCESS != rc) {
        IOT_ERROR("unable to connect AWS IoT Core server: %d", rc);
        return rc;
    }

    /*
     * Enable Auto Reconnect functionality. Minimum and Maximum time of Exponential backoff are set in aws_iot_config.h
     *  #AWS_IOT_MQTT_MIN_RECONNECT_WAIT_INTERVAL
     *  #AWS_IOT_MQTT_MAX_RECONNECT_WAIT_INTERVAL
     */
    rc = aws_iot_mqtt_autoreconnect_set_status(&pclient->c, true);
    if(SUCCESS != rc) {
        IOT_ERROR("unable to enable auto reconnect: %d", rc);
        return rc;
    }

    return rc;
}

static void disconnect_callback_handler(AWS_IoT_Client *pclient, void *data) {
    IoT_Error_t rc = FAILURE;

    IOT_WARN("mqtt connection disconnected");

    if (NULL == pclient) {
        return;
    }

    IOT_UNUSED(data);

    if (aws_iot_is_autoreconnect_enabled(pclient)) {
        IOT_INFO("auto reconnect is enabled, reconnecting attempt will start now");
    } else {
        IOT_WARN("auto reconnect is not enabled, starting manual reconnect...");

        rc = aws_iot_mqtt_attempt_reconnect(pclient);
        if(NETWORK_RECONNECTED == rc) {
            IOT_WARN("manual reconnect successful");
        } else {
            IOT_WARN("manual reconnect failed: %d", rc);
        }
    }
}

