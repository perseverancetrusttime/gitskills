/***************************************************************************
 *
 * Copyright 2015-2020 BES.
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
#include "cmsis.h"
#include "cmsis_nvic.h"
#include "hal_cache.h"
#include "export_fn_rom.h"
#include "hal_location.h"
#include "hal_norflash.h"
#include "hal_trace.h"
#include "hal_wdt.h"
#include "pmu.h"
#include "string.h"
#include "sys_api_programmer.h"
#include "crc16_c.h"
#include "crc32_c.h"
#include "tgt_hardware.h"
#include "hal_bootmode.h"

#if defined(__COMPRESSED_IMAGE__) || defined(__APP_BIN_INTEGRALITY_CHECK__)
#include "ota_bin_check.h"

#if defined(__COMPRESSED_IMAGE__)
#include "hal_sysfreq.h"
static uint8_t xz_mem_pool[1024 * 160];
#endif

#endif

#ifndef OTA_BOOT_APP_BOOT_INFO_OFFSET
#define OTA_BOOT_APP_BOOT_INFO_OFFSET 0
#endif

typedef void (*FLASH_ENTRY)(void);

extern uint32_t __app_entry_address__[];

#define OTA_IMAGE_COPY_ENABLED
#define BOOT_MAGIC_NUMBER 0xBE57EC1C
#define COPY_NEW_IMAGE    0x5a5a5a5a
#define NEW_IMAGE_FLASH_OFFSET	            0x298000
#define NORMAL_BOOT                      BOOT_MAGIC_NUMBER
#define FLASH_SECTOR_SIZE_IN_BYTES       4096
#define OTA_DATA_BUFFER_SIZE_FOR_BURNING FLASH_SECTOR_SIZE_IN_BYTES
#if defined(CHIP_BEST2001) || defined(CHIP_BEST1501)
#define OTA_FLASH_LOGIC_ADDR 0x28000000
#else
#define OTA_FLASH_LOGIC_ADDR 0x3c000000
#endif

#define BOOT_WORD_A 0xAAAAAAAA
#define BOOT_WORD_B 0xBBBBBBBB
#ifndef __APP_IMAGE_FLASH_OFFSET__
#define __APP_IMAGE_FLASH_OFFSET__ (0x18000)
#endif

#ifndef OTA_REMAP_OFFSET
#define OTA_REMAP_OFFSET (0x200000)
#endif

#if defined(_OTA_BOOT_FAST_COPY_)
uint8_t dataBufferForBurning[OTA_DATA_BUFFER_SIZE_FOR_BURNING << 3];
#else
uint8_t dataBufferForBurning[OTA_DATA_BUFFER_SIZE_FOR_BURNING];
#endif

typedef struct {
    uint32_t magicNumber;   // NORMAL_BOOT or COPY_NEW_IMAGE
    uint32_t imageSize;
    uint32_t imageCrc;
    uint32_t newImageFlashOffset;
    uint32_t boot_word;
} FLASH_OTA_BOOT_INFO_T;

FLASH_OTA_BOOT_INFO_T otaBootInfoInFlash __attribute((section(".ota_boot_info"))) = {NORMAL_BOOT, 0, 0, __APP_IMAGE_FLASH_OFFSET__, BOOT_WORD_A};
/**add by lmzhang,20210416*/
uint8_t ota_bin_ver[4] = {1, 0, 4, 0};
/**add by lmzhang,20210416*/
#ifdef OTA_BOOT_FROM_APP_FLASH_NC
#if OTA_BOOT_APP_BOOT_INFO_OFFSET == 0
#error "OTA_BOOT_APP_BOOT_INFO_OFFSET = 0 under OTA_BOOT_FROM_APP_FLASH_NC"
#endif
#define APP_OTA_BOOT_INFO_ADDR (FLASH_BASE + OTA_BOOT_APP_BOOT_INFO_OFFSET)
#define APP_OTA_BOOT_INFO_SIZE (0x1000)
typedef struct {
    FLASH_OTA_BOOT_INFO_T flash_boot_info;
#ifdef SECURE_BOOT_WORKAROUND_SOLUTION
    uint32_t single_mode_en;
#endif
} APP_OTA_BOOT_INFO_FLASH_STRUCT_T;
static const APP_OTA_BOOT_INFO_FLASH_STRUCT_T *app_ota_boot_info_in_flash = (const APP_OTA_BOOT_INFO_FLASH_STRUCT_T *)APP_OTA_BOOT_INFO_ADDR;
#endif

