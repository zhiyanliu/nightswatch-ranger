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
#include <unistd.h>

#include "aws_iot_error.h"
#include "aws_iot_json_utils.h"
#include "aws_iot_log.h"

#include "aws_iot_config.h"
#include "certs.h"
#include "job_dispatch.h"
#include "job_parser.h"
#include "md5_calc.h"
#include "s3_http.h"


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

static int step1_download_pkg_file(pjob_dispatch_param pparam) {
    char pkg_url[4096], free_par_path[PATH_MAX + 1], alter_certs_pkg_file_path[PATH_MAX + 1];
    unsigned char pkg_md5sum_src[MD5_SUM_LENGTH + 1], pkg_md5sum_dst[MD5_SUM_LENGTH + 1];
    int rc;

    rc = parse_job_doc(pparam->pj, pkg_url, 4096, pkg_md5sum_dst, MD5_SUM_LENGTH + 1);
    if (0 != rc) {
        IOT_ERROR("failed to parse job doc for update device ca operate: %d", rc);
        return rc;
    }

    rc = certs_get_free_par_dir(free_par_path, PATH_MAX + 1);
    if (0 != rc) {
        IOT_ERROR("failed to determine alternative certs partition path: %d", rc);
        return rc;
    }

    snprintf(alter_certs_pkg_file_path, PATH_MAX + 1, "%s/certs.zip", free_par_path);

    IOT_INFO("download certs package to %s from %s", alter_certs_pkg_file_path, pkg_url);

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

    rc = md5_calculate(alter_certs_pkg_file_path, pkg_md5sum_src, MD5_SUM_LENGTH);
    if (0 != rc) {
        IOT_ERROR("failed to calculate package md5 sum: %d", rc);
        return rc;
    }

    rc = md5_compare(pkg_md5sum_src, pkg_md5sum_dst, MD5_SUM_LENGTH);
    if (0 != rc) {
        IOT_ERROR("failed to verify job package md5: expected: %s, actual: %.*s",
                pkg_md5sum_dst, MD5_SUM_LENGTH, pkg_md5sum_src);
        return rc;
    }

    IOT_INFO("new certs package is downloaded and md5sum verified ok: %s (%.*s)",
            alter_certs_pkg_file_path, MD5_SUM_LENGTH, pkg_md5sum_src);

    return 0;
}

int op_update_dev_ca_entry(pjob_dispatch_param pparam) {
    step1_download_pkg_file(pparam);

    // fake and test finish
//    dmp_dev_client_job_done(pparam->paws_iot_client, pparam->thing_name, pparam->pj,
//                            "{\"detail\":\"system ota is complete successfully.\"}");

    return 0;
}
