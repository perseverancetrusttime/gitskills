#include "stdio.h"
#include "cmsis.h"
#include "cmsis_os.h"
#include "plat_addr_map.h"
#include "string.h"
#include "hal_norflash.h"
#include "hal_location.h"
#include "hal_trace.h"
#include "ota_bin_update.h"
#include "tgt_hardware.h"

#define BOOT_MAGIC_NUMBER           0xBE57EC1C
#define FLASH_CACHE_SIZE            (1024*4)
#define CACHE_2_UNCACHE(addr)       ((unsigned char *)((unsigned int)(addr) & ~(0x04000000)))

/*
The size must identify with OTA BIN SIZE
*/
#define FLASH_OTA_SIZE              (BOOT_UPDATE_SECTION_SIZE)

extern void syspool_init();
extern int syspool_get_buff(uint8_t **buff, uint32_t size);

static int apps_ota_bin_update(void)
{
    uint32_t write_offset = 0;
    uint32_t write_page_size = 0;
    uint32_t need_write_size = 0;
    uint32_t write_buf = BOOT_MAGIC_NUMBER;
    uint8_t *flash_cache = NULL;

    TRACE(1, "%s.", __func__);

    //hal_norflash_disable_protection(HAL_FLASH_ID_0);

    uint32_t lock = int_lock_global();
    need_write_size = get_ota_bin_size();

    syspool_init();
    syspool_get_buff((uint8_t **)&flash_cache, FLASH_CACHE_SIZE);

    hal_norflash_erase(HAL_FLASH_ID_0, (uint32_t)CACHE_2_UNCACHE(FLASH_BASE), FLASH_OTA_SIZE);

    do{
        write_page_size = (need_write_size-write_offset) < (FLASH_CACHE_SIZE) ?
            need_write_size-write_offset:
            FLASH_CACHE_SIZE;

        memcpy(flash_cache, boot_bin+write_offset, write_page_size);
        hal_norflash_erase(HAL_FLASH_ID_0,(uint32_t)CACHE_2_UNCACHE(FLASH_BASE+write_offset), write_page_size);
        hal_norflash_write(HAL_FLASH_ID_0,(uint32_t)CACHE_2_UNCACHE(FLASH_BASE+write_offset),(uint8_t *)(flash_cache), write_page_size);
        write_offset += write_page_size;
    }while(write_offset < need_write_size);

    hal_norflash_write(HAL_FLASH_ID_0,(uint32_t)CACHE_2_UNCACHE(FLASH_BASE),(uint8_t *)(&write_buf), 4);

    int_unlock_global(lock);

    //hal_norflash_enable_protection(HAL_FLASH_ID_0);

    return 0;
}

int apps_ota_bin_ver_check(void)
{
    TRACE(3, "%s,0x%08x,0x%08x", __func__, FLASH_REGION_BASE, FLASH_BASE);

    if (FLASH_REGION_BASE == FLASH_BASE){
        return 0;
    }

    uint8_t *flash_cache = NULL;
    uint32_t current_ota_bin_version = 0;
    uint32_t new_ota_bin_version = 0;

    syspool_init();
    syspool_get_buff((uint8_t **)&flash_cache, FLASH_CACHE_SIZE);

    /**get ota version*/
    uint32_t lock = int_lock();
    hal_norflash_read(HAL_FLASH_ID_0, 0, flash_cache, FLASH_CACHE_SIZE);
    int_unlock(lock);

    current_ota_bin_version = flash_cache[0];
    current_ota_bin_version <<= 8;
    current_ota_bin_version += flash_cache[1];
    current_ota_bin_version <<= 8;
    current_ota_bin_version += flash_cache[2];
    current_ota_bin_version <<= 8;
    current_ota_bin_version += flash_cache[3];

    new_ota_bin_version = boot_bin[0];
    new_ota_bin_version <<= 8;
    new_ota_bin_version += boot_bin[01];
    new_ota_bin_version <<= 8;
    new_ota_bin_version += boot_bin[02];
    new_ota_bin_version <<= 8;
    new_ota_bin_version += boot_bin[03];

    TRACE(6, "%s,current ota bin version:%d.%d.%d.%d,version number:%d.",
            __func__,
            flash_cache[0],
            flash_cache[1],
            flash_cache[2],
            flash_cache[3],
            current_ota_bin_version);

    TRACE(6, "%s,new ota bin version:%d.%d.%d.%d,version number:%d.",
            __func__,
            boot_bin[0],
            boot_bin[01],
            boot_bin[02],
            boot_bin[03],
            new_ota_bin_version);
            
        TRACE(1, "%s boot bin data:", __func__);
        DUMP8("%02x ", &boot_bin[50], 100);
        TRACE(1, "%s local bin data:", __func__);
        DUMP8("%02x ", &flash_cache[50], 100);

    if (!memcmp((void *)&boot_bin[50], (void *)&flash_cache[50], FLASH_CACHE_SIZE-50)) {
        TRACE(1, "%s the ota bin so same!!!!", __func__);
        return 0;
    }
    /*
    if(current_ota_bin_version >= new_ota_bin_version)
    {
        return 0;
    }
*/
    apps_ota_bin_update();

    return 0;
}


