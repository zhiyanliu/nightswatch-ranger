//
// Created by Zhi Yan Liu on 2019-05-28.
//

#include <stddef.h>

#ifndef IROOTECH_DMP_RP_AGENT_CERTS_H_
#define IROOTECH_DMP_RP_AGENT_CERTS_H_

#define IROOTECH_DMP_RP_AGENT_CERTS_DIR "certs"
#define IROOTECH_DMP_RP_AGENT_CERTS_PARTITION_CURRENT "latest"
#define IROOTECH_DMP_RP_AGENT_CERTS_PARTITION_1 "p1"
#define IROOTECH_DMP_RP_AGENT_CERTS_PARTITION_2 "p2"

#define PARTITION_NAME_LEN 32


int certs_init();

int certs_cur_par_name(char *par_name, size_t par_name_l);

int certs_get_free_par_dir(char *file_path, size_t file_path_l, char *file_name, size_t file_name_l);

int certs_switch_par(char *new_file_path, size_t new_file_path_l, char *new_file_name, size_t new_file_name_l);

int certs_check_par_link();

int certs_reset_par_link();

int certs_par_name_valid(char *par_name);

#endif //IROOTECH_DMP_RP_AGENT_CERTS_H_
