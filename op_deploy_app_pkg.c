//
// Created by Zhi Yan Liu on 2019-07-01.
//

#include <errno.h>
#ifdef __linux__
#include <linux/limits.h>
#elif defined(__APPLE__)
#include <sys/syslimits.h>
#endif
#include <stddef.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "aws_iot_error.h"
#include "aws_iot_json_utils.h"
#include "aws_iot_log.h"

#include "apps.h"
#include "aws_iot_config.h"
#include "client.h"
#include "job_dispatch.h"
#include "job_parser.h"
#include "md5_calc.h"
#include "op_deploy_app_pkg.h"
#include "s3_http.h"
#include "utils.h"


/*
 * we handle job message one by one, no concurrent process.
 * so reuse follow single parser, token vector and token counter to save resource.
 */
static jsmn_parser _json_parser;
static jsmntok_t _json_tok_v[MAX_JSON_TOKEN_EXPECTED];
static int32_t _token_c;


static int parse_job_doc(pjob pj, char *app_name, size_t app_name_l, char *pkg_url, size_t pkg_url_l,
        unsigned char *pkg_md5, size_t pkg_md5_l, char *app_args, size_t app_args_l) {

    IoT_Error_t rc = FAILURE;
    jsmntok_t *tok_app_name, *tok_pkg_url, *tok_pkg_md5, *tok_app_args;

    jsmn_init(&_json_parser);
    _token_c = jsmn_parse(&_json_parser, pj->job_doc, (int)pj->job_doc_l,
            _json_tok_v, MAX_JSON_TOKEN_EXPECTED);
    if(_token_c < 0) {
        IOT_ERROR("failed to parse job document json: %d", _token_c);
        return 1;
    }

    tok_app_name = findToken(JOB_APP_NAME_PROPERTY_NAME, pj->job_doc, _json_tok_v);
    if (NULL == tok_app_name) {
        IOT_WARN("job application name property %s not found, nothing to do", JOB_APP_NAME_PROPERTY_NAME);
        return 2;
    }

    tok_pkg_url = findToken(JOB_PKG_URL_PROPERTY_NAME, pj->job_doc, _json_tok_v);
    if (NULL == tok_pkg_url) {
        IOT_WARN("job package url property %s not found, nothing to do", JOB_PKG_URL_PROPERTY_NAME);
        return 2;
    }

    tok_pkg_md5 = findToken(JOB_PKG_MD5_PROPERTY_NAME, pj->job_doc, _json_tok_v);
    if (NULL == tok_pkg_md5) {
        IOT_WARN("job package md5 property %s not found, nothing to do", JOB_PKG_MD5_PROPERTY_NAME);
        return 3;
    }

    tok_app_args = findToken(JOB_APP_ARGS_PROPERTY_NAME, pj->job_doc, _json_tok_v);
    if (NULL == tok_app_args) {
        IOT_WARN("job application arguments property %s not found, nothing to do", JOB_APP_ARGS_PROPERTY_NAME);
        return 3;
    }

    rc = parseStringValue(app_name, app_name_l, pj->job_doc, tok_app_name);
    if (SUCCESS != rc) {
        IOT_ERROR("failed to parse job application name: %d", rc);
        return rc;
    }

    rc = parseStringValue(pkg_url, pkg_url_l, pj->job_doc, tok_pkg_url);
    if (SUCCESS != rc) {
        IOT_ERROR("failed to parse job package url: %d", rc);
        return rc;
    }

    rc = parseStringValue((char*)pkg_md5, pkg_md5_l, pj->job_doc, tok_pkg_md5);
    if (SUCCESS != rc) {
        IOT_ERROR("failed to parse job package md5: %d", rc);
        return rc;
    }

    rc = parseStringValue((char*)app_args, app_args_l, pj->job_doc, tok_app_args);
    if (SUCCESS != rc) {
        IOT_ERROR("failed to parse application arguments: %d", rc);
        return rc;
    }

    return 0;
}

static int step1_check_app_deployed(pjob_dispatch_param pparam, char *app_name, size_t app_name_l,
        char *pkg_url, size_t pkg_url_l, unsigned char *pkg_md5_dst, size_t pkg_md5_l,
        char *app_args, size_t app_args_l) {

    int rc = 0;

    rc = parse_job_doc(pparam->pj, app_name, app_name_l, pkg_url, pkg_url_l,
            pkg_md5_dst, pkg_md5_l, app_args, app_args_l);
    if (0 != rc) {
        IOT_ERROR("failed to parse job doc for deploy application operation: %d", rc);
        return rc;
    }

    rc = app_exists(app_name);
    if (1 == rc) {
        IOT_ERROR("application %s was deployed on this device already: %d", app_name, rc);
        return rc;
    }

    return rc;
}