uint32_t BOOT_TEXT_FLASH_LOC programmer_get_magic_number(void)
{
    volatile uint32_t *magic;

    magic = (volatile uint32_t *)&otaBootInfoInFlash;

    // First read (and also flush the controller prefetch buffer)
    *(magic + 0x400);

    return *magic;
}

#ifdef OTA_BOOT_FROM_APP_FLASH_NC
#ifdef SECURE_BOOT_WORKAROUND_SOLUTION
bool BOOT_TEXT_FLASH_LOC programmer_get_single_mode_upgrade_from_ota_app_boot_info(void)
{
    return (app_ota_boot_info_in_flash->single_mode_en == 1);
}

static void programmer_update_app_ota_boot_info_magic_number(uint32_t newMagicNumber);
void programmer_timeout_clr_single_mode_upgrade_from_ota_app_boot_info(void)
{
    programmer_update_app_ota_boot_info_magic_number(NORMAL_BOOT);
}
#endif

uint32_t BOOT_TEXT_FLASH_LOC programmer_get_ota_app_boot_info_magic_number(void)
{
    volatile uint32_t *magic;

    magic = (volatile uint32_t *)&(app_ota_boot_info_in_flash->flash_boot_info);

    return *magic;
}
#endif
uint32_t BOOT_TEXT_FLASH_LOC programmer_get_new_image_offset(void)
{
    return otaBootInfoInFlash.newImageFlashOffset;
}
#ifdef OTA_BOOT_FROM_APP_FLASH_NC
uint32_t BOOT_TEXT_FLASH_LOC programmer_get_ota_app_boot_info_new_image_offset(void)
{
    return app_ota_boot_info_in_flash->flash_boot_info.newImageFlashOffset;
}
#endif

bool BOOT_TEXT_FLASH_LOC programmer_need_run_bflash(void)
{
    bool ret = false;

    if (otaBootInfoInFlash.magicNumber == COPY_NEW_IMAGE && otaBootInfoInFlash.boot_word == BOOT_WORD_B) {
        ret = true;
    }

    return ret;
}

#ifdef OTA_BOOT_FROM_APP_FLASH_NC
bool BOOT_TEXT_FLASH_LOC programmer_need_run_bflash_by_ota_app_boot_info(void)
{
    bool ret = false;

    if (app_ota_boot_info_in_flash->flash_boot_info.magicNumber == COPY_NEW_IMAGE && app_ota_boot_info_in_flash->flash_boot_info.boot_word == BOOT_WORD_B) {
        ret = true;
    }
    return ret;
}
#endif

FLASH_OTA_BOOT_INFO_T bes_get_ota_boot_info(void)
{
    return otaBootInfoInFlash;
}

#ifdef OTA_IMAGE_COPY_ENABLED

void programmer_update_magic_number(uint32_t newMagicNumber)
{
    uint32_t lock;

    uint32_t addr = (uint32_t)&otaBootInfoInFlash;

    lock = int_lock();
    pmu_flash_write_config();
    hal_norflash_erase(HAL_FLASH_ID_0, addr, FLASH_SECTOR_SIZE_IN_BYTES);
    hal_norflash_write(HAL_FLASH_ID_0, addr, (uint8_t *)(&newMagicNumber), 4);
    pmu_flash_read_config();
    int_unlock(lock);
}

#ifdef OTA_BOOT_FROM_APP_FLASH_NC
static void programmer_update_app_ota_boot_info_magic_number(uint32_t newMagicNumber)
{
    uint32_t lock;

    uint32_t addr = (uint32_t)app_ota_boot_info_in_flash;

    lock = int_lock();
    pmu_flash_write_config();
    hal_norflash_erase(HAL_FLASH_ID_0, addr, FLASH_SECTOR_SIZE_IN_BYTES);
    hal_norflash_write(HAL_FLASH_ID_0, addr, (uint8_t *)(&newMagicNumber), 4);
    pmu_flash_read_config();
    int_unlock(lock);
}

