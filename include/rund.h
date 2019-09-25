//
// Created by Zhi Yan Liu on 2019/9/23.
//

#ifndef IROOTECH_DMP_RP_AGENT_RUND_H_
#define IROOTECH_DMP_RP_AGENT_RUND_H_

#define IROOTECH_DMP_RP_RUND_APP_SPEC_MAX 512
#define IROOTECH_DMP_RP_RUND_APP_PROCESS_MAX 256

#define RUND_SPEC_APP_PROCESS_PROPERTY_NAME "process"
#define RUND_SPEC_APP_ARGUMENTS_PROPERTY_NAME "args"


int app_run(char *app_home_path_v, char *app_process_pid_path_v, char *app_name);

int app_events(char *app_name);

int app_state(char *app_name);

#endif // IROOTECH_DMP_RP_AGENT_RUND_H_
