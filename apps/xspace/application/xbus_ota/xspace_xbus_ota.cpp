#ifdef __XSPACE_XBUS_OTA__
#include "xspace_xbus_ota.h"
#include "cache.h"
#include "cmsis.h"
#include "hal_norflash.h"
#include "hal_trace.h"
#include "hal_wdt.h"
#include "hmac_sha256.h"
#include "ota_control.h"
#include "pmu.h"
#include "pmu_best1501.h"
#include "xspace_crc16.h"
#include "xspace_dev_info.h"

#define __XSPACE_XBUS_OTA_DEBUG__

#if defined(__XSPACE_XBUS_OTA_DEBUG__)
#define XBUS_OTA_TRACE(num, str, ...) TRACE(num + 1, "[XBUS_OTA] line=%d," #str, __LINE__, ##__VA_ARGS__)
#define XBUS_OTA_DUMP8(str, buf, cnt) DUMP8(str, buf, cnt)
#else
#define XBUS_OTA_TRACE(num, str, ...)
#define XBUS_OTA_DUMP8(str, buf, cnt)
#endif

#define XBUS_OTA_FRAME_DATA_SIZE 2048
#define XBUS_OTA_FRAME_SIZE      (2048 + 8)

#define XBUS_OTA_FLASH_SECTOR_SIZE_IN_BYTES   (4096)
#define XBUS_OTA_BOOT_INFO_FLASH_OFFSET       (0x1000)
#define XBUS_OTA_DATA_BUFFER_SIZE_FOR_BURNING XBUS_OTA_FLASH_SECTOR_SIZE_IN_BYTES

#define XBUS_OTA_START_MAGIC_CODE (0x54534542)   // BEST
#define XBUS_OTA_NORMAL_BOOT      0xBE57EC1C
#define XBUS_OTA_COPY_NEW_IMAGE   0x5a5a5a5a

#define XBUS_OTA_HMAC_SHA256_VALUE_LEN (32)

#define XBUS_OTA_FLASH_BANK2_SIZE       (XSPACE_OTA_BACKUP_SECTION_SIZE)
#define XBUS_OTA_FLASH_BANK2_START_ADDR  (XSPACE_OTA_BACKUP_SECTION_START_ADDR)
#define XBUS_OTA_FLASH_BANK2_END_ADDR   (XSPACE_OTA_BACKUP_SECTION_END_ADDR)

#define XBUS_OTA_NEW_IMAGE_MIN_SIZE (XSPACE_OTA_BACKUP_SECTION_SIZE / 2)
#define XBUS_OTA_NEW_IMAGE_MAX_SIZE (XSPACE_OTA_BACKUP_SECTION_SIZE)

typedef struct {
    uint8_t head;                           /*left ear:0x24, right ear:0x25*/
    uint8_t cmd;                            /* 0x11 */
    uint16_t framenum;                      /* frame number*/
    uint8_t data[XBUS_OTA_FRAME_DATA_SIZE]; /* 2k data*/
    uint8_t finishall;                      /* transfer finished*/
    uint8_t align64k;                       /* 64k alignment*/
    uint16_t crc16;                         /* crc16*/
} xbus_ota_data_frame_e;

typedef union {
    uint8_t data[4];
    uint32_t size;
} xbus_ota_fw_seze_u;

#if defined(__APP_BIN_INTEGRALITY_CHECK__)
#if defined(__PROJ_XSPACE__)
static uint8_t xbus_ota_hmac_key[32] = {
    0xac, 0x0e, 0x2a, 0xbd, 0x6b, 0x40, 0xeb, 0xe7, 
    0xca, 0xfa, 0xef, 0x0e, 0x0c, 0x08, 0x3b, 0x4f, 
    0x12, 0xa8, 0xfa, 0x41, 0xad, 0x00, 0x75, 0x2b, 
    0xf3, 0xc9, 0x3f, 0xaf, 0xd4, 0x45, 0xac, 0x27 
};
#endif
#endif

