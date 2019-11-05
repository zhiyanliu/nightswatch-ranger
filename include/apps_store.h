//
// Created by Zhi Yan Liu on 2019/10/13.
//

#ifndef NIGHTSWATCH_RANGER_APPS_STORE_H_
#define NIGHTSWATCH_RANGER_APPS_STORE_H_

#include <stddef.h>
#include <unistd.h>

#include "aws_iot_mqtt_client_interface.h"

#define NIGHTSWATCH_RANGER_APPS_STORE_FILENAME "registry"


typedef struct {
    char app_name[PATH_MAX + 1];
    int launcher_type;
} app_register_record, *papp_register_record;

typedef struct {
    char apps_store_file_path[PATH_MAX + 1];
    size_t record_count;
    papp_register_record *records; // papp_register_record list
} apps_store, *papps_store;

typedef int (*traverse_func_t)(char *app_name, int launcher_type, AWS_IoT_Client *paws_iot_client);


extern papps_store apps_store_global;


papps_store _apps_store_create();

int _apps_store_free(papps_store pstore);

int _apps_store_load(papps_store pstore);

int _apps_store_save(papps_store pstore);

int _apps_store_add_record(papps_store pstore, char *app_name, int launcher_type);

int _apps_store_del_record(papps_store pstore, char *app_name, int launcher_type);

int apps_store_init();

void apps_store_free();

int apps_traverse_records(AWS_IoT_Client *paws_iot_client, traverse_func_t fun);

int apps_launch(char *app_name, int launcher_type, AWS_IoT_Client *paws_iot_client);

int app_register(char *app_name, int launcher_type);

int app_unregister(char *app_name, int launcher_type);

#endif // NIGHTSWATCH_RANGER_APPS_STORE_H_
