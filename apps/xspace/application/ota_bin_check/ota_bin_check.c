#include "stdio.h"
#include "cmsis.h"
#include "cmsis_os.h"
#include "plat_addr_map.h"
#include "string.h"
#include "hal_norflash.h"
#include "hal_location.h"
#include "hal_trace.h"
#include "ota_bin_check.h"
#include "hmac_sha256.h"
#include "hal_wdt.h"
#include "pmu.h"

#define FLASH_SECTOR_SIZE_IN_BYTES       4096
#define OTA_DATA_BUFFER_SIZE_FOR_BURNING FLASH_SECTOR_SIZE_IN_BYTES

#if defined(CHIP_BEST2001) || defined(CHIP_BEST1501)
#define OTA_FLASH_LOGIC_ADDR 0x28000000
#else
#define OTA_FLASH_LOGIC_ADDR 0x38000000
#endif

#ifdef __COMPRESSED_IMAGE__
#include "xz.h"
#include "xz_stream.h"
#include "crc32_c.h"

#define BOOT_MAGIC_NUMBER 0xBE57EC1C
#define NORMAL_BOOT       BOOT_MAGIC_NUMBER

#define XZ_DICT_MAX          (64 * 1024)
#define crc32(crc, buf, len) crc32_c(crc, buf, len)
typedef struct {
    uint32_t magicNumber;   // NORMAL_BOOT or COPY_NEW_IMAGE
    uint32_t imageSize;
    uint32_t imageCrc;
    uint32_t newImageFlashOffset;
    uint32_t boot_word;
} FLASH_OTA_BOOT_INFO_T;

static uint8_t bufForFlashReading[OTA_DATA_BUFFER_SIZE_FOR_BURNING];
static uint8_t dataBufferForBurning[OTA_DATA_BUFFER_SIZE_FOR_BURNING];

extern void programmer_update_magic_number(uint32_t newMagicNumber);
extern FLASH_OTA_BOOT_INFO_T programmer_get_new_image_offset(void);
extern FLASH_OTA_BOOT_INFO_T bes_get_ota_boot_info(void);
extern void programmer_watchdog_ping(void);
#endif

#if defined(__APP_BIN_INTEGRALITY_CHECK__)
#define HMAC_SHA256_KEY_LEN   (32)
#define HMAC_SHA256_VALUE_LEN (32)
#define HMAC_TEMP_DATA_LEN    (64)

#define HMAC_SHA256_VALUE_LEN (32)

#if defined(__PROJ_XSPACE__)
static uint8_t hmac_key[HMAC_SHA256_KEY_LEN] = {
    //TODO(Mike.Cheng)
    #if 0
    0x72, 0x73, 0xA3, 0x9B, 0x81, 0x00, 0x4C, 0xB0, 0x6B, 0x97, 0xA6, 0x9F, 0x07, 0x7C, 0x34, 0x74,
    0x70, 0xFA, 0x61, 0xA8, 0x97, 0x4E, 0x56, 0xAB, 0x44, 0x65, 0x62, 0x5C, 0x73, 0x22, 0x5F, 0xC4,
    #endif

    0xac, 0x0e, 0x2a, 0xbd, 0x6b, 0x40, 0xeb, 0xe7, 
    0xca, 0xfa, 0xef, 0x0e, 0x0c, 0x08, 0x3b, 0x4f, 
    0x12, 0xa8, 0xfa, 0x41, 0xad, 0x00, 0x75, 0x2b, 
    0xf3, 0xc9, 0x3f, 0xaf, 0xd4, 0x45, 0xac, 0x27 
};
#endif

static uint8_t hmac_value_ori[HMAC_SHA256_VALUE_LEN] = {0};
static uint8_t hmac_value_calc[HMAC_SHA256_VALUE_LEN] = {0};

