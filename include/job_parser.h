//
// Created by Zhi Yan Liu on 2019-05-20.
//

#ifndef IROOTECH_DMP_RP_AGENT_JOB_PARSER_H_
#define IROOTECH_DMP_RP_AGENT_JOB_PARSER_H_

#include <stdbool.h>
#include <stddef.h>


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
    size_t job_doc_l;
} job, *pjob, **ppjob;

Job_Parse_Error_t job_parser_parse(void *payload, size_t payload_l, ppjob ppj,
        char *job_status_cur, size_t job_status_cur_l);

void job_parser_job_free(pjob pj);

/*
 * parse business property from the specific job doc
 *
 * Job_Parse_Error_t job_parser_xxx(pjob pj);
 */

#endif //IROOTECH_DMP_RP_AGENT_JOB_PARSER_H_