static void programmer_destroy_update_app_ota_boot_info_section(void)
{
    uint32_t lock;

    uint32_t addr = (uint32_t)app_ota_boot_info_in_flash;

    lock = int_lock();
    pmu_flash_write_config();
    hal_norflash_erase(HAL_FLASH_ID_0, addr, FLASH_SECTOR_SIZE_IN_BYTES);
    pmu_flash_read_config();
    int_unlock(lock);
}
#endif

void programmer_copy_new_image(uint32_t srcFlashOffset, uint32_t dstFlashOffset)
{
    FLASH_OTA_BOOT_INFO_T otaBootInfo;

#ifdef OTA_BOOT_FROM_APP_FLASH_NC
    bool ota_boot_info_from_app_boot_info = false;
#endif

#ifdef OTA_BOOT_FROM_APP_FLASH_NC
    if (otaBootInfoInFlash.magicNumber == COPY_NEW_IMAGE) {
        memcpy((uint8_t *)&otaBootInfo, (uint8_t *)&otaBootInfoInFlash, sizeof(FLASH_OTA_BOOT_INFO_T));
    } else {
        ota_boot_info_from_app_boot_info = true;
        memcpy((uint8_t *)&otaBootInfo, (uint8_t *)&(app_ota_boot_info_in_flash->flash_boot_info), sizeof(FLASH_OTA_BOOT_INFO_T));
    }
#else
    memcpy((uint8_t *)&otaBootInfo, (uint8_t *)&otaBootInfoInFlash, sizeof(FLASH_OTA_BOOT_INFO_T));
#endif
    TRACE(0, "ota boot info is:");
    DUMP8("0x%02x ", (uint8_t *)&otaBootInfoInFlash, sizeof(FLASH_OTA_BOOT_INFO_T));
    uint32_t copiedDataSize = 0;
#if !defined(__APP_BIN_INTEGRALITY_CHECK__)
#ifdef __CRC16__
    uint32_t crcValue = 0xFFFF;
#else
    uint32_t crcValue = 0;
#endif
#endif
    uint32_t bytesToCopy = 0;
    uint32_t lock;

#ifdef __WATCHER_DOG_RESET__
    uint32_t copiedDataSizeSinceLastWdPing = 0;
    //uint32_t erasedDataSizeSinceLastWdPing = 0;
#endif

#if defined(_OTA_BOOT_FAST_COPY_)
    uint32_t sector_cnt = (otaBootInfo.imageSize / OTA_DATA_BUFFER_SIZE_FOR_BURNING);

    if ((otaBootInfo.imageSize % OTA_DATA_BUFFER_SIZE_FOR_BURNING) != 0)
        sector_cnt += 1;

    lock = int_lock();
    hal_norflash_erase(HAL_FLASH_ID_0, dstFlashOffset, OTA_DATA_BUFFER_SIZE_FOR_BURNING * sector_cnt);
    int_unlock(lock);
#endif

    while (copiedDataSize < otaBootInfo.imageSize) {
#if defined(_OTA_BOOT_FAST_COPY_)
        if (otaBootInfo.imageSize - copiedDataSize > (OTA_DATA_BUFFER_SIZE_FOR_BURNING << 3)) {
            bytesToCopy = (OTA_DATA_BUFFER_SIZE_FOR_BURNING << 3);
        }
#else
        if (otaBootInfo.imageSize - copiedDataSize > OTA_DATA_BUFFER_SIZE_FOR_BURNING) {
            bytesToCopy = OTA_DATA_BUFFER_SIZE_FOR_BURNING;
        }
#endif
        else {
            bytesToCopy = otaBootInfo.imageSize - copiedDataSize;
        }
        lock = int_lock();

        memcpy((uint8_t *)dataBufferForBurning, (uint8_t *)(OTA_FLASH_LOGIC_ADDR + srcFlashOffset + copiedDataSize), bytesToCopy);
#if defined(_OTA_BOOT_FAST_COPY_)
#else
        hal_norflash_erase(HAL_FLASH_ID_0, dstFlashOffset + copiedDataSize, OTA_DATA_BUFFER_SIZE_FOR_BURNING);
#endif
        hal_norflash_write(HAL_FLASH_ID_0, dstFlashOffset + copiedDataSize, dataBufferForBurning, bytesToCopy);

        int_unlock(lock);

        if (0 == copiedDataSize) {
            *(uint32_t *)dataBufferForBurning = 0xFFFFFFFF;
        }

        copiedDataSize += bytesToCopy;

#if !defined(__APP_BIN_INTEGRALITY_CHECK__)
#ifdef __CRC16__
        crcValue = crc16ccitt(crcValue, (uint8_t *)dataBufferForBurning, 0, bytesToCopy);
#else
        crcValue = crc32_c(crcValue, (uint8_t *)dataBufferForBurning, bytesToCopy);
#endif
#endif

#ifdef __WATCHER_DOG_RESET__
        copiedDataSizeSinceLastWdPing += bytesToCopy;
        if (copiedDataSizeSinceLastWdPing > 20 * OTA_DATA_BUFFER_SIZE_FOR_BURNING) {
            programmer_watchdog_ping();
            copiedDataSizeSinceLastWdPing = 0;
        }
#endif
    }

#if !defined(__APP_BIN_INTEGRALITY_CHECK__)
    TRACE_IMM(2, "Original CRC32 is 0x%x Confirmed CRC32 is 0x%x.", otaBootInfo.imageCrc, crcValue);
    if (crcValue == otaBootInfo.imageCrc) {
#ifdef OTA_BOOT_FROM_APP_FLASH_NC
        if (ota_boot_info_from_app_boot_info == false) {
            programmer_update_magic_number(NORMAL_BOOT);
        } else {
            programmer_update_app_ota_boot_info_magic_number(NORMAL_BOOT);
        }
#else
        programmer_update_magic_number(NORMAL_BOOT);
#endif
    } else {
#ifdef OTA_BOOT_FROM_APP_FLASH_NC
        if (ota_boot_info_from_app_boot_info == true) {
            programmer_destroy_update_app_ota_boot_info_section();
        }
#endif
    }

#if defined(_OTA_BOOT_FAST_COPY_)
    lock = int_lock();
    hal_norflash_erase(HAL_FLASH_ID_0, (OTA_FLASH_LOGIC_ADDR + srcFlashOffset), OTA_DATA_BUFFER_SIZE_FOR_BURNING * sector_cnt);
    int_unlock(lock);
#endif

    /*     uint32_t erasedDataSize = 0;

    while (erasedDataSize < otaBootInfo.imageSize)
    {
        lock = int_lock();
        hal_norflash_erase(HAL_FLASH_ID_0, (OTA_FLASH_LOGIC_ADDR + srcFlashOffset + erasedDataSize),
                OTA_DATA_BUFFER_SIZE_FOR_BURNING);
        int_unlock(lock);
        erasedDataSize += OTA_DATA_BUFFER_SIZE_FOR_BURNING;

#ifdef __WATCHER_DOG_RESET__
        erasedDataSizeSinceLastWdPing += erasedDataSize;
        if (erasedDataSizeSinceLastWdPing > 20*OTA_DATA_BUFFER_SIZE_FOR_BURNING)
        {
            hal_wdt_ping(HAL_WDT_ID_0);
            erasedDataSizeSinceLastWdPing = 0;
        }
#endif
    } */

    if (crcValue == otaBootInfo.imageCrc) {
#ifdef SECURE_BOOT_WORKAROUND_SOLUTION
        programmer_watchdog_reconfigure(1);
#endif
        //pmu_reboot();
        hal_cmu_sys_reboot();
        while (1) {
        }
    }
#else
    if (ota_bin_hmac_sha256_check(dataBufferForBurning, dstFlashOffset, otaBootInfoInFlash.imageSize) == false) {
        TRACE_IMM(1, "%s,new image bin hmac-sha256 check failed!", __func__);
    } else {
        app_update_magic_number_of_app_image(dataBufferForBurning, NORMAL_BOOT);
        programmer_update_magic_number(NORMAL_BOOT);

        TRACE_IMM(1, "%s,new image bin hmac-sha256 check success,now reboot!", __func__);
    }
    osDelay(300);
    //pmu_reboot();
    hal_cmu_sys_reboot();
    while (1) {
    }
#endif
}

