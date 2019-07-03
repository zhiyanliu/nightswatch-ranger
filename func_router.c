//
// Created by Zhi Yan Liu on 2019-07-03.
//

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#ifdef __linux__
#include <linux/limits.h>
#elif defined(__APPLE__)
#include <sys/syslimits.h>
#endif
#include <pthread.h>
#include <stddef.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "aws_iot_error.h"
#include "aws_iot_log.h"
#include "aws_iot_mqtt_client_interface.h"

#include "funcs.h"
#include "func_router.h"
#include "utils.h"


// unused function storage, currently, local/offline cache needed
int func_router_home_path(char *func_router_home_path_v, size_t func_router_home_path_l) {
    int len0 = 0, len1 = 0;

    if (NULL == func_router_home_path_v)
        return -1;

    len0 = funcs_path(func_router_home_path_v, func_router_home_path_l);
    if (-1 == len0)
        return -1;

    len1 = snprintf(func_router_home_path_v + len0, func_router_home_path_l - len0, "/%s",
        IROOTECH_DMP_RP_AGENT_FUNC_ROUTER_HOME_DIR);
    if (-1 == len1)
        return -1;

    return len0 + len1;
}

static int func_router_io_sock() {
    int rc = 0, fd, flags;
    struct sockaddr_in addr;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == fd) {
        IOT_ERROR("failed to create function router IO socket: %d", rc);
        return -1;
    }

    flags = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(IROOTECH_DMP_RP_AGENT_FUNC_ROUTER_IO_SOCK_PORT);

    rc = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
    if (-1 == rc) {
        IOT_ERROR("failed to bind function router IO AF_UNIX socket: %d", errno);
        close(fd);
        return -1;
    }

    rc = listen(fd, 5);
    if (-1 == rc) {
        IOT_ERROR("failed to listen function router IO AF_UNIX socket: %d", rc);
        close(fd);
        return -1;
    }

    IOT_INFO("the function router socket created, at 127.0.0.1:%d", IROOTECH_DMP_RP_AGENT_FUNC_ROUTER_IO_SOCK_PORT);

    return fd;
}

int func_router_create(AWS_IoT_Client *paws_iot_client, pfunc_router *pprouter) {
    int fd, rc;

    if (NULL == paws_iot_client)
        return 1;

    if (NULL == pprouter)
        return 1;

    fd = func_router_io_sock();
    if (-1 == fd) {
        IOT_ERROR("failed to create function router IO listener socket");
        return 1;
    }

    *pprouter = malloc(sizeof(func_router));
    if (NULL == *pprouter) {
        IOT_ERROR("failed to allocate function router: %d", errno);
        close(fd);
        return 1;
    }

    (*pprouter)->io_listen_fd = fd;

    rc = pipe((*pprouter)->ctl_pipe_in);
    if (0 != rc) {
        close(fd);
        free(*pprouter);
        return rc;
    }

    rc = pipe((*pprouter)->ctl_pipe_out);
    if (0 != rc) {
        close(fd);
        close((*pprouter)->ctl_pipe_in[0]);
        close((*pprouter)->ctl_pipe_in[1]);
        free(*pprouter);
        return rc;
    }

    (*pprouter)->paws_iot_client = paws_iot_client;

    return 0;
}

int func_router_free(pfunc_router prouter) {
    if (NULL == prouter)
        return 0;

    close(prouter->io_listen_fd);

    close(prouter->ctl_pipe_in[0]);
    close(prouter->ctl_pipe_in[1]);

    close(prouter->ctl_pipe_out[0]);
    close(prouter->ctl_pipe_out[1]);

    free(prouter);

    return 0;
}

