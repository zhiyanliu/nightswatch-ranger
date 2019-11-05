//
// Created by Zhi Yan Liu on 2019-05-20.
//

#ifndef NIGHTSWATCH_RANGER_JOB_PARSER_H_
#define NIGHTSWATCH_RANGER_JOB_PARSER_H_

#include <stdbool.h>
#include <stddef.h>

#define JOB_OPERATE_PROPERTY_NAME "operate"
#define JOB_PKG_URL_PROPERTY_NAME "pkg_url"
#define JOB_PKG_MD5_PROPERTY_NAME "pkg_md5"
#define JOB_APP_NAME_PROPERTY_NAME "app_name"
#define JOB_APP_ARGS_PROPERTY_NAME "app_args"
#define JOB_APP_FORCE_PROPERTY_NAME "force"
#define JOB_APP_USE_CONTAINER_PROPERTY_NAME "use_container"
#define JOB_AGENT_VERSION_PROPERTY_NAME "agent_ver"

typedef enum {
    /** Success return value - no error occurred */
            JOB_PARSE_SUCCESS = 0,
    /** A generic error. Not enough information for a specific error code */
            JOB_PARSE_FAILURE = -1,
    /** Job execution property is empty */
            JOB_EXECUTION_EMPTY = -2,
    /** Job id not found */
            JOB_ID_NOT_FOUND_ERROR = -3,
    /** Job id is invalid */
            JOB_ID_INVALID_ERROR = -4,
    /** Job document property is empty */
            JOB_DOCUMENT_NOT_FOUND_ERROR = -5,
    /** Job status not found */
            JOB_STATUS_NOT_FOUND_ERROR = -6,
    /** Job status is invalid */
            JOB_STATUS_INVALID_ERROR = -7
} Job_Parse_Error_t;

typedef struct {
    void *payload;
    size_t payload_l;

    char job_id[MAX_SIZE_OF_JOB_ID + 1];
    char *job_doc;
    size_t job_doc_l; // job_doc buffer size, NOT include last NULL
} job, *pjob, **ppjob;

Job_Parse_Error_t job_parser_parse(void *payload, size_t payload_l, ppjob ppj,
        char *job_status_recv, size_t job_status_recv_l);

void job_parser_job_free(pjob pj);

/*
 * parse business property from the specific job doc
 *
 * Job_Parse_Error_t job_parser_xxx(pjob pj);
 */

#endif // NIGHTSWATCH_RANGER_JOB_PARSER_H_