#define OTA_RETRY_INFO_ADDR (80 * 1024)

uint8_t get_ota_retry_info(void)
{
    memcpy((uint8_t *)dataBufferForBurning, (uint8_t *)(OTA_FLASH_LOGIC_ADDR + OTA_RETRY_INFO_ADDR), OTA_DATA_BUFFER_SIZE_FOR_BURNING);
    return dataBufferForBurning[0];
}

void update_ota_retry_info(uint8_t retry_cnt, bool uncompress_flag)
{
    enum HAL_NORFLASH_RET_T flash_ret = HAL_NORFLASH_OK;

    memset(dataBufferForBurning, 0x00, sizeof(OTA_DATA_BUFFER_SIZE_FOR_BURNING));
    dataBufferForBurning[0] = (uncompress_flag << 7) | retry_cnt;
    TRACE(1, "ota_retry_info_addr:0x%x...", OTA_RETRY_INFO_ADDR);

    uint32_t lock = int_lock();
    pmu_flash_write_config();
    flash_ret = hal_norflash_erase(HAL_FLASH_ID_0, OTA_RETRY_INFO_ADDR, sizeof(dataBufferForBurning));
    if (flash_ret != HAL_NORFLASH_OK) {
        TRACE(3, "%s: erase %p fail, ret %d", __func__, (uint8_t *)OTA_RETRY_INFO_ADDR, flash_ret);
    }

    flash_ret = hal_norflash_write(HAL_FLASH_ID_0, OTA_RETRY_INFO_ADDR, dataBufferForBurning, sizeof(dataBufferForBurning));
    if (flash_ret != HAL_NORFLASH_OK) {
        TRACE(3, "%s: write %p fail, ret %d", __func__, (uint8_t *)OTA_RETRY_INFO_ADDR, flash_ret);
    }

#if OTA_NON_CACHE_READ_ISSUE_WORKAROUND
    hal_cache_invalidate(HAL_CACHE_ID_D_CACHE, OTA_RETRY_INFO_ADDR, sizeof(dataBufferForBurning));
#endif

    pmu_flash_read_config();
    int_unlock(lock);
    programmer_watchdog_ping();
}

