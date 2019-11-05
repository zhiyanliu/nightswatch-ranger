//
// Created by Liu, Zhiyan on 2019-05-30.
//

#ifndef NIGHTSWATCH_RANGER_UTILS_H_
#define NIGHTSWATCH_RANGER_UTILS_H_

#include <stddef.h>


size_t read_line(int fd, void *buffer, size_t n);

char* read_str_file(char *path, int *len);

#endif // NIGHTSWATCH_RANGER_UTILS_H_
