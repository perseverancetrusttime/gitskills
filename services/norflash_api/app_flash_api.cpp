/**
 * @file app_flash_api.cpp
 * @author BES AI team
 * @version 0.1
 * @date 2020-11-07
 *
 * @copyright Copyright (c) 2015-2020 BES Technic.
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
 */

/*****************************header include********************************/
#include "hal_trace.h"
#include "cmsis.h"
#include "cmsis_os.h"
#include "norflash_drv.h"
#include "app_flash_api.h"
#ifdef RTOS
#include "cmsis_os.h"
#endif
#include "hal_timer.h"
#include "hal_location.h"

#if 1
#define APP_FLASH_TRACE TRACE
#else
#define APP_FLASH_TRACE(level,...)
#endif

/*********************external function declearation************************/

/************************private macro defination***************************/
#define DEFAULT_CACHE_BUF_LEN (2 * 4096)

/************************private type defination****************************/

/**********************private function declearation************************/

/************************private variable defination************************/

/****************************function defination****************************/
void app_flash_register_module(uint8_t module,
                               uint32_t baseAddr,
                               uint32_t len,
                               uint32_t imageHandler)
{
    enum NORFLASH_API_RET_T ret;
    uint32_t block_size = 0;
    uint32_t sector_size = 0;
    uint32_t page_size = 0;

    hal_norflash_get_size(HAL_FLASH_ID_0,
                          NULL,
                          &block_size,
                          &sector_size,
                          &page_size);

    APP_FLASH_TRACE(1,"%s mod:%d, start:0x%x, len:0x%x",
          __func__, module, baseAddr, len);
    ret = norflash_api_register((NORFLASH_API_MODULE_ID_T)module,
                                HAL_FLASH_ID_0,
                                baseAddr,
                                len,
                                block_size,
                                sector_size,
                                page_size,
                                DEFAULT_CACHE_BUF_LEN,
                                (NORFLASH_API_OPERA_CB)imageHandler);

    ASSERT(ret == NORFLASH_API_OK,
           "ota_init_flash: norflash_api register failed,ret = %d.",
           ret);
    APP_FLASH_TRACE(1,"_debug: %s done", __func__);
}

SRAM_TEXT_LOC void app_flash_flush_pending_op(enum NORFLASH_API_MODULE_ID_T module,
                                enum NORFLASH_API_OPRATION_TYPE type)
{
    uint32_t lock;

    do
    {
        lock = int_lock_global();
        hal_trace_pause();
        norflash_api_flush();
        hal_trace_continue();
        int_unlock_global(lock);

        if (NORFLASH_API_ALL != type)
        {
            if (0 == norflash_api_get_used_buffer_count(module, type))
            {
                break;
            }
        }
        else
        {
            if (norflash_api_buffer_is_free(module))
            {
                break;
            }
        }
        osDelay(10);
    } while (1);

    // APP_FLASH_TRACE(1,"%s: module:%d, type:%d done", __func__, module, type);
}

SRAM_TEXT_LOC void app_flash_sector_erase(enum NORFLASH_API_MODULE_ID_T module, uint32_t flashOffset)
{
    /// get flash address according to the flash module and offset
    enum NORFLASH_API_RET_T ret = NORFLASH_API_OK;
    uint32_t lock;
    uint32_t start_addr = flashOffset;

    ASSERT((flashOffset & (FLASH_SECTOR_SIZE - 1)) == 0,
            "%s: addr No sec size alignment! offset = 0x%x",
            __func__,
            flashOffset);
    ret = norflash_api_get_base_addr(module, &start_addr);
    start_addr += flashOffset;
    start_addr = TO_FLASH_NC_ADDR(start_addr);
    APP_FLASH_TRACE(1, "%s: start_addr:%x", __func__, start_addr);
    do
    {
        lock = int_lock_global();
        hal_trace_pause();
        ret = norflash_api_erase(module,
                                 start_addr,
                                 FLASH_SECTOR_SIZE,
                                 true);
        hal_trace_continue();
        int_unlock_global(lock);

        if (NORFLASH_API_OK == ret)
        {
            // APP_FLASH_TRACE(1,"%s: ok!", __func__);
            break;
        }
        else if (NORFLASH_API_BUFFER_FULL == ret)
        {
            // APP_FLASH_TRACE(0,"Flash async cache overflow! To flush it.");
            app_flash_flush_pending_op(module, NORFLASH_API_ERASING);
        }
        else
        {
            uint32_t addr;
            norflash_api_get_base_addr(module, &addr);

            ASSERT(0, "%s: failed. ret=%d, flashOffset:0x%x, modStartAddr:0x%x",
                   __func__, ret, start_addr, addr);
        }
    } while (1);
    // APP_FLASH_TRACE(1,"%s: erase: 0x%x,0x1000 done", __func__, start_addr);
}