static int step2_download_pkg_file(pjob_dispatch_param pparam, char *pkg_url, unsigned char *pkg_md5_dst,
        char *work_dir_path, char *app_name, char *app_home_path, size_t app_home_path_l,
        char *app_pkg_file_path, size_t app_pkg_file_path_l) {

    char cmd[PATH_MAX + 10] = {0};
    int rc = 0;

    if (NULL == work_dir_path)
        return 1;

    if (NULL == app_name)
        return 1;

    if (NULL == app_home_path)
        return 1;

    if (NULL == app_pkg_file_path)
        return 1;

    snprintf(app_home_path, app_home_path_l, "%s/%s/%s",
            work_dir_path, IROOTECH_DMP_RP_AGENT_APPS_DIR, app_name);

    snprintf(cmd, PATH_MAX + 10, "rm -rf %s", app_home_path);

    rc = system(cmd);
    if (0 != rc) {
        IOT_ERROR("failed to unlink existing application at %s: %d", app_home_path, rc);
        return rc; // application name is invalid as a part of path?
    }

    rc = mkdir(app_home_path, S_IRWXU | S_IRWXG);
    if (0 != rc) {
        IOT_ERROR("failed to create application home directory: %d", rc);
        return rc;
    }

    snprintf(app_pkg_file_path, app_pkg_file_path_l, "%s/app.tar.gz", app_home_path);

    IOT_DEBUG("download application package to %s from %s", app_pkg_file_path, pkg_url);

    rc = s3_http_download(pkg_url, app_pkg_file_path);
    if (0 != rc) {
        IOT_ERROR("failed to download package: %d", rc);
        return rc;
    }

    IOT_INFO("application package downloaded to %s", app_pkg_file_path);

    return rc;
}

static int step3_verify_pkg_md5sum(pjob_dispatch_param pparam,
        char *app_pkg_file_path, size_t app_pkg_file_path_l,
        unsigned char *pkg_md5_src, unsigned char *pkg_md5_dst, size_t pkg_md5_l) {

    int rc = 0;

    IOT_DEBUG("verify application package md5")

    rc = md5_calculate(app_pkg_file_path, pkg_md5_src, MD5_SUM_LENGTH);
    if (0 != rc) {
        IOT_ERROR("failed to calculate package md5 sum: %d", rc);
        return rc;
    }

    rc = md5_compare(pkg_md5_src, pkg_md5_dst, pkg_md5_l);
    if (0 != rc) {
        IOT_ERROR("failed to verify application package md5: expected: %s, actual: %.*s",
                  pkg_md5_dst, MD5_SUM_LENGTH, pkg_md5_src);
        return rc;
    }

    IOT_INFO("application package md5 verified ok: %s (%.*s)",
             app_pkg_file_path, MD5_SUM_LENGTH, pkg_md5_src);

    return 0;
}

static int step4_extract_pkg_file(pjob_dispatch_param pparam, char *app_root_path, char *app_pkg_file_path) {
    char cmd[PATH_MAX * 2 + 20] = {0};
    int rc = 0;

    if (NULL == app_root_path)
        return 1;

    if (NULL == app_pkg_file_path)
        return 1;

    rc = mkdir(app_root_path, S_IRWXU | S_IRWXG);
    if (0 != rc) {
        IOT_ERROR("failed to create application rootfs directory: %d", rc);
        return rc;
    }

    snprintf(cmd, PATH_MAX * 2 + 20, "tar zxf %s -C %s", app_pkg_file_path, app_root_path);
    // the package created by this kind of command:
    //   docker export $(docker create busybox) | tar -C rootfs -xvf -
    //   tar -C rootfs -czf app_xxx_pkg.tar.gz --owner=0 --group=0 ./

    rc = system(cmd);
    if (0 != rc) {
        IOT_ERROR("failed to extract application package: %d", rc);
        return rc;
    }

    // remove downloaded package to save disk space
    unlink(app_pkg_file_path);

    IOT_INFO("application package extracted into %s", app_root_path);

    return rc;
}

static int step5_config_runc_spec(pjob_dispatch_param pparam, char *work_dir_path, char *app_home_path,
        char *app_args, char *app_spec_path, size_t app_spec_path_l) {

    // info(zhiyan): https://github.com/opencontainers/runtime-spec/blob/master/config-linux.md

    char cmd[PATH_MAX * 3 + 20] = {0}, app_spec_path_ori[PATH_MAX + 1];
    int rc = 0;

    if (NULL == app_home_path)
        return 1;

    if (NULL == app_spec_path)
        return 1;

    snprintf(app_spec_path_ori, PATH_MAX + 1, "%s/%s/%s.tpl", work_dir_path,
            IROOTECH_DMP_RP_AGENT_APPS_DIR, IROOTECH_DMP_RP_AGENT_APP_SPEC_FILE);
    snprintf(app_spec_path, app_spec_path_l, "%s/%s", app_home_path,
            IROOTECH_DMP_RP_AGENT_APP_SPEC_FILE);

    snprintf(cmd, PATH_MAX * 3 + 20, "sed 's/{args}/%s/g' %s > %s", app_args, app_spec_path_ori, app_spec_path);

    rc = system(cmd);
    if (0 != rc) {
        IOT_ERROR("failed to generate application spec: %d", rc);
        return rc;
    }

    IOT_INFO("application spec generated at %s", app_spec_path);

    return rc;
}

