#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sha256.h"
#include "hmac_sha256.h"

#define SHA256_HASH_SIZE    32
#define SHA_BLOCKSIZE       64

static sha256_context ictx, octx;

/*
char* d1,       // data to be truncated
char* d2,       // truncated data
int len,        // length in bytes to keep
*/
void truncate(unsigned char* d1, unsigned char* d2, int len)
{
    int i;
    
    for (i = 0 ; i < len ; i++)
    {
        d2[i] = d1[i];
    }
}

/*
    const char *k, // secret key
    int lk,     //length of the key in bytes
*/
void hmac_sha256_init(unsigned char *k, int lk)
{
    unsigned char key[SHA256_HASH_SIZE];
    unsigned char buf[SHA_BLOCKSIZE];
    int i;

    if (lk > SHA_BLOCKSIZE)
    {
        sha256_context tctx;

        xspace_sha256_init(&tctx);
        xspace_sha256_starts(&tctx, 0);
        xspace_sha256_update(&tctx, k, lk);
        xspace_sha256_finish(&tctx, key);

        k = key;
        lk = SHA256_HASH_SIZE;
    }

    /**** Inner Digest start ****/
    xspace_sha256_init(&ictx);
    xspace_sha256_starts(&ictx, 0);

    /* Pad the key for inner digest */
    for (i = 0; i < lk; ++i)
        buf[i] = k[i] ^ 0x36;

    for (i = lk; i < SHA_BLOCKSIZE; ++i)
        buf[i] = 0x36;

    xspace_sha256_update(&ictx, buf, SHA_BLOCKSIZE);
    /**** Inner Digest end ****/

    /**** Outter Digest start ****/
    xspace_sha256_init(&octx);
    xspace_sha256_starts(&octx, 0); 

    /* Pad the key for outter digest */
    for (i = 0; i < lk; ++i)
        buf[i] = k[i] ^ 0x5C;

    for (i = lk; i < SHA_BLOCKSIZE; ++i)
        buf[i] = 0x5C;

    xspace_sha256_update(&octx, buf, SHA_BLOCKSIZE);
    /**** Outter Digest end ****/
}

/*
    const char *d,  // data
    int ld,     //length of data in bytes
*/
void hmac_sha256_update(unsigned char *d, int ld)
{
    xspace_sha256_update(&ictx, d, ld);
}

/*
    char *out, //output buffer, at least "t" bytes
    int *t, //output buffer, at least "t" bytes
*/
void hmac_sha256_final(unsigned char *out, int *t)
{
	unsigned char isha[SHA256_HASH_SIZE], osha[SHA256_HASH_SIZE];

	xspace_sha256_finish(&ictx, isha);

	xspace_sha256_update(&octx, isha, SHA256_HASH_SIZE);
	xspace_sha256_finish(&octx, osha);

	/* truncate and print the results */
	*t = *t > SHA256_HASH_SIZE ? SHA256_HASH_SIZE : *t;
	truncate(osha, out, *t);
}
