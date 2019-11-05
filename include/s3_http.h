//
// Created by Liu, Zhiyan on 2019-05-24.
//

#ifndef NIGHTSWATCH_RANGER_S3_HTTP_H_
#define NIGHTSWATCH_RANGER_S3_HTTP_H_

void s3_http_init();

int s3_http_download(char* obj_url, char* out_file_path);

void s3_http_free();

#endif // NIGHTSWATCH_RANGER_S3_HTTP_H_