static int32_t xbus_ota_boot_info_get(FLASH_OTA_BOOT_INFO_T *ota_boot_info);
static int32_t xbus_ota_boot_info_set(FLASH_OTA_BOOT_INFO_T *ota_boot_info);
static int32_t xbus_ota_flash_write(uint8_t *data, uint32_t data_len, uint32_t flash_write_addr);
static int32_t xbus_ota_flash_read(uint8_t *data, uint32_t data_len, uint32_t flash_read_addr);
static int32_t xbus_ota_boot_magicnum_update(uint32_t MagicNumber);
static uint32_t xbus_ota_flush_data_to_flash(uint8_t *ptrSource, uint32_t lengthToBurn, uint32_t offsetInFlashToProgram);

static FLASH_OTA_BOOT_INFO_T xbus_ota_boot_info = {XBUS_OTA_NORMAL_BOOT, 0, 0};

static xbus_ota_fw_seze_u xbus_ota_fw_size = {0};
uint8_t xbus_ota_frame_cache[XBUS_OTA_FRAME_DATA_SIZE] = {0};

static int32_t xbus_ota_boot_info_get(FLASH_OTA_BOOT_INFO_T *ota_boot_info)
{
    uint32_t lock;

    lock = int_lock_global();
    hal_norflash_read(HAL_FLASH_ID_0, XBUS_OTA_BOOT_INFO_FLASH_OFFSET, (uint8_t *)&xbus_ota_boot_info, sizeof(FLASH_OTA_BOOT_INFO_T));
    int_unlock_global(lock);

    *ota_boot_info = xbus_ota_boot_info;

    return 0;
}

static int32_t xbus_ota_boot_info_set(FLASH_OTA_BOOT_INFO_T *ota_boot_info)
{
    uint32_t lock;

    xbus_ota_boot_info = *ota_boot_info;

    XBUS_OTA_TRACE(3, "%s magic num:%08x size:%08x offset 0x%x", __func__, xbus_ota_boot_info.magicNumber, xbus_ota_boot_info.imageSize,xbus_ota_boot_info.newImageFlashOffset);
    if (xbus_ota_boot_info.imageSize % 8 != 0) {
        xbus_ota_boot_info.imageSize = (xbus_ota_boot_info.imageSize / 8 + 1) * 8;
    }

    lock = int_lock_global();
    pmu_flash_write_config();
    hal_norflash_erase(HAL_FLASH_ID_0, XBUS_OTA_BOOT_INFO_FLASH_OFFSET, XBUS_OTA_FLASH_SECTOR_SIZE_IN_BYTES);
    hal_norflash_write(HAL_FLASH_ID_0, XBUS_OTA_BOOT_INFO_FLASH_OFFSET, (uint8_t *)&xbus_ota_boot_info, sizeof(FLASH_OTA_BOOT_INFO_T));
    pmu_flash_read_config();
    int_unlock_global(lock);

    return 0;
}

static int32_t xbus_ota_flash_write(uint8_t *data, uint32_t data_len, uint32_t flash_write_addr)
{
    XBUS_OTA_TRACE(2, "flash write addr:0x%x,len:0x%x!", flash_write_addr, data_len);
    return xbus_ota_flush_data_to_flash(data, data_len, flash_write_addr);
}

static int32_t xbus_ota_flash_read(uint8_t *data, uint32_t data_len, uint32_t flash_read_addr)
{
    XBUS_OTA_TRACE(2, "flash read addr:0x%x, len:0x%x!", flash_read_addr, data_len);

    uint32_t lock = int_lock();
    hal_norflash_read(HAL_FLASH_ID_0, flash_read_addr, data, data_len);
    int_unlock(lock);

    return 0;
}

static int32_t xbus_ota_boot_magicnum_update(uint32_t MagicNumber)
{
    FLASH_OTA_BOOT_INFO_T ota_boot_info = {0};
    xbus_ota_boot_info_get(&ota_boot_info);
    ota_boot_info.magicNumber = MagicNumber;
    xbus_ota_boot_info_get(&ota_boot_info);

    return 0;
}