static void* func_router_io_worker(void *p) {
    pfunc_router_worker_param pparam = (pfunc_router_worker_param)p;
    int rc, read_l, app_name_l;
    char app_name[PATH_MAX + 1];

    IoT_Publish_Message_Params paramsQOS1;
    char payload[4096], topic[PATH_MAX + 50];
    IoT_Error_t iot_rc = SUCCESS;
    int topic_l;

    if (NULL == pparam) {
        rc = 1;
        goto end;
    }

    read_l = read_line(pparam->conn_fd, app_name, PATH_MAX + 1);
    if (0 == read_l) { // EOF
        rc = 0;
        goto end;
    } else if (-1 == read_l) { // defence
        IOT_ERROR("failed to read application name from the application: %d", errno);
        rc = errno;
        goto end;
    }

    app_name[strcspn(app_name, "\r\n")] = 0; // remove newline at end, e.g. LF, CR, CRLF, LFCR

    IOT_DEBUG("router IO worker serves application: %s", app_name);

    paramsQOS1.qos = QOS1;
    paramsQOS1.payload = (void *)payload;
    paramsQOS1.isRetained = 0;

    topic_l = snprintf(topic, PATH_MAX + 50, "irootech-dmp/apps/%s/data", app_name);

    // data follow the application name line, as designed currently
    while (1) {
        read_l = read_line(pparam->conn_fd, payload, 4096);
        if (0 == read_l) { // EOF
            rc = 0;
            break;
        } else if (-1 == read_l) {
            IOT_ERROR("failed to read data from the application: %d", errno);
            rc = errno;
            break;
        }

        paramsQOS1.payloadLen = read_l;

        // send the data to Cloud
        do {
            iot_rc = aws_iot_mqtt_publish(pparam->paws_iot_client, topic, topic_l, &paramsQOS1);
            if (iot_rc == MQTT_REQUEST_TIMEOUT_ERROR) {
                IOT_WARN("application %s data QOS1 publish ack not received, ignored", app_name);
                iot_rc = SUCCESS;
            }

            if (MQTT_CLIENT_NOT_IDLE_ERROR == iot_rc)
                usleep(500); // same as timeout of yield() in main thread loop
        } while (MQTT_CLIENT_NOT_IDLE_ERROR == iot_rc ||
            NETWORK_ATTEMPTING_RECONNECT == iot_rc || NETWORK_RECONNECTED == iot_rc);

        if(SUCCESS != iot_rc) {
            IOT_DEBUG("failed to publish application %s data: %d", app_name, iot_rc);
        }

        IOT_DEBUG("application %s data published to topic %s", app_name, topic);
    }

end:
    close(pparam->conn_fd);
    free(pparam);

    IOT_INFO("function router IO worker exits");

    return (void*)(intptr_t)rc;
}

static void* func_router_io_server(void *p) {
    pfunc_router prouter = (pfunc_router)p;
    int rc;
    pthread_t io_client_thd;
    fd_set fds;
    char *buff = "stopped";
    struct timeval timeout = {0, 0};

    if (NULL == prouter) {
        rc = 1;
        goto end;
    }

    while (1) {
        FD_ZERO(&fds);
        FD_SET(prouter->ctl_pipe_in[0], &fds);

        int conn_fd = accept(prouter->io_listen_fd, NULL, NULL);
        if (conn_fd == -1) {
            if (errno == EWOULDBLOCK) {
                // check control command, e.g. stop server
                switch (select(prouter->ctl_pipe_in[0] + 1, &fds, NULL, NULL, &timeout)) {
                    case -1: // select error
                        IOT_ERROR("[CRITICAL] failed to check control command in function router IO server: %d", errno);
                        break;
                    case 0: // no control command
                        // nothing to do
                        break;
                    default:
                        // only support stop command currently
                        write(prouter->ctl_pipe_out[1], buff, 8);
                        IOT_INFO("function router IO server stopped");
                        rc = 0;
                        goto end;
                }
            } else {
                IOT_ERROR("failed to accept connection in function router IO server, ignored: %d", errno);
            }

            sleep(1);
            continue;
        } else {
            pfunc_router_worker_param pparam;

            IOT_DEBUG("an application connected to function router IO server");

            pparam = malloc(sizeof(func_router_worker_param));
            if (NULL == pparam) {
                IOT_ERROR("failed to allocate function router IO worker param: %d", errno);
                continue;
            }

            pparam->conn_fd = conn_fd;
            pparam->paws_iot_client = prouter->paws_iot_client;

            rc = pthread_create(&io_client_thd, NULL, func_router_io_worker, pparam);
            if (0 != rc) {
                IOT_ERROR("failed to create function router IO worker: %d", rc);
            }

            IOT_INFO("IO worker created by function router IO server");
        }
    }

end:
    return (void*)(intptr_t)rc;
}

int func_router_start(pfunc_router prouter) {
    pthread_t io_server_thd;
    int rc = 0;

    if (NULL == prouter) {
        return 1;
    }

    rc = pthread_create(&io_server_thd, NULL, func_router_io_server, prouter);
    if (0 != rc) {
        IOT_ERROR("failed to create function router IO server: %d", rc);
    }

    return rc;
}

int func_router_stop(pfunc_router prouter) {
    char *cmd = "stop", buff[1];

    if (NULL == prouter) {
        return 1;
    }

    write(prouter->ctl_pipe_in[1], cmd, 5);

    // wait to exits
    read(prouter->ctl_pipe_out[0], buff, 1);

    return 0;
}
