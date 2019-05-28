//
// Created by Zhi Yan Liu on 2019-05-20.
//

#include <stdio.h>
#include <stdlib.h>

#include "aws_iot_error.h"
#include "aws_iot_log.h"

#include "client.h"
#include "job_dispatch_bootstrap.h"
#include "s3_http.h"


IoT_Error_t run(pdmp_dev_client pclient, pjob_dispatcher pdispatcher) {
    IoT_Error_t rc = FAILURE;

    IOT_INFO("subscribe to job topics")

    rc = dmp_dev_client_job_listen(pclient, pdispatcher);
    if (SUCCESS != rc) {
        IOT_ERROR("dmp_dev_client_job_listen returned error: %d", rc);
        return rc;
    }

    IOT_INFO("ask a job to process")

    rc = dmp_dev_client_job_ask(pclient);
    if (SUCCESS != rc) {
        IOT_ERROR("dmp_dev_client_job_ask returned error: %d", rc);
        return rc;
    }

    IOT_INFO("start job mqtt message handle loop")

    rc = dmp_dev_client_job_loop(pclient);
    if (SUCCESS != rc) {
        IOT_ERROR("dmp_dev_client_job_loop returned error: %d", rc);
    }

    return rc;
}

int main(int argc, char **argv) {
    pdmp_dev_client pclient = NULL;
    IoT_Error_t rc = FAILURE;

    s3_http_init();

    pjob_dispatcher pdispatcher = job_dispatcher_bootstrap();

    pclient = dmp_dev_client_create();
    if (NULL == pclient) {
        IOT_ERROR("dmp_dev_client_alloc returned NULL");
        goto ret;
    }

    rc = dmp_dev_client_init(pclient, AWS_IOT_MY_THING_NAME,
                             AWS_IOT_ROOT_CA_FILENAME, AWS_IOT_CERTIFICATE_FILENAME,
                             AWS_IOT_PRIVATE_KEY_FILENAME, AWS_IOT_MQTT_HOST, AWS_IOT_MQTT_PORT);
    if (SUCCESS != rc) {
        IOT_ERROR("dmp_dev_client_init returned error: %d", rc);

        dmp_dev_client_free(pclient);
        goto ret;
    }

    IOT_INFO("connecting to AWS IoT Core: %s:%d", AWS_IOT_MQTT_HOST, AWS_IOT_MQTT_PORT);

    rc = dmp_dev_client_connect(pclient, AWS_IOT_MQTT_CLIENT_ID);
    if (SUCCESS != rc) {
        IOT_ERROR("dmp_dev_client_connect returned error: %d", rc);

        dmp_dev_client_free(pclient);
        goto ret;
    }

    rc = run(pclient, pdispatcher);

    dmp_dev_client_free(pclient);

ret:
    s3_http_free();

    return rc;
}