static uint32_t xbus_ota_flush_data_to_flash(uint8_t *ptrSource, uint32_t lengthToBurn, uint32_t offsetInFlashToProgram)
{
    uint32_t lock;

    XBUS_OTA_TRACE(2, "flush %d bytes to flash offset 0x%x", lengthToBurn, offsetInFlashToProgram);

    lock = int_lock_global();
    pmu_flash_write_config();

    uint32_t preBytes =
        (XBUS_OTA_FLASH_SECTOR_SIZE_IN_BYTES - (offsetInFlashToProgram % XBUS_OTA_FLASH_SECTOR_SIZE_IN_BYTES)) % XBUS_OTA_FLASH_SECTOR_SIZE_IN_BYTES;
    if (lengthToBurn < preBytes) {
        preBytes = lengthToBurn;
    }

    uint32_t middleBytes = 0;
    if (lengthToBurn > preBytes) {
        middleBytes = ((lengthToBurn - preBytes) / XBUS_OTA_FLASH_SECTOR_SIZE_IN_BYTES * XBUS_OTA_FLASH_SECTOR_SIZE_IN_BYTES);
    }
    uint32_t postBytes = 0;
    if (lengthToBurn > (preBytes + middleBytes)) {
        postBytes = (offsetInFlashToProgram + lengthToBurn) % XBUS_OTA_FLASH_SECTOR_SIZE_IN_BYTES;
    }

    XBUS_OTA_TRACE(3, "Prebytes is %d middlebytes is %d postbytes is %d", preBytes, middleBytes, postBytes);

    if (preBytes > 0) {
        hal_norflash_write(HAL_FLASH_ID_0, offsetInFlashToProgram, ptrSource, preBytes);

        ptrSource += preBytes;
        offsetInFlashToProgram += preBytes;
    }

    uint32_t sectorIndexInFlash = offsetInFlashToProgram / XBUS_OTA_FLASH_SECTOR_SIZE_IN_BYTES;

    if (middleBytes > 0) {
        uint32_t sectorCntToProgram = middleBytes / XBUS_OTA_FLASH_SECTOR_SIZE_IN_BYTES;
        for (uint32_t sector = 0; sector < sectorCntToProgram; sector++) {
            hal_norflash_erase(HAL_FLASH_ID_0, sectorIndexInFlash * XBUS_OTA_FLASH_SECTOR_SIZE_IN_BYTES, XBUS_OTA_FLASH_SECTOR_SIZE_IN_BYTES);
            hal_norflash_write(HAL_FLASH_ID_0, sectorIndexInFlash * XBUS_OTA_FLASH_SECTOR_SIZE_IN_BYTES,
                               ptrSource + sector * XBUS_OTA_FLASH_SECTOR_SIZE_IN_BYTES, XBUS_OTA_FLASH_SECTOR_SIZE_IN_BYTES);

            sectorIndexInFlash++;
        }

        ptrSource += middleBytes;
    }

    if (postBytes > 0) {
        hal_norflash_erase(HAL_FLASH_ID_0, sectorIndexInFlash * XBUS_OTA_FLASH_SECTOR_SIZE_IN_BYTES, XBUS_OTA_FLASH_SECTOR_SIZE_IN_BYTES);
        hal_norflash_write(HAL_FLASH_ID_0, sectorIndexInFlash * XBUS_OTA_FLASH_SECTOR_SIZE_IN_BYTES, ptrSource, postBytes);
    }

    pmu_flash_read_config();
    int_unlock_global(lock);

    return 0;
}

int32_t xbus_ota_fw_size_set(uint32_t fw_size)
{
    XBUS_OTA_TRACE(1, "fw size set:0x%04x", fw_size);
    xbus_ota_fw_size.data[0] = (fw_size >> 0 * 8) & 0xff;
    xbus_ota_fw_size.data[1] = (fw_size >> 1 * 8) & 0xff;
    xbus_ota_fw_size.data[2] = (fw_size >> 2 * 8) & 0xff;
    xbus_ota_fw_size.data[3] = (fw_size >> 3 * 8) & 0xff;

    xbus_ota_fw_size.size = fw_size;
    return 0;
}

int32_t xbus_ota_fw_size_get(uint32_t *fw_size)
{
    *fw_size = xbus_ota_fw_size.size;
    XBUS_OTA_TRACE(1, "fw size get:0x%04x", *fw_size);

    return 0;
}

