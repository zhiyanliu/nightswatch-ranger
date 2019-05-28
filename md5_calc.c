//
// Created by Zhi Yan Liu on 2019-05-28.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/md5.h>


int md5_calculate(const char *file_path, unsigned char **ppmd5digest) {
    MD5_CTX ctx;
    int bytes;
    unsigned char data[1024];

    FILE *file = fopen(file_path, "rb");
    if (NULL == file)
        return 1;

    *ppmd5digest = (unsigned char*)malloc(MD5_DIGEST_LENGTH);
    if (NULL == *ppmd5digest)
        return 2;

    MD5_Init(&ctx);

    while ((bytes = fread(data, 1, 1024, file)) != 0)
        MD5_Update(&ctx, data, bytes);

    MD5_Final(*ppmd5digest, &ctx);

    fclose (file);
    return 0;
}

void md5_free(unsigned char *pmd5digest) {
    free(pmd5digest);
}

int md5_compare(unsigned char *pmd5digest_src, unsigned char *pmd5digest_dst) {
    int i;

    if (NULL == pmd5digest_src && NULL == pmd5digest_dst) {
        return 0;
    }

    for (i = 0; i < MD5_DIGEST_LENGTH; i++) {
        if (pmd5digest_src[i] != pmd5digest_dst[i])
            return 1;
    }

    return 0;
}
