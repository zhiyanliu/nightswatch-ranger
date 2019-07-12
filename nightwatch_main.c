//
// Created by Zhi Yan Liu on 2019-07-12.
//

#include <errno.h>
#ifdef __linux__
#include <linux/limits.h>
#elif defined(__APPLE__)
#include <sys/syslimits.h>
#endif
#include <libgen.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "aws_iot_log.h"

#include "agent.h"
#include "flag.h"
#include "utils.h"


static int to_ota_agent_pkg(int argc, char **argv, char **ota_agent_pkg_job_id, char **ota_agent_ver) {
    int rc = 0;
    char *flag_agent_par_name, cur_par_name[PATH_MAX + 1];

    if (NULL == ota_agent_pkg_job_id)
        return 0;

    if (NULL == ota_agent_ver)
        return 0;

    rc = flagged_ota_agent_pkg(argc, argv, ota_agent_pkg_job_id, &flag_agent_par_name, ota_agent_ver);
    if (0 == rc) { // flag not provided
        return 0;
    }

    rc = agent_cur_par_name(cur_par_name, PATH_MAX + 1);
    if (0 != rc) {
        IOT_ERROR("failed to get current agent partition name: %d", rc);
        return 0;
    }

    rc = strncmp(cur_par_name, flag_agent_par_name, PATH_MAX + 1);
    if (0 != rc) {
        IOT_WARN("invalid flag of agent partition name provided, current: %s, flag: %s, skip update device agent",
                cur_par_name, flag_agent_par_name);
        return 0;
    }

    return 1;
}

int main(int argc, char **argv) {
    int rc = 0, ota_agent_pkg = 0;
    char *ota_agent_pkg_job_id = NULL, *ota_agent_ver, self_path[PATH_MAX + 1];

    // disable buffering for logging
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    IOT_INFO("===============(nightwatch)===============");

    rc = setuid(0) + setgid(0);
    if (0 != rc) {
        IOT_ERROR("need root permission, try sudo");
        return rc;
    }

    rc = cur_pid_full_path(self_path, PATH_MAX + 1);
    if (0 != rc) {
        IOT_ERROR("failed to get current process path");
        return rc;
    }

    rc = chdir(dirname(self_path));
    if (0 != rc) {
        IOT_ERROR("failed to change process working path");
        return rc;
    }

    rc = agent_init("..");
    if (0 != rc) {
        IOT_ERROR("failed to init certs");
        return rc;
    }

    ota_agent_pkg = to_ota_agent_pkg(argc, argv, &ota_agent_pkg_job_id, &ota_agent_ver);
    if (0 == ota_agent_pkg) {
        IOT_DEBUG("internal use only")
        return 1;
    }

    IOT_DEBUG("nightwatch continues to apply new agent, job id: %s, agent version: %s",
            ota_agent_pkg_job_id, ota_agent_ver);



    return 0;

}