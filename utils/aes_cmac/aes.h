#ifndef AES_H
#define AES_H

#include "common.h"

// Main Functions
unsigned char* aes_128_encrypt(unsigned char* in, unsigned char* out, unsigned char* key);
unsigned char* aes_128_decrypt(unsigned char* in, unsigned char* out, unsigned char* key);
unsigned char* aes_192_encrypt(unsigned char* in, unsigned char* out, unsigned char* key);
unsigned char* aes_192_decrypt(unsigned char* in, unsigned char* out, unsigned char* key);
unsigned char* aes_256_encrypt(unsigned char* in, unsigned char* out, unsigned char* key);
unsigned char* aes_256_decrypt(unsigned char* in, unsigned char* out, unsigned char* key);

// The Cipher
void Cipher(unsigned char* in, unsigned char* out, unsigned char* w, unsigned char Nk, unsigned char Nr);
void InvCipher(unsigned char* in, unsigned char* out, unsigned char* w, unsigned char Nk, unsigned char Nr);

// KeyExpansion
void KeyExpansion(unsigned char* key, unsigned char* w, unsigned char Nk, unsigned char Nr);
unsigned char* SubWord(unsigned char* word);
unsigned char* RotWord(unsigned char* word);

// Round Ops
void SubBytes(unsigned char state[4][4]);
void ShiftRows(unsigned char state[4][4]);
void MixColumns(unsigned char state[4][4]);
void AddRoundKey(unsigned char state[4][4], unsigned char* key);
unsigned char mul(unsigned char a, unsigned char b);

// Inv Round Ops
void InvSubBytes(unsigned char state[4][4]);
void InvShiftRows(unsigned char state[4][4]);
void InvMixColumns(unsigned char state[4][4]);
void InvAddRoundKey(unsigned char state[4][4], unsigned char* key);

#endif // !AES_H
