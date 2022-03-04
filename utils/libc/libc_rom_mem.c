/***************************************************************************
 *
 * Copyright 2015-2019 BES.
 * All rights reserved. All unpublished rights reserved.
 *
 * No part of this work may be used or reproduced in any form or by any
 * means, or stored in a database or retrieval system, without prior written
 * permission of BES.
 *
 * Use of this work is governed by a license granted by BES.
 * This work contains confidential and proprietary information of
 * BES. which is protected by copyright, trade secret,
 * trademark and other intellectual property rights.
 *
 ****************************************************************************/
#include "string.h"
#include "stdlib.h"
#include "stdio.h"

#include "export_fn_rom.h"

void *
memset(dst0, c0, length)
    void *dst0;
    int c0;
    size_t length;
{
    return __export_fn_rom.memset(dst0, c0, length);
}

void * memcpy(void *dst,const void *src, size_t length)
{
    return __export_fn_rom.memcpy(dst, src, length);
}

void * memmove(void *dst,const void *src, size_t length)
{
    return __export_fn_rom.memmove(dst, src, length);
}

int memcmp(s1, s2, n)
    const void *s1, *s2;
    size_t n;
{
    if (n != 0) {
        const unsigned char *p1 = s1, *p2 = s2;

        do {
            if (*p1++ != *p2++)
                return (*--p1 - *--p2);
        } while (--n != 0);
    }
    return (0);
}
