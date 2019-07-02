//
// Created by Liu, Zhiyan on 2019-05-30.
//

#include <errno.h>
#include <stdio.h>
#if defined(__linux__)
  #include <linux/limits.h>
  #include <unistd.h>
  #include <sys/types.h>
#elif defined(__APPLE__)
  #include <mach-o/dyld.h>
  #include <sys/syslimits.h>
#endif

#include "utils.h"


int cur_pid_full_path(char *path, size_t path_l) {
    int rc = 0, count = 0;

    if (NULL == path)
        return 1;

#if defined(__linux__)
    rc = readlink("/proc/self/exe", path, path_l);
    if (-1 == rc)
        return 1;
    path[rc] = '\0';
#elif defined(__APPLE__)
    rc = _NSGetExecutablePath(path, (unsigned int*)&path_l);
    if (0 != rc)
        return 1;
#endif

    return 0;
}

size_t read_line(int fd, void *buffer, size_t n)
{
    size_t read_len; /* # of bytes fetched by last read() */
    size_t read_total; /* total bytes read so far */
    char *buf;
    char ch;

    if (n <= 0 || buffer == NULL) {
        errno = EINVAL;
        return -1;
    }

    buf = buffer; /* no pointer arithmetic on "void *" */

    read_total = 0;
    for (;;) {
        read_len = read(fd, &ch, 1);

        if (read_len == -1) {
            if (errno == EINTR) /* interrupted --> restart read() */
                continue;
            else
                return -1; /* Some other error */

        } else if (read_len == 0) { /* EOF */
            if (read_total == 0) /* no bytes read; return 0 */
                return 0;
            else /* some bytes read; add '\0' */
                break;

        } else { /* 'read_len' must be 1 if we get here */
            if (read_total < n - 1) { /* discard > (n - 1) bytes */
                read_total++;
                *buf++ = ch;
            }

            if (ch == '\n')
                break;
        }
    }

    *buf = '\0';
    return read_total;
}
