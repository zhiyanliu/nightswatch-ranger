//
// Created by Zhi Yan Liu on 2019-05-20.
//

#include <stdio.h>
#include <stdlib.h>

#include "aws_iot_error.h"
#include "aws_iot_log.h"

#include "client.h"


int main(int argc, char **argv) {
    p_dmp_dev_client client = NULL;
    IoT_Error_t rc = FAILURE;

    client = dmp_dev_client_alloc();
    if (NULL == client) {
        IOT_ERROR("dmp_dev_client_alloc returned NULL");
        return rc;
    }

    rc = dmp_dev_client_init(client, AWS_IOT_MY_THING_NAME,
            AWS_IOT_ROOT_CA_FILENAME, AWS_IOT_CERTIFICATE_FILENAME,
            AWS_IOT_PRIVATE_KEY_FILENAME, AWS_IOT_MQTT_HOST, AWS_IOT_MQTT_PORT);
    if(SUCCESS != rc) {
        IOT_ERROR("dmp_dev_client_init returned error: %d", rc);

        dmp_dev_client_free(client);
        return rc;
    }

    IOT_INFO("connecting to AWS IoT Core: %s:%d", AWS_IOT_MQTT_HOST, AWS_IOT_MQTT_PORT);

    rc = dmp_dev_client_connect(client, AWS_IOT_MQTT_CLIENT_ID);
    if(SUCCESS != rc) {
        IOT_ERROR("dmp_dev_client_connect returned error: %d", rc);

        dmp_dev_client_free(client);
        return rc;
    }

    IOT_INFO("subscribe to job topics")

    rc = dmp_dev_client_job_listen(client);
    if(SUCCESS != rc) {
        IOT_ERROR("dmp_dev_client_job_listen returned error: %d", rc);

        dmp_dev_client_free(client);
        return rc;
    }

    IOT_INFO("ask a job to process")

    rc = dmp_dev_client_job_ask(client);
    if(SUCCESS != rc) {
        IOT_ERROR("dmp_dev_client_job_ask returned error: %d", rc);

        dmp_dev_client_free(client);
        return rc;
    }

    IOT_INFO("start job mqtt message handle loop")

    rc =dmp_dev_client_job_loop(client);
    if(SUCCESS != rc) {
        IOT_ERROR("dmp_dev_client_job_loop returned error: %d", rc);

        dmp_dev_client_free(client);
    }

    return rc;
}
