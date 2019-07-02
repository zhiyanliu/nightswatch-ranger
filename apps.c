//
// Created by Zhi Yan Liu on 2019-07-02.
//

#include <errno.h>
#ifdef __linux__
#include <linux/limits.h>
#elif defined(__APPLE__)
#include <sys/syslimits.h>
#endif
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "aws_iot_log.h"

#include "utils.h"
#include "apps.h"


int apps_path(char *apps_path_v, size_t apps_path_v_l) {
    char work_dir_path[PATH_MAX + 1];

    if (NULL == apps_path_v)
        return -1;

    getcwd(work_dir_path, PATH_MAX + 1);

    return snprintf(apps_path_v, apps_path_v_l, "%s/%s",
             work_dir_path, IROOTECH_DMP_RP_AGENT_APPS_DIR);
}

int app_home_path(char *app_home_path_v, size_t app_home_path_l, char *app_name) {
    int len0 = 0, len1 = 0;

    if (NULL == app_home_path_v)
        return -1;

    if (NULL == app_name)
        return -1;

    len0 = apps_path(app_home_path_v, app_home_path_l);
    if (-1 == len0)
        return -1;

    len1 = snprintf(app_home_path_v + len0, app_home_path_l - len0, "/%s", app_name);
    if (-1 == len1)
        return -1;

    return len0 + len1;
}

int app_root_path(char *app_root_path_v, size_t app_root_path_l, char *app_name) {
    int len0 = 0, len1 = 0;

    if (NULL == app_root_path_v)
        return -1;

    len0 = app_home_path(app_root_path_v, app_root_path_l, app_name);
    if (-1 == len0)
        return -1;

    len1 = snprintf(app_root_path_v + len0, app_root_path_l - len0, "/%s", IROOTECH_DMP_RP_AGENT_APP_ROOT_DIR);
    if (-1 == len1)
        return -1;

    return len0 + len1;
}

int app_spec_tpl_path(char *app_spec_tpl_path_v, size_t app_spec_tpl_path_l) {
    int len0 = 0, len1 = 0;

    if (NULL == app_spec_tpl_path_v)
        return -1;

    len0 = apps_path(app_spec_tpl_path_v, app_spec_tpl_path_l);
    if (-1 == len0)
        return -1;

    len1 = snprintf(app_spec_tpl_path_v + len0, app_spec_tpl_path_l - len0, "/%s.tpl",
            IROOTECH_DMP_RP_AGENT_APP_SPEC_FILE);
    if (-1 == len1)
        return -1;

    return len0 + len1;
}

int app_spec_path(char *app_spec_path_v, size_t app_spec_path_l, char *app_name) {
    int len0 = 0, len1 = 0;

    if (NULL == app_spec_path_v)
        return -1;

    len0 = app_home_path(app_spec_path_v, app_spec_path_l, app_name);
    if (-1 == len0)
        return -1;

    len1 = snprintf(app_spec_path_v + len0, app_spec_path_l - len0, "/%s", IROOTECH_DMP_RP_AGENT_APP_SPEC_FILE);
    if (-1 == len1)
        return -1;

    return len0 + len1;
}

int app_runc_path(char *app_runc_path_v, size_t app_runc_path_l) {
    int len0 = 0, len1 = 0;

    if (NULL == app_runc_path_v)
        return -1;

    len0 = apps_path(app_runc_path_v, app_runc_path_l);
    if (-1 == len0)
        return -1;

    len1 = snprintf(app_runc_path_v + len0, app_runc_path_l - len0, "/%s", IROOTECH_DMP_RP_AGENT_APP_RUNC);
    if (-1 == len1)
        return -1;

    return len0 + len1;

}

int app_console_sock_path(char *console_sock_path_v, size_t console_sock_path_l, char *app_name) {
    int len0 = 0, len1 = 0;

    if (NULL == console_sock_path_v)
        return -1;

    len0 = app_home_path(console_sock_path_v, console_sock_path_l, app_name);
    if (-1 == len0)
        return -1;

    len1 = snprintf(console_sock_path_v + len0, console_sock_path_l - len0, "/%s",
            IROOTECH_DMP_RP_AGENT_APP_CONSOLE_SOCK_DIR);
    if (-1 == len1)
        return -1;

    return len0 + len1;
}

