//
// Created by Zhi Yan Liu on 2019-05-28.
//

#include <linux/limits.h>
#include <stdio.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "certs.h"


int certs_cur_par_name(char *par_name, size_t par_name_l) {
    char work_dir_path[PATH_MAX + 1], link_file_path[PATH_MAX + 1];
    struct stat buf;
    int rc;

    if (NULL == par_name)
        return 0;

    if (0 == par_name_l)
        return 0;

    getcwd(work_dir_path, PATH_MAX + 1);

    snprintf(link_file_path, PATH_MAX + 1, "%s/%s/%s",
             work_dir_path, IROOTECH_DMP_RP_AGENT_CERTS_DIR, IROOTECH_DMP_RP_AGENT_CERTS_PARTITION_CURRENT);

    rc = stat(link_file_path, &buf);
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

    return 0;
}