bool ota_bin_hmac_sha256_check(uint8_t *_4k_pool, uint32_t image_flash_offset, uint32_t image_size)
{
    bool ret = false;
    uint16_t i = 0;
    uint32_t realImageSize = 0;
    uint32_t copiedDataSize = 0;
    uint32_t bytesToCopy = 0;
    uint32_t read_addr = 0x00;

#ifdef __WATCHER_DOG_RESET__
    uint32_t copiedDataSizeSinceLastWdPing = 0;
#endif

    TRACE(3, "%s enter,image_flash_offset:0x%08x,image_size:%d.", __func__, image_flash_offset, image_size);

    uint32_t readIndex = image_flash_offset + image_size;
    if (readIndex % OTA_DATA_BUFFER_SIZE_FOR_BURNING == 0) {
        read_addr = (readIndex / 4096 - 1) * 4096;
        realImageSize = (image_size / 4096 - 1) * 4096;
    } else {
        read_addr = readIndex / 4096 * 4096;
        realImageSize = (image_size / 4096) * 4096;
    }

    TRACE(1, "ota boot info is:read_addr=0x%x", read_addr);

    uint32_t lock = int_lock();
    hal_norflash_read(HAL_FLASH_ID_0, read_addr, _4k_pool, OTA_DATA_BUFFER_SIZE_FOR_BURNING);
    int_unlock(lock);

    uint32_t sha_index = OTA_DATA_BUFFER_SIZE_FOR_BURNING;
    for (i = OTA_DATA_BUFFER_SIZE_FOR_BURNING - 1; i >= 2; i--) {
        if ((_4k_pool[i] == 'a') && (_4k_pool[i - 1] == 'h') && (_4k_pool[i - 2] == 's')) {
            sha_index = i - 2;
            break;
        }
    }

    if (sha_index == OTA_DATA_BUFFER_SIZE_FOR_BURNING) {
        TRACE(2, "%s,Not include sha!!!,i:%d", __func__, i);
        return false;
    }

    realImageSize += sha_index;
    sha_index += 3;
    TRACE(2, "%s,sha_index=%d", __func__, sha_index);

    memcpy(hmac_value_ori, _4k_pool + sha_index, HMAC_SHA256_VALUE_LEN);

    hmac_sha256_init(hmac_key, HMAC_SHA256_KEY_LEN);

    while (copiedDataSize < realImageSize) {
        if (realImageSize - copiedDataSize > OTA_DATA_BUFFER_SIZE_FOR_BURNING) {
            bytesToCopy = OTA_DATA_BUFFER_SIZE_FOR_BURNING;
        } else {
            bytesToCopy = realImageSize - copiedDataSize;
        }

        lock = int_lock();
        hal_norflash_read(HAL_FLASH_ID_0, image_flash_offset + copiedDataSize, _4k_pool, OTA_DATA_BUFFER_SIZE_FOR_BURNING);
        int_unlock(lock);

//dump update ota bin data for debug
#if 0
        {
            //TRACE("%s,bytesToCopy=%d", __func__, bytesToCopy);
            uint16_t dump_temp_len = 0;
            uint16_t dumped_len = 0;
            uint16_t addr = 0;
            while(dumped_len < bytesToCopy)
            {
                if (bytesToCopy - dumped_len > 16)
                {
                    dump_temp_len = 16;
                }
                else
                {
                    dump_temp_len = bytesToCopy - dumped_len;
                }

                DUMP8("0x%02x,", _4k_pool + addr, dump_temp_len);
                if(i % 20 == 0)
                {
                    osDelay(20);
                }
                i++;

                addr += dump_temp_len;
                dumped_len += dump_temp_len;
            }
        }
#endif

        copiedDataSize += bytesToCopy;

        hmac_sha256_update((unsigned char *)_4k_pool, bytesToCopy);

#ifdef __WATCHER_DOG_RESET__
        copiedDataSizeSinceLastWdPing++;
        if (copiedDataSizeSinceLastWdPing % 10 == 0) {
            hal_wdt_ping(HAL_WDT_ID_0);
            copiedDataSizeSinceLastWdPing = 0;
        }
#endif
    }

    int len = HMAC_SHA256_VALUE_LEN;
    hmac_sha256_final((unsigned char *)hmac_value_calc, &len);

    TRACE(0, "hmac_value_ori:");
    DUMP8("0x%02x ", hmac_value_ori, HMAC_SHA256_VALUE_LEN);
    TRACE(0, "hmac_value_calc:");
    DUMP8("0x%02x ", hmac_value_calc, HMAC_SHA256_VALUE_LEN);

    if (memcmp(hmac_value_ori, hmac_value_calc, HMAC_SHA256_VALUE_LEN) == 0) {
        ret = true;
        TRACE(1, "%s,key match!!!", __func__);
    } else {
        TRACE(1, "%s,key not match!!!", __func__);
    }

    return ret;
}
#endif

