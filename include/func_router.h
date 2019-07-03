//
// Created by Zhi Yan Liu on 2019-07-03.
//

#ifndef IROOTECH_DMP_RP_AGENT_FUNC_ROUTER_H_
#define IROOTECH_DMP_RP_AGENT_FUNC_ROUTER_H_

#include <stddef.h>
#include <unistd.h>

#include "aws_iot_mqtt_client_interface.h"


#define IROOTECH_DMP_RP_AGENT_FUNC_ROUTER_HOME_DIR "router"
#define IROOTECH_DMP_RP_AGENT_FUNC_ROUTER_IO_SOCK_PORT 9000


typedef struct {
    int io_listen_fd;
    int ctl_pipe_in[2], ctl_pipe_out[2]; // 0: r, 1: w;
    AWS_IoT_Client *paws_iot_client;
} func_router, *pfunc_router;

typedef struct {
    int conn_fd;
    AWS_IoT_Client *paws_iot_client;
} func_router_worker_param, *pfunc_router_worker_param;


int func_router_home_path(char *func_router_home_path_v, size_t func_router_home_path_l);

int func_router_create(AWS_IoT_Client *paws_iot_client, pfunc_router *pprouter);

int func_router_free(pfunc_router prouter);

int func_router_start(pfunc_router prouter);

int func_router_stop(pfunc_router prouter);

#endif // IROOTECH_DMP_RP_AGENT_FUNC_ROUTER_H_
