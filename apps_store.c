//
// Created by Zhi Yan Liu on 2019/10/13.
//

#include <errno.h>
#ifdef __linux__
#include <linux/limits.h>
#elif defined(__APPLE__)
#include <sys/syslimits.h>
#endif
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "aws_iot_log.h"
#include "aws_iot_mqtt_client_interface.h"

#include "apps.h"
#include "apps_store.h"
#include "utils.h"


papps_store apps_store_global = NULL;

papps_store _apps_store_create() {
    char apps_path_v[PATH_MAX + 1];

    papps_store pstore = (papps_store)malloc(sizeof(apps_store));
    if (NULL != pstore)
        memset(pstore, 0, sizeof(apps_store));

    apps_path(apps_path_v, PATH_MAX + 1);

    snprintf(pstore->apps_store_file_path, PATH_MAX + 1, "%s/%s",
            apps_path_v, IROOTECH_DMP_RP_AGENT_APP_STORE_FILENAME);

    return pstore;
}

int _apps_store_free(papps_store pstore) {
    size_t idx = 0;

    if (NULL == pstore)
        return 1;

    for (idx = 0; idx < pstore->record_count; idx++)
        free(pstore->records[idx]);

    if (NULL != pstore->records)
        // non-empty store
        free(pstore->records);
    free(pstore);

    return 0;
}

int _apps_store_add_record(papps_store pstore, char *app_name, int launcher_type) {
    papp_register_record *record_list = NULL, new_record = NULL;

    if (NULL == pstore)
        return 1;

    if (NULL == app_name)
        return 1;

    // new record
    new_record = malloc(sizeof(app_register_record));
    if (NULL == new_record) {
        IOT_ERROR("failed to allocate application store record");
        return 1;
    }

    // fill the data in to the new record
    strncpy(new_record->app_name, app_name, PATH_MAX + 1);
    new_record->launcher_type = launcher_type;

    // append the new record to the end
    pstore->record_count++;
    record_list = realloc(pstore->records, pstore->record_count * sizeof(papp_register_record));
    if (NULL == record_list) {
        free(new_record);
        pstore->record_count--;
        IOT_ERROR("failed to allocate application store record list");
        return 1;
    }

    record_list[pstore->record_count - 1] = new_record;
    pstore->records = record_list;

    return 0;
}

int _apps_store_del_record(papps_store pstore, char *app_name, int launcher_type) {
    size_t idx = 0;

    if (NULL == pstore)
        return 1;

    if (NULL == app_name)
        return 1;

    for (idx = 0; idx < pstore->record_count; idx++)
         if (strncmp(pstore->records[idx]->app_name, app_name, PATH_MAX + 1) == 0 &&
                 launcher_type == pstore->records[idx]->launcher_type) {
             break;
         }

    if (idx == pstore->record_count)
        return 2; // not found

    free(pstore->records[idx]);
    if (1 == pstore->record_count) {
        free(pstore->records);
        pstore->records = NULL;
    } else { // > 1
        // move rest records up one slot
        while (idx < pstore->record_count - 1) {
            pstore->records[idx] = pstore->records[idx + 1];
            idx++;
        }
    }

    pstore->record_count--;

    return 0;
}

int _apps_store_load(papps_store pstore) {
    int fd, rc = 0, launcher_type;
    char record_line[4096], app_name[PATH_MAX + 1];
    size_t read_len = 1;

    if (NULL == pstore)
        return 1;

    fd = open(pstore->apps_store_file_path, O_RDONLY | O_CREAT, 0640);
    if (fd < 0) {
        IOT_ERROR("failed to open application store file: %s", pstore->apps_store_file_path);
        return 1;
    }

    while (1) {
        read_len = read_line(fd, record_line, 4096);
        if (read_len < 0) {
            IOT_ERROR("failed to read application store record list");
            rc = 1;
            goto end;
        }

        if (0 == read_len)
            break; // EOF

        sscanf(record_line, "%s\t%d", app_name, &launcher_type);

        rc = _apps_store_add_record(pstore, app_name,launcher_type);
        if (0 != rc)
            goto end;
    }

    rc = 0;

end:
    close(fd);

    return rc;
}

