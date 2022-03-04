#if 0
#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_timer.h"
#include "hal_gpio.h"
#include "hal_trace.h"
#include "string.h"
#include "crc32_c.h"
#include "pmu.h"
#include "hal_norflash.h"
#include "nvrecord.h"
#include "hal_cache.h"
#include "cmsis_nvic.h"
#include "hal_bootmode.h"

#include "app_battery.h"
#include "app_tencent_ota.h"
#include "app_tencent_handle.h"
#include "app_tencent_sv_cmd_code.h"
#include "app_tencent_sv_cmd_handler.h"
#include "tgt_hardware.h"

#include "app_key.h"
#include "apps.h"
//#include "btapp.h"


#include "app_bt_stream.h"
#include "ai_thread.h"

extern uint32_t __factory_start[];
static OTA_ENV_T    ota_env;
static uint32_t     user_data_nv_flash_offset;
#define OTA_BOOT_INFO_FLASH_OFFSET                                                       0x1000
#define otaUpgradeLogInFlash    (*(FLASH_OTA_UPGRADE_LOG_FLASH_T *)(OTA_FLASH_LOGIC_ADDR+0x3000))
#define otaUpgradeLog   otaUpgradeLogInFlash

#define LEN_OF_IMAGE_TAIL_TO_FIND_SANITY_CRC    512
static const char* image_info_sanity_crc_key_word = "CRC32_OF_IMAGE=";

/**
 * @brief The OTA command handling
 *
 */

typedef enum
{
    OTA_NO_ERROR = 0,
    OTA_BATTERY_LOW = 31,
    OTA_CONFIG_DATA_CANNOT_PARSE,  //32
    OTA_SEGMENT_CRC_FAILED,   //33
    OTA_FIRMWARE_CRC_FAILED,  //34
    OTA_RECEIVE_SIZE_ERROR,   //35
    OTA_WRITE_FLASH_OFFSET_ERROR,  //36
    OTA_FIRMWARE_SIZE_ERROR,   //37
    OTA_FLASH_ERROR,   //38
} OTA_RSP_STATUS_E;

typedef struct
{
    uint32_t firmware_size;
    uint32_t crc32;
    uint32_t new_firmware_version;
    uint8_t  version_md5[32];
    uint32_t total_segment;
    uint32_t packet_size;
}__attribute__ ((__packed__)) ota_start_t;

typedef struct
{
    uint16_t mtu;
    uint32_t current_firmware_version;
    uint8_t  battery_level;
}__attribute__ ((__packed__)) ota_rsp_start_t;

typedef struct
{
    bool     rename_bt_name_flag;
    uint8_t  new_bt_name[32];
    bool     rename_ble_name_flag;
    uint8_t  new_ble_name[32];
    bool     update_bt_addr_flag;
    uint8_t  new_bt_addr[6];
    bool     update_ble_addr_flag;
    uint8_t  new_ble_addr[6];
    uint32_t crc32;
}__attribute__ ((__packed__)) ota_config_t;

typedef struct
{
    uint16_t mtu;
    uint32_t current_firmware_version;
    uint8_t  battery_level;
}__attribute__ ((__packed__)) ota_rsp_config_t;

typedef struct
{
    uint8_t version_md5[32];
}__attribute__ ((__packed__)) ota_resume_verify_t;

typedef struct
{
    bool breakpoint_flag;
	uint32_t offset;
}__attribute__ ((__packed__)) ota_resume_verify_reponse_t;



static void ota_update_nv_data(void)
{
    uint32_t lock;

    if (ota_env.configuration.isToRenameBT || ota_env.configuration.isToRenameBLE ||
        ota_env.configuration.isToUpdateBTAddr || ota_env.configuration.isToUpdateBLEAddr)
    {
        uint32_t* pOrgFactoryData, *pUpdatedFactoryData;
        pOrgFactoryData = (uint32_t *)(OTA_FLASH_LOGIC_ADDR + ota_env.flasehOffsetOfFactoryDataPool);
        memcpy(ota_env.dataBufferForBurning, (uint8_t *)pOrgFactoryData,
            FLASH_SECTOR_SIZE_IN_BYTES);
        pUpdatedFactoryData = (uint32_t *)&(ota_env.dataBufferForBurning);

        uint32_t nv_record_dev_rev = pOrgFactoryData[dev_version_and_magic] >> 16;

        if (NVREC_DEV_VERSION_1 == nv_record_dev_rev)
        {
            if (ota_env.configuration.isToRenameBT)
            {
                memset((uint8_t *)(pUpdatedFactoryData + dev_name), 0, sizeof(uint32_t)*(dev_bt_addr - dev_name));
                memcpy((uint8_t *)(pUpdatedFactoryData + dev_name), (uint8_t *)(ota_env.configuration.newBTName),
                    NAME_LENGTH);
            }

            if (ota_env.configuration.isToUpdateBTAddr)
            {
                memcpy((uint8_t *)(pUpdatedFactoryData + dev_bt_addr), (uint8_t *)(ota_env.configuration.newBTAddr),
                    BD_ADDR_LENGTH);
            }

            if (ota_env.configuration.isToUpdateBLEAddr)
            {
                memcpy((uint8_t *)(pUpdatedFactoryData + dev_ble_addr), (uint8_t *)(ota_env.configuration.newBLEAddr),
                    BD_ADDR_LENGTH);
            }

            pUpdatedFactoryData[dev_crc] =
                crc32_c(0,(uint8_t *)(&pUpdatedFactoryData[dev_reserv1]),(dev_data_len-dev_reserv1)*sizeof(uint32_t));
        }
        else
        {
            if (ota_env.configuration.isToRenameBT) {
              memset((uint8_t *) (pUpdatedFactoryData + rev2_dev_name), 0,
                     sizeof(uint32_t) * (rev2_dev_bt_addr - rev2_dev_name));
              memcpy((uint8_t *) (pUpdatedFactoryData + rev2_dev_name),
                     (uint8_t *) (ota_env.configuration.newBTName),
                     NAME_LENGTH);
            }

            if (ota_env.configuration.isToRenameBLE) {
                memset((uint8_t *)(pUpdatedFactoryData + rev2_dev_ble_name),
                    0, sizeof(uint32_t)*(rev2_dev_section_end - rev2_dev_ble_name));
                memcpy((uint8_t *)(pUpdatedFactoryData + rev2_dev_ble_name),
                    (uint8_t *)(ota_env.configuration.newBLEName),
                    BLE_NAME_LEN_IN_NV);
            }

            if (ota_env.configuration.isToUpdateBTAddr) {
              memcpy((uint8_t *) (pUpdatedFactoryData + rev2_dev_bt_addr),
                     (uint8_t *) (ota_env.configuration.newBTAddr),
                     BD_ADDR_LENGTH);
            }

            if (ota_env.configuration.isToUpdateBLEAddr) {
              memcpy((uint8_t *) (pUpdatedFactoryData + rev2_dev_ble_addr),
                     (uint8_t *) (ota_env.configuration.newBLEAddr),
                     BD_ADDR_LENGTH);
            }

            pUpdatedFactoryData[rev2_dev_crc] = crc32_c(
                0, (uint8_t *) (&pUpdatedFactoryData[rev2_dev_section_start_reserved]),
                pUpdatedFactoryData[rev2_dev_data_len]);
        }

        lock = int_lock();
        pmu_flash_write_config();
        hal_norflash_erase(HAL_FLASH_ID_0, OTA_FLASH_LOGIC_ADDR + ota_env.flasehOffsetOfFactoryDataPool,
                FLASH_SECTOR_SIZE_IN_BYTES);

        hal_norflash_write(HAL_FLASH_ID_0, OTA_FLASH_LOGIC_ADDR + ota_env.flasehOffsetOfFactoryDataPool,
            (uint8_t *)pUpdatedFactoryData, FLASH_SECTOR_SIZE_IN_BYTES);
        pmu_flash_read_config();
        int_unlock(lock);
    }
}

