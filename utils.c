//
// Created by Liu, Zhiyan on 2019-05-30.
//

#if defined(__linux__)
  #include <unistd.h>
#elif defined(__APPLE__)
  #include <mach-o/dyld.h>
#endif

#include "utils.h"

int cur_pid_full_path(char *path, size_t path_l) {
    int pid, rc = 0, count = 0;

    if (NULL == path)
        return 1;

#if defined(__linux__)
     pid = getpid();
    oss << "/proc/" << pid << "/exe";
    std::string link = oss.str();

    rc = readlink("/proc/self/exe", path, path_l);
    if (-1 == rc)
        return 1;
#elif defined(__APPLE__)
    rc = _NSGetExecutablePath(path, (unsigned int*)&path_l);
    if (0 != rc)
        return 1;
#endif

    return 0;
}