int app_exists(char *app_name) {
    char cmd[PATH_MAX * 2 + 20] = {0}, app_runc_path_v[PATH_MAX + 1], app_home_path_v[PATH_MAX + 1];
    int rc = 0;

    if (NULL == app_name)
        return 1;

    rc = app_runc_path(app_runc_path_v, PATH_MAX + 1);
    if (-1 == rc) {
        IOT_ERROR("failed to get application runc path: %d", rc);
        return 1;
    }

    rc = app_home_path(app_home_path_v, PATH_MAX + 1, app_name);
    if (-1 == rc) {
        IOT_ERROR("failed to get application home path: %d", rc);
        return 1;
    }

    snprintf(cmd, PATH_MAX * 2 + 20, "%s state %s > /dev/null 2>&1", app_runc_path_v, app_name);

    rc = system(cmd);
    return !rc;
}

int app_log_ctrlr_param_create(char *app_name, papp_log_ctlr_param *ppparam) {
    int rc = 0;

    if (NULL == app_name)
        return 1;

    if (NULL == ppparam)
        return 1;

    *ppparam = malloc(sizeof(app_log_ctlr_param));
    if (NULL == *ppparam)
        return 1;

    snprintf((*ppparam)->app_name, PATH_MAX + 1, "%s", app_name);

    rc = pipe((*ppparam)->ctl_pipe_in);
    if (0 != rc)
        return rc;

    rc = pipe((*ppparam)->ctl_pipe_out);

    return rc;
}

int app_log_ctrlr_param_free(papp_log_ctlr_param pparam) {
    if (NULL == pparam)
        return 1;

    close(pparam->ctl_pipe_in[0]);
    close(pparam->ctl_pipe_in[1]);

    close(pparam->ctl_pipe_out[0]);
    close(pparam->ctl_pipe_out[1]);

    free(pparam);

    return 0;
}

int app_event_ctrlr_param_create(char *app_name, papp_event_ctlr_param *ppparam) {
    int rc = 0;

    if (NULL == app_name)
        return 1;

    if (NULL == ppparam)
        return 1;

    *ppparam = malloc(sizeof(app_event_ctlr_param));
    if (NULL == *ppparam)
        return 1;

    snprintf((*ppparam)->app_name, PATH_MAX + 1, "%s", app_name);

    rc = pipe((*ppparam)->ctl_pipe_in);
    if (0 != rc)
        return rc;

    rc = pipe((*ppparam)->ctl_pipe_out);

    return rc;
}

int app_event_ctrlr_param_free(papp_event_ctlr_param pparam) {
    if (NULL == pparam)
        return 1;

    close(pparam->ctl_pipe_in[0]);
    close(pparam->ctl_pipe_in[1]);

    close(pparam->ctl_pipe_out[0]);
    close(pparam->ctl_pipe_out[1]);

    free(pparam);
}