int op_deploy_app_pkg_entry(pjob_dispatch_param pparam) {
    char work_dir_path[PATH_MAX + 1], app_name[PATH_MAX + 1], pkg_url[4096],
        app_home_path[PATH_MAX + 1], app_pkg_file_path[PATH_MAX + 1], app_root_path[PATH_MAX + 1],
        app_args[PATH_MAX + 1], app_spec_path[PATH_MAX + 1];

    unsigned char pkg_md5_src[MD5_SUM_LENGTH + 1], pkg_md5_dst[MD5_SUM_LENGTH + 1];
    int rc = 0;

    // TODO(production): check free disk space, reject the operate if needed.

    getcwd(work_dir_path, PATH_MAX + 1);

    rc = step1_check_app_deployed(pparam, app_name, PATH_MAX + 1, pkg_url, 4096,
            pkg_md5_dst, MD5_SUM_LENGTH + 1, app_args, PATH_MAX + 1);
    if (0 != rc) {
        dmp_dev_client_job_failed(pparam->paws_iot_client, pparam->thing_name, pparam->pj->job_id,
                "{\"detail\":\"Application was deployed on this device already.\"}");
        return rc;
    }

    dmp_dev_client_job_wip(pparam->paws_iot_client, pparam->thing_name, pparam->pj->job_id,
            "{\"detail\":\"Downloading application package to the device.\"}");

    rc = step2_download_pkg_file(pparam, pkg_url, pkg_md5_dst, work_dir_path, app_name,
            app_home_path, PATH_MAX + 1, app_pkg_file_path, PATH_MAX + 1);
    if (0 != rc) {
        dmp_dev_client_job_failed(pparam->paws_iot_client, pparam->thing_name, pparam->pj->job_id,
                "{\"detail\":\"Failed to downloading application package.\"}");
        return rc;
    }

    dmp_dev_client_job_wip(pparam->paws_iot_client, pparam->thing_name, pparam->pj->job_id,
            "{\"detail\":\"Verifying the md5 of application package.\"}");

    rc = step3_verify_pkg_md5sum(pparam, app_pkg_file_path, PATH_MAX + 1,
            pkg_md5_src, pkg_md5_dst, MD5_SUM_LENGTH + 1);
    if (0 != rc) {
        dmp_dev_client_job_failed(pparam->paws_iot_client, pparam->thing_name, pparam->pj->job_id,
                "{\"detail\":\"Failed to verify the md5 of application package.\"}");
        return rc;
    }

    dmp_dev_client_job_wip(pparam->paws_iot_client, pparam->thing_name, pparam->pj->job_id,
            "{\"detail\":\"Extracting application package.\"}");

    snprintf(app_root_path, PATH_MAX + 1, "%s/%s", app_home_path, IROOTECH_DMP_RP_AGENT_APP_ROOT_DIR);

    rc = step4_extract_pkg_file(pparam, app_root_path, app_pkg_file_path);
    if (0 != rc) {
        dmp_dev_client_job_failed(pparam->paws_iot_client, pparam->thing_name, pparam->pj->job_id,
                "{\"detail\":\"Failed to extract application package.\"}");
        return rc;
    }

    dmp_dev_client_job_wip(pparam->paws_iot_client, pparam->thing_name, pparam->pj->job_id,
            "{\"detail\":\"Configure the container spec of application.\"}");

    rc = step5_config_runc_spec(pparam, work_dir_path, app_home_path, app_args, app_spec_path, PATH_MAX + 1);
    if (0 != rc) {
        dmp_dev_client_job_failed(pparam->paws_iot_client, pparam->thing_name, pparam->pj->job_id,
                "{\"detail\":\"Failed to configure the container spec of application.\"}");
        return rc;
    }

    // TODO(production): book the locally deployed application info at somewhere.

    dmp_dev_client_job_wip(pparam->paws_iot_client, pparam->thing_name, pparam->pj->job_id,
            "{\"detail\":\"Deploy application container.\"}");

    rc = app_deploy(app_name, pparam->paws_iot_client);
    if (0 != rc) {
        dmp_dev_client_job_failed(pparam->paws_iot_client, pparam->thing_name, pparam->pj->job_id,
                "{\"detail\":\"Failed to deploy application container.\"}");
        return rc;
    }

    rc = dmp_dev_client_job_done(pparam->paws_iot_client, pparam->thing_name, pparam->pj->job_id,
            "{\"detail\":\"Application deployed.\"}");
    if (SUCCESS != rc) {
        IOT_ERROR("failed to set application deploy job status to SUCCEEDED, "
                  "will redo this job if it still under IN_PROGRESS status: %d", rc);
    } else {
        IOT_INFO("deploy application successfully, job id: %s", pparam->pj->job_id);
    }

    return rc;
}