static void ota_reset_env(void);

void app_tencent_ota_handler_init(void)
{
#ifdef __APP_USER_DATA_NV_FLASH_OFFSET__
    user_data_nv_flash_offset = __APP_USER_DATA_NV_FLASH_OFFSET__;
#else
    user_data_nv_flash_offset = hal_norflash_get_flash_total_size(HAL_FLASH_ID_0) - 2*4096;
#endif

    memset(&ota_env, 0, sizeof(ota_env));
    ota_reset_env();
}

static void ota_reboot_to_use_new_image(void)
{
    TRACE(0,"OTA data receiving is done successfully, systerm will reboot ");
    hal_sw_bootmode_set(HAL_SW_BOOTMODE_ENTER_HIDE_BOOT);
    pmu_reboot();
}

void app_tencent_ota_disconnection_handler(void)
{
    if (ota_env.isPendingForReboot)
    {
        ota_reboot_to_use_new_image();
    }
    else
    {
        ota_reset_env();
    }
}

static void ota_reset_env(void)
{
    ota_env.configuration.startLocationToWriteImage = NEW_IMAGE_FLASH_OFFSET;
    ota_env.offsetInFlashToProgram = ota_env.configuration.startLocationToWriteImage;;
    ota_env.offsetInFlashOfCurrentSegment = ota_env.offsetInFlashToProgram;

    ota_env.configuration.isToRenameBLE = false;
    ota_env.configuration.isToRenameBT = false;
    ota_env.configuration.isToUpdateBLEAddr = false;
    ota_env.configuration.isToUpdateBTAddr = false;
    ota_env.flasehOffsetOfUserDataPool = user_data_nv_flash_offset;
    ota_env.flasehOffsetOfFactoryDataPool = user_data_nv_flash_offset + FLASH_SECTOR_SIZE_IN_BYTES;
    ota_env.crc32OfSegment = 0;
    ota_env.crc32OfImage = 0;
    ota_env.offsetInDataBufferForBurning = 0;
    ota_env.alreadyReceivedDataSizeOfImage = 0;
    ota_env.offsetOfImageOfCurrentSegment = 0;
    ota_env.isOTAInProgress = false;
    ota_env.isPendingForReboot = false;

    if(ota_env.resume_at_breakpoint == false)
    {
        ota_env.breakPoint = 0;
        ota_env.i_log = -1;
    }
}

/** Program the data in the data buffer to flash
  *
  * @param[in] ptrSource  Pointer of the source data buffer to program.
  * @param[in] lengthToBurn  Length of the data to program.
  * @param[in] offsetInFlashToProgram  Offset in bytes in flash to program
  *
  * @return
  *
  * @note
  */
static void ota_flush_data_to_flash(uint8_t* ptrSource, uint32_t lengthToBurn, uint32_t offsetInFlashToProgram)
{
    uint32_t lock;

    TRACE(2,"flush %d bytes to flash offset 0x%x", lengthToBurn, offsetInFlashToProgram);

    lock = int_lock();
    pmu_flash_write_config();

    uint32_t preBytes = (FLASH_SECTOR_SIZE_IN_BYTES - (offsetInFlashToProgram%FLASH_SECTOR_SIZE_IN_BYTES))%FLASH_SECTOR_SIZE_IN_BYTES;
    if (lengthToBurn < preBytes)
    {
        preBytes = lengthToBurn;
    }

    uint32_t middleBytes = 0;
    if (lengthToBurn > preBytes)
    {
       middleBytes = ((lengthToBurn - preBytes)/FLASH_SECTOR_SIZE_IN_BYTES*FLASH_SECTOR_SIZE_IN_BYTES);
    }
    uint32_t postBytes = 0;
    if (lengthToBurn > (preBytes + middleBytes))
    {
        postBytes = (offsetInFlashToProgram + lengthToBurn)%FLASH_SECTOR_SIZE_IN_BYTES;
    }

    TRACE(3,"Prebytes is %d middlebytes is %d postbytes is %d", preBytes, middleBytes, postBytes);

    if (preBytes > 0)
    {
        hal_norflash_write(HAL_FLASH_ID_0, offsetInFlashToProgram, ptrSource, preBytes);

        ptrSource += preBytes;
        offsetInFlashToProgram += preBytes;
    }

    uint32_t sectorIndexInFlash = offsetInFlashToProgram/FLASH_SECTOR_SIZE_IN_BYTES;

    if (middleBytes > 0)
    {
        uint32_t sectorCntToProgram = middleBytes/FLASH_SECTOR_SIZE_IN_BYTES;
        for (uint32_t sector = 0;sector < sectorCntToProgram;sector++)
        {
            hal_norflash_erase(HAL_FLASH_ID_0, sectorIndexInFlash*FLASH_SECTOR_SIZE_IN_BYTES, FLASH_SECTOR_SIZE_IN_BYTES);
            hal_norflash_write(HAL_FLASH_ID_0, sectorIndexInFlash*FLASH_SECTOR_SIZE_IN_BYTES,
                ptrSource + sector*FLASH_SECTOR_SIZE_IN_BYTES, FLASH_SECTOR_SIZE_IN_BYTES);

            sectorIndexInFlash++;
        }

        ptrSource += middleBytes;
    }

    if (postBytes > 0)
    {
        hal_norflash_erase(HAL_FLASH_ID_0, sectorIndexInFlash*FLASH_SECTOR_SIZE_IN_BYTES, FLASH_SECTOR_SIZE_IN_BYTES);
        hal_norflash_write(HAL_FLASH_ID_0, sectorIndexInFlash*FLASH_SECTOR_SIZE_IN_BYTES,
                ptrSource, postBytes);
    }

	pmu_flash_read_config();
	int_unlock(lock);
}