#if defined(__APP_BIN_INTEGRALITY_CHECK__)
bool xbus_ota_fw_hmac_sha256_check(void)
{
    bool ret = false;
    uint32_t realImageSize = 0;
    uint32_t copiedDataSize = 0;
    uint32_t bytesToCopy = 0;
    uint32_t lock;
    uint32_t read_addr = 0x00;
    uint32_t new_image_copy_flash_offset = NEW_IMAGE_FLASH_OFFSET;
#ifdef __WATCHER_DOG_RESET__
    uint32_t copiedDataSizeSinceLastWdPing = 0;
#endif
    uint8_t hmac_value_ori[XBUS_OTA_HMAC_SHA256_VALUE_LEN];
    uint8_t hmac_value_calc[XBUS_OTA_HMAC_SHA256_VALUE_LEN];
    uint16_t i = 0;
    uint8_t *sha256_cache_buffer = NULL;
    int sha256_cache_buffer_length = 0;

    XBUS_OTA_TRACE(1, "%s.", __func__);

    XBUS_OTA_TRACE(3, "ota boot info is: magicNumber=%X, imageSize=%d, imageCrc=%d", xbus_ota_boot_info.magicNumber, xbus_ota_boot_info.imageSize,
                   xbus_ota_boot_info.imageCrc);

    if ((xbus_ota_boot_info.imageSize < XBUS_OTA_NEW_IMAGE_MIN_SIZE) || (xbus_ota_boot_info.imageSize > XBUS_OTA_NEW_IMAGE_MAX_SIZE)) {
        XBUS_OTA_TRACE(1, "Invaild imageSize, imageSize=%d", xbus_ota_boot_info.imageSize);
        xbus_ota_boot_magicnum_update(XBUS_OTA_NORMAL_BOOT);
        return false;
    }

    uint32_t readIndex = new_image_copy_flash_offset + xbus_ota_boot_info.imageSize;

    XBUS_OTA_TRACE(1, "ota boot info is: readIndex=0x%x.", readIndex);

    if (get_4k_cache_pool_used_flag() == true) {
        XBUS_OTA_TRACE(0, "cache bufffer is busy!");
        return false;
    }
    sha256_cache_buffer_length = get_4k_cache_pool((uint8_t **)&sha256_cache_buffer);

    memset(sha256_cache_buffer, 0x00, sha256_cache_buffer_length);

    if (readIndex % XBUS_OTA_DATA_BUFFER_SIZE_FOR_BURNING == 0) {
        read_addr = (readIndex / 4096 - 1) * 4096;
        realImageSize = (xbus_ota_boot_info.imageSize / 4096 - 1) * 4096;
    } else {
        read_addr = readIndex / 4096 * 4096;
        realImageSize = (xbus_ota_boot_info.imageSize / 4096) * 4096;
    }

    XBUS_OTA_TRACE(2, "read_addr 0x%x, realimagesize 0x%x", read_addr, realImageSize);

    lock = int_lock();
    hal_norflash_read(HAL_FLASH_ID_0, read_addr, sha256_cache_buffer, XBUS_OTA_DATA_BUFFER_SIZE_FOR_BURNING);
    int_unlock(lock);

#if 0 
    TRACE(1,"%s, buffer data:\n", __func__);
    for(i=0;i<256;i++){
        DUMP8("0x%02x ", &(sha256_cache_buffer[i*16]), 16);
        TRACE(0,"\n");
    }
#endif

    uint32_t sha_index = XBUS_OTA_DATA_BUFFER_SIZE_FOR_BURNING;
    for (i = XBUS_OTA_DATA_BUFFER_SIZE_FOR_BURNING - 1; i >= 2; i--) {
        if ((sha256_cache_buffer[i] == 'a') && (sha256_cache_buffer[i - 1] == 'h') && (sha256_cache_buffer[i - 2] == 's')) {
            sha_index = i - 2;
            break;
        }
    }

    if (sha_index == XBUS_OTA_DATA_BUFFER_SIZE_FOR_BURNING) {
        XBUS_OTA_TRACE(1, "%s, Not include sha !!!", __func__);
        return false;
    }

    realImageSize += sha_index;
    sha_index += 3;
    XBUS_OTA_TRACE(2, "%s, sha_index=%d", __func__, sha_index);

    memcpy(hmac_value_ori, sha256_cache_buffer + sha_index, XBUS_OTA_HMAC_SHA256_VALUE_LEN);

    TRACE(0, "ori hmac:\n");
    DUMP8("%02x ", hmac_value_ori, sizeof(hmac_value_ori) / sizeof(hmac_value_ori[0]));

    hmac_sha256_init((unsigned char *)xbus_ota_hmac_key, sizeof(xbus_ota_hmac_key));

    while (copiedDataSize < realImageSize) {
        if (realImageSize - copiedDataSize > XBUS_OTA_DATA_BUFFER_SIZE_FOR_BURNING) {
            bytesToCopy = XBUS_OTA_DATA_BUFFER_SIZE_FOR_BURNING;
        } else {
            bytesToCopy = realImageSize - copiedDataSize;
        }

        lock = int_lock();
        hal_norflash_read(HAL_FLASH_ID_0, new_image_copy_flash_offset + copiedDataSize, sha256_cache_buffer, XBUS_OTA_DATA_BUFFER_SIZE_FOR_BURNING);
        int_unlock(lock);

        copiedDataSize += bytesToCopy;

        hmac_sha256_update((unsigned char *)sha256_cache_buffer, bytesToCopy);

#ifdef __WATCHER_DOG_RESET__
        copiedDataSizeSinceLastWdPing += bytesToCopy;
        if (copiedDataSizeSinceLastWdPing > 20 * XBUS_OTA_DATA_BUFFER_SIZE_FOR_BURNING) {
            hal_wdt_ping(HAL_WDT_ID_0);
            copiedDataSizeSinceLastWdPing = 0;
        }
#endif
    }
    _4k_cache_pool_free();

    int len = XBUS_OTA_HMAC_SHA256_VALUE_LEN;
    hmac_sha256_final((unsigned char *)hmac_value_calc, &len);

    XBUS_OTA_TRACE(0, "hmac_value_ori:");
    XBUS_OTA_DUMP8("0x%02x,", hmac_value_ori, XBUS_OTA_HMAC_SHA256_VALUE_LEN);
    XBUS_OTA_TRACE(0, "hmac_value_calc:");
    XBUS_OTA_DUMP8("0x%02x,", hmac_value_calc, XBUS_OTA_HMAC_SHA256_VALUE_LEN);

    if (memcmp(hmac_value_ori, hmac_value_calc, XBUS_OTA_HMAC_SHA256_VALUE_LEN) == 0) {
        ret = true;
        XBUS_OTA_TRACE(1, "%s, hmac-sha256 match !!!", __func__);
    } else {
        ret = false;
        XBUS_OTA_TRACE(1, "%s, hmac-sha256 not match !!!", __func__);
    }

    return ret;
}

