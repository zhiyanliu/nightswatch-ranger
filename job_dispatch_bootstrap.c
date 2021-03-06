//
// Created by Liu, Zhiyan on 2019-05-23.
//

#include <string.h>

#include "job_dispatch.h"
#include "job_dispatch_bootstrap.h"
#include "job_op_types.h"

#include "op_deploy_app_pkg.h"
#include "op_destroy_app_pkg.h"
#include "op_ota_sys_pkg.h"
#include "op_ota_agent_pkg.h"
#include "op_ota_agent_plug.h"
#include "op_remote_op_exec.h"
#include "op_update_agent_cfg.h"
#include "op_update_dev_ca.h"


job_dispatcher job_dispatcher_default = {NULL, 0};

pjob_dispatcher job_dispatcher_bootstrap() {
    // System package OTA operation
    job_dispatcher_reg_executor(&job_dispatcher_default,
            JOB_OP_OTA_SYS_PKG_STR, strlen(JOB_OP_OTA_SYS_PKG_STR) + 1, op_ota_sys_pkg_entry);

    // Agent package OTA operation
    job_dispatcher_reg_executor(&job_dispatcher_default,
            JOB_OP_OTA_AGENT_PKG_STR, strlen(JOB_OP_OTA_AGENT_PKG_STR) + 1, op_ota_agent_pkg_entry);

    // Device certificate update operation
    job_dispatcher_reg_executor(&job_dispatcher_default,
            JOB_OP_UPDATE_DEV_CA_STR, strlen(JOB_OP_UPDATE_DEV_CA_STR) + 1, op_update_dev_ca_entry);

    // Deploy application package
    job_dispatcher_reg_executor(&job_dispatcher_default,
            JOB_DEPLOY_APP_PKG_STR, strlen(JOB_DEPLOY_APP_PKG_STR) + 1, op_deploy_app_pkg_entry);

    // Destroy application package
    job_dispatcher_reg_executor(&job_dispatcher_default,
            JOB_DESTROY_APP_PKG_STR, strlen(JOB_DESTROY_APP_PKG_STR) + 1, op_destroy_app_pkg_entry);

    return &job_dispatcher_default;
}
