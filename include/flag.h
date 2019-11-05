//
// Created by Liu, Zhiyan on 2019-05-30.
//

#ifndef NIGHTSWATCH_RANGER_FLAG_H_
#define NIGHTSWATCH_RANGER_FLAG_H_

#include <stddef.h>

int flagged_update_dev_ca(int argc, char **argv, char **job_id, char** ca_par_name);

int flagged_ota_agent_pkg(int argc, char **argv, char **job_id, char** agent_par_name, char** agent_ver);

#endif // NIGHTSWATCH_RANGER_FLAG_H_
