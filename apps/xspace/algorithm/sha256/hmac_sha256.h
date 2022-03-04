#ifndef __HMAC_SHA256_H__
#define __HMAC_SHA256_H__

#ifdef __cplusplus
extern "C"{
#endif

void hmac_sha256_init(unsigned char *k, int lk);
void hmac_sha256_update(unsigned char *d, int ld);
void hmac_sha256_final(unsigned char *out, int *t);

#ifdef __cplusplus
}
#endif

#endif /* __HMAC_SHA256_H__ */
