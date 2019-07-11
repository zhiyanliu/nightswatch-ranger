//
// Created by Zhi Yan Liu on 2019-07-11.
//

#include <errno.h>
#include <libgen.h>
#ifdef __linux__
#include <linux/limits.h>
#elif defined(__APPLE__)
#include <sys/syslimits.h>
#endif
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "aws_iot_log.h"

#include "agent.h"


int agent_init() {
    int rc = 0;

    rc = agent_check_par_link();
    if (0 == rc)
        return rc;

    IOT_WARN("invalid agent partition detected, reset to default one - par#1");

    rc = agent_reset_par_link();
    if (0 == rc) {
        IOT_INFO("agent partition has been fixed");
    } else {
        IOT_ERROR("failed to reset agent partition: %d", rc);
    }

    return rc;
}

int agent_cur_par_name(char *par_name, size_t par_name_l) {
    char work_dir_path[PATH_MAX + 1], link_file_path[PATH_MAX + 1], *filename;
    struct stat buf;
    int rc;

    if (NULL == par_name)
        return 0;

    if (0 == par_name_l)
        return 0;

    getcwd(work_dir_path, PATH_MAX + 1);

    snprintf(link_file_path, PATH_MAX + 1, "%s/%s/%s",
            work_dir_path, IROOTECH_DMP_RP_AGENT_AGENT_DIR, IROOTECH_DMP_RP_AGENT_AGENT_TARGET_CURRENT);

    rc = lstat(link_file_path, &buf);
    if (0 != rc)
        return rc;

    if (!S_ISLNK(buf.st_mode))
        return 1;

    rc = readlink(link_file_path, par_name, par_name_l);
    if (rc >= 0) {
        par_name[rc] = '\0';
    } else {
        return rc;
    }

    snprintf(par_name, par_name_l, "%s",
            basename(par_name)); // internal statically allocated result buffer, can not be concurrent

    return 0;
}

int agent_get_free_par_dir(char *file_path, size_t file_path_l, char *file_name, size_t file_name_l) {
    char cur_par_name[PATH_MAX + 1], work_dir_path[PATH_MAX + 1], *par_name = NULL;
    int rc = 0;

    rc = agent_cur_par_name(cur_par_name, PATH_MAX + 1);
    if (0 != rc)
        return rc;

    getcwd(work_dir_path, PATH_MAX + 1);

    if (0 == strncmp(IROOTECH_DMP_RP_AGENT_AGENT_PARTITION_1, cur_par_name, 2))
        par_name = IROOTECH_DMP_RP_AGENT_AGENT_PARTITION_2;
    else if (0 == strncmp(IROOTECH_DMP_RP_AGENT_AGENT_PARTITION_2, cur_par_name, 2))
        par_name = IROOTECH_DMP_RP_AGENT_AGENT_PARTITION_1;
    else
        return 1; // link to invalid partition dir

    snprintf(file_path, file_path_l, "%s/%s/%s",
             work_dir_path, IROOTECH_DMP_RP_AGENT_AGENT_DIR, par_name);

    if (NULL != file_name) {
        snprintf(file_name, file_name_l, "%s", par_name);
    }

    return 0;
}

int agent_switch_par(char *new_file_path, size_t new_file_path_l, char *new_file_name, size_t new_file_name_l) {
    char work_dir_path[PATH_MAX + 1], free_par_name[PATH_MAX + 1], link_file_path[PATH_MAX + 1],
            target_file_path[PATH_MAX + 1], target_file_name[PATH_MAX + 1];
    int rc = 0;

    rc = agent_get_free_par_dir(free_par_name, PATH_MAX + 1, target_file_name, PATH_MAX + 1);
    if (0 != rc)
        return rc;

    getcwd(work_dir_path, PATH_MAX + 1);

    rc = snprintf(target_file_path, PATH_MAX + 1, "./%s", target_file_name);
    if (0 == rc)
        return 1;

    rc = snprintf(link_file_path, PATH_MAX + 1, "%s/%s/%s",
            work_dir_path, IROOTECH_DMP_RP_AGENT_AGENT_DIR, IROOTECH_DMP_RP_AGENT_AGENT_TARGET_CURRENT);
    if (0 == rc)
        return 1;

    rc = unlink(link_file_path);
    if (0 != rc) { // OMG, no worries, can reset
        return rc;
    }

    rc = symlink(target_file_path, link_file_path);
    if (0 != rc)
        return rc;

    if (NULL != new_file_path) {
        snprintf(new_file_path, new_file_path_l, "%s/%s/%s",
                work_dir_path, IROOTECH_DMP_RP_AGENT_AGENT_DIR, target_file_name);
    }

    if (NULL != new_file_name) {
        snprintf(new_file_name, new_file_name_l, "%s", target_file_name);
    }

    return rc;
}

int agent_check_par_link() {
    char cur_par_name[PATH_MAX + 1];
    int rc = 0;

    rc = agent_cur_par_name(cur_par_name, PATH_MAX + 1);
    if (0 != rc)
        return rc;

    if (0 == strncmp(IROOTECH_DMP_RP_AGENT_AGENT_PARTITION_1, cur_par_name, 2))
        return 0;
    else if (0 == strncmp(IROOTECH_DMP_RP_AGENT_AGENT_PARTITION_2, cur_par_name, 2))
        return 0;
    return 1;
}

int agent_reset_par_link() {
    char work_dir_path[PATH_MAX + 1], link_file_path[PATH_MAX + 1],
            target_file_path[PATH_MAX + 1], target_file_name[PATH_MAX + 1];
    int rc = 0;

    rc = snprintf(target_file_path, PATH_MAX + 1, "./%s", IROOTECH_DMP_RP_AGENT_AGENT_PARTITION_1);
    if (0 == rc)
        return 1;

    getcwd(work_dir_path, PATH_MAX + 1);

    rc = snprintf(link_file_path, PATH_MAX + 1, "%s/%s/%s",
            work_dir_path, IROOTECH_DMP_RP_AGENT_AGENT_DIR, IROOTECH_DMP_RP_AGENT_AGENT_TARGET_CURRENT);
    if (0 == rc)
        return 1;

    rc = unlink(link_file_path); // no directory handle currently, due to it's not a normal failure case
    if (0 != rc && ENOENT != errno) {
        return rc; // hmm..
    }

    rc = symlink(target_file_path, link_file_path);
    if (0 != rc)
        return rc; // oops..

    return rc;
}