/**
 * ota_control_flush_data_to_flash() doesn't block erase first on non-4KB aligned addresses,
 * So the erase_segment() function is supplemented.
 */
static void erase_segment(uint32_t addr)
{
    uint32_t bytes = addr & (FLASH_SECTOR_SIZE_IN_BYTES - 1);  // The number of bytes current address minus the previous 4k aligned address.
    if(bytes)
    {
    	memcpy(ota_env.dataBufferForBurning, (uint8_t *)addr, bytes);
        uint32_t lock;
        lock = int_lock();
        pmu_flash_write_config();
		hal_norflash_erase(HAL_FLASH_ID_0, addr, FLASH_SECTOR_SIZE_IN_BYTES);
        hal_norflash_write(HAL_FLASH_ID_0, addr, ota_env.dataBufferForBurning, bytes);
        pmu_flash_read_config();
        int_unlock(lock);
    }
}

static uint32_t ota_check_whole_image_crc(void)
{
    uint32_t verifiedDataSize = 0;
    uint32_t crc32Value = 0;
    uint32_t verifiedBytes = 0;

    while (verifiedDataSize < ota_env.totalImageSize)
    {
        if (ota_env.totalImageSize - verifiedDataSize > OTA_DATA_BUFFER_SIZE_FOR_BURNING)
        {
            verifiedBytes = OTA_DATA_BUFFER_SIZE_FOR_BURNING;
        }
        else
        {
            verifiedBytes = ota_env.totalImageSize - verifiedDataSize;
        }

		memcpy(ota_env.dataBufferForBurning, (uint8_t *)(OTA_FLASH_LOGIC_ADDR+NEW_IMAGE_FLASH_OFFSET+verifiedDataSize),
			OTA_DATA_BUFFER_SIZE_FOR_BURNING);

        if (0 == verifiedDataSize)
        {
            if (*(uint32_t *)ota_env.dataBufferForBurning != NORMAL_BOOT)
            {
                return false;
            }
            else
            {
                *(uint32_t *)ota_env.dataBufferForBurning = 0xFFFFFFFF;
            }
        }

        crc32Value = crc32_c(crc32Value, (uint8_t *)ota_env.dataBufferForBurning, verifiedBytes);

        verifiedDataSize += verifiedBytes;
    }

    return crc32Value;
}


static void app_update_magic_number_of_app_image(uint32_t newMagicNumber)
{
    uint32_t lock;

	memcpy(ota_env.dataBufferForBurning, (uint8_t *)(OTA_FLASH_LOGIC_ADDR+ota_env.dstFlashOffsetForNewImage),
		FLASH_SECTOR_SIZE_IN_BYTES);

    *(uint32_t *)ota_env.dataBufferForBurning = newMagicNumber;

    lock = int_lock();
    pmu_flash_write_config();
    hal_norflash_erase(HAL_FLASH_ID_0, ota_env.dstFlashOffsetForNewImage,
        FLASH_SECTOR_SIZE_IN_BYTES);
    hal_norflash_write(HAL_FLASH_ID_0, ota_env.dstFlashOffsetForNewImage,
        ota_env.dataBufferForBurning, FLASH_SECTOR_SIZE_IN_BYTES);
    pmu_flash_read_config();
    int_unlock(lock);
}

static void ota_update_boot_info(FLASH_OTA_BOOT_INFO_T* otaBootInfo)
{
    uint32_t lock;

    lock = int_lock_global();

    hal_norflash_disable_protection(HAL_FLASH_ID_0);
    pmu_flash_write_config();
    hal_norflash_erase(HAL_FLASH_ID_0, OTA_BOOT_INFO_FLASH_OFFSET, FLASH_SECTOR_SIZE_IN_BYTES);
    hal_norflash_write(HAL_FLASH_ID_0, OTA_BOOT_INFO_FLASH_OFFSET, (uint8_t*)otaBootInfo, sizeof(FLASH_OTA_BOOT_INFO_T));
    pmu_flash_read_config();

    int_unlock_global(lock);
}


void ota_version_md5_log(uint8_t version_md5[])
{
    uint32_t lock;
    lock = int_lock_global();
    pmu_flash_write_config();
    hal_norflash_erase(HAL_FLASH_ID_0, (uint32_t)&otaUpgradeLog, FLASH_SECTOR_SIZE_IN_BYTES);
    hal_norflash_write(HAL_FLASH_ID_0, (uint32_t)&otaUpgradeLog, version_md5, sizeof(otaUpgradeLog.version_md5));
    pmu_flash_read_config();
    int_unlock_global(lock);
}

void ota_upgradeSize_log(void)
{
    uint32_t lock;

    lock = int_lock_global();
    pmu_flash_write_config();

    if(++ota_env.i_log >= sizeof(otaUpgradeLog.upgradeSize)/4)
    {
        ota_env.i_log = 0;
		memcpy(ota_env.dataBufferForBurning, (uint8_t *)&otaUpgradeLog, sizeof(otaUpgradeLog.version_md5));
        hal_norflash_erase(HAL_FLASH_ID_0, (uint32_t)&otaUpgradeLog, FLASH_SECTOR_SIZE_IN_BYTES);
        hal_norflash_write(HAL_FLASH_ID_0, (uint32_t)&otaUpgradeLog, ota_env.dataBufferForBurning, sizeof(otaUpgradeLog.version_md5));
    }

    hal_norflash_write(HAL_FLASH_ID_0, (uint32_t)&otaUpgradeLog.upgradeSize[ota_env.i_log],
                    (uint8_t*)&ota_env.alreadyReceivedDataSizeOfImage, 4);

    TRACE(3,"{i_log: %d, RecSize: 0x%x, FlashWrSize: 0x%x}", ota_env.i_log, ota_env.alreadyReceivedDataSizeOfImage, otaUpgradeLog.upgradeSize[ota_env.i_log]);

	pmu_flash_read_config();
	int_unlock_global(lock);
}