void app_update_magic_number_of_app_image(uint8_t *_4k_pool, uint32_t newMagicNumber)
{
    uint8_t *dstFlashPtr = (uint8_t *)OTA_FLASH_LOGIC_ADDR + __APP_IMAGE_FLASH_OFFSET__;

    memcpy((uint8_t *)_4k_pool, (uint8_t *)dstFlashPtr, OTA_DATA_BUFFER_SIZE_FOR_BURNING);
    *(uint32_t *)_4k_pool = newMagicNumber;
    uint32_t lock = int_lock();
    pmu_flash_write_config();
    hal_norflash_erase(HAL_FLASH_ID_0, __APP_IMAGE_FLASH_OFFSET__, OTA_DATA_BUFFER_SIZE_FOR_BURNING);
    hal_norflash_write(HAL_FLASH_ID_0, __APP_IMAGE_FLASH_OFFSET__, _4k_pool, OTA_DATA_BUFFER_SIZE_FOR_BURNING);
    pmu_flash_read_config();
    int_unlock(lock);
}

#ifdef __COMPRESSED_IMAGE__
bool app_check_compressed_image(uint32_t srcFlashOffset, uint32_t imageSize)
{
    uint8_t *srcFlashPtr = (uint8_t *)OTA_FLASH_LOGIC_ADDR + srcFlashOffset;
    uint8_t *srcFlashEndPtr = 0;
    uint32_t crc32_result = 0;
    uint16_t end_offset = 0;

    //Check header
    TRACE(2, "OTA_FLASH_LOGIC_ADDR 0x%x srcFlashPtr:0x%x.", (uint32_t)OTA_FLASH_LOGIC_ADDR,(uint32_t)srcFlashPtr);
    if (memcmp(srcFlashPtr, HEADER_MAGIC, HEADER_MAGIC_SIZE)) {
        TRACE(1, "%s: header magic is wrong!!!", __func__);
        DUMP8("%02x ", srcFlashPtr, HEADER_MAGIC_SIZE);
#if 0
        TRACE(0,"\n");
        DUMP8("%02x ", HEADER_MAGIC, HEADER_MAGIC_SIZE);
#endif
        return false;
    }

    crc32_result = crc32(0, srcFlashPtr + HEADER_MAGIC_SIZE, 2);
    if (memcmp((uint8_t *)&crc32_result, srcFlashPtr + HEADER_MAGIC_SIZE + 2, 4)) {
        TRACE(2, "%s: header crc32 is wrong, result %08x", __func__, crc32_result);
        DUMP8("%02x ", (uint8_t *)srcFlashOffset + HEADER_MAGIC_SIZE, 6);
        return false;
    }

    //Check footer
    srcFlashEndPtr = srcFlashPtr + imageSize;
    TRACE(2, "%s: otaBootInfo.imageSize %d", __func__, imageSize);
    for (end_offset = 0; end_offset < FLASH_SECTOR_SIZE_IN_BYTES; end_offset += 2) {
        if (!memcmp(srcFlashEndPtr - end_offset - FOOTER_MAGIC_SIZE, FOOTER_MAGIC, FOOTER_MAGIC_SIZE)) {
            TRACE(2, "%s: footer magic is right, offset %d.", __func__, end_offset);
            srcFlashEndPtr -= end_offset;
            break;
        }
    }

    if (end_offset >= FLASH_SECTOR_SIZE_IN_BYTES) {
        TRACE(1, "%s: footer magic is not found!!!", __func__);
        for (uint16_t i = 0; i < FLASH_SECTOR_SIZE_IN_BYTES; i += 64) {
            DUMP8("%02x ", (uint8_t *)srcFlashEndPtr - FLASH_SECTOR_SIZE_IN_BYTES + i, 64);
        }
        return false;
    }

    crc32_result = crc32(0, srcFlashEndPtr - FOOTER_MAGIC_SIZE - 6, 6);
    if (memcmp((uint8_t *)&crc32_result, srcFlashEndPtr - FOOTER_MAGIC_SIZE - 10, 4)) {
        TRACE(2, "%s: footer crc32 is wrong, result %08x", __func__, crc32_result);
        DUMP8("%02x ", (uint8_t *)srcFlashEndPtr - HEADER_MAGIC_SIZE - 10, 10);
        return false;
    }

    return true;
}

