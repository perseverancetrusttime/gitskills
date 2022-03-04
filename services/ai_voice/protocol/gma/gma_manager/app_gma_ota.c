#ifdef __GMA_VOICE__
#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_timer.h"
#include "hal_gpio.h"
#include "hal_trace.h"
#include "string.h"
#include "crc16_c.h"
#include "crc32_c.h"
#include "pmu.h"
#include "hal_norflash.h"
#include "norflash_drv.h"
#include "nvrecord.h"
#include "hal_cache.h"
#include "cmsis_nvic.h"
#include "hal_bootmode.h"
#include "norflash_api.h"
#include "btapp.h"

#include "app_utils.h"
#include "app_bt.h"
#include "app_gma_state_env.h"
#include "app_gma_ota.h"
#include "app_gma_cmd_handler.h"
#include "app_gma_ota_tws.h"
#include "app_ibrt_ota_cmd.h"
#include "app_tws_ibrt.h"
#include "ai_thread.h"
#include "app_tws_ctrl_thread.h"
#include "apps.h"
#include "app.h"
#include "app_ai_tws.h"


static GMA_OTA_ENV_T  otaEnv;
static uint8_t isInGMAOtaState = false;

#ifdef __GMA_OTA_TWS__
extern GMA_OTA_TWS_ENV_T gma_ota_tws_env;
#endif

//extern uint32_t __factory_start[];
extern uint32_t __ota_upgrade_log_start[];

static uint32_t user_data_nv_flash_offset;

#define LEN_OF_IMAGE_TAIL_TO_FIND_SANITY_CRC    512
static const char* image_info_sanity_crc_key_word = "CRC32_OF_IMAGE=0x";
static const char* old_image_info_sanity_crc_key_word = "CRC32_OF_IMAGE=";

FLASH_OTA_UPGRADE_LOG_FLASH_T *otaUpgradeLogInFlash = (FLASH_OTA_UPGRADE_LOG_FLASH_T *)__ota_upgrade_log_start;
#define otaUpgradeLog   (*otaUpgradeLogInFlash)

static const bool norflash_api_mode_async = true;
bool g_is_ota_packet = false;

GMA_DEV_FW_VER_T g_FW_Version = {0x00, 0x01, 0x01, 0x00, 0x00};


/****************************************orignal function start*********************************/
//static void ota_update_nv_data(void)
//{

//}


static void ota_reboot_to_use_new_image(void)
{
    TRACE(0,"OTA data receiving is done successfully, systerm will reboot");

    hal_sw_bootmode_set(HAL_SW_BOOTMODE_ENTER_HIDE_BOOT);
    hal_cmu_sys_reboot();
}

static void GmaFlushPendingFlashOp(enum NORFLASH_API_OPRATION_TYPE type)
{
    hal_trace_pause();
    do
    {
        norflash_api_flush();
        if (NORFLASH_API_ALL != type)
        {
            if (0 == norflash_api_get_used_buffer_count(NORFLASH_API_MODULE_ID_GMA_OTA, type))
            {
                break;
            }
        }
        else
        {
            if (norflash_api_buffer_is_free(NORFLASH_API_MODULE_ID_GMA_OTA))
            {
                break;
            }
        }

        osDelay(10);
    } while(1);

    hal_trace_continue();
}

void GmaOtaProgram(uint32_t flashOffset, uint8_t* ptr, uint32_t len,bool synWrite)
{
    uint32_t                lock;
    enum NORFLASH_API_RET_T ret;
    bool is_async;

    flashOffset &= 0xFFFFFF;

    is_async = norflash_api_mode_async;
    if (synWrite)
    {
        is_async = false;
    }

    do
    {
        lock = int_lock_global();
        hal_trace_pause();

        ret = norflash_api_write(NORFLASH_API_MODULE_ID_GMA_OTA,
            (OTA_FLASH_LOGIC_ADDR + flashOffset), ptr, len,is_async);


        hal_trace_continue();

        int_unlock_global(lock);

        if (NORFLASH_API_OK == ret)
        {
            TRACE(1,"%s: norflash_api_write ok!", __func__);
            break;
        }
        else if (NORFLASH_API_BUFFER_FULL == ret)
        {
            TRACE(0,"Flash async cache overflow! To flush it.");
            GmaFlushPendingFlashOp(NORFLASH_API_WRITTING);
        }
        else
        {
            ASSERT(0, "GmaOtaProgram: norflash_api_write failed. ret = %d", ret);
        }
    } while (1);


}

void GmaOtaErase(uint32_t flashOffset)
{
        // check whether the flash has been erased
    bool      isEmptyPage       = true;
    uint32_t *ptrStartFlashAddr = ( uint32_t * )(OTA_FLASH_LOGIC_ADDR + flashOffset);
    for (uint32_t index = 0; index < FLASH_SECTOR_SIZE_IN_BYTES / sizeof(uint32_t); index++)
    {
        if (0xFFFFFFFF != ptrStartFlashAddr[index])
        {
            isEmptyPage = false;
            break;
        }
    }

    if (isEmptyPage)
    {
        return;
    }

    uint32_t                lock;
    enum NORFLASH_API_RET_T ret;

    flashOffset &= 0xFFFFFF;
    do
    {
        lock = int_lock_global();
        hal_trace_pause();
        ret = norflash_api_erase(NORFLASH_API_MODULE_ID_GMA_OTA, (OTA_FLASH_LOGIC_ADDR + flashOffset),
            FLASH_SECTOR_SIZE_IN_BYTES,norflash_api_mode_async);
        hal_trace_continue();
        int_unlock_global(lock);

        if (NORFLASH_API_OK == ret)
        {
            TRACE(1,"%s: norflash_api_erase ok!", __func__);
            break;
        }
        else if(NORFLASH_API_BUFFER_FULL == ret)
        {
            TRACE(0,"Flash async cache overflow! To flush it.");
            GmaFlushPendingFlashOp(NORFLASH_API_ERASING);
        }
        else
        {
            ASSERT(0, "GmaOtaErase: norflash_api_erase failed. ret = %d", ret);
        }
    } while (1);
}


