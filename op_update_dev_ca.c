//
// Created by Liu, Zhiyan on 2019-05-23.
//

#include <errno.h>
#ifdef __linux__
  #include <linux/limits.h>
#elif defined(__APPLE__)
  #include <sys/syslimits.h>
#endif
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

#include "aws_iot_error.h"
#include "aws_iot_json_utils.h"
#include "aws_iot_log.h"

#include "apps.h"
#include "aws_iot_config.h"
#include "certs.h"
#include "client.h"
#include "funcs.h"
#include "job_dispatch.h"
#include "job_parser.h"
#include "md5_calc.h"
#include "s3_http.h"
#include "utils.h"


/*
 * we handle job message one by one, no concurrent process.
 * so reuse follow single parser, token vector and token counter to save resource.
 */
static jsmn_parser _json_parser;
static jsmntok_t _json_tok_v[MAX_JSON_TOKEN_EXPECTED];
static int32_t _token_c;


static int parse_job_doc(pjob pj, char *pkg_url, size_t pkg_url_l, unsigned char *pkg_md5, size_t pkg_md5_l) {
    IoT_Error_t rc = FAILURE;
    jsmntok_t *tok_pkg_url, *tok_pkg_md5;

    jsmn_init(&_json_parser);
    _token_c = jsmn_parse(&_json_parser, pj->job_doc, (int)pj->job_doc_l,
                          _json_tok_v, MAX_JSON_TOKEN_EXPECTED);
    if(_token_c < 0) {
        IOT_ERROR("failed to parse job document json: %d", _token_c);
        return 1;
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

    return 0;
}

static int step1_download_pkg_file(pjob_dispatch_param pparam, char *alter_par_path, size_t alter_par_path_l,
        char *alter_certs_pkg_file_path, size_t alter_certs_pkg_file_path_l,
        unsigned char *pkg_md5_dst, size_t pkg_md5_l) {

    char pkg_url[4096];
    int rc = 0;

    if (NULL == alter_par_path)
        return 1;

    if (NULL == alter_certs_pkg_file_path)
        return 1;

    rc = parse_job_doc(pparam->pj, pkg_url, 4096, pkg_md5_dst, pkg_md5_l);
    if (0 != rc) {
        IOT_ERROR("failed to parse job doc for update device ca operate: %d", rc);
        return rc;
    }

    rc = certs_get_free_par_dir(alter_par_path, alter_par_path_l, NULL, 0);
    if (0 != rc) {
        IOT_ERROR("failed to determine alternative certs partition path: %d", rc);
        return rc;
    }

    snprintf(alter_certs_pkg_file_path, alter_certs_pkg_file_path_l, "%s/certs.zip", alter_par_path);

    IOT_DEBUG("download certs package to %s from %s", alter_certs_pkg_file_path, pkg_url);

    rc = unlink(alter_certs_pkg_file_path);
    if (0 != rc && ENOENT != errno) {
        IOT_ERROR("failed to unlink existing alternative certs package at %s: %d", alter_certs_pkg_file_path, rc);
        return rc;
    }

    rc = s3_http_download(pkg_url, alter_certs_pkg_file_path);
    if (0 != rc) {
        IOT_ERROR("failed to download package: %d", rc);
        return rc;
    }

    IOT_INFO("certs package downloaded to %s", alter_certs_pkg_file_path);

    return rc;
}

static int step2_verify_pkg_md5sum(pjob_dispatch_param pparam,
        char *alter_certs_pkg_file_path, size_t alter_certs_pkg_file_path_l,
        unsigned char *pkg_md5_src, unsigned char *pkg_md5_dst, size_t pkg_md5_l) {
    int rc = 0;

    IOT_DEBUG("verify certs package md5")

    rc = md5_calculate(alter_certs_pkg_file_path, pkg_md5_src, MD5_SUM_LENGTH);
    if (0 != rc) {
        IOT_ERROR("failed to calculate package md5 sum: %d", rc);
        return rc;
    }

    rc = md5_compare(pkg_md5_src, pkg_md5_dst, pkg_md5_l);
    if (0 != rc) {
        IOT_ERROR("failed to verify certs package md5: expected: %s, actual: %.*s",
                pkg_md5_dst, MD5_SUM_LENGTH, pkg_md5_src);
        return rc;
    }

    IOT_INFO("certs package md5 verified ok: %s (%.*s)",
            alter_certs_pkg_file_path, MD5_SUM_LENGTH, pkg_md5_src);

    return 0;
}

static int step3_unzip_pkg_file(pjob_dispatch_param pparam, char *alter_par_path, char *alter_certs_pkg_file_path) {
    char cmd[PATH_MAX * 2 + 20] = {0};
    int rc = 0;

    if (NULL == alter_par_path)
        return 1;

    if (NULL == alter_certs_pkg_file_path)
        return 1;

    snprintf(cmd, PATH_MAX * 2 + 20, "unzip -o %s -d %s", alter_certs_pkg_file_path, alter_par_path);

    rc = system(cmd);
    if (0 != rc) {
        IOT_ERROR("failed to unzip certs package: %d", rc);
        return rc;
    }

    // remove downloaded zip package to save disk space
    unlink(alter_certs_pkg_file_path);

    IOT_INFO("certs package unzipped into %s", alter_par_path);

    return rc;
}

static int step4_switch_certs_par(pjob_dispatch_param pparam, char *target_file_name, size_t target_file_name_l) {
    char target_file_path[PATH_MAX + 1];
    int rc = 0;

    rc = certs_switch_par(target_file_path, PATH_MAX + 1, target_file_name, target_file_name_l);
    if (0 != rc) {
        IOT_ERROR("[CRITICAL] certs partition symbolic link might be broken, will reset in next start");
        return rc;
    }

    IOT_INFO("certs partition is switched to %s", target_file_path);

    return rc;
}

static int step5_restart_app(pjob_dispatch_param pparam, char *self_path, char *alter_par_name) {
    int rc = 0;

    IOT_DEBUG("shutdown functions");
    funcs_shutdown();

    IOT_DEBUG("stop all applications");
    apps_kill();

    IOT_INFO("restarting the application, switch to using new certs");

    rc = execl(self_path, self_path, "--upd_dev_ca_job_id", pparam->pj->job_id,
            "--upd_dev_ca_par_name", alter_par_name, NULL);
    if (0 != rc) {
        IOT_ERROR("[CRITICAL] failed to restart the myself: %d", errno);
        return rc;
    }

    return rc;
}

int op_update_dev_ca_entry(pjob_dispatch_param pparam) {
    char self_path[PATH_MAX + 1], alter_par_path[PATH_MAX + 1], alter_par_name[PATH_MAX + 1],
        alter_certs_pkg_file_path[PATH_MAX + 1];
    unsigned char pkg_md5_src[MD5_SUM_LENGTH + 1], pkg_md5_dst[MD5_SUM_LENGTH + 1];
    int rc = 0;

    // TODO(production): check free disk space, reject the operate if needed.

    rc = cur_pid_full_path(self_path, PATH_MAX + 1);
    if (0 != rc) {
        dmp_dev_client_job_failed(pparam->paws_iot_client, pparam->thing_name, pparam->pj->job_id,
                "{\"detail\":\"Failed to prepare execution.\"}");
        return rc;
    }

    dmp_dev_client_job_wip(pparam->paws_iot_client, pparam->thing_name, pparam->pj->job_id,
            "{\"detail\":\"Downloading new certs package to update.\"}");

    rc = step1_download_pkg_file(pparam, alter_par_path, PATH_MAX + 1, alter_certs_pkg_file_path, PATH_MAX + 1,
            pkg_md5_dst, MD5_SUM_LENGTH + 1);
    if (0 != rc) {
        dmp_dev_client_job_failed(pparam->paws_iot_client, pparam->thing_name, pparam->pj->job_id,
                "{\"detail\":\"Failed to downloading new certs package.\"}");
        return rc;
    }

    dmp_dev_client_job_wip(pparam->paws_iot_client, pparam->thing_name, pparam->pj->job_id,
            "{\"detail\":\"Verifying the md5 of new certs package.\"}");

    rc = step2_verify_pkg_md5sum(pparam, alter_certs_pkg_file_path, PATH_MAX + 1,
            pkg_md5_src, pkg_md5_dst, MD5_SUM_LENGTH + 1);
    if (0 != rc) {
        dmp_dev_client_job_failed(pparam->paws_iot_client, pparam->thing_name, pparam->pj->job_id,
                "{\"detail\":\"Failed to verify the md5 of new certs package.\"}");
        return rc;
    }

    dmp_dev_client_job_wip(pparam->paws_iot_client, pparam->thing_name, pparam->pj->job_id,
            "{\"detail\":\"Extracting new certs package.\"}");

    rc = step3_unzip_pkg_file(pparam, alter_par_path, alter_certs_pkg_file_path);
    if (0 != rc) {
        dmp_dev_client_job_failed(pparam->paws_iot_client, pparam->thing_name, pparam->pj->job_id,
                "{\"detail\":\"Failed to extract new certs package.\"}");
        return rc;
    }

    dmp_dev_client_job_wip(pparam->paws_iot_client, pparam->thing_name, pparam->pj->job_id,
            "{\"detail\":\"Configure device to use new certs.\"}");

    rc = step4_switch_certs_par(pparam, alter_par_name, PATH_MAX + 1);
    if (0 != rc) {
        dmp_dev_client_job_failed(pparam->paws_iot_client, pparam->thing_name, pparam->pj->job_id,
                "{\"detail\":\"Failed to switch to using new certs.\"}");
        return rc;
    }

    // TODO(production): wait all ongoing executions on the device finish.
    // TODO(production): close all resources, e.g. opened fd.

    dmp_dev_client_job_wip(pparam->paws_iot_client, pparam->thing_name, pparam->pj->job_id,
                           "{\"detail\":\"Restarting device application to apply new certs.\"}");

    rc = step5_restart_app(pparam, self_path, alter_par_name);
    if (0 != rc) {
        dmp_dev_client_job_failed(pparam->paws_iot_client, pparam->thing_name, pparam->pj->job_id,
                "{\"detail\":\"Failed to restart the application.\"}");
        return rc;
    }

    return rc;
}
