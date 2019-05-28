//
// Created by Zhi Yan Liu on 2019-05-28.
//

#ifndef IROOTECH_DMP_RP_AGENT_CERTS_H_
#define IROOTECH_DMP_RP_AGENT_CERTS_H_

#define IROOTECH_DMP_RP_AGENT_CERTS_DIR "certs"
#define IROOTECH_DMP_RP_AGENT_CERTS_PARTITION_CURRENT "latest"
#define IROOTECH_DMP_RP_AGENT_CERTS_PARTITION_1 "p1"
#define IROOTECH_DMP_RP_AGENT_CERTS_PARTITION_2 "p2"

#define PARTITION_NAME_LEN 32

#include <stddef.h>

int certs_cur_par_name(char *par_name, size_t par_name_l);

#endif //IROOTECH_DMP_RP_AGENT_CERTS_H_
