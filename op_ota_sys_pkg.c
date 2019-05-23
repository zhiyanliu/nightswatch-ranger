//
// Created by Liu, Zhiyan on 2019-05-23.
//

#include "aws_iot_log.h"

#include "op_ota_sys_pkg.h"

int op_ota_sys_pkg_entry(pjob_dispatch_param pparam) {

    IOT_INFO("thing_name: %s, job id: %s, job doc: %.*s, status_recv: %s", pparam->thing_name, pparam->pj->job_id,
            (int)pparam->pj->job_doc_l, pparam->pj->job_doc, pparam->job_status_recv);

    return 0;
}