int32_t xbus_ota_sha256_check()
{
    FLASH_OTA_BOOT_INFO_T ota_boot_info = {0};

    if (xbus_ota_fw_hmac_sha256_check()) {
        XBUS_OTA_TRACE(0, "whole file sha256 check ok, bank2 update success.");
        xbus_ota_boot_info_get(&ota_boot_info);
        ota_boot_info.magicNumber = XBUS_OTA_COPY_NEW_IMAGE;
        xbus_ota_boot_info_set(&ota_boot_info);
        return 0;
    } else {
        XBUS_OTA_TRACE(0, "whole file sha256 check fail.");
        return -1;
    }
}
#endif

xbus_ota_err_e xbus_ota_frame_receive(uint8_t *frame, uint32_t len)
{
    xbus_ota_data_frame_e *p_ota_frame = NULL;
    FLASH_OTA_BOOT_INFO_T ota_boot_info = {0};
    uint32_t flash_addr = 0;
    uint32_t write_flash_try = 3;
    uint32_t framenum = 0;
    uint16_t calc_crc16 = 0, recv_crc16 = 0;

    p_ota_frame = (xbus_ota_data_frame_e *)frame;
    framenum = ((p_ota_frame->framenum >> 8) & 0x00ff) | ((p_ota_frame->framenum << 8) & 0xff00);

    XBUS_OTA_TRACE(3, "frame head:0x%x, cmd:0x%x, framenum:0x%04x", p_ota_frame->head, p_ota_frame->cmd, framenum);
    XBUS_OTA_TRACE(3, "frame align64k:0x%x, finishall:0x%x, crc16:0x%04x", p_ota_frame->align64k, p_ota_frame->finishall, p_ota_frame->crc16);

    calc_crc16 = xspace_crc16_ccitt_false(&p_ota_frame->head, XBUS_OTA_FRAME_SIZE - 2, XSPACE_CRC_INIT);
    recv_crc16 = ((p_ota_frame->crc16 >> 8) & 0x00ff) | ((p_ota_frame->crc16 << 8) & 0xff00);
    if (calc_crc16 != recv_crc16) {
        XBUS_OTA_TRACE(0, "frame crc err,calc crc16(0x%04x) != recv crc16(0x%04x)", calc_crc16, p_ota_frame->crc16);
        return XBUS_OTA_FRAME_RETRANS;
    }

    flash_addr = XBUS_OTA_FLASH_BANK2_START_ADDR + framenum * (uint32_t)XBUS_OTA_FRAME_DATA_SIZE;
    //XBUS_OTA_TRACE(4,"0x%x, 0x%x, 0x%x, 0x%x",XSPACE_OTA_BACKUP_SECTION_SIZE,XSPACE_OTA_BACKUP_SECTION_START_ADDR, NEW_IMAGE_FLASH_START_ADDR, NEW_IMAGE_FLASH_OFFSET);
    XBUS_OTA_TRACE(3, "write flash addr 0x%x,bank2 end addr 0x%x, bank2 size 0x%x", flash_addr, XBUS_OTA_FLASH_BANK2_END_ADDR, XBUS_OTA_FLASH_BANK2_SIZE);
    XBUS_OTA_TRACE(4, "backup size 0x%x,backup start 0x%x,  newimg start 0x%x, newimg offset 0x%x", XSPACE_OTA_BACKUP_SECTION_SIZE, XSPACE_OTA_BACKUP_SECTION_START_ADDR, NEW_IMAGE_FLASH_START_ADDR,NEW_IMAGE_FLASH_OFFSET);

    if (flash_addr > XBUS_OTA_FLASH_BANK2_END_ADDR) {
        XBUS_OTA_TRACE(0, "frame flash addr err, out of range");
        return XBUS_OTA_FAILED;
    }
    memset(xbus_ota_frame_cache, 0, XBUS_OTA_FRAME_DATA_SIZE);

    while (write_flash_try--) {
        xbus_ota_flash_write(p_ota_frame->data, XBUS_OTA_FRAME_DATA_SIZE, flash_addr);
        xbus_ota_flash_read(xbus_ota_frame_cache, XBUS_OTA_FRAME_DATA_SIZE, flash_addr);
        int i = 0;
        for (i = 0; i < XBUS_OTA_FRAME_DATA_SIZE; i++) {
            if (xbus_ota_frame_cache[i] != p_ota_frame->data[i]) {
                XBUS_OTA_TRACE(1, "frame write flash err, i:0x%x,recv byte(0x%x) != flash byte(0x%x)", i, p_ota_frame->data[i], xbus_ota_frame_cache[i]);
                break;
            }
        }

        if (XBUS_OTA_FRAME_DATA_SIZE == i) {
            break;
        }
    }

    if (0 == write_flash_try) {
        XBUS_OTA_TRACE(1, "frame write err,frame retrans, frame:%d", p_ota_frame->framenum);
        return XBUS_OTA_FRAME_RETRANS;
    }

    if (p_ota_frame->finishall) {
        uint32_t image_size = ota_boot_info.imageSize;
        xbus_ota_boot_info_get(&ota_boot_info);
        xbus_ota_fw_size_get(&image_size);
        ota_boot_info.imageSize = image_size;
        ota_boot_info.newImageFlashOffset = XBUS_OTA_FLASH_BANK2_START_ADDR;
        xbus_ota_boot_info_set(&ota_boot_info);
        XBUS_OTA_TRACE(0, "frame write finished");
        return XBUS_OTA_FINISHED;
    }

    XBUS_OTA_TRACE(0, "frame write next.");
    return XBUS_OTA_FRAME_NEXT;
}

#endif /* __XSPACE_UART_OTA__ */