static void ota_flush_data_to_flash(uint8_t* ptrSource, uint32_t lengthToBurn, uint32_t offsetInFlashToProgram,bool synWrite)
{
    TRACE(2,"flush %d bytes to flash offset 0x%x", lengthToBurn, offsetInFlashToProgram);

    uint32_t preBytes = (FLASH_SECTOR_SIZE_IN_BYTES - (offsetInFlashToProgram % FLASH_SECTOR_SIZE_IN_BYTES)) % FLASH_SECTOR_SIZE_IN_BYTES;
    if (lengthToBurn < preBytes)
    {
        preBytes = lengthToBurn;
    }

    uint32_t middleBytes = 0;
    if (lengthToBurn > preBytes)
    {
        middleBytes = ((lengthToBurn - preBytes) / FLASH_SECTOR_SIZE_IN_BYTES * FLASH_SECTOR_SIZE_IN_BYTES);
    }

    uint32_t postBytes = 0;
    if (lengthToBurn > (preBytes + middleBytes))
    {
        postBytes = (offsetInFlashToProgram + lengthToBurn) % FLASH_SECTOR_SIZE_IN_BYTES;
    }

    TRACE(3,"Prebytes is %d middlebytes is %d postbytes is %d", preBytes, middleBytes, postBytes);

    uint8_t preSectorIndex = 0;
    if (preBytes > 0)
    {
        GmaOtaProgram(offsetInFlashToProgram, ptrSource, preBytes, synWrite);

        ptrSource += preBytes;
        offsetInFlashToProgram += preBytes;
        preSectorIndex = 1;
    }

    uint32_t sectorIndexInFlash = offsetInFlashToProgram / FLASH_SECTOR_SIZE_IN_BYTES + preSectorIndex;
    if (middleBytes > 0)
    {
        uint32_t sectorCntToProgram = middleBytes / FLASH_SECTOR_SIZE_IN_BYTES;
        for (uint32_t sector = 0; sector < sectorCntToProgram; sector++)
        {
            GmaOtaErase(sectorIndexInFlash * FLASH_SECTOR_SIZE_IN_BYTES);
            GmaOtaProgram(sectorIndexInFlash * FLASH_SECTOR_SIZE_IN_BYTES,
                             ptrSource + sector * FLASH_SECTOR_SIZE_IN_BYTES,
                             FLASH_SECTOR_SIZE_IN_BYTES,
                             synWrite);

            sectorIndexInFlash++;
        }

        ptrSource += middleBytes;
    }

    if (postBytes > 0)
    {
        GmaOtaErase(sectorIndexInFlash * FLASH_SECTOR_SIZE_IN_BYTES);
        GmaOtaProgram(sectorIndexInFlash * FLASH_SECTOR_SIZE_IN_BYTES,
                         ptrSource,
                         postBytes,
                         synWrite);
    }

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

static uint8_t asciiToHex(uint8_t asciiCode)
{
    if ((asciiCode >= '0') && (asciiCode <= '9'))
    {
        return asciiCode - '0';
    }
    else if ((asciiCode >= 'a') && (asciiCode <= 'f'))
    {
        return asciiCode - 'a' + 10;
    }
    else if ((asciiCode >= 'A') && (asciiCode <= 'F'))
    {
        return asciiCode - 'A' + 10;
    }
    else
    {
        return 0xff;
    }
}

/**
 * Currently, this function uses the CRC that was inserted by the build process.
 * This CRC check does not verify the last x bytes of the image.  x varies
 * depending on order of the key/value pair order but is < 512 bytes.
 */
static bool ota_check_image_data_sanity_crc(void) {
  // find the location of the CRC key word string
  uint8_t* ptrOfTheLast4KImage = (uint8_t *)(OTA_FLASH_LOGIC_ADDR+NEW_IMAGE_FLASH_OFFSET+
    otaEnv.totalImageSize-LEN_OF_IMAGE_TAIL_TO_FIND_SANITY_CRC);

  uint32_t sanityCrc32;
  uint32_t crc32ImageOffset;
  int32_t sanity_crc_location = find_key_word(ptrOfTheLast4KImage,
    LEN_OF_IMAGE_TAIL_TO_FIND_SANITY_CRC,
    (uint8_t *)image_info_sanity_crc_key_word,
    strlen(image_info_sanity_crc_key_word));
  if (-1 == sanity_crc_location)
  {
      sanity_crc_location = find_key_word(ptrOfTheLast4KImage,
        LEN_OF_IMAGE_TAIL_TO_FIND_SANITY_CRC,
        (uint8_t *)old_image_info_sanity_crc_key_word,
        strlen(old_image_info_sanity_crc_key_word));
    if (-1 == sanity_crc_location)
    {
      // if no sanity crc, fail the check
      TRACE(0,"no sanity crc");
      return false;
    }
    else
    {
      crc32ImageOffset = sanity_crc_location+otaEnv.totalImageSize-
        LEN_OF_IMAGE_TAIL_TO_FIND_SANITY_CRC+strlen(old_image_info_sanity_crc_key_word);
      sanityCrc32 = *(uint32_t *)(OTA_FLASH_LOGIC_ADDR+NEW_IMAGE_FLASH_OFFSET+crc32ImageOffset);
    }
  }
  else
  {
      crc32ImageOffset = sanity_crc_location+otaEnv.totalImageSize-
        LEN_OF_IMAGE_TAIL_TO_FIND_SANITY_CRC+strlen(image_info_sanity_crc_key_word);

    sanityCrc32 = 0;
    uint8_t* crcString = (uint8_t *)(OTA_FLASH_LOGIC_ADDR+NEW_IMAGE_FLASH_OFFSET+crc32ImageOffset);

    for (uint8_t index = 0;index < 8;index++)
    {
        sanityCrc32 |= (asciiToHex(crcString[index]) << (28-4*index));
    }
  }

  TRACE(1,"Bytes to generate crc32 is %d", crc32ImageOffset);
  TRACE(1,"sanity_crc_location is %d", sanity_crc_location);

  TRACE(1,"sanityCrc32 is 0x%x", sanityCrc32);

  // generate the CRC from image data
  uint32_t calculatedCrc32 = 0;
  calculatedCrc32 = crc32_c(calculatedCrc32, (uint8_t *)(OTA_FLASH_LOGIC_ADDR+NEW_IMAGE_FLASH_OFFSET),
    crc32ImageOffset);

  TRACE(1,"calculatedCrc32 is 0x%x", calculatedCrc32);

  if (sanityCrc32 == calculatedCrc32)
  {
    TRACE(0,"sanity is true");
    return true;
  }

  return false;
}

static void ota_update_boot_info(FLASH_OTA_BOOT_INFO_T* otaBootInfo)
{
    uint32_t lock;

    lock = int_lock();
    
    hal_norflash_disable_protection(HAL_FLASH_ID_0);
    pmu_flash_write_config();
    hal_norflash_erase(HAL_FLASH_ID_0, OTA_BOOT_INFO_FLASH_OFFSET, FLASH_SECTOR_SIZE_IN_BYTES);
    hal_norflash_write(HAL_FLASH_ID_0, OTA_BOOT_INFO_FLASH_OFFSET, (uint8_t*)otaBootInfo, sizeof(FLASH_OTA_BOOT_INFO_T));
    pmu_flash_read_config();

    int_unlock(lock);
}

static void app_update_magic_number_of_app_image(uint32_t newMagicNumber)
{
    uint32_t lock;

    memcpy(otaEnv.dataBufferForBurning, (uint8_t *)(OTA_FLASH_LOGIC_ADDR+otaEnv.dstFlashOffsetForNewImage),
        FLASH_SECTOR_SIZE_IN_BYTES);
    
    *(uint32_t *)otaEnv.dataBufferForBurning = newMagicNumber;
    
    lock = int_lock();
    pmu_flash_write_config();
    hal_norflash_erase(HAL_FLASH_ID_0, otaEnv.dstFlashOffsetForNewImage,
        FLASH_SECTOR_SIZE_IN_BYTES);
    hal_norflash_write(HAL_FLASH_ID_0, otaEnv.dstFlashOffsetForNewImage,
        otaEnv.dataBufferForBurning, FLASH_SECTOR_SIZE_IN_BYTES);
    pmu_flash_read_config();
    int_unlock(lock);    
   
}

void ota_upgradeSize_log(void)
{
}

static int ota_check_whole_image_crc(void)
{
    uint32_t verifiedDataSize = 0;
    int crc32Value = 0xFFFF;
    uint32_t verifiedBytes = 0;

    while (verifiedDataSize < otaEnv.totalImageSize)
    {
        if (otaEnv.totalImageSize - verifiedDataSize > OTA_DATA_BUFFER_SIZE_FOR_BURNING)
        {
            verifiedBytes = OTA_DATA_BUFFER_SIZE_FOR_BURNING;
        }
        else
        {
            verifiedBytes = otaEnv.totalImageSize - verifiedDataSize;
        }

        memcpy(otaEnv.dataBufferForBurning, (uint8_t *)(OTA_FLASH_LOGIC_ADDR+NEW_IMAGE_FLASH_OFFSET+verifiedDataSize),
            OTA_DATA_BUFFER_SIZE_FOR_BURNING);

        if (0 == verifiedDataSize)
        {
            if (*(uint32_t *)otaEnv.dataBufferForBurning != NORMAL_BOOT)
            {
                return false;
            }
            else
            {
                *(uint32_t *)otaEnv.dataBufferForBurning = 0xFFFFFFFF;
            }
        }

        crc32Value = crc16ccitt(crc32Value, (uint8_t *)otaEnv.dataBufferForBurning, 0, verifiedBytes);
        //crc32Value = crc32_c(crc32Value, (uint8_t *)otaEnv.dataBufferForBurning, verifiedBytes);

        verifiedDataSize += verifiedBytes;
    }

    return crc32Value;
}

void ota_upgradeLog_destroy(void)
{
  /*  uint32_t lock;
    lock = int_lock();
    otaEnv.resume_at_breakpoint = false;
    pmu_flash_write_config();
    hal_norflash_erase(HAL_FLASH_ID_0, (uint32_t)&otaUpgradeLog, FLASH_SECTOR_SIZE_IN_BYTES);
    pmu_flash_read_config();
    int_unlock(lock);

    TR_DEBUG(0,"Destroyed upgrade log in flash.");
    */
}



/**************************orignal function end******************************/

static void ota_reset_env(void)
{
     memset(&otaEnv, 0, sizeof(otaEnv));

    otaEnv.configuration.startLocationToWriteImage = NEW_IMAGE_FLASH_OFFSET;

    otaEnv.configuration.isToRenameBLE = false;
    otaEnv.configuration.isToRenameBT = false;
    otaEnv.configuration.isToUpdateBLEAddr = false;
    otaEnv.configuration.isToUpdateBTAddr = false;

    otaEnv.offsetInFlashToProgram = NEW_IMAGE_FLASH_OFFSET;
    otaEnv.flashOffsetOfUserDataPool = user_data_nv_flash_offset;  
    otaEnv.dstFlashOffsetForNewImage = otaEnv.offsetInFlashToProgram;


    otaEnv.flasehOffsetOfFactoryDataPool = user_data_nv_flash_offset + FLASH_SECTOR_SIZE_IN_BYTES;

    otaEnv.crc16OfImage = 0;
    otaEnv.offsetInDataBufferForBurning = 0;
    otaEnv.alreadyReceivedDataSizeOfImage = 0;
    //otaEnv.offsetOfImageOfCurrentSegment = 0;

    otaEnv.isOTAInProgress = false;
    otaEnv.isReadyToApply = false;

}

static void gma_ota_opera_callback(void *param)
{

}


void app_gma_ota_handler_init(void)
{
    enum NORFLASH_API_RET_T ret;
    uint32_t                block_size  = 0;
    uint32_t                sector_size = 0;
    uint32_t                page_size   = 0;
    
    #ifdef __APP_USER_DATA_NV_FLASH_OFFSET__
        user_data_nv_flash_offset = __APP_USER_DATA_NV_FLASH_OFFSET__;
    #else
        user_data_nv_flash_offset = hal_norflash_get_flash_total_size(HAL_FLASH_ID_0) - 2*4096;
    #endif


    hal_norflash_get_size(HAL_FLASH_ID_0, NULL, &block_size, &sector_size, &page_size);
    ret = norflash_api_register(NORFLASH_API_MODULE_ID_GMA_OTA,
                                HAL_FLASH_ID_0,
                                0x0,
                                (OTA_FLASH_LOGIC_ADDR + (user_data_nv_flash_offset & 0xffffff)),
                                block_size,
                                sector_size,
                                page_size,
                                FLASH_SECTOR_SIZE_IN_BYTES,
                                gma_ota_opera_callback);
    
    ASSERT(ret == NORFLASH_API_OK, "ota_control_init: norflash_api register failed,ret = %d.", ret);

    #ifdef __GMA_OTA_TWS__
        app_gma_ota_tws_init();
    #endif

    ota_reset_env();
}

void gma_enter_ota_state(void)
{
    if (isInGMAOtaState)
    {
        return;
    }

    // 1. switch to the highest freq
    app_sysfreq_req(APP_SYSFREQ_USER_OTA, APP_SYSFREQ_208M);

    // 2. exit bt sniff mode    
    app_bt_active_mode_set(ACTIVE_MODE_KEEPER_OTA, UPDATE_ACTIVE_MODE_FOR_ALL_LINKS);
    
    isInGMAOtaState = true;

    app_ble_update_conn_param_mode(BLE_CONN_PARAM_MODE_OTA, true);
}

void gma_exit_ota_state(void)
{
    if (!isInGMAOtaState)
    {
        return;
    }

    app_sysfreq_req(APP_SYSFREQ_USER_OTA, APP_SYSFREQ_32K);
    app_bt_active_mode_clear(ACTIVE_MODE_KEEPER_OTA, UPDATE_ACTIVE_MODE_FOR_ALL_LINKS);

    isInGMAOtaState = false;

    app_ble_update_conn_param_mode(BLE_CONN_PARAM_MODE_OTA, false);
    app_ble_update_conn_param_mode(BLE_CONN_PARAM_MODE_OTA_SLOWER, false);
}

void regroup_and_send_tws_data(GMA_OTA_CONTROL_PACKET_TYPE_E packetType, const uint8_t *dataPtr, uint16_t length)
{
    osEvent               evt;
    uint8_t               frameNum = 0;
    uint16_t              frameLen = 0;
    GMA_OTA_TWS_DATA_T tCmd     = {
        packetType,
    };

    // split packet into servel frame
    if (length % GMA_OTA_TWS_PAYLOAD_SIZE)
    {
        frameNum = length / GMA_OTA_TWS_PAYLOAD_SIZE + 1;
    }
    else
    {
        frameNum = length / GMA_OTA_TWS_PAYLOAD_SIZE;
    }

    TRACE(1,"packet will split into %d frames.", frameNum);

    if (1 < frameNum)
    {
        for (uint8_t i = 0; i < frameNum - 1; i++)
        {
            frameLen       = GMA_OTA_TWS_PAYLOAD_SIZE + GMA_OTA_TWS_HEAD_SIZE;
            tCmd.magicCode = GMApacketIncompleteMagicCode;
            tCmd.length    = GMA_OTA_TWS_PAYLOAD_SIZE;
            memcpy(tCmd.data, dataPtr + i * GMA_OTA_TWS_PAYLOAD_SIZE, tCmd.length);

            app_gma_ota_tws_push_data_into_cqueue(&gma_ota_tws_env.txQueue,
                                                 app_gma_ota_tws_get_tx_queue_mutex(),
                                                 ( uint8_t * )&tCmd,
                                                 frameLen);

            gma_ota_tws_env.rxThreadId = osThreadGetId();
            osSignalSet(gma_ota_tws_env.txThreadId, (1 << GMA_THREAD_SIGNAL));
            evt = osSignalWait((1 << GMA_SIGNAL_NUM), GMA_OTA_TWS_WAITTIME);

            if (evt.status == osEventTimeout)
            {
                TRACE(1,"[%s] SignalWait TIMEOUT!", __func__);
                app_gma_ota_update_peer_result(WAITING_RSP_TIMEOUT);  
            }

            if (NO_ERROR != app_gma_ota_get_peer_result())
            {
                return;
            }
        }
    }

    tCmd.magicCode = GMApacketCompleteMagicCode;
    tCmd.length    = length % GMA_OTA_TWS_PAYLOAD_SIZE;
    frameLen       = tCmd.length + GMA_OTA_TWS_HEAD_SIZE;
    memcpy(tCmd.data, dataPtr + (frameNum - 1) * GMA_OTA_TWS_PAYLOAD_SIZE, tCmd.length);

    app_gma_ota_tws_push_data_into_cqueue(&gma_ota_tws_env.txQueue,
                                         app_gma_ota_tws_get_tx_queue_mutex(),
                                         ( uint8_t * )&tCmd,
                                         frameLen);

    if (GMA_OTA_DATA != packetType)
    {
        TRACE(0,"data to be sent is:");
        DUMP8("0x%02x ", ( uint8_t * )&tCmd, frameLen);
    }
}



//   GMA ota 

void app_gma_ota_FW_Ver_get(GMA_DEV_FW_VER_T *FW_Ver)
{
    TRACE(1,"[%s]", __func__);

    if (FW_Ver)
    {
        memcpy(FW_Ver, &g_FW_Version, sizeof(GMA_DEV_FW_VER_T));
        //TR_DEBUG(0,"", );
    }
    else
    {
        TRACE(2,"[%s] %d NULL param", __func__, __LINE__);
    }
}

bool app_gma_ota_FW_valid(GMA_DEV_FW_VER_T *FW_Ver)
{
    if (!FW_Ver)
    {
        TRACE(2,"[%s] %d NULL param", __func__, __LINE__);
        return false;
    }
    if ((FW_Ver->revise_nb > 0x63) || 
        (FW_Ver->second_ver_nb > 0x63) ||
        (FW_Ver->primary_ver_nb > 0x63))
        return false;
    else
        return true;
}

int8_t app_gma_ota_FW_Ver_compare(GMA_DEV_FW_VER_T *FW1, GMA_DEV_FW_VER_T *FW2)
{
    TRACE(1,"[%s]", __func__);
    if (app_gma_ota_FW_valid(FW1) && app_gma_ota_FW_valid(FW2))
    {
        if (FW1->primary_ver_nb > FW2->primary_ver_nb)
            return 1;
        else if (FW1->primary_ver_nb < FW2->primary_ver_nb)
            return -1;
        else
        {
            if (FW1->second_ver_nb > FW2->second_ver_nb)
                return 1;
            else if (FW1->second_ver_nb < FW2->second_ver_nb)
                return -1;
            else
            {
                if (FW1->revise_nb > FW2->revise_nb)
                    return 1;
                else if (FW1->revise_nb < FW2->revise_nb)
                    return -1;
                else
                    return 0;
            }
        }
    }
    else
    {
        TRACE(2,"[%s] %d invalid param", __func__, __LINE__);
        return -1;
    }
}

void app_gma_ota_FW_Ver_print(GMA_DEV_FW_VER_T *FW_Ver)
{
    TRACE(1,"[%s]", __func__);
    TRACE(4,"FW Version: 0x%02x 0x%02x 0x%02x 0x%02x",    FW_Ver->revise_nb, 
                                                        FW_Ver->second_ver_nb, 
                                                        FW_Ver->primary_ver_nb, 
                                                        FW_Ver->reservation);
}


//
static void app_gma_ota_get_veriosn_handler(APP_GMA_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen, uint8_t device_id)
{
    TRACE(2,"[%s] funcCode:0x%02x", __func__, funcCode);

    GMA_DEV_FW_VER_T FW_Version;
    app_gma_ota_FW_Ver_get(&FW_Version);

    //reply 0x21
    app_gma_send_command(GMA_OP_OTA_VER_GET_RSP, (uint8_t*)&FW_Version, sizeof(GMA_DEV_FW_VER_T), false, device_id);
}

uint32_t app_gma_ota_get_already_received_size()
{
    return (otaEnv.alreadyReceivedDataSizeOfImage);
}

void app_gma_ota_start_handler(APP_GMA_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen, uint8_t device_id)
{
    TRACE(1,"%s", __func__);
    AI_TRANS_TYPE_E transport_type = app_ai_get_transport_type(AI_SPEC_ALI,ai_manager_get_foreground_ai_conidx());
    
    #ifdef __GMA_OTA_TWS__
        APP_GMA_CMD_RET_STATUS_E ret=NO_ERROR;

        if(APP_AI_TWS_MASTER == app_ai_tws_get_local_role())
        {
            if(AI_TRANSPORT_BLE == transport_type)
            {
                regroup_and_send_tws_data(GMA_OTA_BEGIN,(const uint8_t *)ptrParam, paramLen);
                ret = app_gma_ota_get_peer_result();

                if (NO_ERROR != ret)
                {
                    app_gma_ota_update_peer_result(NO_ERROR);
                    return ;
                }
                gma_ota_tws_env.rxThreadId = osThreadGetId();
                osSignalSet(gma_ota_tws_env.txThreadId,(1<<GMA_THREAD_SIGNAL));
            }
        }
    #endif

    ota_reset_env();
    gma_enter_ota_state();

    GMA_OTA_UPGRADE_CMD_T *ptrData = (GMA_OTA_UPGRADE_CMD_T *)ptrParam;
//    uint32_t version;
//    uint8_t upgradeType;

//    version = ptrData->version;   //the version;
//    upgradeType = ptrData ->flag;   // 0 is upgrade from start; 1 upgrade from breakpoint;


    //otaEnv.offsetInFlashToProgram = NEW_IMAGE_FLASH_OFFSET;
    //otaEnv.offsetInFlashOfCurrentSegment = otaEnv.offsetInFlashToProgram;
    //otaEnv.dstFlashOffsetForNewImage = otaEnv.offsetInFlashOfCurrentSegment;
    //otaEnv.isOTAInProgress = true;         //set ota progress is true

    otaEnv.totalImageSize = ptrData->imageSize;    //image size
    otaEnv.crc16OfImage = ptrData ->crc16;        //crc16;

     TRACE(2,"image size:0x%x %x", otaEnv.totalImageSize, otaEnv.crc16OfImage);


    #ifdef __GMA_OTA_TWS__
        osEvent evt;
        if (APP_AI_TWS_MASTER == app_ai_tws_get_local_role())
        {
            if(AI_TRANSPORT_BLE == transport_type)
            {
                evt = osSignalWait((1 << GMA_SIGNAL_NUM), GMA_OTA_TWS_WAITTIME);
                if (evt.status == osEventTimeout)
                {
                    ret = WAITING_RSP_TIMEOUT;
                    TRACE(1,"[%s]SignalWait TIMEOUT!", __func__);
                }
                    else if (evt.status == osEventSignal)
                {
                    ret = app_gma_ota_get_peer_result();
                }

                if(ret == NO_ERROR)
                {
                    GMA_OTA_UPGRADE_RSP_T pRsp;
                    pRsp.upgradeornot  =  DO_PERMIT_UPGRADE;
                    pRsp.size=0x00;     //upgrade from start;
                    pRsp.maxNumOfFrame  = OTA_MAX_NUMBER_OF_FRAMES;    //

                    app_gma_send_command(GMA_OP_OTA_UPDATE_RSP, (uint8_t*)& pRsp, sizeof(pRsp), false, device_id);
                }
            }
            else
            {
                gma_start_ota = GMA_OP_OTA_UPDATE_RSP;
            }
        }
        else if (APP_AI_TWS_SLAVE == app_ai_tws_get_local_role())
        {
            if(AI_TRANSPORT_SPP != transport_type)
            {
                app_gma_ota_tws_rsp(NO_ERROR);
            }
            else
            {
                GMA_OTA_UPGRADE_RSP_T pRsp;
                pRsp.upgradeornot  =  DO_PERMIT_UPGRADE;
                pRsp.size=0x00;     //upgrade from start;
                pRsp.maxNumOfFrame  = OTA_MAX_NUMBER_OF_FRAMES;    //
                tws_ctrl_send_cmd(IBRT_GMA_OTA, (uint8_t *)&pRsp, sizeof(pRsp));
            }
            ret = NO_ERROR;
        }

        TRACE(2,"[%s]Final result = %d", __func__, ret);
        app_gma_ota_update_peer_result(NO_ERROR);

    #endif

    //0x24 command to notfify the phone 

}

static uint8_t receviedData[APP_TWS_MAXIMUM_CMD_DATA_SIZE] = {0};
static uint32_t receviedSize = 0;
static uint8_t receviedSeq = 0;
void app_gma_ota_upgrade_data_handler(APP_GMA_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen, uint8_t device_id)
{
    otaEnv.g_current_frame_number =  app_gma_state_env_sequence_get() & 0x0f;
    TRACE(3,"%s line:%d curr:0x%02x", __func__, __LINE__, otaEnv.g_current_frame_number);

    AI_TRANS_TYPE_E transport_type = app_ai_get_transport_type(AI_SPEC_ALI,ai_manager_get_foreground_ai_conidx());

    if(APP_AI_TWS_MASTER == app_ai_tws_get_local_role() && AI_TRANSPORT_BLE == transport_type)
    {
        memcpy(&receviedData[receviedSize], ptrParam, paramLen);
        receviedSize += paramLen;
        receviedSeq++;

        otaEnv.alreadyReceivedDataSizeOfImage += paramLen;
        if(receviedSeq < 2 && paramLen >= 248)
        {
            if(otaEnv.g_current_frame_number == OTA_MAX_NUMBER_OF_FRAMES)
            {
                //reply 0x24 command;
                GMA_OTA_REPORT_CMD_T pRsp;
                pRsp.current_frame_num = otaEnv.g_current_frame_number;
                pRsp.total_received_size =  otaEnv.alreadyReceivedDataSizeOfImage;
            
                TRACE(2,"curr_frame:0x%02x, total_received_size is %d", otaEnv.g_current_frame_number,pRsp.total_received_size);
                app_gma_send_command(GMA_OP_OTA_PKG_RECV_RSP, (uint8_t*)&pRsp, sizeof(GMA_OTA_REPORT_CMD_T), false, device_id);
            }
            return;
        }
    }

    #ifdef __GMA_OTA_TWS__
        APP_GMA_CMD_RET_STATUS_E  ret= NO_ERROR;
        if(APP_AI_TWS_MASTER == app_ai_tws_get_local_role())
        {
            if(AI_TRANSPORT_BLE == transport_type)
            {
                regroup_and_send_tws_data(GMA_OTA_DATA,(const uint8_t *)receviedData, receviedSize);
                ret = app_gma_ota_get_peer_result();

                if(NO_ERROR != ret)
                {
                    app_gma_ota_update_peer_result(NO_ERROR);
                    return ;
                }
                gma_ota_tws_env.rxThreadId = osThreadGetId();
                osSignalSet(gma_ota_tws_env.txThreadId,(1<<GMA_THREAD_SIGNAL));
            }
        }
    #endif

    uint8_t * rawDataPtr = 0;
    uint32_t rawDataSize  = 0;

    if(APP_AI_TWS_MASTER == app_ai_tws_get_local_role() && AI_TRANSPORT_BLE == transport_type)
    {
        rawDataPtr = receviedData;
        rawDataSize  = receviedSize;
    }
    else
    {
        rawDataPtr = ptrParam;
        rawDataSize  = paramLen;
    }

    TRACE(1,"Received image data size %d", rawDataSize);
    uint32_t leftDataSize = rawDataSize;
    uint32_t offsetInReceivedRawData = 0;

    TRACE(1,"first otaEnv.offsetInDataBufferForBurning is %d", otaEnv.offsetInDataBufferForBurning);

    do
    {
        uint32_t bytesToCopy;
        if((otaEnv.offsetInDataBufferForBurning + leftDataSize) > OTA_DATA_BUFFER_SIZE_FOR_BURNING)
        {
            bytesToCopy = OTA_DATA_BUFFER_SIZE_FOR_BURNING - otaEnv.offsetInDataBufferForBurning;
        }
        else
        {
            bytesToCopy = leftDataSize;
            TRACE(1,"it is here,bytesToCopy is %d",bytesToCopy);
        }

        leftDataSize -=bytesToCopy;

        memcpy(&otaEnv.dataBufferForBurning[otaEnv.offsetInDataBufferForBurning],
            &rawDataPtr[offsetInReceivedRawData],bytesToCopy);

        offsetInReceivedRawData +=bytesToCopy;
        otaEnv.offsetInDataBufferForBurning +=bytesToCopy;

        ASSERT(otaEnv.offsetInDataBufferForBurning <= OTA_DATA_BUFFER_SIZE_FOR_BURNING, "bad math in %s", __func__);

        TRACE(2,"offsetInDataBufferForBurning is %d %d",otaEnv.offsetInDataBufferForBurning, bytesToCopy);

        if(OTA_DATA_BUFFER_SIZE_FOR_BURNING == otaEnv.offsetInDataBufferForBurning)
        {
                TRACE(0,"Program the image to flash.");

                ota_flush_data_to_flash(otaEnv.dataBufferForBurning, 
                                        OTA_DATA_BUFFER_SIZE_FOR_BURNING, 
                                        otaEnv.offsetInFlashToProgram,
                                        false);            
                otaEnv.offsetInFlashToProgram += OTA_DATA_BUFFER_SIZE_FOR_BURNING;
                otaEnv.offsetInDataBufferForBurning = 0;
        }
    }  while( offsetInReceivedRawData < rawDataSize);

    if (APP_AI_TWS_SLAVE == app_ai_tws_get_local_role() || AI_TRANSPORT_SPP == transport_type)
    {
        otaEnv.alreadyReceivedDataSizeOfImage += rawDataSize;
    }
    TRACE(2,"Image already received %d %d", otaEnv.alreadyReceivedDataSizeOfImage, otaEnv.g_current_frame_number);

    #ifdef __GMA_OTA_TWS__
        osEvent evt;
        if (APP_AI_TWS_MASTER == app_ai_tws_get_local_role())
        {
            if(AI_TRANSPORT_BLE == transport_type)
            {
                evt = osSignalWait((1 << GMA_SIGNAL_NUM), GMA_OTA_TWS_WAITTIME);
                if (evt.status == osEventTimeout)
                {
                    ret = WAITING_RSP_TIMEOUT;
                    TRACE(1,"[%s] SignalWait TIMEOUT!", __func__);
                }
                else if (evt.status == osEventSignal)
                {
                    ret = app_gma_ota_get_peer_result();
                }

                if(ret == NO_ERROR)
                {
                    if(otaEnv.g_current_frame_number == OTA_MAX_NUMBER_OF_FRAMES)
                    {
                        //reply 0x24 command;
                        GMA_OTA_REPORT_CMD_T pRsp;
                        pRsp.current_frame_num = otaEnv.g_current_frame_number;
                        pRsp.total_received_size =  otaEnv.alreadyReceivedDataSizeOfImage;

                        TRACE(2,"curr_frame:0x%02x, total_received_size is %d", otaEnv.g_current_frame_number,pRsp.total_received_size);
                        app_gma_send_command(GMA_OP_OTA_PKG_RECV_RSP, (uint8_t*)&pRsp, sizeof(GMA_OTA_REPORT_CMD_T), false, device_id);
                    }
                }
            }
            else
            {
                gma_start_ota = GMA_OP_OTA_PKG_RECV_RSP;
            }
        }
        else if (APP_AI_TWS_SLAVE == app_ai_tws_get_local_role())
        {
            if(AI_TRANSPORT_SPP != transport_type)
            {
                app_gma_ota_tws_rsp(ret);
            }
            else
            {
                GMA_OTA_REPORT_CMD_T pRsp;
                pRsp.current_frame_num = otaEnv.g_current_frame_number;
                pRsp.total_received_size =  otaEnv.alreadyReceivedDataSizeOfImage;

                TRACE(2,"curr_frame:0x%02x, total_received_size is %d", otaEnv.g_current_frame_number,pRsp.total_received_size);
                tws_ctrl_send_cmd(IBRT_GMA_OTA, (uint8_t *)&pRsp, sizeof(pRsp));
            }
        }

        TRACE(2,"[%s] Final result = %d", __func__, ret);
        app_gma_ota_update_peer_result(NO_ERROR);

        memset(receviedData, 0, APP_TWS_MAXIMUM_CMD_DATA_SIZE);
        receviedSize = 0;
        receviedSeq = 0;

        return ;
    #endif

}

//crc check
void app_gma_ota_whole_crc_validate_handler(APP_GMA_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen, uint8_t device_id)
{
    AI_TRANS_TYPE_E transport_type = app_ai_get_transport_type(AI_SPEC_ALI,ai_manager_get_foreground_ai_conidx());
    
    #ifdef __GMA_OTA_TWS__
        APP_GMA_CMD_RET_STATUS_E  result= NO_ERROR;
        if(APP_AI_TWS_MASTER == app_ai_tws_get_local_role())
        {
            if(AI_TRANSPORT_BLE == transport_type)
            {
                regroup_and_send_tws_data(GMA_OTA_CRC,(const uint8_t *)ptrParam, paramLen);
                result = app_gma_ota_get_peer_result();

                if(NO_ERROR != result)
                {
                    app_gma_ota_update_peer_result(NO_ERROR);
                    return ;
                }

                gma_ota_tws_env.rxThreadId = osThreadGetId();
                osSignalSet(gma_ota_tws_env.txThreadId,(1<<GMA_THREAD_SIGNAL));
            }
        }
    #endif

    int firmware_crc16;
    GMA_OTA_REPORT_CRC_CMD_T pRsp;

    //if(otaEnv.alreadyReceivedDataSizeOfImage == otaEnv.totalImageSize )
    //{
    //    ota_flush_data_to_flash(otaEnv.dataBufferForBurning, otaEnv.offsetInDataBufferForBurning, otaEnv.offsetInFlashToProgram,false);
    //    otaEnv.offsetInFlashToProgram += otaEnv.offsetInDataBufferForBurning;
    //    otaEnv.offsetInDataBufferForBurning = 0;

    //}

    //backup of the information in case the verification of current segment failed
    //otaEnv.offsetInFlashOfCurrentSegment = otaEnv.offsetInFlashToProgram;
    //otaEnv.offsetOfImageOfCurrentSegment = otaEnv.alreadyReceivedDataSizeOfImage;


    TRACE(2,"total size is %d already received %d", otaEnv.totalImageSize,otaEnv.alreadyReceivedDataSizeOfImage);

    if(otaEnv.alreadyReceivedDataSizeOfImage == otaEnv.totalImageSize )
    {
        // flush the remaining data to flash

        if( otaEnv.offsetInDataBufferForBurning !=0)
        {
            ota_flush_data_to_flash(otaEnv.dataBufferForBurning,
                                    otaEnv.offsetInDataBufferForBurning,
                                    otaEnv.offsetInFlashToProgram,true);
        }

        bool ret  =  ota_check_image_data_sanity_crc();

        if(ret)
        {
            TRACE(0,"data sanity crc success");
           // update the magic code of the application image
            app_update_magic_number_of_app_image(NORMAL_BOOT);

        }
        else
        {
           TRACE(0,"data sanity crc failed.");
            ota_reset_env();
        }

        firmware_crc16 =  ota_check_whole_image_crc();  //this is get the crc32 value;
        TRACE(1,"firmware_crc16 is %d",firmware_crc16);

        if(ret)
        {
            if(firmware_crc16 ==  otaEnv.crc16OfImage)
            {
                FLASH_OTA_BOOT_INFO_T otaBootInfo = {COPY_NEW_IMAGE, otaEnv.totalImageSize, otaEnv.crc16OfImage};                    
                ota_update_boot_info(&otaBootInfo);                 
                //ota_update_nv_data();

                TRACE(0,"Whole image verification pass.");
                otaEnv.isReadyToApply = true;
                pRsp.result = 0x01;

                if (otaEnv.isReadyToApply && app_tws_ibrt_tws_link_connected())
                {
                    TRACE(0,"ota success, it will reboot");

                    app_start_postponed_reset();
                    //osDelay(100);
                    //ota_reboot_to_use_new_image();
                }
                else
                {
                   TRACE(0,"it is error,ota failure");
                   ota_reset_env();

                  // ota_upgradeLog_destroy();
                }

                result= NO_ERROR;
            }
            else
            {
                TRACE(0,"Whole image verification failed.");

                pRsp.result = 0x00;

                result = WHOLE_DATA_CRC_CHECK_FAILED;

                ota_reset_env();

                gma_exit_ota_state();

            }
        }

    }
    else
    {
        TRACE(0,"image size is error");

        pRsp.result = 0x00;

        result= DATA_XFER_LEN_NOT_MATCHED;

        ota_reset_env();

        gma_exit_ota_state();

    }

    #ifdef __GMA_OTA_TWS__
        osEvent evt;
        if (APP_AI_TWS_MASTER == app_ai_tws_get_local_role())
        {
            if(AI_TRANSPORT_BLE == transport_type)
            {
                evt = osSignalWait((1 << GMA_SIGNAL_NUM), GMA_OTA_TWS_WAITTIME);
                if (evt.status == osEventTimeout)
                {
                    result = WAITING_RSP_TIMEOUT;
                    TRACE(1,"[%s] SignalWait TIMEOUT!", __func__);
                }
                else if (evt.status == osEventSignal)
                {
                    result = ((app_gma_ota_get_peer_result() == NO_ERROR) &&
                                     (result == NO_ERROR))
                                        ? NO_ERROR
                                        : WHOLE_DATA_CRC_CHECK_FAILED;
                }

                app_gma_send_command(GMA_OP_OTA_COMP_CHECK_RSP, (uint8_t*)&pRsp, sizeof(pRsp), false, device_id);
            }
            else
            {
                gma_crc = pRsp.result;
                gma_start_ota = GMA_OP_OTA_COMP_CHECK_RSP;
            }
        }
        else if (APP_AI_TWS_SLAVE == app_ai_tws_get_local_role())
        {
            if(AI_TRANSPORT_SPP != transport_type)
            {
                app_gma_ota_tws_rsp(result);
            }
            else
            {
                tws_ctrl_send_cmd(IBRT_GMA_OTA, (uint8_t *)&pRsp, sizeof(pRsp));
            }
        }

        TRACE(2,"[%s] Final result = %d", __func__, result);
        app_gma_ota_update_peer_result(NO_ERROR);
        return ;
    #endif

}

void app_gma_ota_reboot(void)
{
    if (otaEnv.isReadyToApply && app_tws_ibrt_tws_link_connected())
    {
        TRACE(0,"ota success, it will reboot");

        app_start_postponed_reset();
        //osDelay(100);
        //ota_reboot_to_use_new_image();
    }
    else
    {
         TRACE(0,"it is error,ota failure");
         ota_reset_env();

         // ota_upgradeLog_destroy();
    }
}

void app_gma_ota_finished_handler(void)
{
    #ifdef __GMA_OTA_TWS__
        APP_GMA_CMD_RET_STATUS_E  ret= NO_ERROR;
        if(APP_AI_TWS_MASTER == app_ai_tws_get_local_role())
        {
            regroup_and_send_tws_data(GMA_OTA_APPLY,NULL, 0);
            ret = app_gma_ota_get_peer_result();

            if(NO_ERROR != ret)
            {
                app_gma_ota_update_peer_result(NO_ERROR);
                return ;
            }
            gma_ota_tws_env.rxThreadId = osThreadGetId();
            osSignalSet(gma_ota_tws_env.txThreadId,(1<<GMA_THREAD_SIGNAL));
        }
    #endif


    if (otaEnv.isReadyToApply && app_tws_ibrt_tws_link_connected())
    {
        TRACE(0,"ota success, it will reboot");

        app_start_postponed_reset();
        //osDelay(100);
        ota_reboot_to_use_new_image();
    }
    else
    {
       TRACE(0,"it is error,ota failure");
       ota_reset_env();

      // ota_upgradeLog_destroy();
    }

    #ifdef __GMA_OTA_TWS__
        osEvent evt;
        if (APP_AI_TWS_MASTER == app_ai_tws_get_local_role())
        {
            evt = osSignalWait((1 << GMA_SIGNAL_NUM), GMA_OTA_TWS_WAITTIME);
            if (evt.status == osEventTimeout)
            {
                ret = NO_ERROR;
                TRACE(1,"[%s]SignalWait TIMEOUT!", __func__);
            }
            else if (evt.status == osEventSignal)
            {
                ret = ((app_gma_ota_get_peer_result() == NO_ERROR) &&
                       (ret == NO_ERROR))
                          ? NO_ERROR
                          : WAITING_RSP_TIMEOUT;
            }
        }
        else if (APP_AI_TWS_SLAVE == app_ai_tws_get_local_role())
            app_gma_ota_tws_rsp(ret);

        TRACE(2,"[%s]Final result = %d", __func__, ret);
        app_gma_ota_update_peer_result(NO_ERROR);
#endif

}


void app_gma_ota_report_current_frame_and_length_handler(APP_GMA_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen, uint8_t device_id)
{
    //GMA_OTA_REPORT_CMD_T pRsp;
    //pRsp.current_frame_num = otaEnv.g_current_frame_number;
    //pRsp.total_received_size =  otaEnv.alreadyReceivedDataSizeOfImage;

    app_gma_send_command(GMA_OP_OTA_PKG_RECV_RSP, NULL, 0, false, device_id);
}

CUSTOM_COMMAND_TO_ADD(GMA_OP_OTA_VER_GET_CMD,       app_gma_ota_get_veriosn_handler, false, 0, NULL );  //0x20
CUSTOM_COMMAND_TO_ADD(GMA_OP_OTA_UPDATE_CMD,        app_gma_ota_start_handler, false, 0, NULL );   //0x21
CUSTOM_COMMAND_TO_ADD(GMA_OP_OTA_COMP_NTF_CMD,      app_gma_ota_whole_crc_validate_handler, false, 0, NULL );  //0x25
CUSTOM_COMMAND_TO_ADD(GMA_OP_OTA_GET_RECVED_CMD,    app_gma_ota_report_current_frame_and_length_handler, false, 0, NULL );    //0x27
CUSTOM_COMMAND_TO_ADD(GMA_OP_OTA_SEND_PKG_CMD,      app_gma_ota_upgrade_data_handler, false, 0, NULL );   //data handler



#endif
