//
// Created by Zhi Yan Liu on 2019/9/23.
//

#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "aws_iot_log.h"

#include "rund.h"


int main(int argc, char **argv) {
    int rc;
    char *self_path;

    setvbuf(stdout, NULL, _IOLBF, 0);
    setvbuf(stderr, NULL, _IOLBF, 0);

    self_path = realpath(argv[0], 0);
    if (NULL == self_path) {
        IOT_ERROR("failed to get current process path");
        return rc;
    }

    rc = chdir(dirname(self_path));
    if (0 != rc) {
        IOT_ERROR("failed to change process working path");
        return rc;
    }

    // ['rund', 'run', '--bundle', '/nightswatch/apps/app_name',
    //  '--pid-file', '/nightswatch/apps/app_name/pid', 'app_name']
    if (7 == argc && 0 == strncmp(argv[1], "run", 4) &&
            0 == strncmp(argv[2], "--bundle", 9) &&
            0 == strncmp(argv[4], "--pid-file", 11))
        return app_run(argv[3], argv[5], argv[6]);

    // ['rund', 'events', 'app_name']
    if (3 == argc && 0 == strncmp(argv[1], "events", 7))
        return app_events(argv[2]);

    // ['rund', 'state', 'app_name']
    if (3 == argc && 0 == strncmp(argv[1], "state", 6))
        return app_state(argv[2]);

    // unknown arguments
    return 1;
}