void copy_new_image(void)
{
    uint32_t magicNumber = programmer_get_magic_number();
    uint32_t new_image_copy_flash_offset;
    uint32_t ota_flash_offset_of_image;
    uint8_t ret = 0;

    TRACE(1, "magic num 0x%x imgsize 0x%02x", magicNumber,otaBootInfoInFlash.imageSize);
#ifdef OTA_BOOT_FROM_APP_FLASH_NC
    uint32_t app_ota_boot_info_magic_number = programmer_get_ota_app_boot_info_magic_number();
    uint32_t app_ota_boot_info_new_image_copy_flash_offset = programmer_get_ota_app_boot_info_new_image_offset();
    TRACE(1, "app boot info magic num 0x%x", app_ota_boot_info_magic_number);
#ifdef SECURE_BOOT_WORKAROUND_SOLUTION
    TRACE(1, "singleMode %d", programmer_get_single_mode_upgrade_from_ota_app_boot_info());
#endif
#endif

    new_image_copy_flash_offset = programmer_get_new_image_offset();

#ifdef OTA_BOOT_FROM_APP_FLASH_NC
    if ((COPY_NEW_IMAGE == magicNumber) || (COPY_NEW_IMAGE == app_ota_boot_info_magic_number))
#else
    if (COPY_NEW_IMAGE == magicNumber)
#endif
    {
        hal_norflash_disable_protection(HAL_FLASH_ID_0);
        ota_flash_offset_of_image = __APP_IMAGE_FLASH_OFFSET__;
        TRACE(2, "Start copying new image from 0x%x to 0x%x...", new_image_copy_flash_offset, ota_flash_offset_of_image);

#if defined(__COMPRESSED_IMAGE__)
        hal_sysfreq_req(HAL_SYSFREQ_USER_APP_0, HAL_CMU_FREQ_104M);

        uint8_t try_cnt = (get_ota_retry_info() & 0x7f);
        uint8_t failed_flag = 0;
        bool uncompress_flag = (bool)(get_ota_retry_info() >> 7);
        if (get_ota_retry_info() == 0xff) {
            try_cnt = 0;
            uncompress_flag = false;
        }

        TRACE(3, "%s,try_cnt:%d,uncompress_flag:%d.", __func__, try_cnt, uncompress_flag);
        if (try_cnt > 5) {
            try_cnt = 0;
            update_ota_retry_info(try_cnt, uncompress_flag);
            if (uncompress_flag == false) {
                app_update_magic_number_of_app_image(dataBufferForBurning, NORMAL_BOOT);
                programmer_update_magic_number(NORMAL_BOOT);
                TRACE_IMM(1, "%s,uncompress image failed,but not uncompress,now reboot!", __func__);
                osDelay(300);
                pmu_reboot();
            } else {
                TRACE_IMM(1, "%s,uncompress image failed and upcompressed,now shutdown!", __func__);
                osDelay(300);
                hal_sw_bootmode_clear(HAL_SW_BOOTMODE_REBOOT);
                pmu_shutdown();
            }
            while (1) {
            };
        }

        if (!app_check_compressed_image(new_image_copy_flash_offset, otaBootInfoInFlash.imageSize)) {
            //check compressed bin xz crc-head magic failed
            failed_flag = 1;
            goto exit;
        }

        ret = app_copy_compressed_image(new_image_copy_flash_offset, ota_flash_offset_of_image, xz_mem_pool, sizeof(xz_mem_pool));

        if (ret == 2) {
            //attempt to decompress failed
            failed_flag = 2;
            goto exit;
        } else if (ret == 3) {
            //uncompress ota bin failed or hmac-sha256 check failed
            failed_flag = 3;
            goto exit;
        } else {
            //uncompress ota bin success
            failed_flag = 0;
        }

    exit:
        if (failed_flag == 0) {
            app_update_magic_number_of_app_image(dataBufferForBurning, NORMAL_BOOT);
            programmer_update_magic_number(NORMAL_BOOT);
            try_cnt = 0;
            uncompress_flag = false;
            TRACE_IMM(1, "%s,uncompress new image bin hmac-sha256 check success,now reboot!", __func__);
        } else {
            try_cnt++;
            if (failed_flag == 3) {
                uncompress_flag = true;
            }
            TRACE_IMM(3, "%s,%d,check compressed image failed,now reboot,%d!", __func__, failed_flag, try_cnt);
        }
        update_ota_retry_info(try_cnt, uncompress_flag);
        osDelay(300);
        pmu_reboot();
        while (1) {
        };
#elif defined(OTA_BOOT_FROM_APP_FLASH_NC)
        if (COPY_NEW_IMAGE == magicNumber) {
            TRACE(2, "Start copying new image from 0x%x to 0x%x...", new_image_copy_flash_offset, ota_flash_offset_of_image);
            //we will reboot once copy success
            programmer_copy_new_image(new_image_copy_flash_offset, ota_flash_offset_of_image);
        } else {
            TRACE(2, "Start copying new image from 0x%x to 0x%x...", app_ota_boot_info_new_image_copy_flash_offset, ota_flash_offset_of_image);
            //we will reboot once copy success
            programmer_copy_new_image(app_ota_boot_info_new_image_copy_flash_offset, ota_flash_offset_of_image);
        }

#else
        //we will reboot once copy success
        programmer_copy_new_image(new_image_copy_flash_offset, ota_flash_offset_of_image);
        //TRACE(0,"Copying new image failed, enter OTA mode.");
        TRACE(0, "Copying new image failed, enter Single Download mode.");
#endif
    }
}
#endif