SRAM_TEXT_LOC void app_flash_erase(enum NORFLASH_API_MODULE_ID_T module, uint32_t flashOffset, uint32_t len)
{
    enum NORFLASH_API_RET_T ret = NORFLASH_API_OK;
    uint32_t lock;
    uint32_t t_size = 0;
    uint32_t b_size = 0;
    uint32_t s_size = 0;
    uint32_t p_size = 0;
    uint32_t s_len;
    uint32_t b_len;
    uint32_t i;
    uint32_t start_time = hal_sys_timer_get();
    uint32_t start_addr = 0;

    hal_norflash_get_size(HAL_FLASH_ID_0, &t_size, &b_size, &s_size, &p_size);

    ASSERT(((flashOffset & (s_size - 1)) == 0 &&
            (len & (s_size - 1)) == 0),
            "%s: No sec size alignment! offset = 0x%x, len = 0x%x",
            __func__,
            flashOffset,
            len);

    ret = norflash_api_get_base_addr(module, &start_addr);

    start_addr += flashOffset;
    start_addr = TO_FLASH_NC_ADDR(start_addr);

    APP_FLASH_TRACE(1, "%s: start_addr: start_addr = 0x%x, len = 0x%x",
          __func__, start_addr, len);

    // erase pre-sectors.
    if(len > ((b_size-1) & flashOffset))
    {
        s_len = ((b_size-1) & flashOffset);
    }
    else
    {
        s_len = len;
    }

    start_addr = TO_FLASH_NC_ADDR(start_addr);

    APP_FLASH_TRACE(1, "%s: pre sectors erase: start_addr = 0x%x, s_len = 0x%x",
          __func__, start_addr, s_len);
    for(i = 0; i < s_len/s_size; i++)
    {
        // APP_FLASH_TRACE(1,"%s: sector(0x%x) erase", __func__, start_addr);
        do{
            lock = int_lock_global();
            hal_trace_pause();
            ret = norflash_api_erase(module,
                                     start_addr,
                                     s_size,
                                     true);
            hal_trace_continue();
            int_unlock_global(lock);
            if (NORFLASH_API_OK == ret)
            {
                // APP_FLASH_TRACE(1,"%s: ok!", __func__);
                break;
            }
            else if (NORFLASH_API_BUFFER_FULL == ret)
            {
                // APP_FLASH_TRACE(0,"Flash async cache overflow! To flush it.");
                app_flash_flush_pending_op(module, NORFLASH_API_ERASING);
            }
            else
            {
                uint32_t addr;
                norflash_api_get_base_addr(module, &addr);

                ASSERT(0, "%s: failed. ret=%d, flashOffset:0x%x, modStartAddr:0x%x, size: 0x%x",
                       __func__, ret, start_addr, addr, s_size);
            }
        }while(1);
        start_addr += s_size;
    }

    // erase blocks.
    b_len = ((len - s_len)/b_size)*b_size ;
    APP_FLASH_TRACE(1, "%s: blocks erase: start_addr = 0x%x, b_len = 0x%x",
          __func__, start_addr, b_len);
    for(i = 0; i < b_len/b_size; i++)
    {
        // APP_FLASH_TRACE(1,"%s: block(0x%x) erase", __func__, start_addr);
        do {
            lock = int_lock_global();
            hal_trace_pause();
            ret = norflash_api_erase(module,
                                     start_addr,
                                     b_size,
                                     true);
            hal_trace_continue();
            int_unlock_global(lock);
            if (NORFLASH_API_OK == ret)
            {
                // APP_FLASH_TRACE(1,"%s: ok!", __func__);
                break;
            }
            else if (NORFLASH_API_BUFFER_FULL == ret)
            {
                // APP_FLASH_TRACE(0,"Flash async cache overflow! To flush it.");
                app_flash_flush_pending_op(module, NORFLASH_API_ERASING);
            }
            else
            {
                uint32_t addr;
                norflash_api_get_base_addr(module, &addr);

                ASSERT(0, "%s: failed. ret=%d, flashOffset:0x%x, modStartAddr:0x%x, size: 0x%x",
                       __func__, ret, start_addr, addr, b_size);
            }
        }while(1);
        start_addr += b_size;
    }

    // erase post sectors.
    s_len = len - s_len - b_len;
    APP_FLASH_TRACE(1, "%s: last sectors erase: start_addr = 0x%x, s_len = 0x%x",
          __func__, start_addr, s_len);
    for(i = 0; i < s_len/s_size; i++)
    {
        // APP_FLASH_TRACE(1,"%s: sector(0x%x) erase", __func__, start_addr);
        do {
            lock = int_lock_global();
            hal_trace_pause();
            ret = norflash_api_erase(module,
                                     start_addr,
                                     s_size,
                                     true);
            hal_trace_continue();
            int_unlock_global(lock);
            if (NORFLASH_API_OK == ret)
            {
                // APP_FLASH_TRACE(1,"%s: ok!", __func__);
                break;
            }
            else if (NORFLASH_API_BUFFER_FULL == ret)
            {
                // APP_FLASH_TRACE(0,"Flash async cache overflow! To flush it.");
                app_flash_flush_pending_op(module, NORFLASH_API_ERASING);
            }
            else
            {
                uint32_t addr;
                norflash_api_get_base_addr(module, &addr);

                ASSERT(0, "%s: failed. ret=%d, flashOffset:0x%x, modStartAddr:0x%x, size: 0x%x",
                       __func__, ret, start_addr, addr, s_size);
            }
        }while(1);
        start_addr += s_size;
    }
    APP_FLASH_TRACE(1,"%s: erase: 0x%x,0x%x, time= %d(ms)done",
        __func__, start_addr, len, TICKS_TO_MS(hal_sys_timer_get() - start_time));
}

