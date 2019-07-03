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
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "aws_iot_log.h"
#include "aws_iot_mqtt_client_interface.h"

#include "utils.h"
#include "apps.h"


// TODO(zhiyan): dynamic allocate the list
static int apps_deployed_pid_v[32] = {0};
static size_t apps_deployed_pid_l = 32;

static int add_app_deployed_pid(int pid) {
    size_t i = 0;

    for (i = 0; i < apps_deployed_pid_l; i++) {
        if (0 == apps_deployed_pid_v[i]) {
            apps_deployed_pid_v[i] = pid;
            return 0;
        }
    }

    return 1;
}

static int del_app_deployed_pid(int pid) {
    size_t i = 0;

    for (i = 0; i < apps_deployed_pid_l; i++) {
        if (pid == apps_deployed_pid_v[i]) {
            apps_deployed_pid_v[i] = 0;
            return 0;
        }
    }

    return 1;
}

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

// unused console way, currently
int app_console_sock_path(char *console_sock_path_v, size_t console_sock_path_l, char *app_name) {
    int len0 = 0, len1 = 0;

    if (NULL == console_sock_path_v)
        return -1;

    len0 = app_home_path(console_sock_path_v, console_sock_path_l, app_name);
    if (-1 == len0)
        return -1;

    len1 = snprintf(console_sock_path_v + len0, console_sock_path_l - len0, "/%s",
            IROOTECH_DMP_RP_AGENT_APP_CONSOLE_SOCK_FILE);
    if (-1 == len1)
        return -1;

    return len0 + len1;
}

