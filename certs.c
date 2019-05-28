//
// Created by Zhi Yan Liu on 2019-05-28.
//

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
#include "certs.h"


int certs_cur_par_name(char *par_name, size_t par_name_l) {
    char work_dir_path[PATH_MAX + 1], link_file_path[PATH_MAX + 1], *filename;
    struct stat buf;
    int rc;

    if (NULL == par_name)
        return 0;

    if (0 == par_name_l)
        return 0;

    getcwd(work_dir_path, PATH_MAX + 1);

    snprintf(link_file_path, PATH_MAX + 1, "%s/%s/%s",
             work_dir_path, IROOTECH_DMP_RP_AGENT_CERTS_DIR, IROOTECH_DMP_RP_AGENT_CERTS_PARTITION_CURRENT);

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

int certs_get_free_par_dir(char *file_path, size_t file_path_l) {
    char cur_par_name[PATH_MAX + 1], work_dir_path[PATH_MAX + 1], *par_name = NULL;
    int rc = 0;
    size_t len = 0;

    rc = certs_cur_par_name(cur_par_name, PATH_MAX + 1);
    if (0 != rc)
        return rc;

    getcwd(work_dir_path, PATH_MAX + 1);

    len = file_path_l > PATH_MAX + 1 ? PATH_MAX + 1 : file_path_l; // min

    if (0 == strncmp(IROOTECH_DMP_RP_AGENT_CERTS_PARTITION_1, cur_par_name, len))
        par_name = IROOTECH_DMP_RP_AGENT_CERTS_PARTITION_2;
    else if (0 == strncmp(IROOTECH_DMP_RP_AGENT_CERTS_PARTITION_2, cur_par_name, len))
        par_name = IROOTECH_DMP_RP_AGENT_CERTS_PARTITION_1;
    else
        return 1;

    snprintf(file_path, file_path_l, "%s/%s/%s",
             work_dir_path, IROOTECH_DMP_RP_AGENT_CERTS_DIR, par_name);

    return 0;
}