SRAM_TEXT_LOC void app_flash_program(enum NORFLASH_API_MODULE_ID_T module,
                       uint32_t flashOffset,
                       uint8_t *ptr,
                       uint32_t len,
                       bool synWrite)
{
    enum NORFLASH_API_RET_T ret = NORFLASH_API_OK;
    uint32_t lock;
    bool is_async;
    uint32_t start_addr = 0;
    uint32_t sec_num;
    uint32_t write_len;
    uint32_t written_len = 0;
    uint32_t i;

    /// get logic address to write
    ret = norflash_api_get_base_addr(module, &start_addr);
    start_addr += flashOffset;

    if (synWrite)
    {
        is_async = false;
    }
    else
    {
        is_async = true;
    }

    // APP_FLASH_TRACE(1,"%s: write: 0x%x,0x%x", __func__, start_addr, len);
    sec_num = (len + NORFLASH_API_SECTOR_SIZE -1)/NORFLASH_API_SECTOR_SIZE;

    for(i = 0; i < sec_num; i++)
    {
        if(len - written_len > NORFLASH_API_SECTOR_SIZE)
        {
            write_len = NORFLASH_API_SECTOR_SIZE;
        }
        else
        {
            write_len = len - written_len ;
        }

        do
        {
            lock = int_lock_global();
            hal_trace_pause();
            ret = norflash_api_write(module,
                                     start_addr + written_len,
                                     ptr + written_len,
                                     write_len,
                                     is_async);
            hal_trace_continue();
            int_unlock_global(lock);

            if (NORFLASH_API_OK == ret)
            {
                APP_FLASH_TRACE(1,"%s: norflash_api_write ok!", __func__);
                written_len += write_len;
                break;
            }
            else if (NORFLASH_API_BUFFER_FULL == ret)
            {
                APP_FLASH_TRACE(1,"%s:buffer full! To flush it.", __func__);
                app_flash_flush_pending_op(module, NORFLASH_API_WRITTING);
            }
            else
            {
                ASSERT(0, "%s: norflash_api_write failed. ret = %d", __FUNCTION__, ret);
            }
        } while (1);
    }
    // APP_FLASH_TRACE(1,"%s: write: 0x%x,0x%x done.", __func__, start_addr, len);
}

