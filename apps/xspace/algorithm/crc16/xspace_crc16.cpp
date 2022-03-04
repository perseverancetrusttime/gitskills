#include "xspace_crc16.h"

uint16_t xspace_crc16_ccitt_false(const uint8_t *buffer, uint32_t size, uint16_t crc)
{

    uint16_t crc_in = crc;
    uint16_t poly = 0x1021;
    uint16_t tmp = 0;

    if (NULL != buffer && size > 0) {
        while (size--) {
            tmp = *(buffer++);
            crc_in ^= (tmp << 8);
            for (int i = 0; i < 8; i++) {
                if (crc_in & 0x8000) {
                    crc_in = (crc_in << 1) ^ poly;
                } else {
                    crc_in = crc_in << 1;
                }
            }
        }
    }
    return crc_in;
}

uint16_t xspace_crc16_xmodem(uint8_t *buf, uint32_t len)
{

    uint16_t crc_in = 0x0000;
    uint16_t poly = 0x1021;
    uint16_t tmp = 0;

    if (NULL != buf && len > 0) {
        while (len--) {
            tmp = *(buf++);
            crc_in ^= (tmp << 8);
            for (int i = 0; i < 8; i++) {
                if (crc_in & 0x8000) {
                    crc_in = (crc_in << 1) ^ poly;
                } else {
                    crc_in = crc_in << 1;
                }
            }
        }
    }
    return crc_in;
}