void ota_upgradeLog_destroy(void)
{
    uint32_t lock;
    lock = int_lock_global();
    ota_env.resume_at_breakpoint = false;
    pmu_flash_write_config();
    hal_norflash_erase(HAL_FLASH_ID_0, (uint32_t)&otaUpgradeLog, FLASH_SECTOR_SIZE_IN_BYTES);
    pmu_flash_read_config();
    int_unlock_global(lock);

    TRACE(0,"Destroyed upgrade log in flash.");
}

uint32_t get_upgradeSize_log(void)
{
    int32_t *p = (int32_t*)otaUpgradeLog.upgradeSize,
            left = 0, right = sizeof(otaUpgradeLog.upgradeSize)/4 - 1, mid;

    if(p[0] != -1)
    {
        while(left < right)
        {
            mid = (left + right) / 2;
            if(p[mid] == -1)
                right = mid - 1;
            else
                left = mid + 1;
        }
    }
    if(p[left]==-1)
        left--;

    ota_env.i_log = left;
    ota_env.breakPoint = left!=-1 ? p[left] : 0;
    ota_env.resume_at_breakpoint = ota_env.breakPoint?true:false;

    TRACE(1,"ota_env.i_log: %d", ota_env.i_log);
    return ota_env.breakPoint;
}

static int32_t find_key_word(uint8_t* targetArray, uint32_t targetArrayLen,
    uint8_t* keyWordArray,
    uint32_t keyWordArrayLen)
{
    if ((keyWordArrayLen > 0) && (targetArrayLen >= keyWordArrayLen))
    {
        uint32_t index = 0, targetIndex = 0;
        for (targetIndex = 0;targetIndex < targetArrayLen;targetIndex++)
        {
            for (index = 0;index < keyWordArrayLen;index++)
            {
                if (targetArray[targetIndex + index] != keyWordArray[index])
                {
                    break;
                }
            }

            if (index == keyWordArrayLen)
            {
                return targetIndex;
            }
        }

        return -1;
    }
    else
    {
        return -1;
    }
}


static bool ota_check_image_data_sanity_crc(void) {
  // find the location of the CRC key word string
  uint8_t* ptrOfTheLast4KImage = (uint8_t *)(OTA_FLASH_LOGIC_ADDR+NEW_IMAGE_FLASH_OFFSET+
    ota_env.totalImageSize-LEN_OF_IMAGE_TAIL_TO_FIND_SANITY_CRC);

  int32_t sanity_crc_location = find_key_word(ptrOfTheLast4KImage,
    LEN_OF_IMAGE_TAIL_TO_FIND_SANITY_CRC,
    (uint8_t *)image_info_sanity_crc_key_word,
    strlen(image_info_sanity_crc_key_word));
  if (-1 == sanity_crc_location)
  {
    // if no sanity crc, the image has the old format, just ignore such a check
    return true;
  }

  TRACE(1,"sanity_crc_location is %d", sanity_crc_location);

  uint32_t crc32ImageOffset = sanity_crc_location+ota_env.totalImageSize-
    LEN_OF_IMAGE_TAIL_TO_FIND_SANITY_CRC+strlen(image_info_sanity_crc_key_word);
  TRACE(1,"Bytes to generate crc32 is %d", crc32ImageOffset);

  uint32_t sanityCrc32 = *(uint32_t *)(OTA_FLASH_LOGIC_ADDR+NEW_IMAGE_FLASH_OFFSET+crc32ImageOffset);

  TRACE(1,"sanityCrc32 is 0x%x", sanityCrc32);

  // generate the CRC from image data
  uint32_t calculatedCrc32 = 0;
  calculatedCrc32 = crc32_c(calculatedCrc32, (uint8_t *)(OTA_FLASH_LOGIC_ADDR+NEW_IMAGE_FLASH_OFFSET),
    crc32ImageOffset);

  TRACE(1,"calculatedCrc32 is 0x%x", calculatedCrc32);

  return (sanityCrc32 == calculatedCrc32);
}

#define IMAGE_RECV_FLASH_CHECK       1  // It's best to turn it on durning development and not a big deal off in the release.
#define MAX_IMAGE_SIZE              ((uint32_t)(NEW_IMAGE_FLASH_OFFSET - __APP_IMAGE_FLASH_OFFSET__))
#define MIN_SEG_ALIGN               256

void tencent_ota_update_MTU(uint16_t mtu)
{
    // remove the 3 bytes of overhead
    ota_env.dataPacketSize = mtu - 3;
    TRACE(1,"updated data packet size is %d", ota_env.dataPacketSize);
}

int app_tencent_get_ota_status()
{
   return  ota_env.isOTAInProgress;
}

extern void app_bt_key(APP_KEY_STATUS *status, void *param);

