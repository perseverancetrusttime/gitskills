#include <string.h>

#include "sha256.h"
#include "uECC.h"
#include "aes.h"
#include "stdlib.h"
#include "stdbool.h"
#include "gma_crypto.h"

#define RAND_CHAR_CAP    ('A')
#define RAND_CHAR_LOW    ('a')

uint32_t gma_SHA256_hash(const void* in_data, int len, void* out_data)
{
    SHA256_hash(in_data, len, out_data);
    return GMA_SUCCESS;
}

int gma_rand_generator(uint8_t *dest, unsigned size) {

    uint8_t temp = 0;
    while (size--) {
        temp = rand()%(52);
        if(temp > 25)
        {
            *dest++ = temp -25 -1 + RAND_CHAR_LOW;
        }
        else
        {
            *dest++ = temp + RAND_CHAR_CAP;
        }
    }
    return 1;
}

extern void AES128_CBC_encrypt_buffer(uint8_t* output, uint8_t* input, uint32_t length, const uint8_t* key, const uint8_t* iv);
extern void AES128_CBC_decrypt_buffer(uint8_t* output, uint8_t* input, uint32_t length, const uint8_t* key, const uint8_t* iv);
void gma_crypto_encrypt(uint8_t* output, uint8_t* input, uint32_t length, const uint8_t* key, const uint8_t* iv)
{
    AES128_CBC_encrypt_buffer(output, input, length, key, iv);
}

void gma_crypto_decrypt(uint8_t* output, uint8_t* input, uint32_t length, const uint8_t* key, const uint8_t* iv)
{
    AES128_CBC_decrypt_buffer(output, input, length, key, iv);
}

