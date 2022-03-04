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
#include "cmsis.h"
#include "cmsis_nvic.h"
#include "hal_cache.h"
#include "hal_cmu.h"
#include "hal_iomux.h"
#include "hal_bootmode.h"
#include "hal_location.h"
#include "tool_msg.h"
#include "tgt_hardware.h"

typedef void (*FLASH_ENTRY)(void);

extern void programmer_start(void);

extern uint32_t __boot_sram_start__[];
extern uint32_t __boot_sram_end__[];

extern uint32_t __boot_sram_start_flash__[];
extern uint32_t __boot_sram_end_flash__[];

extern uint32_t __app_entry_address__[];

#define EXEC_CODE_BASE (FLASH_BASE + 96 * 1024)
static struct boot_struct_t *boot_struct = (struct boot_struct_t *)EXEC_CODE_BASE;

extern uint32_t programmer_get_magic_number(void);
extern bool programmer_need_run_bflash(void);
#ifdef OTA_BOOT_FROM_APP_FLASH_NC
extern uint32_t programmer_get_ota_app_boot_info_magic_number(void);
extern bool programmer_need_run_bflash_by_ota_app_boot_info(void);
extern bool programmer_get_single_mode_upgrade_from_ota_app_boot_info(void);
#endif

#ifdef SINGLE_WIRE_DOWNLOAD
static POSSIBLY_UNUSED int BOOT_TEXT_FLASH_LOC programmer_inflash_is_single_line_download_mode(void)
{
    uint32_t bootmode = hal_sw_bootmode_get();

    return (bootmode & HAL_SW_BOOTMODE_SINGLE_LINE_DOWNLOAD);
}
#else
static POSSIBLY_UNUSED int BOOT_TEXT_FLASH_LOC programmer_inflash_is_single_line_download_mode(void)
{
    return false;
}
#endif

int BOOT_TEXT_FLASH_LOC ProgrammerInflashEnterApp(void)
{
    boot_struct = (struct boot_struct_t *)__app_entry_address__[0];

    if (BOOT_MAGIC_NUMBER == boot_struct->hdr.magic) {
        FLASH_ENTRY entry;
        // Disable all IRQs
        NVIC_DisableAllIRQs();
        // Ensure in thumb state
        entry = (FLASH_ENTRY)FLASH_TO_FLASHX(&boot_struct->hdr + 1);
        entry = (FLASH_ENTRY)((uint32_t)entry | 1);
        entry();
        //can't run to here
        return 0;
    }
    return -1;
}

void BOOT_TEXT_FLASH_LOC ProgrammerInflashBootSelect(void)
{
#if defined(SINGLE_WIRE_DOWNLOAD) && defined(UNCONDITIONAL_ENTER_SINGLE_WIRE_DOWNLOAD)
    return;
#else

    bool isEnterSecondaryBootloader = false;
    if (is_ota_remap_enabled()) {
#ifdef SECURE_BOOT_WORKAROUND_SOLUTION
        if (programmer_inflash_is_single_line_download_mode() || programmer_get_single_mode_upgrade_from_ota_app_boot_info())
#else
        if (programmer_inflash_is_single_line_download_mode())
#endif
        {
            isEnterSecondaryBootloader = true;
        } else {
#ifdef OTA_BOOT_FROM_APP_FLASH_NC
            if (programmer_need_run_bflash() || programmer_need_run_bflash_by_ota_app_boot_info())
#else
            if (programmer_need_run_bflash())
#endif
            {
                isEnterSecondaryBootloader = true;
            }
        }
    } else {
        uint32_t magicNumber = programmer_get_magic_number();
#ifdef OTA_BOOT_FROM_APP_FLASH_NC
        uint32_t app_ota_boot_info_magic_number = programmer_get_ota_app_boot_info_magic_number();
#endif
#ifdef SECURE_BOOT_WORKAROUND_SOLUTION
        if (programmer_inflash_is_single_line_download_mode() || programmer_get_single_mode_upgrade_from_ota_app_boot_info())
#else
        if (programmer_inflash_is_single_line_download_mode())
#endif
        {
            isEnterSecondaryBootloader = true;
        } else {
#ifdef OTA_BOOT_FROM_APP_FLASH_NC
            if ((0x5a5a5a5a == magicNumber) || (0x5a5a5a5a == app_ota_boot_info_magic_number))
#else
            if (0x5a5a5a5a == magicNumber)
#endif
            {
                isEnterSecondaryBootloader = true;
            }
        }
    }

    if (isEnterSecondaryBootloader) {
        return;
    }
#endif

    // Workaround for reboot: controller in standard SPI mode while FLASH in QUAD mode
    // First read will fail when FLASH in QUAD mode, but it will make FLASH roll back to standard SPI mode
    // Second read will succeed
    ProgrammerInflashEnterApp();
}

void BOOT_TEXT_FLASH_LOC ProgrammerInflashBootInit(void)
{
    uint32_t *dst, *src;

    // Enable icache
    hal_cache_enable(HAL_CACHE_ID_I_CACHE);
    // Enable dcache
    hal_cache_enable(HAL_CACHE_ID_D_CACHE);
    // Enable write buffer
    hal_cache_writebuffer_enable(HAL_CACHE_ID_D_CACHE);

    // Init boot sections
    for (dst = __boot_sram_start__, src = __boot_sram_start_flash__; src < __boot_sram_end_flash__; dst++, src++) {
        *dst = *src;
    }
}

int BOOT_TEXT_FLASH_LOC programmer_inflash_main(int argc, char *argv[])
{
    programmer_start();
    hal_cmu_sys_reboot();
    return 0;
}

int _start(int, char **) __attribute__((weak, alias("programmer_inflash_main")));