bool app_check_compressed_image2(uint32_t srcFlashOffset, uint8_t *xz_mem_pool, uint32_t xz_mem_pool_size)
{
    //NOTE(Mike.Cheng):unuseless api
    return true;
}

uint8_t app_copy_compressed_image(uint32_t srcFlashOffset, uint32_t dstFlashOffset, uint8_t *xz_mem_pool, uint32_t xz_mem_pool_size)
{
    FLASH_OTA_BOOT_INFO_T otaBootInfo = bes_get_ota_boot_info();
    uint8_t *srcFlashPtr = (uint8_t *)OTA_FLASH_LOGIC_ADDR + srcFlashOffset;
    uint32_t readedDataSize = 0, burnedDataSize = 0;
    uint32_t bytesToRead = 0;
    uint32_t lock;
#ifdef __WATCHER_DOG_RESET__
    uint32_t writedDataSizeSinceLastWdPing = 0;
#endif
    enum HAL_NORFLASH_RET_T flash_ret;

    struct xz_buf xz_b;
    struct xz_dec *xz_status;
    enum xz_ret ret;

    //init crc32 for xz
    xz_crc32_init();

    //Use 16k dict
    xz_status = xz_dec_init(XZ_PREALLOC, XZ_DICT_MAX, xz_mem_pool, xz_mem_pool_size);
    if (!xz_status) {
        TRACE(1, "xr_dec_init failed, dict_max %d", XZ_DICT_MAX);
        return 2;
    }

    xz_b.in = bufForFlashReading;
    xz_b.in_size = sizeof(bufForFlashReading);
    xz_b.in_pos = xz_b.in_size;

    xz_b.out = dataBufferForBurning;
    xz_b.out_size = sizeof(dataBufferForBurning);
    xz_b.out_pos = 0;

    TRACE(1, "%s: start uncompress...", __func__);
    TRACE(2, "%s: otaBootInfo.imageSize %d", __func__, otaBootInfo.imageSize);
    programmer_watchdog_ping();

    do {
        if (xz_b.in_pos == xz_b.in_size) {
            if ((otaBootInfo.imageSize - readedDataSize) > xz_b.in_size) {
                bytesToRead = xz_b.in_size;
            } else {
                bytesToRead = otaBootInfo.imageSize - readedDataSize;
            }

            TRACE(3, "%s: Read %d bytes from %p...", __func__, bytesToRead, srcFlashPtr);
            memcpy((uint8_t *)xz_b.in, srcFlashPtr, bytesToRead);

            xz_b.in_pos = 0;
            readedDataSize += bytesToRead;
            srcFlashPtr += bytesToRead;
        }

        ret = xz_dec_run(xz_status, &xz_b);
        if (xz_b.out_pos == xz_b.out_size || ret == XZ_STREAM_END) {
            TRACE(1, "%s: Write %d bytes to %p...", __func__, OTA_DATA_BUFFER_SIZE_FOR_BURNING, (uint8_t *)dstFlashOffset + burnedDataSize);

            lock = int_lock();
            pmu_flash_write_config();

            flash_ret = hal_norflash_erase(HAL_FLASH_ID_0, dstFlashOffset + burnedDataSize, OTA_DATA_BUFFER_SIZE_FOR_BURNING);
            if (flash_ret != HAL_NORFLASH_OK) {
                TRACE(3, "%s: erase %p fail, ret %d", __func__, (uint8_t *)dstFlashOffset + burnedDataSize, flash_ret);
            }

            flash_ret = hal_norflash_write(HAL_FLASH_ID_0, dstFlashOffset + burnedDataSize, dataBufferForBurning, OTA_DATA_BUFFER_SIZE_FOR_BURNING);
            if (flash_ret != HAL_NORFLASH_OK) {
                TRACE(3, "%s: write %p fail, ret %d", __func__, (uint8_t *)dstFlashOffset + burnedDataSize, flash_ret);
            }
#if OTA_NON_CACHE_READ_ISSUE_WORKAROUND
            hal_cache_invalidate(HAL_CACHE_ID_D_CACHE, OTA_FLASH_LOGIC_ADDR + dstFlashOffset + burnedDataSize, OTA_DATA_BUFFER_SIZE_FOR_BURNING);
#endif

            pmu_flash_read_config();
            int_unlock(lock);

#ifdef __WATCHER_DOG_RESET__
            writedDataSizeSinceLastWdPing += OTA_DATA_BUFFER_SIZE_FOR_BURNING;
            if (writedDataSizeSinceLastWdPing > 10 * OTA_DATA_BUFFER_SIZE_FOR_BURNING) {
                programmer_watchdog_ping();
                writedDataSizeSinceLastWdPing = 0;
                TRACE(1, "%s: watch dog reset.", __func__);
            }
#endif

            burnedDataSize += xz_b.out_pos;
            xz_b.out_pos = 0;
        }

        if (ret == XZ_STREAM_END)
            break;
    } while (ret == XZ_OK);

    if (ret != XZ_STREAM_END) {
        TRACE(2, "%s: uncompress failed!!! reason %d", __func__, ret);
        return 2;
    }

    xz_dec_end(xz_status);
    TRACE(1, "%s: uncompress is done.", __func__);

#ifdef __WATCHER_DOG_RESET__
    uint32_t erasedDataSizeSinceLastWdPing = 0;
    programmer_watchdog_ping();
#endif

    TRACE(3, "%s,uncompress bin size:%d(0x%x).", __func__, burnedDataSize, burnedDataSize);

#if defined(__APP_BIN_INTEGRALITY_CHECK__)
    if (ota_bin_hmac_sha256_check(bufForFlashReading, dstFlashOffset, burnedDataSize) == false) {
        TRACE_IMM(1, "%s,uncompress new image bin hmac-sha256 check failed!", __func__);
        osDelay(300);
        pmu_reboot();
        while (1) {
        }
    }
#endif

    app_update_magic_number_of_app_image(bufForFlashReading, NORMAL_BOOT);
    programmer_update_magic_number(NORMAL_BOOT);

    TRACE(0, "ota boot info:");
    DUMP8("0x%02x ", (uint8_t *)&otaBootInfo, sizeof(FLASH_OTA_BOOT_INFO_T));

    uint32_t erasedDataSize = 0;
    while (erasedDataSize < otaBootInfo.imageSize) {
        lock = int_lock();
        hal_norflash_erase(HAL_FLASH_ID_0, (OTA_FLASH_LOGIC_ADDR + srcFlashOffset + erasedDataSize), OTA_DATA_BUFFER_SIZE_FOR_BURNING);
        int_unlock(lock);
        erasedDataSize += OTA_DATA_BUFFER_SIZE_FOR_BURNING;
        TRACE(2, "%s: erase addr:%08x.", __func__, OTA_FLASH_LOGIC_ADDR + srcFlashOffset + erasedDataSize);

#ifdef __WATCHER_DOG_RESET__
        erasedDataSizeSinceLastWdPing += erasedDataSize;
        if (erasedDataSizeSinceLastWdPing > 10 * OTA_DATA_BUFFER_SIZE_FOR_BURNING) {
            programmer_watchdog_ping();
            TRACE(1, "%s: watch dong ping.", __func__);
            erasedDataSizeSinceLastWdPing = 0;
        }
#endif
    }

    TRACE_IMM(1, "%s,uncompress new image bin hmac-sha256 check success,now reboot!", __func__);
    osDelay(300);
    pmu_reboot();
    while (1) {
    }

    return 1;
}
#endif
