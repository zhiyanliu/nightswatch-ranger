//
// Created by Zhi Yan Liu on 2019-07-03.
//

#ifndef NIGHTSWATCH_RANGER_FUNCS_H_
#define NIGHTSWATCH_RANGER_FUNCS_H_

#include <stddef.h>
#include <unistd.h>

#define NIGHTSWATCH_RANGER_FUNCS_DIR "funcs"


int funcs_path(char *funcs_path, size_t funcs_path_l);

int funcs_bootstrap();

int funcs_shutdown();

#endif // NIGHTSWATCH_RANGER_FUNCS_H_
