//
// Created by Zhi Yan Liu on 2019-05-28.
//

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/md5.h>

#include "md5_calc.h"


int md5_calculate(const char *file_path, unsigned char *pmd5sum, size_t md5sum_l) {
    MD5_CTX ctx;
    int bytes, i;
    unsigned char data[1024], md5digest[MD5_DIGEST_LENGTH];

    if (NULL == pmd5sum)
        return 1;

    FILE *file = fopen(file_path, "rb");
    if (NULL == file)
        return 1;

    MD5_Init(&ctx);

    while ((bytes = fread(data, 1, 1024, file)) != 0)
        MD5_Update(&ctx, data, bytes);

    MD5_Final(md5digest, &ctx);

    fclose(file);

    for(i = 0; i < MD5_DIGEST_LENGTH && i < md5sum_l; i++) {
        sprintf((char*)(pmd5sum + i*2), "%02x", (unsigned int)md5digest[i]);
    }

    return 0;
}

int md5_compare(unsigned char *pmd5sum_src, unsigned char *pmd5sum_dst, size_t md5sum_l) {
    int i;

    if (0 == md5sum_l)
        return 0;

    if (NULL == pmd5sum_src || NULL == pmd5sum_dst) {
        return 1;
    }

    for (i = 0; i < md5sum_l; i++) {
        if (pmd5sum_src[i] != pmd5sum_dst[i])
            return 1;
    }

    return 0;
}