void remap_start_b_image(void)
{
    const struct boot_struct_t *boot_struct;
    FLASH_ENTRY entry;
    uint32_t magicNumber = programmer_get_magic_number();
#ifdef OTA_BOOT_FROM_APP_FLASH_NC
    uint32_t app_ota_boot_info_magic_number = programmer_get_ota_app_boot_info_magic_number();
#endif
    uint32_t lock = 0;

    TRACE(2, "magic num 0x%x addr %p", magicNumber, &otaBootInfoInFlash);

#ifdef OTA_BOOT_FROM_APP_FLASH_NC
    if ((COPY_NEW_IMAGE == magicNumber) || (COPY_NEW_IMAGE == app_ota_boot_info_magic_number))
#else
    if (COPY_NEW_IMAGE == magicNumber)
#endif
    {
        uint32_t ota_remap_offset;
#ifdef OTA_BOOT_FROM_APP_FLASH_NC
        if (magicNumber == COPY_NEW_IMAGE) {
            ota_remap_offset = programmer_get_new_image_offset();
        } else {
            ota_remap_offset = programmer_get_ota_app_boot_info_new_image_offset();
        }
#else
        ota_remap_offset = programmer_get_new_image_offset();
#endif
        uint32_t dstLogicAddr = FLASH_BASE + __APP_IMAGE_FLASH_OFFSET__;
        uint32_t len = ota_remap_offset - __APP_IMAGE_FLASH_OFFSET__;
        uint32_t srcPhysicalAddr = ota_remap_offset;

        // Remap boot
        lock = int_lock();
        enum HAL_NORFLASH_RET_T ret = hal_norflash_boot_config_remap(HAL_FLASH_ID_0, dstLogicAddr, len, srcPhysicalAddr);
        if (ret != HAL_NORFLASH_OK) {
            TRACE_IMM(1, "*** ERROR: Failed to config remap: %d", ret);
            goto _err;
        }
        ret = hal_norflash_boot_enable_remap(HAL_FLASH_ID_0);
        if (ret != HAL_NORFLASH_OK) {
            TRACE_IMM(1, "*** ERROR: Failed to enable remap: %d", ret);
            goto _err;
        }
        hal_cache_invalidate(HAL_CACHE_ID_I_CACHE, dstLogicAddr, len);
        hal_cache_invalidate(HAL_CACHE_ID_D_CACHE, dstLogicAddr, len);

        boot_struct = (const struct boot_struct_t *)(FLASH_NC_BASE + __APP_IMAGE_FLASH_OFFSET__);
        TRACE_IMM(1, "---> Remap boot magic 0x%x", boot_struct->hdr.magic);
        TRACE_IMM(2, "app_entry_address %p boot %p", (struct boot_struct_t *)__app_entry_address__[0], boot_struct);
        TRACE_IMM(1, "otaBootInfoInFlash addr %p", &otaBootInfoInFlash);
        TRACE_IMM(1, "%s boot_word 0x%x", __func__, otaBootInfoInFlash.boot_word);
        if (boot_struct->hdr.magic == BOOT_MAGIC_NUMBER) {
            int_unlock(lock);
            entry = (FLASH_ENTRY)FLASH_TO_FLASHX(FLASH_NC_TO_C((uint32_t)(&boot_struct->hdr + 1)));
            entry = (FLASH_ENTRY)((uint32_t)entry | 1);
            // Disable all IRQs
            NVIC_DisableAllIRQs();
            entry();
        }
    }

_err:
    int_unlock(lock);
    return;
}

static void ota_copy_init(void)
{
    /**add by lmzhang, 20210416*/
    TRACE(4, "The OTA software rev is %d.%d.%d.%d", ota_bin_ver[0], ota_bin_ver[1], ota_bin_ver[2], ota_bin_ver[3]);
    /**add by lmzhang, 20210416*/

    if (is_ota_remap_enabled()) {
        remap_start_b_image();
    } else {
#if defined(OTA_IMAGE_COPY_ENABLED)
        TRACE(0, "begin copy_new_image");
        // flash has been initialized in init_download_context()
        //init_flash();       //add for hal_norflash_open failed when copy image we need operate falash
        copy_new_image();
        TRACE(0, "end copy_new_image");
#endif
    }
}

static const CUST_CMD_INIT_T CUST_CMD_INIT_TBL_LOC mod_init[] = {
    ota_copy_init,
};
