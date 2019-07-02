//
// Created by Zhi Yan Liu on 2019-07-01.
//

#ifndef IROOTECH_DMP_RP_AGENT_APPS_H_
#define IROOTECH_DMP_RP_AGENT_APPS_H_

#define IROOTECH_DMP_RP_AGENT_APPS_DIR "apps"
#define IROOTECH_DMP_RP_AGENT_APP_ROOT_DIR "rootfs"
#define IROOTECH_DMP_RP_AGENT_APP_SPEC_FILE "config.json"
#define IROOTECH_DMP_RP_AGENT_APP_RUNC "runc"
#define IROOTECH_DMP_RP_AGENT_APP_CONSOLE_SOCK_DIR "console.socket"

#include <stddef.h>
#include <unistd.h>


typedef struct {
    char app_name[PATH_MAX + 1];
    int ctl_pipe_in[2], ctl_pipe_out[2]; // 0: r, 1: w;
} app_event_ctlr_param, *papp_event_ctlr_param;

typedef struct {
    char app_name[PATH_MAX + 1];
    int ctl_pipe_in[2], ctl_pipe_out[2]; // 0: r, 1: w;
} app_log_ctlr_param, *papp_log_ctlr_param;


int apps_path(char *apps_path, size_t apps_path_l);

int app_home_path(char *app_home_path_v, size_t app_home_path_l, char *app_name);

int app_root_path(char *app_root_path_v, size_t app_root_path_l, char *app_name);

int app_spec_tpl_path(char *app_spec_tpl_path_v, size_t app_spec_tpl_path_l);

int app_spec_path(char *app_spec_path_v, size_t app_spec_path_l, char *app_name);

int app_runc_path(char *app_runc_path_v, size_t app_runc_path_l);

int app_console_sock_path(char *console_sock_path_v, size_t console_sock_path_l, char *app_name);

int app_log_ctrlr_param_create(char *app_name, papp_log_ctlr_param *ppparam);

int app_log_ctrlr_param_free(papp_log_ctlr_param pparam);

int app_event_ctrlr_param_create(char *app_name, papp_event_ctlr_param *ppparam);

int app_event_ctrlr_param_free(papp_event_ctlr_param pparam);

int app_exists(char *app_name);

int app_deploy(char *app_name);

#endif //IROOTECH_DMP_RP_AGENT_APPS_H_