// unused console way
static int app_console_sock(char *app_name) {
    char app_home_path_v[PATH_MAX + 1], console_sock_path_v[PATH_MAX + 1];
    int rc = 0, fd;
    struct sockaddr_un addr;

    if (NULL == app_name)
        return -1;

    rc = app_home_path(app_home_path_v, PATH_MAX + 1, app_name);
    if (-1 == rc) {
        IOT_ERROR("failed to get application home path: %d", rc);
        return -1;
    }

    rc = app_console_sock_path(console_sock_path_v, PATH_MAX + 1, app_name);
    if (-1 == rc) {
        IOT_ERROR("failed to get application console socket path: %d", rc);
        return -1;
    }

    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (-1 == fd) {
        IOT_ERROR("failed to create application console socket: %d", rc);
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;

    strncpy(addr.sun_path, console_sock_path_v, sizeof(addr.sun_path)-1);

    rc = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
    if (-1 == rc) {
        IOT_ERROR("failed to bind application console AF_UNIX socket: %d", rc);
        close(fd);
        return -1;
    }

    rc = listen(fd, 5);
    if (-1 == rc) {
        IOT_ERROR("failed to listen application console AF_UNIX socket: %d", rc);
        close(fd);
        return -1;
    }

    IOT_INFO("the container console socket of application created, at %s", console_sock_path_v);

    return fd;
}

static void* app_log_controller(void *p) {
    char cmd[PATH_MAX * 2 + 20] = {0}, app_runc_path_v[PATH_MAX + 1], app_home_path_v[PATH_MAX + 1];
    papp_log_ctlr_param pparam = (papp_log_ctlr_param)p;
    pid_t pid;
    int fd[2], rc = 0;

    if (NULL == pparam) {
        rc = 1;
        goto release;
    }

    rc = app_runc_path(app_runc_path_v, PATH_MAX + 1);
    if (-1 == rc) {
        IOT_ERROR("failed to get application runc path: %d", rc);
        rc = 1;
        goto release;
    }

    rc = app_home_path(app_home_path_v, PATH_MAX + 1, pparam->app_name);
    if (-1 == rc) {
        IOT_ERROR("failed to get application home path: %d", rc);
        rc = 1;
        goto release;
    }

    pipe(fd);
    pid = fork();
    if (pid == 0) // child
    {
        int ori_stdout = dup(STDOUT_FILENO), ori_stderr = dup(STDERR_FILENO);

        dup2(fd[1], STDOUT_FILENO);
        dup2(fd[1], STDERR_FILENO);
        close(fd[0]);
        close(fd[1]);

        rc = execlp(app_runc_path_v, app_runc_path_v, "run", "--bundle", app_home_path_v, pparam->app_name, NULL);
        if (0 != rc) {
            IOT_ERROR("[CRITICAL] failed to create and start container for application %s: %d",
                    pparam->app_name, errno);
            exit(rc);
        }

        close(STDOUT_FILENO);
        close(STDERR_FILENO);

        // restore stdout
        dup2(ori_stdout, STDOUT_FILENO);
        close(ori_stdout);

        // restore stderr
        dup2(ori_stderr, STDERR_FILENO);
        close(ori_stderr);

        IOT_INFO("container of application %s exited", pparam->app_name);

        exit(0); // child process exits
    } else { // parent
        fd_set fds;
        int max_fd;
        char log[2048];
        size_t read_len = 0;

        close(fd[1]);

        while (1) {
            FD_ZERO(&fds);
            FD_SET(fd[0], &fds);
            FD_SET(pparam->ctl_pipe_in[0], &fds);

            max_fd = fd[0] > pparam->ctl_pipe_in[0] ? fd[0] : pparam->ctl_pipe_in[0];

            switch (select(max_fd + 1, &fds, NULL, NULL, NULL)) {
                case -1: // select error
                    IOT_ERROR("[CRITICAL] failed to listen container for application %s", pparam->app_name);
                    sleep(2);
                    break;
                case 0:
                    break; // doesn't make sense, re-listen
                default:
                    if (FD_ISSET(fd[0], &fds)) {
                        read_len = read(fd[0], log, 2048);

                        if (0 == read_len) { // EOF, container exits?
                            sleep(1); // defence
                            continue;
                        }

                        // send to Cloud
                        IOT_DEBUG("LOG (%d %d) ========> \"%s\"", read_len, errno, log);
                    } else if (FD_ISSET(pparam->ctl_pipe_in[0], &fds)) { // control
                        IOT_INFO("========= stop");
                        goto release;
                    }
            }
        }
    }

release:
    app_log_ctrlr_param_free(pparam);

    return NULL; // rc == 0
}


static void* app_event_controller(void *p) {
    char cmd[PATH_MAX * 2 + 20] = {0}, app_runc_path_v[PATH_MAX + 1];
    papp_event_ctlr_param pparam = (papp_event_ctlr_param)p;
    pid_t pid;
    int fd[2], rc = 0;

    if (NULL == pparam) {
        rc = 1;
        goto release;
    }

    rc = app_runc_path(app_runc_path_v, PATH_MAX + 1);
    if (-1 == rc) {
        IOT_ERROR("failed to get application runc path: %d", rc);
        rc = 1;
        goto release;
    }

    pipe(fd);
    pid = fork();
    if (pid == 0) // child
    {
        int ori_stdout = dup(STDOUT_FILENO);

        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]);
        close(fd[1]);

        // stats collection interval default: 5s
        rc = execlp(app_runc_path_v, app_runc_path_v, "events", pparam->app_name, NULL);
        if (0 != rc) {
            IOT_ERROR("[CRITICAL] failed to query container event for application %s: %d", pparam->app_name, errno);
            exit(rc);
        }

        close(STDOUT_FILENO);

        // restore stdout
        dup2(ori_stdout, STDOUT_FILENO);
        close(ori_stdout);

        IOT_INFO("container of application %s exited", pparam->app_name);

        exit(0); // child process exits
    } else { // parent
        fd_set fds;
        int max_fd;
        char events[2048];
        size_t read_len = 0;

        close(fd[1]);

        while (1) {
            FD_ZERO(&fds);
            FD_SET(fd[0], &fds);
            FD_SET(pparam->ctl_pipe_in[0], &fds);

            max_fd = fd[0] > pparam->ctl_pipe_in[0] ? fd[0]: pparam->ctl_pipe_in[0];

            switch (select(max_fd + 1, &fds, NULL, NULL, NULL)) {
                case -1: // select error
                    IOT_ERROR("[CRITICAL] failed to listen container for application %s", pparam->app_name);
                    break;
                case 0:
                    break; // doesn't make sense, re-listen
                default:
                    if (FD_ISSET(fd[0], &fds)) {
                        read_len = read_line(fd[0], events, 2048);

                        if (0 == read_len) // EOF
                            // container doesn't provide event, e.g. not running
                            sleep(2);

                        // send to Cloud
                        IOT_DEBUG("EVENTS (%d %d) ========> %s", read_len, errno, events);
                    } else if (FD_ISSET(pparam->ctl_pipe_in[0], &fds)) { // control
                        IOT_INFO("========= stop")
                        goto release;
                    }
            }
        }
    }