static uint8_t stop_music_tag=0;
static void ota_start_handler(APP_TENCENT_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
  //when ota upgrade,play music, stop it;
  APP_KEY_STATUS key_status;
  key_status.code = APP_KEY_CODE_FN4;
  key_status.event = APP_KEY_EVENT_CLICK;
  AI_TRANS_TYPE_E transport_type = app_ai_get_transport_type();
   if(app_bt_stream_isrun(APP_BT_STREAM_A2DP_SBC))
   	{
     	TRACE(0,"stop playing");
		stop_music_tag =1;
   	    app_bt_key(&key_status, NULL);
   	}
    ota_start_t *ptParam = (ota_start_t *)(ptrParam);
    uint32_t current_version;
	char *p =(char *) &current_version;
    p[0] = 0;
    p[1] = BT_FIRMWARE_VERSION[0]-'0';
    p[2] = BT_FIRMWARE_VERSION[2]-'0';
    p[3] = BT_FIRMWARE_VERSION[4]-'0';

    TRACE(0,"Receive command start request:");
    ota_reset_env();    //reset ota varible
    ota_env.totalImageSize = ptParam->firmware_size;
    ota_env.crc32OfImage = ptParam->crc32;
    ota_env.isOTAInProgress = true;
	ota_env.dataPacketSize = app_ai_get_mtu() - 3 - TENCENT_SMARTVOICE_CMD_CODE_SIZE;

    TRACE(2,"Image size is 0x%x, crc32 of image is 0x%x",
        ota_env.totalImageSize, ota_env.crc32OfImage);

	 if (AI_TRANSPORT_BLE == transport_type)
	 {
	     TRACE(0,"it is ble");
	     ota_env.dataPacketSize =
            (ota_env.dataPacketSize >= OTA_BLE_DATA_PACKET_MAX_SIZE)?OTA_BLE_DATA_PACKET_MAX_SIZE:ota_env.dataPacketSize;
	 }
	 else if (AI_TRANSPORT_SPP == transport_type)
	 {
	      TRACE(0,"it is spp");
	     ota_env.dataPacketSize =
            (ota_env.dataPacketSize >= OTA_BLE_DATA_PACKET_MAX_SIZE)?OTA_BT_DATA_PACKET_MAX_SIZE:ota_env.dataPacketSize;
	 }
	 else
	 {
	    TRACE(0,"it is not spp or ble connect");
	 }

    uint8_t battery_current_level;
    app_battery_get_info(NULL, &battery_current_level, NULL);
    battery_current_level = (battery_current_level + 1) * 10;

	TRACE(2,"ota_env.dataPacketSize is %d, battery_current_level is %d",ota_env.dataPacketSize,battery_current_level);


    int len=sizeof( uint16_t)+sizeof( uint32_t)+sizeof( uint8_t);
    ota_rsp_start_t rsp = {ota_env.dataPacketSize,
                            current_version,
                            battery_current_level};
    if(battery_current_level <= 40)
    {
        app_tencent_sv_send_response_to_command(APP_TENCENT_OTA_START, OTA_BATTERY_LOW, (uint8_t*)&rsp, len);
        ota_env.isOTAInProgress = FALSE;
    }
    else
    {
        app_tencent_sv_send_response_to_command(APP_TENCENT_OTA_START, OTA_NO_ERROR, (uint8_t*)&rsp, len);
    }
}

static void ota_config_handler(APP_TENCENT_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
    ota_config_t *config = (ota_config_t *)ptrParam;

	 TRACE(0,"param is:");
     DUMP8("0x%x", ptrParam,paramLen);

    ota_env.configuration.isToRenameBT = *ptrParam;
    if(*ptrParam)
    {
        memcpy(ota_env.configuration.newBTName, ptrParam, NAME_LENGTH);
        ptrParam+=NAME_LENGTH;
    }
    ota_env.configuration.isToRenameBLE =*(ptrParam++);
    if(*ptrParam)
    {
        memcpy(ota_env.configuration.newBLEName, ptrParam, NAME_LENGTH);
        ptrParam+=NAME_LENGTH;
    }
    ota_env.configuration.isToUpdateBTAddr =  *(ptrParam++);
    if(*ptrParam)
    {
        memcpy(ota_env.configuration.newBTAddr, ptrParam, BD_ADDR_LENGTH);
        ptrParam+=BD_ADDR_LENGTH;
    }
    ota_env.configuration.isToUpdateBLEAddr = *(ptrParam++);
    if(*ptrParam)
    {
        memcpy(ota_env.configuration.newBLEAddr, ptrParam, BD_ADDR_LENGTH);
        ptrParam+=BD_ADDR_LENGTH;
    }

    ptrParam++;

    TRACE(1,"isToRenameBT %d", ota_env.configuration.isToRenameBT);
    TRACE(1,"isToRenameBLE %d", ota_env.configuration.isToRenameBLE);
    TRACE(1,"isToUpdateBTAddr %d", ota_env.configuration.isToUpdateBTAddr);
    TRACE(1,"isToUpdateBLEAddr %d", ota_env.configuration.isToUpdateBLEAddr);
    TRACE(0,"crc original is:");
	DUMP8("0x%x", config,paramLen-4);

	TRACE(1,"app crc32 is 0x%x",*(uint32_t *)(ptrParam));
	TRACE(1,"crc32 is 0x%x", (uint32_t)crc32_c(0, (uint8_t*)config, paramLen-4));

    // check CRC
    if (*(uint32_t *)(ptrParam) == crc32_c(0, (uint8_t*)config, paramLen-4))
    {
        if(ota_env.totalImageSize > MAX_IMAGE_SIZE)
        {
            TRACE(2,"ImageSize 0x%x greater than 0x%x! Terminate the upgrade.", ota_env.totalImageSize, MAX_IMAGE_SIZE);
            app_tencent_sv_send_response_to_command(APP_TENCENT_OTA_CONFIG, OTA_FIRMWARE_SIZE_ERROR, NULL, 0);
			ota_env.isOTAInProgress = FALSE;
            return;
        }

        ota_env.offsetInFlashToProgram = NEW_IMAGE_FLASH_OFFSET;
        ota_env.offsetInFlashOfCurrentSegment = ota_env.offsetInFlashToProgram;
        ota_env.dstFlashOffsetForNewImage = ota_env.offsetInFlashOfCurrentSegment;

        TRACE(0,"OTA config pass.");

        TRACE(1,"Start writing the received image to flash offset 0x%x", ota_env.offsetInFlashToProgram);

        // send response
       // uint8_t battery_current_level;
      //  app_battery_get_info(NULL, &battery_current_level, NULL);
       /* ota_rsp_config_t rsp = { ota_env.dataPacketSize,
                                battery_current_level * 10};*/
		//int len = sizeof( uint16_t) +sizeof( uint32_t) +sizeof( uint8_t);
        app_tencent_sv_send_response_to_command(APP_TENCENT_OTA_CONFIG, OTA_NO_ERROR, NULL, 0);
    }
    else
    {
        TRACE(0,"OTA config failed, CRC is not the same");
        app_tencent_sv_send_response_to_command(APP_TENCENT_OTA_CONFIG, OTA_CONFIG_DATA_CANNOT_PARSE, NULL, 0);
		ota_env.isOTAInProgress = FALSE;
    }
}