SRAM_TEXT_LOC void app_flash_read(enum NORFLASH_API_MODULE_ID_T module,
                    uint32_t flashOffset,
                    uint8_t *ptr,
                    uint32_t len)
{
    /// get flash address according to the flash module and offset
    enum NORFLASH_API_RET_T ret = NORFLASH_API_OK;
    uint32_t start_addr = flashOffset;
    uint32_t lock;
    uint32_t sec_num;
    uint32_t read_len;
    uint32_t has_read_len = 0;
    uint32_t i;

    ret = norflash_api_get_base_addr(module, &start_addr);
    start_addr += flashOffset;
    ASSERT(NORFLASH_API_OK == ret,
           "%s %d ret:%d, module:%d, offset:%d, len:%d",
           __func__, __LINE__, ret, module, flashOffset, len);

    sec_num = (len + NORFLASH_API_SECTOR_SIZE -1)/NORFLASH_API_SECTOR_SIZE;
    for(i = 0; i < sec_num; i++)
    {
        if(len - has_read_len > NORFLASH_API_SECTOR_SIZE)
        {
            read_len = NORFLASH_API_SECTOR_SIZE;
        }
        else
        {
            read_len = len - has_read_len ;
        }

        lock = int_lock_global();
        /// read flash, OTA_MODULE should take flash re-map into consideration if it is enabled
        ret = norflash_api_read(module, start_addr + has_read_len, ptr + has_read_len, read_len);
        int_unlock_global(lock);
        ASSERT(NORFLASH_API_OK == ret, "%s %d ret:%d", __func__, __LINE__, ret);
        has_read_len += read_len;
    }
}

SRAM_TEXT_LOC void app_flash_sync_read(enum NORFLASH_API_MODULE_ID_T module,
                    uint32_t flashOffset,
                    uint8_t *ptr,
                    uint32_t len)
{
    /// get flash address according to the flash module and offset
    enum NORFLASH_API_RET_T ret = NORFLASH_API_OK;
    uint32_t start_addr = flashOffset;
    uint32_t lock;
    uint32_t sec_num;
    uint32_t read_len;
    uint32_t has_read_len = 0;
    uint32_t i;

    ret = norflash_api_get_base_addr(module, &start_addr);
    start_addr += flashOffset;
    ASSERT(NORFLASH_API_OK == ret,
           "%s %d ret:%d, module:%d, offset:%d, len:%d",
           __func__, __LINE__, ret, module, flashOffset, len);

    sec_num = (len + NORFLASH_API_SECTOR_SIZE -1)/NORFLASH_API_SECTOR_SIZE;
    for(i = 0; i < sec_num; i++)
    {
        if(len - has_read_len > NORFLASH_API_SECTOR_SIZE)
        {
            read_len = NORFLASH_API_SECTOR_SIZE;
        }
        else
        {
            read_len = len - has_read_len ;
        }

        lock = int_lock_global();
        /// read flash, OTA_MODULE should take flash re-map into consideration if it is enabled
        ret = norflash_sync_read(module, start_addr + has_read_len, ptr + has_read_len, read_len);
        int_unlock_global(lock);
        ASSERT(NORFLASH_API_OK == ret, "%s %d ret:%d", __func__, __LINE__, ret);
        has_read_len += read_len;
    }
}

