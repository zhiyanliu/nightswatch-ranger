//
// Created by Liu, Zhiyan on 2019-05-30.
//

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "utils.h"


size_t read_line(int fd, void *buffer, size_t n)
{
    int read_len; /* # of bytes fetched by last read() */
    int read_total; /* total bytes read so far */
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

char* read_str_file(char *path, int *len)
{
    FILE *f;
    char *data;

    f = fopen(path, "rb");
    if (NULL == f)
        return NULL;

    fseek(f, 0, SEEK_END);
    *len = ftell(f);

    data = (char*)malloc((*len + 1) * sizeof(char));
    if (NULL == data) {
        fclose(f);
        return NULL;
    }

    rewind(f);
    *len = fread(data, 1, *len, f);
    data[*len] = '\0';

    fclose(f);
    return data;
}
