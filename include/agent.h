//
// Created by Zhi Yan Liu on 2019-07-11.
//

#ifndef NIGHTSWATCH_RANGER_AGENT_H_
#define NIGHTSWATCH_RANGER_AGENT_H_

#define NIGHTSWATCH_RANGER_HOME_DIR "../.."
#define NIGHTSWATCH_RANGER_BIN_DIR_NAME "bin"
#define NIGHTSWATCH_RANGER_BIN_DIR NIGHTSWATCH_RANGER_HOME_DIR"/"NIGHTSWATCH_RANGER_BIN_DIR_NAME
#define NIGHTSWATCH_RANGER_TARGET_CURRENT "ranger"
#define NIGHTSWATCH_RANGER_FILENAME "nightswatch-ranger"
#define NIGHTSWATCH_RANGER_PARTITION_1 "p1"
#define NIGHTSWATCH_RANGER_PARTITION_2 "p2"

#define NIGHTSWATCH_RANGER_PARTITION_NAME_LEN 32
#define NIGHTSWATCH_RANGER_VERSION_LEN 100


int agent_init();

int agent_cur_par_name(char *par_name, size_t par_name_l);

int agent_get_free_par_dir(char *file_path, size_t file_path_l, char *file_name, size_t file_name_l);

int agent_switch_par(char *new_file_path, size_t new_file_path_l, char *new_file_name, size_t new_file_name_l);

int agent_check_par_link();

int agent_reset_par_link();

int agent_par_name_valid(char *par_name);

#endif // NIGHTSWATCH_RANGER_AGENT_H_