int _apps_store_save(papps_store pstore) {
    int rc = 0, fd, len;
    size_t idx;
    char tmp_file[PATH_MAX + 1] = "/tmp/irootech-dmp-rp-agent.apps-store.XXXXXX",
        record_line[2048], cmd[PATH_MAX * 2 + 20] = {0};

    if (NULL == pstore)
        return 1;

    fd = mkstemp(tmp_file);
    if (-1 == fd) {
        IOT_ERROR("failed to open temporarily application store file to save record: %d", errno);
        return errno;
    }

    for (idx = 0; idx < pstore->record_count; idx++) {
        // fill the data in to the new record
        len = snprintf(record_line, 2048, "%s\t%d\n",
                pstore->records[idx]->app_name, pstore->records[idx]->launcher_type);
        rc = write(fd, record_line, len);
        if (-1 == rc) {
            IOT_ERROR("failed to write temporarily application store file to save record: %d", errno);
            close(fd);
            unlink(tmp_file);
            return errno;
        }
    }

    close(fd);

    // update real application store file more atomically
    snprintf(cmd, PATH_MAX * 2 + 20, "cp -f %s %s", tmp_file, pstore->apps_store_file_path);

    rc = system(cmd);
    if (0 != rc)
        IOT_ERROR("failed to update application store file to save record: %d", rc);

    unlink(tmp_file); // temp file, ignore to check result code

    return rc;
}

int apps_store_init() {
    int rc = 0;

    if (NULL != apps_store_global) {
        IOT_ERROR("application store has already been initialized");
        return 1;
    }

    apps_store_global = _apps_store_create();
    if (NULL == apps_store_global) {
        IOT_ERROR("failed to create application store, ignored");
        return 1;
    }

    rc = _apps_store_load(apps_store_global);
    if (0 != rc) {
        IOT_ERROR("failed to read application store: %d", rc);
        return rc;
    }

    return 0;
}

void apps_store_free() {
    int rc = 0;

    if (NULL == apps_store_global) {
        IOT_ERROR("application store is not initialized yet");
        return;
    }

    rc = _apps_store_free(apps_store_global);
    if (0 != rc)
        IOT_ERROR("failed to free application store");
}

int apps_traverse_records(AWS_IoT_Client *paws_iot_client, traverse_func_t fun) {
    size_t idx = 0;
    int handle_rc = 0, rc = 0;

    for (idx = 0; idx < apps_store_global->record_count; idx++) {
        IOT_DEBUG("handle application %s (launcher-type=%d)...",
                apps_store_global->records[idx]->app_name, apps_store_global->records[idx]->launcher_type);
        handle_rc = (*fun)(apps_store_global->records[idx]->app_name, apps_store_global->records[idx]->launcher_type,
                paws_iot_client);
        if (0 != handle_rc)
            rc++;
        IOT_INFO("application %s (launcher-type=%d) handled (rc=%d)",
                apps_store_global->records[idx]->app_name, apps_store_global->records[idx]->launcher_type, handle_rc);
    }

    return rc;
}

int apps_launch(char *app_name, int launcher_type, AWS_IoT_Client *paws_iot_client) {
    int rc = app_deploy(app_name, launcher_type, paws_iot_client);
    if (0 != rc)
        IOT_ERROR("failed to launch deployed application %s (launcher-type=%d)", app_name, launcher_type);
    return rc;
}

int app_register(char *app_name, int launcher_type) {
    int rc = 0;

    if (NULL == app_name) {
        IOT_ERROR("invalid application name");
        return 1;
    }

    if (NULL == apps_store_global) {
        IOT_ERROR("application store is not initialized yet");
        return 1;
    }

    rc = _apps_store_add_record(apps_store_global, app_name, launcher_type);
    if (0 != rc) {
        IOT_ERROR("failed to add the application %s (launcher-type=%d) record to the store", app_name, launcher_type);
        return rc;
    }

    rc = _apps_store_save(apps_store_global);
    if (0 != rc)
        IOT_ERROR("failed to save application store");

    return rc;
}

int app_unregister(char *app_name, int launcher_type) {
    int rc = 0;

    if (NULL == app_name) {
        IOT_ERROR("invalid application name");
        return 1;
    }

    if (NULL == apps_store_global) {
        IOT_ERROR("application store is not initialized yet");
        return 1;
    }

    rc = _apps_store_del_record(apps_store_global, app_name, launcher_type);
    if (1 == rc) {
        IOT_ERROR("failed to delete the application %s (launcher-type=%d) record from application store: %d",
                app_name, launcher_type, rc);
        return rc;
    } else if (2 == rc) {
        IOT_WARN("failed to delete the application %s (launcher-type=%d) record from application store, "
                 "record not found", app_name, launcher_type);
        return rc;
    }

    rc = _apps_store_save(apps_store_global);
    if (0 != rc)
        IOT_ERROR("failed to save application store");

    return rc;
}