static void ota_resume_verify_handler(APP_TENCENT_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
    ota_resume_verify_t *ptParam = (ota_resume_verify_t*)ptrParam;
    uint32_t breakPoint;
	ota_resume_verify_reponse_t rsp;

    TRACE(0,"Receive command resuming verification");

    TRACE(0,"Receive md5:");
    DUMP8("%02x ", ptParam->version_md5,paramLen);

    TRACE(0,"Device's md5:");
    DUMP8("%02x ", otaUpgradeLog.version_md5, sizeof(otaUpgradeLog.version_md5));

    breakPoint = get_upgradeSize_log();
	TRACE(1,"breakpoint is %d",breakPoint);

    if(!memcmp(otaUpgradeLog.version_md5, ptParam->version_md5, sizeof(otaUpgradeLog.version_md5)))
    {
        TRACE(1,"OTA can resume. Resuming from the breakpoint at: 0x%x.", breakPoint);
    }
    else
    {
        TRACE(1,"OTA can't resume because the version_md5 is inconsistent. [breakPoint: 0x%x]", breakPoint);

        breakPoint = ota_env.breakPoint = ota_env.resume_at_breakpoint = 0;
        ota_version_md5_log(ptParam->version_md5);
    }

    if(ota_env.resume_at_breakpoint == true)
    {
        ota_env.alreadyReceivedDataSizeOfImage = ota_env.breakPoint;
        ota_env.offsetOfImageOfCurrentSegment = ota_env.alreadyReceivedDataSizeOfImage;
        ota_env.offsetInFlashOfCurrentSegment = ota_env.dstFlashOffsetForNewImage + ota_env.offsetOfImageOfCurrentSegment;
        ota_env.offsetInFlashToProgram = ota_env.offsetInFlashOfCurrentSegment;
    }

	if(breakPoint ==0)
	{
	  app_tencent_sv_send_response_to_command(APP_TENCENT_OTA_RESUME_VERIFY, OTA_NO_ERROR, (uint8_t*)&breakPoint, 1);
	}
	else
	{
	   rsp.breakpoint_flag =1;
	   rsp.offset= breakPoint;

	   int len=sizeof(bool)+sizeof(uint32_t);

    // send response
    app_tencent_sv_send_response_to_command(APP_TENCENT_OTA_RESUME_VERIFY, OTA_NO_ERROR, (uint8_t*)&rsp, len);
	}
}

static void ota_data_packet_handler(APP_TENCENT_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
    if (!ota_env.isOTAInProgress)
    {
        return;
    }

    uint8_t* rawDataPtr = ptrParam;
    uint32_t rawDataSize = paramLen;
    TRACE(1,"Received image data size %d", rawDataSize);
    uint32_t leftDataSize = rawDataSize;
    uint32_t offsetInReceivedRawData = 0;

    do
    {
        uint32_t bytesToCopy;
        // copy to data buffer
        if ((ota_env.offsetInDataBufferForBurning + leftDataSize) >
            OTA_DATA_BUFFER_SIZE_FOR_BURNING)
        {
            bytesToCopy = OTA_DATA_BUFFER_SIZE_FOR_BURNING - ota_env.offsetInDataBufferForBurning;
        }
        else
        {
            bytesToCopy = leftDataSize;
        }

        leftDataSize -= bytesToCopy;

        memcpy(&ota_env.dataBufferForBurning[ota_env.offsetInDataBufferForBurning],
                &rawDataPtr[offsetInReceivedRawData], bytesToCopy);
        offsetInReceivedRawData += bytesToCopy;
        ota_env.offsetInDataBufferForBurning += bytesToCopy;
        TRACE(1,"offsetInDataBufferForBurning is %d", ota_env.offsetInDataBufferForBurning);
        if (OTA_DATA_BUFFER_SIZE_FOR_BURNING <= ota_env.offsetInDataBufferForBurning)
        {
            TRACE(0,"Program the image to flash.");

           #if (IMAGE_RECV_FLASH_CHECK == 1)
            if((ota_env.offsetInFlashToProgram - ota_env.dstFlashOffsetForNewImage > ota_env.totalImageSize) ||
                (ota_env.totalImageSize > MAX_IMAGE_SIZE))
            {
                TRACE(0,"ERROR: IMAGE_RECV_FLASH_CHECK");
                TRACE(0," ota_env(.offsetInFlashToProgram - .dstFlashOffsetForNewImage >= .totalImageSize)");
                TRACE(1," or (ota_env.totalImageSize > %d)", MAX_IMAGE_SIZE);
                TRACE(0," or .offsetInFlashToProgram isn't segment aligned");
                TRACE(3,".offsetInFlashToProgram:0x%x  .dstFlashOffsetForNewImage:0x%x  .totalImageSize:%d", ota_env.offsetInFlashToProgram, ota_env.dstFlashOffsetForNewImage, ota_env.totalImageSize);
                //ota_upgradeLog_destroy();  // In order to reduce unnecessary erasures and retransmissions we don't imeediately destory the log but reset ota, because a boundary check is performed before flashing and if there is really wrong we'll catch when an image CRC32 check finally.
				ota_upgradeLog_destroy();  // In order to reduce unnecessary erasures and retransmissions we don't imeediately destory the log but reset ota, because a boundary check is performed before flashing and if there is really wrong we'll catch when an image CRC32 check finally.
                app_tencent_sv_send_response_to_command(APP_TENCENT_OTA_DATA_PACKET, OTA_WRITE_FLASH_OFFSET_ERROR, NULL, 0);
				ota_env.isOTAInProgress = FALSE;
                return;
            }
            #endif

            ota_flush_data_to_flash(ota_env.dataBufferForBurning, OTA_DATA_BUFFER_SIZE_FOR_BURNING,
                ota_env.offsetInFlashToProgram);
            ota_env.offsetInFlashToProgram += OTA_DATA_BUFFER_SIZE_FOR_BURNING;
            ota_env.offsetInDataBufferForBurning = 0;
        }
    } while (offsetInReceivedRawData < rawDataSize);

    ota_env.alreadyReceivedDataSizeOfImage += rawDataSize;
    TRACE(1,"Image already received %d", ota_env.alreadyReceivedDataSizeOfImage);

 #if (IMAGE_RECV_FLASH_CHECK == 1)
    if((ota_env.alreadyReceivedDataSizeOfImage > ota_env.totalImageSize) ||
        (ota_env.totalImageSize > MAX_IMAGE_SIZE))
    {
        TRACE(0,"ERROR: IMAGE_RECV_FLASH_CHECK");
        TRACE(0," ota_env(.alreadyReceivedDataSizeOfImage > .totalImageSize)");
        TRACE(1," or (ota_env.totalImageSize > %d)", MAX_IMAGE_SIZE);
        TRACE(2,".alreadyReceivedDataSizeOfImage:%d  .totalImageSize:%d", ota_env.alreadyReceivedDataSizeOfImage, ota_env.totalImageSize);
        //ota_upgradeLog_destroy();  // In order to reduce unnecessary erasures and retransmissions we don't imeediately destory the log but reset ota, because a boundary check is performed before flashing and if there is really wrong we'll catch when an image CRC32 check finally.
		ota_upgradeLog_destroy();  // In order to reduce unnecessary erasures and retransmissions we don't imeediately destory the log but reset ota, because a boundary check is performed before flashing and if there is really wrong we'll catch when an image CRC32 check finally.
        app_tencent_sv_send_response_to_command(APP_TENCENT_OTA_DATA_PACKET, OTA_RECEIVE_SIZE_ERROR, NULL, 0);
		ota_env.isOTAInProgress = FALSE;
        return;
    }
#endif

    app_tencent_sv_send_response_to_command(APP_TENCENT_OTA_DATA_PACKET, OTA_NO_ERROR, NULL, 0);
}

