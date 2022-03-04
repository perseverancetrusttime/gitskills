/***************************************************************************
 *
 * Copyright 2015-2021 BES.
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
#ifdef RAM_DUMP_TO_FLASH
#include <stdio.h>
#include <string.h>
#include <string.h>
#include "cmsis.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "hal_norflash.h"
#include "norflash_api.h"
#include "watchdog.h"
#include "ramdump_section.h"

#define RAM_DUMP_PREFIX     "__LOG_DUMP:"

#if 1
#define RAM_DUMP_TRACE(num,fmt, ...)  TRACE(num,fmt, ##__VA_ARGS__)
#else
#define RAM_DUMP_TRACE(num,fmt, ...)
#endif

extern uint32_t __ramdump_section_start;
extern uint32_t __ramdump_section_end;

static const uint32_t ramdump_flash_start_addr = (uint32_t)&__ramdump_section_start;
static const uint32_t ramdump_flash_end_addr =  (uint32_t)&__ramdump_section_end;
static uint32_t ramdump_ram_start = RAM_DUMP_START;
static uint32_t ramdump_ram_size = RAM_DUMP_SIZE;
static bool is_inited = false;

void ramdump_to_flash_init(void)
{
    uint32_t block_size = 0;
    uint32_t sector_size = 0;
    uint32_t page_size = 0;
    uint32_t buffer_len = 0;
    enum NORFLASH_API_RET_T result;

    hal_norflash_get_size(HAL_FLASH_ID_0,NULL, &block_size, &sector_size, &page_size);
    buffer_len = sector_size;
    RAM_DUMP_TRACE(4,RAM_DUMP_PREFIX"%s: ramdump_flash_start_addr = 0x%x, ramdump_flash_end_addr = 0x%x, buff_len = 0x%x.",
                    __func__,
                    ramdump_flash_start_addr,
                    ramdump_flash_end_addr,
                    buffer_len);

    result = norflash_api_register(
                    NORFLASH_API_MODULE_ID_RAMDUMP,
                    HAL_FLASH_ID_0,
                    ramdump_flash_start_addr,
                    ramdump_flash_end_addr - ramdump_flash_start_addr,
                    block_size,
                    sector_size,
                    page_size,
                    buffer_len,
                    NULL
                    );
    if(result == NORFLASH_API_OK)
    {
        RAM_DUMP_TRACE(0,RAM_DUMP_PREFIX"ramdump_to_flash_init ok.");
    }
    else
    {
        RAM_DUMP_TRACE(4,RAM_DUMP_PREFIX"ramdump_to_flash_init failed,result = %d.",result);
    }
#if !(defined(DUMP_LOG_ENABLE) || defined(DUMP_CRASH_ENABLE) || defined(TOTA_CRASH_DUMP_TOOL_ENABLE))
    hal_trace_app_register(ramdump_to_flash_handler, NULL);
#endif
    is_inited = true;
}

void ramdump_to_flash_handler(enum HAL_TRACE_STATE_T state)
{
    uint32_t i;
    uint32_t sector_count;
    uint32_t sector_size = 0;
    uint32_t addr, len, offs;
    uint8_t *pbuff;
    enum NORFLASH_API_RET_T result;

    RAM_DUMP_TRACE(1,RAM_DUMP_PREFIX"%s: RAM: 0x%x-0x%x,FLASH: 0x%x-0x%x",
            __func__,
            ramdump_ram_start,
            ramdump_ram_start + ramdump_ram_size,
            ramdump_flash_start_addr,
            ramdump_flash_end_addr
            );

    if(!is_inited)
    {
        RAM_DUMP_TRACE(0,RAM_DUMP_PREFIX"%s: uninit!", __func__);
        return;
    }

    if(state != HAL_TRACE_STATE_CRASH_END)
    {
        return;
    }
    watchdog_hw_stop();
    hal_norflash_get_size(HAL_FLASH_ID_0,NULL, NULL, &sector_size, NULL);
    sector_count = (ramdump_flash_end_addr - ramdump_flash_start_addr)/sector_size;
    for(i = 0; i < sector_count; i++)
    {
        offs = i * sector_size;
        addr = ramdump_flash_start_addr + offs;
        pbuff = (uint8_t*)ramdump_ram_start + offs;

        len = offs + sector_size <=  ramdump_ram_size ? sector_size : ramdump_ram_size - offs;
        if(offs + sector_size <=  ramdump_ram_size )
        {
            len = sector_size;
        }
        else
        {
            if(offs < ramdump_ram_size)
            {
                len = ramdump_ram_size - offs;
            }
            else
            {
                // RAM dump completed.
                len = 0;
                break;
            }
        }
        result = norflash_api_erase(NORFLASH_API_MODULE_ID_RAMDUMP,
                                    addr,
                                    sector_size,
                                    false);
        if(result != NORFLASH_API_OK)
        {
            RAM_DUMP_TRACE(0,RAM_DUMP_PREFIX"ram dump: erase failed, result = %d, addr = 0x%x", result,addr);
            break;
        }

        result = norflash_api_write(NORFLASH_API_MODULE_ID_RAMDUMP,
                                    addr,
                                    pbuff,
                                    len,
                                    false);
        if(result != NORFLASH_API_OK)
        {
            RAM_DUMP_TRACE(0,RAM_DUMP_PREFIX"ram dump: write failed, result = %d, addr = 0x%x", result,addr);
            break;
        }
    }
#if 0
    for(i = 0; i < sector_count; i++)
    {
        offs = i * sector_size;
        addr = ramdump_flash_start_addr + offs;
        pbuff = (uint8_t*)ramdump_ram_start + offs;

        len = offs + sector_size <=  ramdump_ram_size ? sector_size : ramdump_ram_size - offs;
        if(offs + sector_size <=  ramdump_ram_size )
        {
            len = sector_size;
        }
        else
        {
            if(offs < ramdump_ram_size)
            {
                len = ramdump_ram_size - offs;
            }
            else
            {
                // RAM dump completed.
                len = 0;
                break;
            }
        }
        if(memcmp((uint8_t*)addr,pbuff,len) != 0)
        {
            RAM_DUMP_TRACE(1,RAM_DUMP_PREFIX"%s writting checking error.addr = 0x%x", __func__,addr);
            DUMP8("0x%x,", (uint8_t*)addr,64);
            break;
        }
    }
#endif
    watchdog_hw_start(15);
    RAM_DUMP_TRACE(1,RAM_DUMP_PREFIX"%s done", __func__);

}

#endif
