//
// Created by Zhi Yan Liu on 2019-05-28.
//

#ifndef IROOTECH_DMP_RP_AGENT_MD5_CALC_H_
#define IROOTECH_DMP_RP_AGENT_MD5_CALC_H_

#define MD5_SUM_LENGTH 32

int md5_calculate(const char *file_path, unsigned char *pmd5sum, size_t md5sum_l);

int md5_compare(unsigned char *pmd5sum_src, unsigned char *pmd5sum_dst, size_t md5sum_l);

#endif //IROOTECH_DMP_RP_AGENT_MD5_CALC_H_