static void ota_segmemt_verify_handler(APP_TENCENT_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
   TRACE(2,"funccode is %d, len is %d",funcCode,paramLen);
   TRACE(1,"CRC32 param  is 0x%x", *(uint32_t *)ptrParam);
   TRACE(1,"CRC32 param	is %p", ptrParam);

    #define MAX_SEG_VERIFY_RETEY    3
    static uint32_t seg_verify_retry = MAX_SEG_VERIFY_RETEY;

    if (!ota_env.isOTAInProgress)
    {
        return;
    }

    #if (IMAGE_RECV_FLASH_CHECK == 1)
    if((ota_env.offsetInFlashToProgram - ota_env.dstFlashOffsetForNewImage > ota_env.totalImageSize) ||
        (ota_env.totalImageSize > MAX_IMAGE_SIZE) /*||
        (ota_env.offsetInFlashToProgram & (MIN_SEG_ALIGN - 1))*/)
    {
        TRACE(0,"ERROR: IMAGE_RECV_FLASH_CHECK");
        TRACE(0," ota_env(.offsetInFlashToProgram - .dstFlashOffsetForNewImage >= .totalImageSize)");
        TRACE(1," or (ota_env.totalImageSize > %d)", MAX_IMAGE_SIZE);
        TRACE(0," or .offsetInFlashToProgram isn't segment aligned");
        TRACE(3,".offsetInFlashToProgram:0x%x  .dstFlashOffsetForNewImage:0x%x  .totalImageSize:%d", ota_env.offsetInFlashToProgram, ota_env.dstFlashOffsetForNewImage, ota_env.totalImageSize);
        //ota_upgradeLog_destroy();  // In order to reduce unnecessary erasures and retransmissions we don't imeediately destory the log but reset ota, because a boundary check is performed before flashing and if there is really wrong we'll catch when an image CRC32 check finally.
		ota_upgradeLog_destroy();  // In order to reduce unnecessary erasures and retransmissions we don't imeediately destory the log but reset ota, because a boundary check is performed before flashing and if there is really wrong we'll catch when an image CRC32 check finally.
        app_tencent_sv_send_response_to_command(APP_TENCENT_OTA_SEGMENT_VERIFY, OTA_WRITE_FLASH_OFFSET_ERROR, NULL, 0);
		ota_env.isOTAInProgress = FALSE;
        return;
    }
    #endif


    ota_flush_data_to_flash(ota_env.dataBufferForBurning, ota_env.offsetInDataBufferForBurning, ota_env.offsetInFlashToProgram);
    ota_env.offsetInFlashToProgram += ota_env.offsetInDataBufferForBurning;
    ota_env.offsetInDataBufferForBurning = 0;

    TRACE(0,"Calculate the crc32 of the segment.");

    uint32_t startFlashAddr = OTA_FLASH_LOGIC_ADDR + ota_env.dstFlashOffsetForNewImage + ota_env.offsetOfImageOfCurrentSegment;
    uint32_t lengthToDoCrcCheck = ota_env.alreadyReceivedDataSizeOfImage-ota_env.offsetOfImageOfCurrentSegment;

    ota_env.crc32OfSegment = crc32_c(0, (uint8_t *)(startFlashAddr), lengthToDoCrcCheck);

    TRACE(1,"CRC32 of the segement is 0x%x", ota_env.crc32OfSegment);
   // TRACE(1,"CRC32 param  is 0x%x", *(uint32_t *)ptrParam);
	TRACE(2,"startFlashAddr is 0x%x, length is %d",startFlashAddr,lengthToDoCrcCheck);

    if (*(uint32_t *)ptrParam == ota_env.crc32OfSegment)
    {
        app_tencent_sv_send_response_to_command(APP_TENCENT_OTA_SEGMENT_VERIFY, OTA_NO_ERROR, NULL, 0);

        ota_upgradeSize_log();
        seg_verify_retry = MAX_SEG_VERIFY_RETEY;

        // backup of the information in case the verification of current segment failed
        ota_env.offsetInFlashOfCurrentSegment = ota_env.offsetInFlashToProgram;
        ota_env.offsetOfImageOfCurrentSegment = ota_env.alreadyReceivedDataSizeOfImage;
    }
    else
    {
        TRACE(0,"erase segment");
        erase_segment(startFlashAddr);

        if(--seg_verify_retry == 0)
        {
            seg_verify_retry = MAX_SEG_VERIFY_RETEY;

            TRACE(0,"ERROR: segment verification retry too much!");
//            ota_upgradeLog_destroy();  // Yes, destory it and retransmit the entire image.
            app_tencent_sv_send_response_to_command(APP_TENCENT_OTA_SEGMENT_VERIFY, OTA_FLASH_ERROR, NULL, 0);
            ota_env.isOTAInProgress = FALSE;
            return;
        }

        // restore the offset
        ota_env.offsetInFlashToProgram = ota_env.offsetInFlashOfCurrentSegment;
        ota_env.alreadyReceivedDataSizeOfImage = ota_env.offsetOfImageOfCurrentSegment;
        app_tencent_sv_send_response_to_command(APP_TENCENT_OTA_SEGMENT_VERIFY, OTA_SEGMENT_CRC_FAILED, NULL, 0);
		ota_env.isOTAInProgress = FALSE;
    }

    // reset the CRC32 value of the segment
    ota_env.crc32OfSegment = 0;

    // reset the data buffer
    TRACE(2,"total size is %d already received %d", ota_env.totalImageSize,
        ota_env.alreadyReceivedDataSizeOfImage);
}

