//
// Created by Zhi Yan Liu on 2019-07-03.
//

#ifdef __linux__
#include <linux/limits.h>
#elif defined(__APPLE__)
#include <sys/syslimits.h>
#endif

#include <stdio.h>

#include "aws_iot_log.h"

#include "funcs.h"
#include "func_router.h"


int funcs_path(char *funcs_path_v, size_t funcs_path_l) {
    char work_dir_path[PATH_MAX + 1];

    if (NULL == funcs_path_v)
        return -1;

    getcwd(work_dir_path, PATH_MAX + 1);

    return snprintf(funcs_path_v, funcs_path_l, "%s/%s",
            work_dir_path, IROOTECH_DMP_RP_AGENT_FUNCS_DIR);
}

static pfunc_router prouter = NULL;

int funcs_bootstrap(AWS_IoT_Client *paws_iot_client) {
    int rc = 0;

    // router function
    rc = func_router_create(paws_iot_client, &prouter);
    if (0 != rc) {
        IOT_ERROR("failed to create function router: %d", rc);
        return rc;
    }

    IOT_DEBUG("function router created");

    rc = func_router_start(prouter);
    if (0 != rc) {
        func_router_free(prouter);
        IOT_ERROR("failed to start function router: %d", rc);
        return rc;
    }

    IOT_INFO("function router started");

    return rc;
}

int funcs_shutdown() {
    int rc = 0;

    // router function
    rc = func_router_stop(prouter);
    if (0 != rc) {
        IOT_ERROR("failed to stop function router: %d", rc);
        return rc;
    }

    IOT_DEBUG("function router stopped");

    rc = func_router_free(prouter);
    if (0 != rc) {
        IOT_ERROR("failed to release function router: %d", rc);
        return rc;
    }

    IOT_INFO("function router released");

    return rc;
}