release:
    app_event_ctrlr_param_free(pparam);

    return NULL; // rc == 0
}

int app_deploy(char *app_name) {
    papp_log_ctlr_param pparam_log;
    papp_event_ctlr_param pparam_event;

    pthread_t app_log_ctl_thd, app_event_ctl_thd;
    int rc = 0, cnt = 0;

    if (NULL == app_name)
        return 1;

    rc = app_log_ctrlr_param_create(app_name, &pparam_log);
    if (0 != rc) {
        IOT_ERROR("failed to create log controller param for application %s: %d", app_name, rc);
        return rc;
    }

    rc = pthread_create(&app_log_ctl_thd, NULL, app_log_controller, pparam_log);
    if (0 != rc) {
        IOT_ERROR("failed to create log controller for application %s: %d", app_name, rc);
        app_log_ctrlr_param_free(pparam_log);
        return rc;
    }

    // wait controller ready
    while (0 == app_exists(app_name)) {
        if (cnt++ < 5)
            sleep(1);
        else
            return 1;
    }

    rc = app_event_ctrlr_param_create(app_name, &pparam_event);
    if (0 != rc) {
        IOT_ERROR("failed to create event controller param for application %s: %d", app_name, rc);
        return rc;
    }

    rc = pthread_create(&app_event_ctl_thd, NULL, app_event_controller, pparam_event);
    if (0 != rc) {
        IOT_ERROR("failed to create event controller for application %s: %d", app_name, rc);
        app_event_ctrlr_param_free(pparam_event);
        return rc;
    }

    IOT_INFO("controller created for application %s", app_name);

    return rc;
}
