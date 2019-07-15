//
// Created by Liu, Zhiyan on 2019-05-30.
//

#include <string.h>

#include "certs.h"
#include "flag.h"


int flagged_update_dev_ca(int argc, char **argv, char **job_id, char** ca_par_name) {
    int i, has_job_id = 0, has_par_name = 0;
    char *given_par_name = NULL;

    for (i = 1; i < argc; i++) {
        if (0 == strcmp(argv[i], "--upd_dev_ca_job_id") && (i + 1) < argc) {
            *job_id = argv[i + 1];
            has_job_id = 1;
            i++;
        }

        if (0 == strcmp(argv[i], "--upd_dev_ca_par_name") && (i + 1) < argc) {
            *ca_par_name = argv[i + 1];
            has_par_name = 1;
            i++;
        }
    }

    if (0 == has_job_id || 0 == has_par_name)
        return 0; // no provided

    return certs_par_name_valid(*ca_par_name);
}
