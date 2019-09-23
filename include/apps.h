//
// Created by Zhi Yan Liu on 2019-07-01.
//

#ifndef IROOTECH_DMP_RP_AGENT_APPS_H_
#define IROOTECH_DMP_RP_AGENT_APPS_H_

#include <stddef.h>
#include <unistd.h>

#include "aws_iot_mqtt_client_interface.h"


#define IROOTECH_DMP_RP_AGENT_APPS_DIR "../../apps"
#define IROOTECH_DMP_RP_AGENT_APP_ROOT_DIR "rootfs"
#define IROOTECH_DMP_RP_AGENT_APP_SPEC_FILE "config.json"
#define IROOTECH_DMP_RP_AGENT_APP_PROCESS_PID_FILE "pid"

#define IROOTECH_DMP_RP_AGENT_APP_LAUNCHER_TYPE_RUNC (1)
#define IROOTECH_DMP_RP_AGENT_APP_LAUNCHER_TYPE_RUND (2)
#define IROOTECH_DMP_RP_AGENT_APP_LAUNCHER_NAME_RUNC "runc"
#define IROOTECH_DMP_RP_AGENT_APP_LAUNCHER_NAME_RUND "rund"


typedef struct {
    int launcher_type;
    char app_name[PATH_MAX + 1];
    int ctl_pipe_in[2], ctl_pipe_out[2]; // 0: r, 1: w;
    AWS_IoT_Client *paws_iot_client;
} app_event_ctlr_param, *papp_event_ctlr_param;

typedef struct {
    int launcher_type;
    char app_name[PATH_MAX + 1];
    int ctl_pipe_in[2], ctl_pipe_out[2]; // 0: r, 1: w;
    AWS_IoT_Client *paws_iot_client;
} app_log_ctlr_param, *papp_log_ctlr_param;


int apps_path(char *apps_path, size_t apps_path_l);

int app_home_path(char *app_home_path_v, size_t app_home_path_l, char *app_name);

int app_root_path(char *app_root_path_v, size_t app_root_path_l, char *app_name);

int app_spec_tpl_path(char *app_spec_tpl_path_v, size_t app_spec_tpl_path_l, int launcher_type);

int app_spec_path(char *app_spec_path_v, size_t app_spec_path_l, char *app_name);

int app_launcher_path(char *app_launcher_path_v, size_t app_launcher_path_l, int launcher_type);

int app_process_pid_path(char *process_pid_path_v, size_t process_pid_path_l, char *app_name);

int app_log_ctrlr_param_create(int launcher_type, char *app_name, AWS_IoT_Client *paws_iot_client,
        papp_log_ctlr_param *ppparam);

int app_log_ctrlr_param_free(papp_log_ctlr_param pparam);

int app_event_ctrlr_param_create(int launcher_type, char *app_name, AWS_IoT_Client *paws_iot_client,
        papp_event_ctlr_param *ppparam);

int app_event_ctrlr_param_free(papp_event_ctlr_param pparam);

int app_exists(char *app_name, int launcher_type);

int app_pid(char *app_name, int wait_app, int launcher_type);

int app_deploy(char *app_name, AWS_IoT_Client *paws_iot_client, int launcher_type);

int app_destroy(char *app_name, int launcher_type);

int app_send_signal(int pid, int signo);

int apps_send_signal(int signo);

int app_kill();

int apps_kill();

#endif // IROOTECH_DMP_RP_AGENT_APPS_H_