int app_container_pid_path(char *container_pid_path_v, size_t container_pid_path_l, char *app_name) {
    int len0 = 0, len1 = 0;

    if (NULL == container_pid_path_v)
        return -1;

    len0 = app_home_path(container_pid_path_v, container_pid_path_l, app_name);
    if (-1 == len0)
        return -1;

    len1 = snprintf(container_pid_path_v + len0, container_pid_path_l - len0, "/%s",
            IROOTECH_DMP_RP_AGENT_APP_CONTAINER_PID_FILE);
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

int app_log_ctrlr_param_create(char *app_name, AWS_IoT_Client *paws_iot_client, papp_log_ctlr_param *ppparam) {
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
    if (0 != rc) {
        free(*ppparam);
        return rc;
    }

    rc = pipe((*ppparam)->ctl_pipe_out);
    if (0 != rc) {
        free(*ppparam);
        close((*ppparam)->ctl_pipe_in[0]);
        close((*ppparam)->ctl_pipe_in[1]);
        return rc;
    }

    (*ppparam)->paws_iot_client = paws_iot_client;

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

int app_event_ctrlr_param_create(char *app_name, AWS_IoT_Client *paws_iot_client, papp_event_ctlr_param *ppparam) {
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
    if (0 != rc) {
        free(*ppparam);
        return rc;
    }

    rc = pipe((*ppparam)->ctl_pipe_out);
    if (0 != rc) {
        close((*ppparam)->ctl_pipe_in[0]);
        close((*ppparam)->ctl_pipe_in[1]);
        free(*ppparam);
        return rc;
    }

    (*ppparam)->paws_iot_client = paws_iot_client;

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

// unused console way, currently
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
    char cmd[PATH_MAX * 2 + 20] = {0}, app_runc_path_v[PATH_MAX + 1], app_home_path_v[PATH_MAX + 1],
        app_container_pid_path_v[PATH_MAX + 1];
    papp_log_ctlr_param pparam = (papp_log_ctlr_param)p;

    pid_t pid;
    int fd[2], rc = 0, container_pid;

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

    rc = app_container_pid_path(app_container_pid_path_v, PATH_MAX + 1, pparam->app_name);
    if (-1 == rc) {
        IOT_ERROR("failed to get application pid file path: %d", rc);
        rc = 1;
        goto release;
    }

    pipe(fd);
    pid = fork();
    if (pid == 0) // child
    {
        dup2(fd[1], STDOUT_FILENO);
        dup2(fd[1], STDERR_FILENO);
        close(fd[0]);
        close(fd[1]);

        execlp(app_runc_path_v, app_runc_path_v, "run", "--bundle", app_home_path_v, "--pid-file",
               app_container_pid_path_v, pparam->app_name, NULL);
    } else { // parent
        fd_set fds;
        int max_fd;
        size_t read_len = 0;

        IoT_Publish_Message_Params paramsQOS1;
        char payload[4096], topic[PATH_MAX + 50];
        IoT_Error_t rc = SUCCESS;
        int topic_l, container_pid_l;

        char *container_pid_s;

        close(fd[1]);

        // wait container startup
        while (access(app_container_pid_path_v, F_OK) == -1);
        // wait pid file flush
        sleep(1);

        container_pid_s = read_str_file(app_container_pid_path_v, &container_pid_l);

        if (NULL == container_pid_s) {
            IOT_ERROR("failed to get application container pid from %s", app_container_pid_path_v);
            rc = 1;
            goto release;
        }

        container_pid = atoi(container_pid_s);
        free(container_pid_s);

        add_app_deployed_pid(container_pid);

        paramsQOS1.qos = QOS1;
        paramsQOS1.payload = (void *)payload;
        paramsQOS1.isRetained = 0;

        topic_l = snprintf(topic, PATH_MAX + 50, "irootech-dmp/apps/%s/log", pparam->app_name);

        while (1) {
            FD_ZERO(&fds);
            FD_SET(fd[0], &fds);
            FD_SET(pparam->ctl_pipe_in[0], &fds);

            max_fd = fd[0] > pparam->ctl_pipe_in[0] ? fd[0] : pparam->ctl_pipe_in[0];

            switch (select(max_fd + 1, &fds, NULL, NULL, NULL)) {
                case -1: // select error
                    IOT_ERROR("[CRITICAL] failed to listen container for application %s", pparam->app_name);
                    sleep(1);
                    break;
                case 0:
                    break; // doesn't make sense, re-listen
                default:
                    if (FD_ISSET(fd[0], &fds)) {
                        read_len = read(fd[0], payload, 4096);

                        if (0 == read_len) { // EOF
                            // container child process existed
                            del_app_deployed_pid(container_pid);
                            IOT_INFO("application %s log controller existed", pparam->app_name);
                            rc = 0;
                            goto release;
                        }

                        // TODO(production): 1) combine logs to save cost of MQTT calls; 2) json formation.

                        paramsQOS1.payloadLen = read_len;

                        // send the logs to Cloud
                        do {
                            rc = aws_iot_mqtt_publish(pparam->paws_iot_client, topic, topic_l, &paramsQOS1);
                            if (rc == MQTT_REQUEST_TIMEOUT_ERROR) {
                                IOT_WARN("application %s log QOS1 publish ack not received, ignored",
                                        pparam->app_name);
                                rc = SUCCESS;
                            }

                            if (MQTT_CLIENT_NOT_IDLE_ERROR == rc)
                                usleep(500); // same as timeout of yield() in main thread loop
                        } while (MQTT_CLIENT_NOT_IDLE_ERROR == rc ||
                            NETWORK_ATTEMPTING_RECONNECT == rc || NETWORK_RECONNECTED == rc);

                        if(SUCCESS != rc) {
                            IOT_DEBUG("failed to publish application %s log: %d", pparam->app_name, rc);
                        }

                        IOT_DEBUG("application %s log published to topic %s", pparam->app_name, topic);
                    } else if (FD_ISSET(pparam->ctl_pipe_in[0], &fds)) { // control
                        IOT_INFO("application %s log controller stopped", pparam->app_name);
                        rc = 0;
                        goto release;
                    }
            }
        }
    }

release:
    app_log_ctrlr_param_free(pparam);

    return (void*)(intptr_t)rc;
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
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]);
        close(fd[1]);

        // stats collection interval default: 5s
        execlp(app_runc_path_v, app_runc_path_v, "events", pparam->app_name, NULL);
    } else { // parent
        fd_set fds;
        int max_fd;
        size_t read_len = 0;

        IoT_Publish_Message_Params paramsQOS1;
        char payload[4096], topic[PATH_MAX + 50];
        IoT_Error_t rc = SUCCESS;
        int topic_l;

        paramsQOS1.qos = QOS1;
        paramsQOS1.payload = (void *)payload;
        paramsQOS1.isRetained = 0;

        topic_l = snprintf(topic, PATH_MAX + 50, "irootech-dmp/apps/%s/event", pparam->app_name);

        close(fd[1]);

        while (1) {
            FD_ZERO(&fds);
            FD_SET(fd[0], &fds);
            FD_SET(pparam->ctl_pipe_in[0], &fds);

            max_fd = fd[0] > pparam->ctl_pipe_in[0] ? fd[0]: pparam->ctl_pipe_in[0];

            switch (select(max_fd + 1, &fds, NULL, NULL, NULL)) {
                case -1: // select error
                    IOT_ERROR("[CRITICAL] failed to listen container for application %s", pparam->app_name);
                    sleep(1);
                    break;
                case 0:
                    break; // doesn't make sense, re-listen
                default:
                    if (FD_ISSET(fd[0], &fds)) {
                        read_len = read_line(fd[0], payload, 4096);

                        if (0 == read_len) { // EOF
                            // container child process existed
                            IOT_INFO("application %s event controller existed", pparam->app_name);
                            rc = 0;
                            goto release;
                        }

                        paramsQOS1.payloadLen = strlen(payload);

                        // send the events to Cloud
                        do {
                            rc = aws_iot_mqtt_publish(pparam->paws_iot_client, topic, topic_l, &paramsQOS1);
                            if (rc == MQTT_REQUEST_TIMEOUT_ERROR) {
                                IOT_WARN("application %s event QOS1 publish ack not received, ignored",
                                        pparam->app_name);
                                rc = SUCCESS;
                            }

                            if (MQTT_CLIENT_NOT_IDLE_ERROR == rc)
                                usleep(500); // same as timeout of yield() in main thread loop
                        } while (MQTT_CLIENT_NOT_IDLE_ERROR == rc ||
                            NETWORK_ATTEMPTING_RECONNECT == rc || NETWORK_RECONNECTED == rc);

                        if(SUCCESS != rc) {
                            IOT_DEBUG("failed to publish application %s event: %d", pparam->app_name, rc);
                        }

                        IOT_DEBUG("application %s event published to topic %s", pparam->app_name, topic);
                    } else if (FD_ISSET(pparam->ctl_pipe_in[0], &fds)) { // control
                        IOT_INFO("application %s event controller stopped", pparam->app_name);
                        rc = 0;
                        goto release;
                    }
            }
        }
    }

release:
    app_event_ctrlr_param_free(pparam);

    return (void *) (intptr_t)rc;
}

int app_deploy(char *app_name, AWS_IoT_Client *paws_iot_client) {
    papp_log_ctlr_param pparam_log;
    papp_event_ctlr_param pparam_event;

    pthread_t app_log_ctl_thd, app_event_ctl_thd;
    int rc = 0, cnt = 0;

    if (NULL == app_name)
        return 1;

    rc = app_log_ctrlr_param_create(app_name, paws_iot_client, &pparam_log);
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

    rc = app_event_ctrlr_param_create(app_name, paws_iot_client, &pparam_event);
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

int apps_send_signal(int signo) {
    size_t i = 0;
    int pid = -1;

    for (i = 0; i < apps_deployed_pid_l; i++) {
        pid = apps_deployed_pid_v[i];

        if (0 < pid) {
            IOT_DEBUG("send signal %d to pid %d", signo, pid);
            kill(pid, signo);
        }
    }

    return 0;
}