static uint8_t ota_upgrade_status;
static void ota_firmware_verify_handler(APP_TENCENT_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
    uint32_t firmware_crc32;

    // check whether all image data have been received
    if (ota_env.alreadyReceivedDataSizeOfImage == ota_env.totalImageSize)
    {
        TRACE(0,"The final image programming and crc32 check.");

        // flush the remaining data to flash

		if( ota_env.offsetInDataBufferForBurning !=0)
		{
		    ota_flush_data_to_flash(ota_env.dataBufferForBurning,
		                            ota_env.offsetInDataBufferForBurning,
                                    ota_env.offsetInFlashToProgram);
		}

		bool ret  =  ota_check_image_data_sanity_crc();

		if(ret)
		{
           // update the magic code of the application image
            app_update_magic_number_of_app_image(NORMAL_BOOT);

		}
		else
		{
		   TRACE(0,"data sanity crc failed.");
            app_tencent_sv_send_response_to_command(APP_TENCENT_OTA_FIRMWARE_VERIFY, OTA_FIRMWARE_CRC_FAILED, NULL, 0);
			ota_reset_env();
			return;
		}

        // check the crc32 of the image
        firmware_crc32 = ota_check_whole_image_crc();

        if(firmware_crc32 == ota_env.crc32OfImage)
        {
            FLASH_OTA_BOOT_INFO_T otaBootInfo = {COPY_NEW_IMAGE, ota_env.totalImageSize, ota_env.crc32OfImage};
            ota_update_boot_info(&otaBootInfo);
            ota_update_nv_data();
           // ota_send_result_response(ret);
            TRACE(0,"Whole image verification pass.");
            ota_env.isPendingForReboot = true;
            app_tencent_sv_send_response_to_command(APP_TENCENT_OTA_FIRMWARE_VERIFY, OTA_NO_ERROR, NULL, 0);
        }
        else
        {
            TRACE(0,"Whole image verification failed.");
            app_tencent_sv_send_response_to_command(APP_TENCENT_OTA_FIRMWARE_VERIFY, OTA_FIRMWARE_CRC_FAILED, NULL, 0);
			ota_env.isOTAInProgress = FALSE;
        }

        ota_upgradeLog_destroy();
	   if(firmware_crc32 == ota_env.crc32OfImage)
		{
              ota_upgrade_status = OTA_NO_ERROR;
		}
	    else
		{
		       ota_upgrade_status =OTA_FIRMWARE_CRC_FAILED;
			   ota_env.isOTAInProgress = FALSE;
		}
        app_tencent_sv_send_command(APP_TENCENT_OTA_RESULT, (uint8_t*)&ota_upgrade_status, sizeof(uint8_t));
    }
    else
    {
        app_tencent_sv_send_response_to_command(APP_TENCENT_OTA_FIRMWARE_VERIFY, OTA_FIRMWARE_CRC_FAILED, NULL, 0);
		ota_env.isOTAInProgress = FALSE;
    }

}

static void ota_result_handler(APP_TENCENT_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen)
{
     //send ota success comand;
    return;
}

static int command_time =0;
static void ota_result_rsp_handler(APP_TENCENT_SV_CMD_RET_STATUS_E retStatus, uint8_t* ptrParam, uint32_t paramLen)
{
    if(retStatus == (APP_TENCENT_SV_CMD_RET_STATUS_E)OTA_NO_ERROR)
    {
         TRACE(0,"it is success");
         TRACE(1,"ota_env.isPendingForReboot is %d",ota_env.isPendingForReboot);
		 osDelay(600);

         ota_reboot_to_use_new_image();   //reboot

     }
		 else if((retStatus == TENCENT_WAITING_RESPONSE_TIMEOUT) && (command_time < 3))
	{
	     TRACE(0,"ota_result_rsp_handler,time is out");
         command_time++;
		 app_tencent_sv_send_command(APP_TENCENT_OTA_RESULT, (uint8_t*)&ota_upgrade_status, sizeof(uint8_t));
    }
	 else
	{
	    TRACE(1,"error is %d ",retStatus);
	}
}

static void ota_cancel_handler(APP_TENCENT_SV_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen)
{

	APP_KEY_STATUS key_status;
	key_status.code = APP_KEY_CODE_FN4;
	key_status.event = APP_KEY_EVENT_CLICK;
	 if(stop_music_tag)
	  {
		  TRACE(0,"resume playing");
	  stop_music_tag =0;
	  app_bt_key(&key_status, NULL);
	  }
    ota_env.isOTAInProgress = FALSE;
    app_tencent_sv_send_response_to_command(APP_TENCENT_OTA_CANCEL, OTA_NO_ERROR, NULL, 0);
}

CUSTOM_COMMAND_TO_ADD(APP_TENCENT_OTA_START, ota_start_handler, FALSE, 0,    NULL );
CUSTOM_COMMAND_TO_ADD(APP_TENCENT_OTA_CONFIG, ota_config_handler, FALSE, 0,    NULL );
CUSTOM_COMMAND_TO_ADD(APP_TENCENT_OTA_RESUME_VERIFY, ota_resume_verify_handler, FALSE, 0,    NULL );
CUSTOM_COMMAND_TO_ADD(APP_TENCENT_OTA_DATA_PACKET, ota_data_packet_handler, FALSE, 0,   NULL );
CUSTOM_COMMAND_TO_ADD(APP_TENCENT_OTA_SEGMENT_VERIFY, ota_segmemt_verify_handler, FALSE, 0,    NULL );
CUSTOM_COMMAND_TO_ADD(APP_TENCENT_OTA_FIRMWARE_VERIFY, ota_firmware_verify_handler, FALSE, 0,    NULL );
CUSTOM_COMMAND_TO_ADD(APP_TENCENT_OTA_RESULT, ota_result_handler, TRUE, 2000, ota_result_rsp_handler);
CUSTOM_COMMAND_TO_ADD(APP_TENCENT_OTA_CANCEL, ota_cancel_handler, FALSE,0, NULL);
#endif

