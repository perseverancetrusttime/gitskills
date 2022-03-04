#include "hal_trace.h"
#include "hal_timer.h"
#include "cmsis_os.h"
#include "cqueue.h"
#include <stdlib.h>
#include "app_audio.h"
#include "btapp.h"
#include "app_bt.h"
#include "apps.h"
#include "app_key.h"
#include "hal_location.h"
#include "audioflinger.h"
#include "tgt_hardware.h"
#include "nvrecord.h"
#include "nvrecord_env.h"
#include "ai_transport.h"
#include "ai_manager.h"
#include "ai_control.h"
#include "ai_spp.h"
#include "ai_thread.h"
#include "app_ai_voice.h"
#include "app_recording_handle.h"
#include "voice_compression.h"
#include "bt_drv_interface.h"
#include "audio_dump.h"
#include "bt_drv_reg_op.h"
#include "app_utils.h"
#include "hal_dma.h"

#if defined(IBRT)
#include "app_tws_ctrl_thread.h"
#include "crc32_c.h"
#include "app_ibrt_if.h"
#include "app_ibrt_customif_cmd.h"
#include "app_tws_ibrt.h"
#include "app_ibrt_if.h"
#endif

#ifdef __THIRDPARTY
#include "app_thirdparty.h"
#endif

/***************************************************/
/***************************************************/
#define SLAVE_ENCODED_DATA_STORAGE_BUF_SIZE         (ENCODED_DATA_BUF_SIZE / 2)
#define LOCAL_ENCODED_DATA_STORAGE_BUF_SIZE         (ENCODED_DATA_BUF_SIZE / 2)

#define SIZE_PER_SAMPLE                             (2) //!< 16bit = 2 bytes

#ifdef IBRT
#define APP_CAPTURE_AUDIO_SYNC_DELAY_US         (500*1000)
#define APP_VOICE_CAPTURE_SYNC_TRIGGER_TIMEROUT (APP_CAPTURE_AUDIO_SYNC_DELAY_US/1000*2)
#endif

#ifdef VOC_ENCODE_SCALABLE
#define RECORD_ONESHOT_PROCESS_SAMPLE_CNT   (480) //!< on the request of SamSung
#define RECORD_ONESHOT_PROCESS_LEN          (RECORD_ONESHOT_PROCESS_SAMPLE_CNT * SIZE_PER_SAMPLE)
#endif

#ifdef VOC_ENCODE_OPUS
#define RECORD_ONESHOT_PROCESS_LEN          (VOICE_OPUS_PCM_DATA_SIZE_PER_FRAME)
#endif

#if (SLAVE_ENCODED_DATA_STORAGE_BUF_SIZE + LOCAL_ENCODED_DATA_STORAGE_BUF_SIZE > ENCODED_DATA_BUF_SIZE)
#error "Encoded data buffer exceed!"
#endif

#if defined(RECORDING_USE_SCALABLE) && defined(RECORDING_USE_OPUS)
#error "Too many compression codec enabled for recording"
#endif

#if !defined(RECORDING_USE_SCALABLE) && !defined(RECORDING_USE_OPUS)
#error "No compression codec enabled for recording"
#endif

#undef _ENUM_DEF
#define _ENUM_DEF(a, b) #b

void VoiceShareThread(void const *argument);
osThreadDef(VoiceShareThread, (osPriorityAboveNormal), 1, (1024 * 3), "voice_share");
osThreadId voiceTid = NULL;
static osMutexId slave_mem_mutex_id = NULL;
osMutexDef(slave_mem_mutex);

static osMutexId local_mem_mutex_id = NULL;
osMutexDef(local_mem_mutex);

/***************************************************/
/*Variable                                         */
/***************************************************/
static APP_VOICE_SHARE_ENV_T voice_mem_for_slave;
static APP_VOICE_SHARE_ENV_T voice_mem_for_local;
static uint8_t recording_enable = 0x00;
static uint32_t outCursor = STARTING_DATA_INDEX;
static uint32_t curCursor = STARTING_DATA_INDEX;
static uint32_t revCursorPeer = STARTING_DATA_INDEX;
static uint8_t gather_count = 0;
static uint32_t sTime = 0;
static uint32_t delayRT = 0;
static bool reportSent = false;
static bool cacheBufInitDone = false;

#if defined(VOC_ENCODE_SCALABLE)
#define L_SYNC_WORD             (DEFAULT_SYNC_WORD)
#define R_SYNC_WORD             (0xB7)
#define L_INVALID_SYNC_WORD     (0x55)
#define R_INVALID_SYNC_WORD     (0x77)
#define CAPTURE_SAMPLE_RATE     (AUD_SAMPRATE_32000)
#define BITRATE_UPDATE_RETRY    (5)

static void _scalable_bitrate_update_arbitor(void);

typedef struct
{
    uint8_t expectedPeerBitrate;
    uint8_t expectedBitrateUpdateCursor;
    uint8_t bitrateUpdateRetryCnt;
    uint8_t cacheBufLvl;
    uint8_t peerCacheBufLvl;
    uint8_t bitrateUpdateState;
    uint8_t lastBitrate;
    uint8_t currentBitrate;
    uint8_t futureBitrate;
    bool    bitrateUpdateFlag;
} SCALABLE_RECORDING_CTL_T;

SCALABLE_RECORDING_CTL_T _ctl;

static const char* _buf_lvl_str[CACHE_BUF_LVL_CNT] = {
    CACHE_BUF_LVL_LIST,
};

POSSIBLY_UNUSED static const char* _br_update_state_str[BITRATE_UPDATE_STATE_CNT] = {
    BITRATE_UPDATE_STATE_LIST,
};

/// map cache buffer level to scalabel bitrate
static const uint8_t _buf_lvl_2_bitrate[CACHE_BUF_LVL_CNT] = {
    [CACHE_BUF_LVL_MORE_THAN_HALF_LEFT]     = BITRATE_MODE_1, //!< avoid packet length caused crash
    [CACHE_BUF_LVL_MORE_THAN_QUARTER_LEFT]  = BITRATE_MODE_1,
    [CACHE_BUF_LVL_LESS_THAN_QUARTER_LEFT]  = BITRATE_MODE_2,
};

uint8_t scalableTempBUf[FRAME_LENGTH_MAX] = {0,};

#elif defined(VOC_ENCODE_OPUS)
#define CAPTURE_SAMPLE_RATE     ((AUD_SAMPRATE_T) VOICE_OPUS_SAMPLE_RATE)
#ifdef RECORDING_USE_OPUS_LOW_BANDWIDTH
//OPUS bitrate:32k sample rate: 48K
static const uint8_t encoded_zero[10] = {0x4b, 0x41, 0x46, 0x0b, 0xe4, 0x55, 0xbf, 0x14, 0x98, 0x7c};
static const uint8_t encoded_zero_size = 10;
#else
//OPUS bitrate:64k sample rate: 48K
static const uint8_t encoded_zero[9] = {0x4b, 0x41, 0x97, 0x06, 0xe3, 0x7d, 0x8e, 0xc8, 0x58};
static const uint8_t encoded_zero_size = 9;
#endif
#endif

APP_RECORDING_DATA_T recording_data;
uint8_t mobileAddr[6] = {0};

#ifdef IBRT
static uint32_t _trigger_tg_tick = 0;
static uint32_t _trigger_enable = 0;

static void _trigger_timeout_cb(void const *n);
osTimerDef(APP_IBRT_VOICE_CAPTURE_TRIGGER_TIMEOUT, _trigger_timeout_cb);
osTimerId _trigger_timeout_id = NULL;
#endif

static void app_recording_update_status(uint8_t newStatus)
{
    TRACE(0, "recording_enable update:0x%x->0x%x", recording_enable, newStatus);
    recording_enable = newStatus;
}

void LOCK_SLAVE_MEM_FIFO(void)
{
    osMutexWait(slave_mem_mutex_id, osWaitForever);
}

void UNLOCK_SLAVE_MEM_FIFO(void)
{
    osMutexRelease(slave_mem_mutex_id);
}

void LOCK_LOCAL_MEM_FIFO(void)
{
    osMutexWait(local_mem_mutex_id, osWaitForever);
}

void UNLOCK_LOCAL_MEM_FIFO(void)
{
    osMutexRelease(local_mem_mutex_id);
}

void _share_resource_init(void)
{
    TRACE(1, "%s +++", __func__);

    if (local_mem_mutex_id == NULL)
    {
        local_mem_mutex_id = osMutexCreate((osMutex(local_mem_mutex)));
    }

    if (slave_mem_mutex_id == NULL)
    {
        slave_mem_mutex_id = osMutexCreate((osMutex(slave_mem_mutex)));
    }

    if (!cacheBufInitDone)
    {
        app_ai_if_mempool_get_buff(&(voice_mem_for_local.encodedDataBuf),
                                   LOCAL_ENCODED_DATA_STORAGE_BUF_SIZE);
        LOCK_LOCAL_MEM_FIFO();
        InitCQueue(&(voice_mem_for_local.ShareDataQueue), LOCAL_ENCODED_DATA_STORAGE_BUF_SIZE,
                   (CQItemType *)(voice_mem_for_local.encodedDataBuf));
        UNLOCK_LOCAL_MEM_FIFO();

        app_ai_if_mempool_get_buff(&(voice_mem_for_slave.encodedDataBuf),
                                   SLAVE_ENCODED_DATA_STORAGE_BUF_SIZE);
        LOCK_SLAVE_MEM_FIFO();
        InitCQueue(&(voice_mem_for_slave.ShareDataQueue), SLAVE_ENCODED_DATA_STORAGE_BUF_SIZE,
                   (CQItemType *)(voice_mem_for_slave.encodedDataBuf));
        UNLOCK_SLAVE_MEM_FIFO();

        cacheBufInitDone = true;
    }
}

static void _share_resource_deinit(void)
{
    TRACE(1, "%s", __func__);
    cacheBufInitDone = false;
}

#ifdef RECORDING_USE_SCALABLE
static uint8_t analysisData[HEADER_SIZE + RECORD_DATA_PACKET_GATHER_LIMIT * FRAME_LENGTH_MAX];

static uint16_t _cached_scalable_frame_len_getter(CQueue *q)
{
    PeekCQueueToBuf(q, (CQItemType *)analysisData, ARRAY_SIZE(analysisData));

    SCALABLE_FRAME_HEADER_T *hdr = (SCALABLE_FRAME_HEADER_T *)(&analysisData[HEADER_SIZE]);
    /// check the frame validity
    ASSERT((DEFAULT_SYNC_WORD == hdr->sync) || (R_SYNC_WORD == hdr->sync) || (R_INVALID_SYNC_WORD == hdr->sync),
           "%s invalid sync word:0x%x", __func__, hdr->sync);
    ASSERT(BITRATE_MODE_NUM > hdr->bitrate, "%s invalid bitrate:%d", __func__, hdr->bitrate);

    SCALABLE_FRAME_HEADER_T *hdr1 = (SCALABLE_FRAME_HEADER_T *)(&analysisData[HEADER_SIZE + voice_compression_get_scalable_frame_length(hdr->bitrate)]);
    /// check the frame validity
    ASSERT((DEFAULT_SYNC_WORD == hdr->sync) || (R_SYNC_WORD == hdr->sync) || (R_INVALID_SYNC_WORD == hdr->sync),
           "%s invalid sync word:0x%x", __func__, hdr1->sync);
    ASSERT(BITRATE_MODE_NUM > hdr1->bitrate, "%s invalid bitrate:%d", __func__, hdr1->bitrate);
    ASSERT(hdr->bitrate == hdr1->bitrate, "%s bitrate not equal:%d|%d", __func__, hdr->bitrate, hdr1->bitrate);
    // TRACE(0, "QUEUED data header:");
    // DUMP8("0x%02x ", hdr, 3);
    // DUMP8("0x%02x ", hdr1, 3);

    /// get frame length
    uint16_t length = HEADER_SIZE + RECORD_DATA_PACKET_GATHER_LIMIT * voice_compression_get_scalable_frame_length(hdr->bitrate);

    return length;
}

uint8_t app_recording_scalable_update_bitrate_check(uint32_t cursor)
{
    uint32_t lock = int_lock_global();
    if (_ctl.bitrateUpdateFlag &&
        (0 == (cursor % RECORD_DATA_PACKET_GATHER_LIMIT)) &&
        cursor >= _ctl.expectedBitrateUpdateCursor)
    {
        TRACE(2, "bitrate update:%d->%d, cursor:%d", _ctl.currentBitrate, _ctl.futureBitrate, cursor);
        _ctl.bitrateUpdateFlag  = false;
        _ctl.lastBitrate        = _ctl.currentBitrate;
        _ctl.currentBitrate     = _ctl.futureBitrate;

        _ctl.bitrateUpdateRetryCnt  = 0;
    }
    uint8_t ret = _ctl.currentBitrate;
    int_unlock_global(lock);

    return ret;
}

#ifdef IBRT
static void _config_peer_scalable_bitrate(uint8_t br, uint32_t cursor)
{
    TRACE(2, "config peer scalable bitrate:%d, cursor:%d", br, cursor);
    SCALABLE_CONFIG_BITRATE_T info = {
        .bitrate = br,
        .cursor = cursor,
    };

    /// send bitrate update to peer
    tws_ctrl_send_cmd(APP_IBRT_CUSTOM_CMD_UPDATE_BITRATE, (uint8_t*)&info, sizeof(info));
}

static void _report_cache_buf_lvl(uint8_t lvl)
{
    TRACE(2, "report buffer level:%s", _buf_lvl_str[lvl]);
    uint8_t bufLvl = lvl;

    /// send bitrate update to peer
    tws_ctrl_send_cmd(APP_IBRT_CUSTOM_CMD_REPORT_BUF_LVL , &bufLvl, sizeof(bufLvl));
}

void app_recording_on_peer_buf_lvl_received(uint8_t lvl)
{
    TRACE(2, "Peer buffer lvl update:%s->%s", _buf_lvl_str[_ctl.peerCacheBufLvl], _buf_lvl_str[lvl]);

    uint32_t lock = int_lock_global();
    _ctl.peerCacheBufLvl = lvl;
    _scalable_bitrate_update_arbitor();
    int_unlock_global(lock);
}
#endif /// #ifdef IBRT

static uint32_t _get_bitrate_update_cursor(void)
{
    return (voice_compression_get_encode_cnt() + 10);
}

static void _scalable_bitrate_update_arbitor(void)
{
    uint8_t expectedBitrate = _buf_lvl_2_bitrate[_ctl.cacheBufLvl];
    uint32_t updateCursor = 0;
#ifdef IBRT
    if (app_tws_ibrt_tws_link_connected())
    {
        if (TWS_UI_MASTER == app_ibrt_if_get_ui_role())
        {
            expectedBitrate = (expectedBitrate >= _buf_lvl_2_bitrate[_ctl.peerCacheBufLvl])
                                  ? expectedBitrate
                                  : _buf_lvl_2_bitrate[_ctl.peerCacheBufLvl];
            /// update bitrate
            if (expectedBitrate != _ctl.currentBitrate)
            {
                updateCursor = _get_bitrate_update_cursor();
                app_recording_update_scalable_bitrate(expectedBitrate, updateCursor);
            }
        }
        else if (TWS_UI_SLAVE == app_ibrt_if_get_ui_role())
        {
            if ((expectedBitrate != _ctl.currentBitrate) &&
                (expectedBitrate != _ctl.expectedPeerBitrate))
            {
                _ctl.expectedPeerBitrate = expectedBitrate;
                _report_cache_buf_lvl(_ctl.cacheBufLvl);
            }
        }
        else
        {
            TRACE(0, "Warning: unknon role");
        }
    }
    else
#endif
    {
        /// update bitrate
        if (expectedBitrate != _ctl.currentBitrate)
        {
            app_recording_update_scalable_bitrate(expectedBitrate, updateCursor);
        }
    }
}

void app_recording_update_scalable_bitrate(uint8_t bitrate, uint32_t cursor)
{
    uint32_t lock = int_lock_global();
    if (bitrate != _ctl.currentBitrate)
    {
        _ctl.bitrateUpdateFlag  = true;
        _ctl.futureBitrate      = bitrate;
        _ctl.expectedBitrateUpdateCursor = cursor;

#ifdef IBRT
        if (app_tws_ibrt_tws_link_connected())
        {
            if (TWS_UI_MASTER == app_ibrt_if_get_ui_role())
            {
                _config_peer_scalable_bitrate(bitrate, cursor);
            }
            else if (TWS_UI_SLAVE == app_ibrt_if_get_ui_role())
            {
                TRACE(2, "Expected br:%d, incoming br:%d", _ctl.expectedPeerBitrate, bitrate);
                _ctl.expectedPeerBitrate = BITRATE_MODE_NUM;
            }
        }
#endif
    }
    int_unlock_global(lock);
}
#endif

void VoiceShareThread(void const *argument)
{
    while (1)
    {
        osEvent evt = osSignalWait(0x0, osWaitForever);
        int32_t signals = evt.value.signals;
        FRAME_LEN_GETTER_T frameLenGetter = NULL;
        int status = CQ_OK;

        /// handling corner case when recording is disabled
        if (0 == recording_enable)
        {
            continue;
        }

#ifdef RECORDING_USE_SCALABLE
        SCALABLE_FRAME_HEADER_T *hdr = NULL;
        frameLenGetter = (FRAME_LEN_GETTER_T)_cached_scalable_frame_len_getter;
#endif

        if (signals & SIGNAL_LOCAL_DATA_READY)
        {
            if (!app_tws_ibrt_tws_link_connected())
            {
                if (TWS_UI_MASTER == app_ibrt_if_get_ui_role())
                {
                    app_recording_disconnect();
                }
            }
            else if (voice_compression_get_encode_buf_size())
            {
                uint32_t length = ENCODED_DATA_SIZE;
#ifdef RECORDING_USE_SCALABLE
                uint32_t qAvailable = 0;
                voice_compression_peek_encoded_data((uint8_t *)recording_data.dataBuf, SCALABLE_HEADER_SIZE);
                hdr = (SCALABLE_FRAME_HEADER_T *)recording_data.dataBuf;
                ASSERT(DEFAULT_SYNC_WORD == hdr->sync, "INVALID sync word:0x%02x, please check", hdr->sync);
                length = RECORD_DATA_PACKET_GATHER_LIMIT * voice_compression_get_scalable_frame_length(hdr->bitrate);

                if (length > voice_compression_get_encode_buf_size())
                {
                    goto transmit_data_to_master;
                }
                else
                {
                    voice_compression_get_encoded_data(((uint8_t *)recording_data.dataBuf), length);
                }

                if (!app_ibrt_if_is_left_side())
                {
                    for (uint8_t i = 0; i < RECORD_DATA_PACKET_GATHER_LIMIT; i++)
                    {
                        hdr = (SCALABLE_FRAME_HEADER_T *)(recording_data.dataBuf + i * voice_compression_get_scalable_frame_length(hdr->bitrate));
                        hdr->sync = R_SYNC_WORD;
                    }
                }
#else
                voice_compression_get_encoded_data((uint8_t *)recording_data.dataBuf, length);
#endif
                recording_data.dataIndex = curCursor;
                TRACE(1, "len:%d, index:%d, local cached data:", length, recording_data.dataIndex);
                DUMP8("0x%02x ", recording_data.dataBuf, 3);
#ifdef RECORDING_USE_SCALABLE
                DUMP8("0x%02x ", recording_data.dataBuf + voice_compression_get_scalable_frame_length(hdr->bitrate), 3);
#endif

                if (curCursor == MAX_DATA_INDEX)
                {
                    curCursor = STARTING_DATA_INDEX;
                }
                curCursor++;

                LOCK_LOCAL_MEM_FIFO();
                TRACE(1, "local mem write offset:%d", GetCQueueWriteOffset(&(voice_mem_for_local.ShareDataQueue)));
                EnCQueue_AI(&(voice_mem_for_local.ShareDataQueue),
                            (CQItemType *)&recording_data, (HEADER_SIZE + length), frameLenGetter);
                TRACE(1, "local len:%d", LengthOfCQueue(&(voice_mem_for_local.ShareDataQueue)));
                UNLOCK_LOCAL_MEM_FIFO();

#ifdef RECORDING_USE_SCALABLE
                /// cache buffer analysis
                LOCK_LOCAL_MEM_FIFO();
                qAvailable = AvailableOfCQueue(&(voice_mem_for_local.ShareDataQueue));
                UNLOCK_LOCAL_MEM_FIFO();

                if (qAvailable * 4 < LOCAL_ENCODED_DATA_STORAGE_BUF_SIZE)
                {
                    _ctl.cacheBufLvl = CACHE_BUF_LVL_LESS_THAN_QUARTER_LEFT;
                }
                else if (qAvailable * 2 < LOCAL_ENCODED_DATA_STORAGE_BUF_SIZE)
                {
                    _ctl.cacheBufLvl = CACHE_BUF_LVL_MORE_THAN_QUARTER_LEFT;
                }
                else
                {
                    _ctl.cacheBufLvl = CACHE_BUF_LVL_MORE_THAN_HALF_LEFT;
                }
                /// arbitrate the bitrate
                _scalable_bitrate_update_arbitor();

transmit_data_to_master:
#endif
                if ((app_tws_ibrt_tws_link_connected()) &&
                    (TWS_UI_SLAVE == app_ibrt_if_get_ui_role()))
                {
                    /// slave send data to master
                    tws_ctrl_send_cmd(APP_IBRT_CUSTOM_CMD_DMA_AUDIO , 0, 0);
                }
            }
            else if (recording_enable == (ENABLE_TRANSFER | ENABLE_GET_ENCODED))
            {
                app_recording_update_status(ENABLE_TRANSFER);
            }
        }

        if (signals & SIGNAL_PEER_DATA_READY)
        {
            if ((app_tws_ibrt_tws_link_connected()) &&
                (TWS_UI_MASTER == app_ibrt_if_get_ui_role()))
            {
                if ((recording_enable == ENABLE_TRANSFER) &&
                    !LengthOfCQueue(&(voice_mem_for_slave.ShareDataQueue)) &&
                    !LengthOfCQueue(&(voice_mem_for_local.ShareDataQueue)))
                {
                    app_recording_stop_voice_stream();
                }

                if (reportSent == false)
                {
                    uint8_t delayReport[8];
                    delayReport[0] = 0x12;
                    delayReport[1] = 0x80;
                    delayReport[2] = 0x04;
                    delayReport[3] = 0x00;
                    memcpy(&delayReport[4], (uint8_t *)&delayRT, 0x04);
                    TRACE(2, "%s Delay report %x", __func__, delayRT);
                    if (app_recording_spp_send_data((uint8_t *)&delayReport, 0x08))
                    {
                        reportSent = true;
                    }
                    else
                    {
                        TRACE(1, "[Recording warning]%s: delay report sent failed spp send busy", __func__);
                    }
                }

                if ((curCursor - outCursor) > CUMULATE_PACKET_THRESHOLD &&
                    (revCursorPeer - outCursor) > CUMULATE_PACKET_THRESHOLD)
                {
                    unsigned char *e1 = NULL, *e2 = NULL;
                    unsigned int len1 = 0, len2 = 0;
                    uint8_t *pLocal = NULL, *pSlave = NULL, *pBase = NULL, *pLchnl = NULL, *pRchnl = NULL;
                    bool local_free = true, peer_free = true;
                    uint32_t localIndex = 0, peerIndex = 0;
                    uint32_t appFrameLen = FRAME_LENGTH_WITH_APP;
                    uint32_t twsFrameLen = FRAME_LENGTH_WITH_TWS;
                    uint32_t encodedSize = ENCODED_DATA_SIZE;
                    uint32_t twsFrameLenSlave = FRAME_LENGTH_WITH_TWS;
                    uint32_t sendLen = 0; //!< length of data to send
#ifdef RECORDING_USE_SCALABLE
                    uint32_t encodedFrameSize = FRAME_LENGTH_MAX;
                    uint32_t appFrameLenSlave = FRAME_LENGTH_WITH_APP;
                    uint8_t plcFlag = FRAME_BOTH_NORMAL;

                    LOCK_LOCAL_MEM_FIFO();
                    status = PeekCQueue(&(voice_mem_for_local.ShareDataQueue),
                                        (HEADER_SIZE + SCALABLE_HEADER_SIZE),
                                        &e1, &len1, &e2, &len2);
                    UNLOCK_LOCAL_MEM_FIFO();

                    if (status == CQ_OK)
                    {
                        memcpy((uint8_t *)&recording_data, e1, len1);
                        memcpy(((uint8_t *)&recording_data + len1), e2, len2);
                        TRACE(1, "local mem read offset:%d", GetCQueueReadOffset(&(voice_mem_for_local.ShareDataQueue)));
                        TRACE(0, "master cached data:");
                        DUMP8("0x%02x ", &recording_data, (HEADER_SIZE + SCALABLE_HEADER_SIZE));
                    }
                    else
                    {
                        TRACE(1, "error %d", __LINE__);
                    }

                    hdr = (SCALABLE_FRAME_HEADER_T *)recording_data.dataBuf;
                    TRACE(2, "index:%d, bitrate:%d", hdr->index, hdr->bitrate);
                    encodedFrameSize = voice_compression_get_scalable_frame_length(hdr->bitrate);
                    encodedSize = encodedFrameSize * RECORD_DATA_PACKET_GATHER_LIMIT;
                    twsFrameLen = encodedSize + HEADER_SIZE;
                    appFrameLen = 2 * twsFrameLen;
                    TRACE(1, "twsFrameLen:%d, appFrameLen:%d, masterIdx:%d", twsFrameLen, appFrameLen, hdr->index);

                    /// peer header analysis
                    LOCK_SLAVE_MEM_FIFO();
                    status = PeekCQueue(&(voice_mem_for_slave.ShareDataQueue),
                                        (HEADER_SIZE + SCALABLE_HEADER_SIZE),
                                        &e1, &len1, &e2, &len2);
                    UNLOCK_SLAVE_MEM_FIFO();

                    if (status == CQ_OK)
                    {
                        memcpy((uint8_t *)&recording_data, e1, len1);
                        memcpy(((uint8_t *)&recording_data + len1), e2, len2);
                        TRACE(0, "slave cached data:");
                        DUMP8("0x%02x ", &recording_data, (HEADER_SIZE + SCALABLE_HEADER_SIZE));
                    }
                    else
                    {
                        TRACE(1, "error %d", __LINE__);
                    }

                    hdr = (SCALABLE_FRAME_HEADER_T *)recording_data.dataBuf;
                    twsFrameLenSlave = voice_compression_get_scalable_frame_length(hdr->bitrate) * 2 + HEADER_SIZE;
                    appFrameLenSlave = 2 * twsFrameLenSlave;
                    TRACE(1, "twsFrameLenSlave:%d, appFrameLenSlave:%d, slaveIdx:%d", twsFrameLenSlave, appFrameLenSlave, hdr->index);
                    ASSERT((L_SYNC_WORD == hdr->sync) || (R_SYNC_WORD == hdr->sync) || (L_INVALID_SYNC_WORD == hdr->sync) || (R_INVALID_SYNC_WORD == hdr->sync),
                           "invalid sync word:0x%02x", hdr->sync);

                    if (twsFrameLen != twsFrameLenSlave)
                    {
                        TRACE(2, "WARNING: masterFrameLen:%d, slaveFrameLen:%d", twsFrameLen, twsFrameLenSlave);
                    }
#endif

                    for (uint8_t cumulated_packet = 0; cumulated_packet < CUMULATE_PACKET_THRESHOLD; cumulated_packet++)
                    {
                        /***********************************************************************************************/
                        /*DATA FORMAT***********************************************************************************/
                        /*Secondary DATA + PRIMARY DATA*****************************************************************/
                        /*DATA(Bitrate 32kbps):HEADER(4bytes)+ENCODED DATA(800bytes)+GARBAGE DATA(800bytes)*************/
                        /*DATA(Bitrate 64kbps):HEADER(4bytes)+ENCODED DATA(160bytes)+GARBAGE DATA(160bytes)*************/
                        /***********************************************************************************************/
                        pBase = (uint8_t *)&recording_data + cumulated_packet * appFrameLen;
                        pLchnl = pBase;
                        pRchnl = pBase + twsFrameLen;
#ifdef RECORDING_USE_OPUS
                        pRchnl += encodedSize;
#endif

                        /// pLocal/pSlave config
                        if (app_ibrt_if_is_left_side())
                        {
                            pLocal = pLchnl;
                            pSlave = pRchnl;
                        }
                        else
                        {
                            pLocal = pRchnl;
                            pSlave = pLchnl;
                        }

                        //Combine voice data and then send to APP by spp
                        LOCK_LOCAL_MEM_FIFO();
                        status = PeekCQueue(&(voice_mem_for_local.ShareDataQueue),
                                            twsFrameLen, &e1, &len1, &e2, &len2);
                        UNLOCK_LOCAL_MEM_FIFO();

                        if (status == CQ_OK)
                        {
                            memcpy(pLocal, e1, len1);
                            memcpy((pLocal + len1), e2, len2);
                            //DeCQueue(&(voice_mem_for_local.ShareDataQueue), 0, twsFrameLen);
                        }
                        else
                        {
                            TRACE(1, "[Recording warning]%s: local data not ready", __func__);
                            memset(pLocal, 0x00, twsFrameLen);
                        }

                        LOCK_SLAVE_MEM_FIFO();
                        status = PeekCQueue(&(voice_mem_for_slave.ShareDataQueue),
                                            twsFrameLenSlave, &e1, &len1, &e2, &len2);
                        UNLOCK_SLAVE_MEM_FIFO();

                        if (status == CQ_OK)
                        {
                            memcpy(pSlave, e1, len1);
                            memcpy((pSlave + len1), e2, len2);
#ifdef RECORDING_USE_SCALABLE
                            if (twsFrameLen != twsFrameLenSlave)
                            {
                                TRACE(0, "[Recording warning]: TWS bitrate mismatch");
                                _ctl.bitrateUpdateRetryCnt++;
                                if (BITRATE_UPDATE_RETRY == _ctl.bitrateUpdateRetryCnt)
                                {
                                    /// make sure bitrate update happens when cnt is even
                                    _config_peer_scalable_bitrate(_ctl.currentBitrate,
                                                                  _get_bitrate_update_cursor());
                                }
                                _ctl.bitrateUpdateRetryCnt %= BITRATE_UPDATE_RETRY;

                                /// force tws slave to use the 
                                for (uint8_t i = 0; i < RECORD_DATA_PACKET_GATHER_LIMIT; i++)
                                {
                                    memset((pSlave + HEADER_SIZE + i * encodedFrameSize + SCALABLE_HEADER_SIZE), 
                                            0, (encodedFrameSize - SCALABLE_HEADER_SIZE));
                                }
                            }
#endif
                        }
                        else
                        {
                            TRACE(1, "[Recording warning]%s: slave data not ready", __func__);
                            memset(pSlave, 0x00, twsFrameLenSlave);
                        }

                        localIndex = ((APP_RECORDING_DATA_T *)pLocal)->dataIndex;
                        peerIndex = ((APP_RECORDING_DATA_T *)pSlave)->dataIndex;

                        //data sanity check
                        if ((localIndex == outCursor) && (localIndex == peerIndex))
                        {
                            /// Both sides check pass
                        }
#ifdef RECORDING_USE_SCALABLE
                        else
                        {
                            TRACE(4, "[Recording warning]: index:%d %d Cursor: out->%d cur->%d encodeSize:%d",
                                  localIndex, peerIndex, outCursor, curCursor, encodedSize);

                            /// localIndex is greater than peerIndex
                            if (localIndex > peerIndex)
                            {
                                /// both index abnormal
                                if (localIndex != outCursor && peerIndex != outCursor)
                                {
                                    plcFlag = FRAME_BOTH_PLC;
                                }
                                /// peer index normal
                                else if (localIndex != outCursor && peerIndex == outCursor)
                                {
                                    if (app_ibrt_if_is_left_side())
                                    {
                                        plcFlag = FRAME_LEFT_PLC;
                                    }
                                    else
                                    {
                                        plcFlag = FRAME_RIGHT_PLC;
                                    }
                                }
                                else
                                {
                                    ASSERT(0, "CHECK ME1!");
                                }

                                /// compensate the local data with 0
                                for (uint8_t i = 0; i < RECORD_DATA_PACKET_GATHER_LIMIT; i++)
                                {
                                    hdr = (SCALABLE_FRAME_HEADER_T *)(pLocal + HEADER_SIZE + i * encodedFrameSize);
                                    if (app_ibrt_if_is_left_side())
                                    {
                                        hdr->sync = L_INVALID_SYNC_WORD;
                                    }
                                    else
                                    {
                                        hdr->sync = R_INVALID_SYNC_WORD;
                                    }
                                    memset(((uint8_t *)hdr + SCALABLE_HEADER_SIZE), 0, encodedFrameSize - SCALABLE_HEADER_SIZE);
                                }

                                local_free = false;
                                outCursor = peerIndex;
                            }
                            /// peerIndex is greater than localIndex
                            else if (localIndex < peerIndex)
                            {
                                /// both index abnormal
                                if (localIndex != outCursor && peerIndex != outCursor)
                                {
                                    plcFlag = FRAME_BOTH_PLC;
                                }
                                /// local index normal
                                else if (localIndex == outCursor && peerIndex != outCursor)
                                {
                                    if (app_ibrt_if_is_left_side())
                                    {
                                        plcFlag = FRAME_RIGHT_PLC;
                                    }
                                    else
                                    {
                                        plcFlag = FRAME_LEFT_PLC;
                                    }
                                }
                                else
                                {
                                    ASSERT(0, "CHECK ME2!");
                                }

                                /// compensate the peer data with 0
                                for (uint8_t i = 0; i < RECORD_DATA_PACKET_GATHER_LIMIT; i++)
                                {
                                    hdr = (SCALABLE_FRAME_HEADER_T *)(pSlave + HEADER_SIZE + i * encodedFrameSize);
                                    if (app_ibrt_if_is_left_side())
                                    {
                                        hdr->sync = R_INVALID_SYNC_WORD;
                                    }
                                    else
                                    {
                                        hdr->sync = L_INVALID_SYNC_WORD;
                                    }
                                    memset(((uint8_t *)hdr + SCALABLE_HEADER_SIZE), 0, encodedFrameSize - SCALABLE_HEADER_SIZE);
                                }

                                peer_free = false;
                                outCursor = localIndex;
                            }
                            /// both side not equal to outCursor
                            else
                            {
                                plcFlag = FRAME_BOTH_PLC;
                                outCursor = localIndex;
                            }
                        }

                        *pLchnl = plcFlag;
                        *pRchnl = plcFlag;
                        memmove(pRchnl + PLC_FLAG_SIZE, pRchnl + HEADER_SIZE, encodedSize);
                        memmove(pLchnl + PLC_FLAG_SIZE, pLchnl + HEADER_SIZE, 2 * encodedSize + PLC_FLAG_SIZE);
                        /// structure: plc_flag + L chnl + L chnl + plc_flag + R chnl + R chnl => plc_flag + L chnl + R chnl + plc_flag + L chnl + R chnl
                        memcpy(scalableTempBUf, pLchnl + PLC_FLAG_SIZE + encodedFrameSize, encodedFrameSize);
                        memcpy(pLchnl + PLC_FLAG_SIZE + encodedFrameSize, pLchnl + 2 * PLC_FLAG_SIZE + encodedSize, encodedFrameSize);
                        memcpy(pLchnl + 2 * PLC_FLAG_SIZE + encodedSize, scalableTempBUf, encodedFrameSize);

                        sendLen = appFrameLen * CUMULATE_PACKET_THRESHOLD -
                                  RECORD_DATA_PACKET_GATHER_LIMIT * HEADER_SIZE +
                                  RECORD_DATA_PACKET_GATHER_LIMIT * PLC_FLAG_SIZE;
                        TRACE(8, "%s: index:%d %d Cursor:out->%d cur->%d, sendLen:%d, plc:%d|%d",
                              __func__, localIndex, peerIndex, outCursor, curCursor, sendLen, *pLchnl, *(pLchnl + encodedSize + PLC_FLAG_SIZE));
#else
                        else if ((localIndex == peerIndex) && (localIndex != outCursor))
                        {
                            //Both sides check failure.But local and peer is same, it means send to phone blocked too long,just skip
                            TRACE(4, "[Recording warning]-1: index:%d %d Cursor: out->%d cur->%d",
                                  localIndex, peerIndex, outCursor, curCursor);
                            outCursor = localIndex;
                            memcpy(pLocal + HEADER_SIZE, encoded_zero, encoded_zero_size);
                            memset(pLocal + encoded_zero_size + HEADER_SIZE, 0x00, encodedSize - encoded_zero_size);
                            memcpy(pSlave + HEADER_SIZE, encoded_zero, encoded_zero_size);
                            memset(pSlave + encoded_zero_size + HEADER_SIZE, 0x00, encodedSize - encoded_zero_size);
                        }
                        else if ((localIndex != outCursor) && (peerIndex != outCursor))
                        {
                            //Both sides check failure.Don't free queue
                            TRACE(4, "[Recording warning]-2: index:%d %d Cursor: out->%d cur->%d",
                                  localIndex, peerIndex, outCursor, curCursor);
                            local_free = false;
                            peer_free = false;
                            memcpy(pLocal + HEADER_SIZE, encoded_zero, encoded_zero_size);
                            memset(pLocal+ encoded_zero_size + HEADER_SIZE, 0x00, encodedSize - encoded_zero_size);
                            memcpy(pSlave + HEADER_SIZE, encoded_zero, encoded_zero_size);
                            memset(pSlave+ encoded_zero_size + HEADER_SIZE, 0x00, encodedSize - encoded_zero_size);
                        }
                        else if (localIndex != outCursor)
                        {
                            //Local check failure
                            TRACE(4, "[Recording warning]-3: index:%d %d Cursor: out->%d cur->%d",
                                  localIndex, peerIndex, outCursor, curCursor);
                            local_free = false;
                            memcpy(pLocal + HEADER_SIZE, encoded_zero, encoded_zero_size);
                            memset(pLocal+ encoded_zero_size + HEADER_SIZE, 0x00, encodedSize - encoded_zero_size);
                        }
                        else if (peerIndex != outCursor)
                        {
                            //Peer check failure
                            TRACE(4, "[Recording warning]-4: index:%d %d Cursor: out->%d cur->%d",
                                  localIndex, peerIndex, outCursor, curCursor);
                            peer_free = false;
                            memcpy(pSlave + HEADER_SIZE, encoded_zero, encoded_zero_size);
                            memset(pSlave + encoded_zero_size + HEADER_SIZE, 0x00, encodedSize - encoded_zero_size);
                        }
                        else
                        {
                            TRACE(4, "[Recording warning]-5: something wrong, index:%d %d Cursor: out->%d cur->%d",
                                  localIndex, peerIndex, outCursor, curCursor);
                            local_free = false;
                            peer_free = false;
                            memcpy(pLocal + HEADER_SIZE, encoded_zero, encoded_zero_size);
                            memset(pLocal + encoded_zero_size + HEADER_SIZE, 0x00, encodedSize - encoded_zero_size);
                            memcpy(pSlave + HEADER_SIZE, encoded_zero, encoded_zero_size);
                            memset(pSlave + encoded_zero_size + HEADER_SIZE, 0x00, encodedSize - encoded_zero_size);
                        }

                        //simulate 128kbps
                        memset(pBase + twsFrameLen, 0x5A, encodedSize);
                        memset(pBase + twsFrameLen + encodedSize + twsFrameLen, 0x5A, encodedSize);

                        sendLen = appFrameLen * CUMULATE_PACKET_THRESHOLD;
                        TRACE(5, "%s: index:%d %d Cursor: out->%d cur->%d",
                              __func__, localIndex, peerIndex, outCursor, curCursor);

#ifdef RECORD_DUMP_DATA
                        data_dump_run("LC_dump", (pBase + HEADER_SIZE), encodedSize);
                        data_dump_run("RC_dump", (pBase + twsFrameLen + twsFrameLen), encodedSize);
#endif
#endif

                        if ((cumulated_packet == CUMULATE_PACKET_THRESHOLD - 1) &&
                            app_recording_spp_send_data((uint8_t *)&recording_data, sendLen))
                        {
                            if (outCursor == MAX_DATA_INDEX)
                            {
                                outCursor = STARTING_DATA_INDEX;
                            }
                            outCursor++;
                            if (local_free)
                            {
                                LOCK_LOCAL_MEM_FIFO();
                                DeCQueue(&(voice_mem_for_local.ShareDataQueue), 0, twsFrameLen);
                                UNLOCK_LOCAL_MEM_FIFO();
                            }
                            if (peer_free)
                            {
                                LOCK_SLAVE_MEM_FIFO();
                                DeCQueue(&(voice_mem_for_slave.ShareDataQueue), 0, twsFrameLenSlave);
                                UNLOCK_SLAVE_MEM_FIFO();
                            }
                        }
                        else
                        {
#ifdef RECORDING_USE_OPUS_LOW_BANDWIDTH
                            if (outCursor == MAX_DATA_INDEX)
                            {
                                outCursor = STARTING_DATA_INDEX;
                            }
                            outCursor++;
                            if (cumulated_packet == CUMULATE_PACKET_THRESHOLD - 1)
                            {
                                TRACE(1, "[Recording warning]%s: spp send busy", __func__);
                            }
                            if (local_free)
                            {
                                LOCK_LOCAL_MEM_FIFO();
                                DeCQueue(&(voice_mem_for_local.ShareDataQueue), 0, twsFrameLen);
                                UNLOCK_LOCAL_MEM_FIFO();
                            }
                            if (peer_free)
                            {
                                LOCK_SLAVE_MEM_FIFO();
                                DeCQueue(&(voice_mem_for_slave.ShareDataQueue), 0, twsFrameLen);
                                UNLOCK_SLAVE_MEM_FIFO();
                            }
#else
                            TRACE(1, "[Recording warning]%s: spp send busy", __func__);
#endif
                        }
                    }
                }
                else
                {
                    TRACE(2, "curCursor:%d,revCursorPeer:%d,outCursor:%d", curCursor, revCursorPeer, outCursor);
                }
            }
        }
    }
}

void _recording_init(void)
{
    // shall be replaced with variable according to different bit rate
    voice_compression_reset_encode_buf();
    curCursor = outCursor = revCursorPeer = STARTING_DATA_INDEX;

#ifdef VOC_ENCODE_SCALABLE
    memset(&_ctl, 0, sizeof(_ctl));
    _ctl.currentBitrate = BITRATE_MODE_1;
#endif

#ifdef IBRT
    if (NULL == voiceTid)
    {
        voiceTid = osThreadCreate(osThread(VoiceShareThread), NULL);
    }
#endif
}

void app_recording_store_peer_data(void *param1, uint32_t param2)
{
    if (param2 > FRAME_LENGTH_WITH_TWS)
    {
        TRACE(1, "[Recording warning]%s: data length not correct %d", __func__, param2);
        return;
    }

    if (0 == recording_enable)
    {
        TRACE(0, "[Recording warning]: data received when recording is disabled");
        return;
    }

    // TRACE(0, "peer data received:");
    // DUMP8("0x%02x ", (uint8_t *)param1, 7);
    FRAME_LEN_GETTER_T frameLenGetter = NULL;
#ifdef RECORDING_USE_SCALABLE
    frameLenGetter = (FRAME_LEN_GETTER_T)_cached_scalable_frame_len_getter;
#endif

    LOCK_SLAVE_MEM_FIFO();
    EnCQueue_AI(&(voice_mem_for_slave.ShareDataQueue),
                (CQItemType *)param1, param2, frameLenGetter);
    UNLOCK_SLAVE_MEM_FIFO();

    if (revCursorPeer == MAX_DATA_INDEX)
    {
        revCursorPeer = STARTING_DATA_INDEX;
    }

    if ((revCursorPeer % CUMULATE_PACKET_THRESHOLD) == 0)
    {
        osSignalSet(voiceTid, SIGNAL_PEER_DATA_READY);
    }
    revCursorPeer++;
    return;
}

APP_RECORDING_DATA_T temp_recording_data;
void app_recording_send_data_to_master(void)
{
    unsigned char *e1 = NULL, *e2 = NULL;
    unsigned int len1 = 0, len2 = 0;
    int status = CQ_OK;

    uint32_t twsFrameLen = FRAME_LENGTH_WITH_TWS;

    LOCK_LOCAL_MEM_FIFO();
#ifdef RECORDING_USE_SCALABLE
    status = PeekCQueue(&(voice_mem_for_local.ShareDataQueue),
                        (HEADER_SIZE + SCALABLE_HEADER_SIZE), &e1, &len1, &e2, &len2);
    if (status == CQ_OK)
    {
        memcpy((uint8_t *)&temp_recording_data, e1, len1);
        memcpy(((uint8_t *)&temp_recording_data + len1), e2, len2);
        TRACE(0, "%s data:", __func__);
        DUMP8("0x%02x ", (uint8_t *)&temp_recording_data, 7);
    }
    else
    {
        TRACE(0, "error");
    }

    SCALABLE_FRAME_HEADER_T* hdr = (SCALABLE_FRAME_HEADER_T *)temp_recording_data.dataBuf;
    uint32_t encodedFrameSize = voice_compression_get_scalable_frame_length(hdr->bitrate);

    twsFrameLen = encodedFrameSize * 2 + HEADER_SIZE;
    TRACE(1, "twsFrameLen:%d", twsFrameLen);
#endif

    status = PeekCQueue(&(voice_mem_for_local.ShareDataQueue),
                        twsFrameLen, &e1, &len1, &e2, &len2);

    if (status == CQ_OK)
    {
        memcpy((uint8_t *)&temp_recording_data, e1, len1);
        memcpy(((uint8_t *)&temp_recording_data + len1), e2, len2);
        // TRACE(0, "slave cached data:");
        // DUMP8("0x%02x ", &temp_recording_data, (HEADER_SIZE + SCALABLE_HEADER_SIZE));
        // DUMP8("0x%02x ", temp_recording_data.dataBuf + encodedFrameSize, SCALABLE_HEADER_SIZE);
    }
    else
    {
        TRACE(1, "[Recording warning]%s: local data not ready", __func__);
#ifdef RECORDING_USE_SCALABLE
        for (uint8_t i = 0; i < RECORD_DATA_PACKET_GATHER_LIMIT; i++)
        {
            hdr = (SCALABLE_FRAME_HEADER_T *)(temp_recording_data.dataBuf + i * encodedFrameSize);
            if (app_ibrt_if_is_left_side())
            {
                hdr->sync = L_INVALID_SYNC_WORD;
            }
            else
            {
                hdr->sync = R_INVALID_SYNC_WORD;
            }
            memset(((uint8_t *)hdr + SCALABLE_HEADER_SIZE), 0, encodedFrameSize - SCALABLE_HEADER_SIZE);
        }
#else
        memcpy((uint8_t *)&temp_recording_data, encoded_zero, encoded_zero_size);
        memset((uint8_t *)&temp_recording_data + encoded_zero_size, 0x00, twsFrameLen - encoded_zero_size);
#endif
    }

    /// local data sanity check
    if (temp_recording_data.dataIndex == outCursor)
    {
        /// check pass
    }
    else if ((temp_recording_data.dataIndex > outCursor) && (temp_recording_data.dataIndex - outCursor) % LOCAL_RESERVER_BUFFER_NUM == 0)
    {
        /// local data over n round,for slave nothing need to do
    }
    else
    {
        TRACE(1, "[Recording warning]%s data wrong,please check %d %d", __func__, outCursor, temp_recording_data.dataIndex);
        outCursor = temp_recording_data.dataIndex;
    }

    temp_recording_data.dataIndex = outCursor; //replace with outCursor for primary do the sanity check

    if (app_ibrt_send_cmd_without_rsp(APP_IBRT_CUSTOM_CMD_DMA_AUDIO, (uint8_t *)&temp_recording_data, twsFrameLen))
    {
        //TWS data send out success, so dequeue data and increase cursor
        if (outCursor == MAX_DATA_INDEX)
        {
            outCursor = STARTING_DATA_INDEX;
        }
        outCursor++;
        DeCQueue(&(voice_mem_for_local.ShareDataQueue), 0, twsFrameLen);
    }
    else
    {
        TRACE(1, "[Recording warning]%s busy", __func__);
    }

    UNLOCK_LOCAL_MEM_FIFO();

    return;
}

#define AI_VOICE_SYNC_CAPTURE_TUNING_DEBUG

static int32_t ai_voice_mobile_master_clk_init_val;

static uint32_t ai_voice_bt_clk_cnt_per_frame;
static uint32_t ai_voice_us_per_frame;
static bool ai_voice_is_capture_triggered = false;

static void app_ai_voice_update_initial_mobile_master_clk(int32_t clkVal)
{
    ai_voice_mobile_master_clk_init_val = clkVal+ai_voice_bt_clk_cnt_per_frame;
#ifdef AI_VOICE_SYNC_CAPTURE_TUNING_DEBUG
    TRACE(0, "ai voice tune init bt clk %d", ai_voice_mobile_master_clk_init_val);
#endif
}

static void app_ai_voice_update_frame_length_in_ms(int32_t value)
{
    ai_voice_bt_clk_cnt_per_frame = (uint32_t)((float)(2*value)/0.625);
    ai_voice_us_per_frame = value*1000;

#ifdef AI_VOICE_SYNC_CAPTURE_TUNING_DEBUG
    TRACE(0, "ai voice tune btclk per f %d us %d",
          ai_voice_bt_clk_cnt_per_frame,
          ai_voice_us_per_frame);
#endif
}

static void app_ai_voice_update_capture_triggered_flag(bool isTriggered)
{
    ai_voice_is_capture_triggered = isTriggered;
}

#if defined(IBRT)
extern void app_tws_ibrt_audio_mobile_clkcnt_get(uint8_t device_id, uint32_t btclk, uint16_t btcnt,
                                                 uint32_t *tws_master_clk, uint16_t *tws_master_cnt);

void app_ai_voice_mobile_clkcnt_get(uint32_t btclk, uint16_t btcnt,
                                    uint32_t *tws_master_clk,
                                    uint16_t *tws_master_cnt)
{
    uint8_t curr_recording_device = app_ai_get_device_id_from_index(AI_SPEC_RECORDING);
    app_tws_ibrt_audio_mobile_clkcnt_get(curr_recording_device, btclk, btcnt, tws_master_clk, tws_master_cnt);
}

void app_ibrt_tws_master_clkcnt_get(uint16_t tws_conhdl, uint32_t btclk, uint16_t btcnt,
                                    uint32_t *tws_master_clk, uint16_t *tws_master_cnt)
{
    int32_t clock_offset;
    uint16_t bit_offset;

    if (app_tws_is_connected())
    {
        if (tws_conhdl != INVALID_HANDLE)
        {
            bt_drv_reg_op_piconet_clk_offset_get(tws_conhdl, &clock_offset, &bit_offset);
            btdrv_slave2master_clkcnt_convert(btclk, btcnt,
                                              clock_offset, bit_offset,
                                              tws_master_clk, tws_master_cnt);
#ifdef AI_VOICE_SYNC_CAPTURE_TUNING_DEBUG
            TRACE(4, "tws master piconet clk:%d bit:%d tws master clk:%x cnt:%d",
                  clock_offset, bit_offset, *tws_master_clk, *tws_master_cnt);
#endif
        }
        else
        {
            TRACE(3, "%s warning conhdl NULL conhdl:%x", __func__, tws_conhdl);
            *tws_master_clk = 0;
            *tws_master_cnt = 0;
        }
    }
}

static void app_ai_voice_capture_sync_tuning(void)
{
    uint32_t btclk;
    uint16_t btcnt;

    uint32_t tws_master_clk;
    uint16_t tws_master_cnt;

    uint32_t tws_master_clk_offset;
    int32_t tws_master_cnt_offset;

    static float fre_offset = 0.0f;

    static float fre_offset_long_time = 0.0f;
    static int32_t fre_offset_flag = 0;

    static int32_t tws_master_cnt_offset_former;
    
    static bool isFirstlyCalibrated = true;
    
    uint8_t adma_ch = af_stream_get_dma_chan(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
    if(adma_ch != HAL_DMA_CHAN_NONE)
    {
        ibrt_ctrl_t *p_ibrt_ctrl = app_ibrt_if_get_bt_ctrl_ctx();
        bt_drv_reg_op_dma_tc_clkcnt_get_by_ch(&btclk, &btcnt, adma_ch & 0xF);
        app_ibrt_tws_master_clkcnt_get(p_ibrt_ctrl->tws_conhandle, btclk, btcnt,
                                       &tws_master_clk, &tws_master_cnt);
    }

#ifdef AI_VOICE_SYNC_CAPTURE_TUNING_DEBUG
    TRACE(0, "ai voice tune btclk:%d,btcnt:%d - %d, %d", 
    btclk, btcnt, tws_master_clk, tws_master_cnt);
#endif

    tws_master_clk_offset = (tws_master_clk - ai_voice_mobile_master_clk_init_val)%ai_voice_bt_clk_cnt_per_frame;
    tws_master_cnt_offset = (int32_t)((float)(tws_master_clk_offset*312.5) + (312.5 - (float)tws_master_cnt));

#ifdef AI_VOICE_SYNC_CAPTURE_TUNING_DEBUG
    TRACE(0, "ai voice tune tws_master_cnt_offset:%d,", tws_master_cnt_offset);
#endif

    if (!ai_voice_is_capture_triggered)
    {    
        app_ai_voice_update_capture_triggered_flag(true);
        isFirstlyCalibrated = true;

        fre_offset = 0.0f;

        fre_offset_long_time = 0.0f;
        fre_offset_flag = 0;
    }

    {
        if (tws_master_cnt_offset > 0)
        {
            fre_offset = 0.00005f + fre_offset_long_time;

            isFirstlyCalibrated = true;
            fre_offset_flag = 1;
            if (tws_master_cnt_offset >= tws_master_cnt_offset_former)
            {
                fre_offset_long_time = fre_offset_long_time + 0.0000001f;
                fre_offset_flag = 0;
            }
        }
        else if (tws_master_cnt_offset < 0)
        {
            fre_offset = -0.00005f + fre_offset_long_time;

            isFirstlyCalibrated = true;
            fre_offset_flag = -1;
            if (tws_master_cnt_offset <= tws_master_cnt_offset_former)
            {
                fre_offset_long_time = fre_offset_long_time - 0.0000001f;
                fre_offset_flag = 0;
            }
        }
        else
        {
            fre_offset = fre_offset_long_time + fre_offset_flag*0.0000001f;
            fre_offset_long_time = fre_offset;
            isFirstlyCalibrated = false;
            fre_offset_flag = 0;
        }

        if (fre_offset_long_time > 0.0001f) 
        {
            fre_offset_long_time = 0.0001f;
        }
        
        if (fre_offset_long_time < -0.0001f) 
        {
            fre_offset_long_time = -0.0001f;
        }

        tws_master_cnt_offset_former = tws_master_cnt_offset;
        
#ifdef AI_VOICE_SYNC_CAPTURE_TUNING_DEBUG
        {
            int freq_ppm_int = (int)(fre_offset_long_time*1000000.0f);
            int freq_ppm_Fra = (int)(fre_offset_long_time*10000000.0f)-
                ((int)(fre_offset_long_time*1000000.0f))*10;
            if (freq_ppm_Fra < 0) 
            {
                freq_ppm_Fra = -freq_ppm_Fra;
            }

            TRACE(1, "freq_offset_long_time(ppm):%d.%d",freq_ppm_int,freq_ppm_Fra);
        }
#endif
    }

#ifdef AI_VOICE_SYNC_CAPTURE_TUNING_DEBUG
    {
        POSSIBLY_UNUSED int freq_ppm_int=(int)(fre_offset*1000000.0f);
        int freq_ppm_Fra = (int)(fre_offset*10000000.0f)-((int)(fre_offset*1000000.0f))*10;
        if(freq_ppm_Fra<0) freq_ppm_Fra = -freq_ppm_Fra;
        TRACE(0, "ai voice time_offset(us):%d",tws_master_cnt_offset);
        TRACE(0, "ai voice freq_offset(ppm):%d.%d",freq_ppm_int,freq_ppm_Fra);
    }
#endif

    if(!isFirstlyCalibrated)
    {
        if(fre_offset>0.0001f)fre_offset=0.0001f;
        if(fre_offset<-0.0001f)fre_offset=-0.0001f;
    }

#if defined(__AUDIO_RESAMPLE__) && defined(SW_PLAYBACK_RESAMPLE)
    app_resample_tune(playback_samplerate_codecpcm, fre_offset);
#else
    af_codec_tune(AUD_STREAM_NUM, fre_offset);
#endif

    return;
}

static void _trigger_timeout_cb(void const *n)
{
    TRACE(0, "[TRIG][TIMEOUT]");
    // TODO: restart voice capture
}

static int _trigger_checker_init(void)
{
    if (_trigger_timeout_id == NULL)
    {
        _trigger_timeout_id =
            osTimerCreate(osTimer(APP_IBRT_VOICE_CAPTURE_TRIGGER_TIMEOUT), osTimerOnce, NULL);
    }

    return 0;
}

static int _trigger_checker_start(void)
{
    _trigger_enable = true;
    osTimerStart(_trigger_timeout_id, APP_VOICE_CAPTURE_SYNC_TRIGGER_TIMEROUT);

    return 0;
}

static int app_ibrt_voice_capture_trigger_checker_stop(void)
{
    _trigger_enable = false;
    osTimerStop(_trigger_timeout_id);

    return 0;
}

static int _trigger_check(void)
{
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

    if (p_ibrt_ctrl->init_done)
    {
        if (_trigger_enable)
        {
            TRACE(0, "[TRIG][OK]");
            _trigger_enable = false;
            osTimerStop(_trigger_timeout_id);
        }
    }
    else
    {
        TRACE(1, "%s ibrt not init done!", __func__);
    }

    return 0;
}

static void _set_trigger_time(uint32_t tg_tick)
{
    if (tg_tick)
    {
        ibrt_ctrl_t *p_ibrt_ctrl = app_ibrt_if_get_bt_ctrl_ctx();
        btif_connection_role_t connection_role = app_tws_ibrt_get_local_tws_role();

        btdrv_syn_trigger_codec_en(0);
        btdrv_syn_clr_trigger(0);
        btdrv_enable_playback_triggler(ACL_TRIGGLE_MODE);

        if (connection_role == BTIF_BCR_MASTER)
        {
            bt_syn_set_tg_ticks(tg_tick, p_ibrt_ctrl->tws_conhandle, BT_TRIG_MASTER_ROLE,0,false);
        }
        else if (connection_role == BTIF_BCR_SLAVE)
        {
            bt_syn_set_tg_ticks(tg_tick, p_ibrt_ctrl->tws_conhandle, BT_TRIG_SLAVE_ROLE,0,false);
        }

        af_stream_dma_tc_irq_enable(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        uint8_t adma_ch = af_stream_get_dma_chan(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        if(adma_ch != HAL_DMA_CHAN_NONE)
        {
            bt_drv_reg_op_enable_dma_tc(adma_ch&0xF);
        }

        btdrv_syn_trigger_codec_en(1);
        af_codec_sync_config(AUD_STREAM_CAPTURE, AF_CODEC_SYNC_TYPE_BT, true);
        app_ai_voice_update_initial_mobile_master_clk(tg_tick);
        _trigger_checker_start();
        TRACE(1, "[TRIG] set trigger tg_tick:%08x", tg_tick);
    }
    else
    {
        btdrv_syn_trigger_codec_en(0);
        btdrv_syn_clr_trigger(0);
        app_ibrt_voice_capture_trigger_checker_stop();
        TRACE(0, "[TRIG] trigger clear");
    }
}

static int _trigger_init(void)
{
    int ret = 0;
    ibrt_ctrl_t *p_ibrt_ctrl = app_ibrt_if_get_bt_ctrl_ctx();
    TWS_UI_ROLE_E currentRole = app_ibrt_if_get_ui_role();

    uint32_t curr_ticks = bt_syn_get_curr_ticks(p_ibrt_ctrl->tws_conhandle);
    uint32_t tg_tick = 0;

    if (TWS_UI_MASTER == currentRole)
    {
        tg_tick = curr_ticks + US_TO_BTCLKS(APP_CAPTURE_AUDIO_SYNC_DELAY_US);

        af_codec_sync_config(AUD_STREAM_CAPTURE, AF_CODEC_SYNC_TYPE_BT, false);
        af_codec_sync_config(AUD_STREAM_CAPTURE, AF_CODEC_SYNC_TYPE_BT, true);
        _set_trigger_time(tg_tick);

        if (p_ibrt_ctrl->tws_mode == IBRT_SNIFF_MODE)
        {
            TRACE(0, "[TRIG][INIT][EXIT_SNIFF] force_exit_with_tws");
            app_tws_ibrt_exit_sniff_with_tws();
        }

        /// inform peer to open mic
        ai_cmd_instance_t open_mic_cmd;
        open_mic_cmd.cmdcode = AI_CMD_DUAL_MIC_OPEN;
        open_mic_cmd.param_length = 4;
        open_mic_cmd.param = (uint8_t *)&tg_tick;
        app_ai_send_cmd_to_peer(&open_mic_cmd);

        TRACE(2, "[TRIG][INIT] MASTER trigger [%08x]-->[%08x]", bt_syn_get_curr_ticks(p_ibrt_ctrl->tws_conhandle), tg_tick);
    }
    else if (TWS_UI_SLAVE == currentRole)
    {
        tg_tick = _trigger_tg_tick;
        if (curr_ticks < tg_tick && tg_tick != 0)
        {
            af_codec_sync_config(AUD_STREAM_CAPTURE, AF_CODEC_SYNC_TYPE_BT, false);
            af_codec_sync_config(AUD_STREAM_CAPTURE, AF_CODEC_SYNC_TYPE_BT, true);
            _set_trigger_time(tg_tick);
        }
        _trigger_tg_tick = 0;
        if (tg_tick <= curr_ticks)
        {
            TRACE(0, "[TRIG][INIT] Sync Capture TRIGGER ERROR!!!");
            ret = -1;
        }

        TRACE(2, "[TRIG][INIT] SLAVE trigger [%08x]-->[%08x]", bt_syn_get_curr_ticks(p_ibrt_ctrl->tws_conhandle), tg_tick);
    }

    return ret;
}

static int _trigger_deinit(void)
{
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

    if (p_ibrt_ctrl->init_done)
    {
        af_codec_sync_config(AUD_STREAM_CAPTURE, AF_CODEC_SYNC_TYPE_BT, false);
        _set_trigger_time(0);
    }
    else
    {
        TRACE(0, "ibrt not init done!");
    }

    return 0;
}

static void _trigger_ticks_update(uint32_t triggerTicks)
{
    TRACE(2, "trigger ticks update:%d->%d", _trigger_tg_tick, triggerTicks);
    _trigger_tg_tick = triggerTicks;
}

void app_recording_start_capture_stream_on_specific_ticks(uint32_t triggerTicks)
{
    _trigger_ticks_update(triggerTicks);
    app_recording_start_voice_stream();
}

static int _trigger_setup_before_capture_stream_started(void)
{
    TRACE(0, "setup trigger.");
    int ret = 0;
    struct AF_STREAM_CONFIG_T *cfg = (struct AF_STREAM_CONFIG_T*)app_ai_voice_get_stream_config();

    app_ai_voice_update_capture_triggered_flag(false);
    app_ai_voice_update_frame_length_in_ms(
        (1000*cfg->data_size/2)/(cfg->channel_num*cfg->sample_rate*cfg->bits/8));

    /// start trigger setup
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    if (p_ibrt_ctrl->init_done)
    {
        _trigger_checker_init();
        if (app_tws_ibrt_tws_link_connected())
        {
            /// clear the trigger configurations
            _trigger_deinit();

            /// init trigger
            ret = _trigger_init();
        }
        else
        {
            TRACE(0, "[TRIG] fail: tws not connected.");
        }
    }
    else
    {
        TRACE(1, "%s ibrt not init done.", __func__);
    }

    return ret;
}
#endif

static int _config_right_before_capture_stream_started(void)
{
    int ret = -1;

    /// trigger config start
    struct AF_STREAM_CONFIG_T *cfg = (struct AF_STREAM_CONFIG_T*)app_ai_voice_get_stream_config();
    app_ai_voice_update_capture_triggered_flag(false);
    app_ai_voice_update_frame_length_in_ms(
        (1000*cfg->data_size/2)/(cfg->channel_num*cfg->sample_rate*cfg->bits/8));

    /// start trigger setup
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    if (p_ibrt_ctrl->init_done)
    {
        _trigger_checker_init();
        if (app_tws_ibrt_tws_link_connected())
        {
            /// clear the trigger configurations
            _trigger_deinit();

            /// init trigger
            ret = _trigger_init();
        }
        else
        {
            TRACE(0, "[TRIG] fail: tws not connected.");
        }
    }
    else
    {
        TRACE(1, "%s ibrt not init done.", __func__);
    }
    /// trigger config stop

    if (0 == ret)
    {
        gather_count = 0;
        app_ai_set_stream_opened(true, AI_SPEC_RECORDING);
        _share_resource_init();
        app_ai_voice_stream_init(AIV_USER_REC);
        app_ai_voice_update_handle_frame_len(RECORD_ONESHOT_PROCESS_LEN);
        app_ai_voice_upstream_control(true, AIV_USER_REC, 0);
    }

    return ret;
}

static void _config_right_before_capture_stream_stopped(void)
{
    TRACE(0, "Recording handle on capture stream stopped");
    _trigger_deinit();

    _share_resource_deinit();

    /// restore capture stream close by recording
    app_ai_voice_restore_capture_stream_after_super_user_stream_closed();

    app_ibrt_if_prevent_sniff_clear(mobileAddr, AI_VOICE_RECORD);
    app_sysfreq_req(APP_SYSFREQ_USER_AI_VOICE, APP_SYSFREQ_32K);
}

static void _functionality_init(void)
{
    TRACE(1, "%s", __func__);
    app_ai_voice_set_slave_open_mic_flag(AIV_USER_REC, true);
}

void app_recording_start_voice_stream(void)
{
    TRACE(2, "%s", __func__);

    if (recording_enable)
    {
        app_ai_voice_register_trigger_init_handler((TRIGGER_INIT_T)_trigger_setup_before_capture_stream_started);

#ifdef IBRT
        app_ibrt_if_prevent_sniff_set(mobileAddr, AI_VOICE_RECORD);

        /// guarantee performance->decrease the communication interval of TWS connection
        ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
        app_tws_ibrt_request_modify_tws_bandwidth(TWS_TIMING_CONTROL_USER_AI_VOICE, true);

        if (app_tws_ibrt_tws_link_connected() &&
            (TWS_UI_MASTER == app_ibrt_if_get_ui_role()) &&
            p_ibrt_ctrl->p_tws_remote_dev)
        {
#if defined(IBRT_UI_V1)
            app_ibrt_if_exit_sniff_with_mobile();
#else
            app_ibrt_if_exit_sniff_with_mobile(mobileAddr);
#endif
            btif_me_stop_sniff(p_ibrt_ctrl->p_tws_remote_dev);
        }
#endif // #ifdef IBRT

#ifdef RECORD_DUMP_DATA
        audio_dump_init(480, sizeof(short), 1);
#endif

#ifdef RECORD_DUMP_DATA
        data_dump_init();
#endif

        /// reconfig the sample rate
        struct AF_STREAM_CONFIG_T *cfg = (struct AF_STREAM_CONFIG_T *)app_ai_voice_get_stream_config();
        cfg->sample_rate = CAPTURE_SAMPLE_RATE;
        cfg->bits = AUD_BITS_16;
        app_ai_voice_stream_control(true, AIV_USER_REC);
    }
    else
    {
        TRACE(0, "WARN: recording not enabled");
    }
}

void app_recording_stop_voice_stream(void)
{
    TRACE(2, "%s recording_enable:%d", __func__, recording_enable);
    if (recording_enable)
    {
        app_recording_update_status(recording_enable & (~(ENABLE_RECORDING | ENABLE_TRANSFER | ENABLE_GET_ENCODED)));
        app_ai_voice_register_trigger_init_handler(NULL);
        /// resume defult stream configuration
        struct AF_STREAM_CONFIG_T *cfg = (struct AF_STREAM_CONFIG_T *)app_ai_voice_get_stream_config();
        cfg->sample_rate = AUD_SAMPRATE_16000;
        cfg->bits = AUD_BITS_16;
        /// close voice stream
        app_ai_voice_stream_control(false, AIV_USER_REC);
        /// clear super user flag
        app_ai_voice_set_super_user_flag(AIV_USER_REC, false);

        reportSent = false;
        delayRT = sTime = 0;
        app_tws_ibrt_request_modify_tws_bandwidth(TWS_TIMING_CONTROL_USER_AI_VOICE, false);

        uint8_t stopRsp[6];
        stopRsp[0] = 0x00;
        stopRsp[1] = 0x80;
        stopRsp[2] = 0x02;
        stopRsp[3] = 0x00;
        stopRsp[4] = 0x05;
        stopRsp[5] = 0x80;
        app_recording_spp_send_data((uint8_t *)&stopRsp, 0x06);
    }
}

static void _cmd_come(uint8_t *buf, uint32_t len)
{
    ASSERT(buf || len == 3, "voice_cmd_cb:error cmd %d", len);
    APP_VOICE_CMD_PAYLOAD_T *payload = NULL;
    uint8_t originStatus = recording_enable;

    TRACE(2, "%s cmd:0x%x", __func__, *(uint16_t *)buf);

    payload = (APP_VOICE_CMD_PAYLOAD_T *)buf;

    if ((RECORDING_CMD_STEREO_START == payload->cmdCode) && (0 == recording_enable))
    {
        /// update recording status
        app_recording_update_status(ENABLE_RECORDING | ENABLE_TRANSFER | ENABLE_GET_ENCODED);
        app_ai_voice_set_super_user_flag(AIV_USER_REC, true);
        /// close mic if mic is open
        app_ai_voice_stream_control(false, AIV_USER_REC);

        if (app_tws_ibrt_tws_link_connected() &&
            (TWS_UI_SLAVE == app_ibrt_if_get_ui_role()))
        {
            TRACE(1, "%s Current role is slave", __func__);
            return;
        }

        if (0x00 == originStatus)
        {
            delayRT = sTime = 0;
            reportSent = false;
            sTime = FAST_TICKS_TO_US(hal_fast_sys_timer_get());
            TRACE(1, "sTime %x", sTime);
            /// open super user mode for recording
            app_recording_start_voice_stream();
        }
    }
    else if (RECORDING_CMD_STOP == payload->cmdCode) //!< Stop recording
    {
        // if (voice_compression_get_encode_buf_size() ||
        //     LengthOfCQueue(&(voice_mem_for_slave.ShareDataQueue)) || LengthOfCQueue(&(voice_mem_for_local.ShareDataQueue)))
        // {
        //     TRACE(0, "data send not complete");
        //     recording_enable &= ~ENABLE_RECORDING;
        // }
        // else
        {
            app_recording_stop_voice_stream();
        }
    }
    else
    {
        TRACE(1, "[Recording warning]%s wrong cmd received", __func__);
    }
}

static void _encoded_data_come(uint8_t *buf, uint32_t len)
{
    /// check the trigger
    _trigger_check();
#ifdef IBRT
    app_ai_voice_capture_sync_tuning();
#endif

    if (recording_enable & ENABLE_RECORDING)
    {
        if (delayRT == 0)
        {
            TRACE(2, "sTime %x, eTime %x", sTime, FAST_TICKS_TO_US(hal_fast_sys_timer_get()));
            delayRT = (FAST_TICKS_TO_US(hal_fast_sys_timer_get()) - sTime);
        }

#ifdef RECORD_DUMP_DATA
        audio_dump_add_channel_data(0, (short *)buf, len / 2);
#endif

#ifdef RECORDING_USE_SCALABLE
        gather_count++;

        if (RECORD_DATA_PACKET_GATHER_LIMIT == gather_count)
        {
            gather_count = 0;
            osSignalSet(voiceTid, SIGNAL_LOCAL_DATA_READY);
        }
#else
        osSignalSet(voiceTid, SIGNAL_LOCAL_DATA_READY);
#endif

#ifdef RECORD_DUMP_DATA
        audio_dump_run();
#endif
    }
    else if (recording_enable & ENABLE_GET_ENCODED)
    {
        osSignalSet(voiceTid, SIGNAL_LOCAL_DATA_READY);
    }
    else if (recording_enable & ENABLE_TRANSFER)
    {
        osSignalSet(voiceTid, SIGNAL_PEER_DATA_READY);
    }
    return;
}

bool app_recording_spp_send_data(uint8_t *buf, uint32_t len)
{
    return app_ai_spp_send(buf, len, AI_SPEC_RECORDING, app_ai_get_device_id_from_index(AI_SPEC_RECORDING));
}

void app_recording_bt_connect_cb(bt_connect_e bt_state)
{
    TRACE(2, "%s incoming event:%d", __func__, bt_state);

    if ((BLE_CONNECT == bt_state) ||
        (SPP_CONNECT == bt_state))
    {
        /// register AI voice related handlers
        AIV_HANDLER_BUNDLE_T handlerBundle = {
            .streamStarted          = _config_right_before_capture_stream_started,
            .streamStopped          = _config_right_before_capture_stream_stopped,
            .upstreamDataHandler    = app_ai_voice_default_upstream_data_handler,
        };
        app_ai_voice_register_ai_handler(AIV_USER_REC, &handlerBundle);
    }
    else
    {
        /// register AI voice related handlers
        AIV_HANDLER_BUNDLE_T handlerBundle = {
            .streamStarted          = NULL,
            .streamStopped          = NULL,
            .upstreamDataHandler    = NULL,
        };
        app_ai_voice_register_ai_handler(AIV_USER_REC, &handlerBundle);
    }
}

uint32_t app_recording_connected(void *param1, uint32_t param2, uint8_t dest_id)
{
    ASSERT(param1, "%s null pointer received.", __func__);
    bt_bdaddr_t *btaddr = (bt_bdaddr_t *)param1;
    AI_TRANS_TYPE_E transType = (AI_TRANS_TYPE_E)param2;

    TRACE(1, "%s mobile addr:", __func__);
    DUMP8("%02x ", btaddr->address, BT_ADDR_OUTPUT_PRINT_NUM);
    memcpy((uint8_t *)&mobileAddr, btaddr->address, sizeof(bt_bdaddr_t));
    app_ai_connect_handle(transType, AI_SPEC_RECORDING, dest_id, btaddr->address);


    if (AI_TRANSPORT_SPP == transType)
    {
        app_recording_bt_connect_cb(SPP_CONNECT);
        // ai_data_transport_init(app_ai_spp_send, AI_SPEC_RECORDING, transType);
        // ai_cmd_transport_init(app_ai_spp_send, AI_SPEC_RECORDING, transType);
        // app_ai_set_mtu(TX_SPP_MTU_SIZE, AI_SPEC_RECORDING);
        // app_ai_set_data_trans_size(SPP_TRANS_SIZE, AI_SPEC_RECORDING);
    }
    else if (AI_TRANSPORT_BLE == transType)
    {
        app_recording_bt_connect_cb(BLE_CONNECT);
    }

    return 0;
}

void app_recording_disconnect(void)
{
    app_ai_spp_disconnlink(AI_SPEC_RECORDING, 0);
}

uint32_t app_recording_disconnected(void *param1, uint32_t param2)
{
    TRACE(1, "%s", __func__);

    app_recording_stop_voice_stream();

    if (param2 == AI_TRANSPORT_SPP)
    {
        app_recording_bt_connect_cb(SPP_DISCONNECT);
    }
    else if (param2 == AI_TRANSPORT_BLE)
    {
        app_recording_bt_connect_cb(BLE_DISCONNECT);
    }

    return 0;
}

void app_recording_audio_send_done(void)
{
    TRACE(1, "%s: curCursor %d, outCursor %d", __func__, curCursor, outCursor);

    if (curCursor >= (outCursor + RECORD_DATA_PACKET_GATHER_LIMIT))
    {
        osSignalSet(voiceTid, SIGNAL_LOCAL_DATA_READY);
    }
}

uint32_t app_recording_send_done(void *param1, uint32_t param2)
{
    TRACE(1, "%s: curCursor %d, outCursor %d", __func__, curCursor, outCursor);

    if ((curCursor > outCursor) && recording_enable)
    {
        osSignalSet(voiceTid, SIGNAL_PEER_DATA_READY);
    }

    return 0;
}

/// COMMON handlers function definition
const struct ai_func_handler recording_handler_function_list[] =
    {
        {API_HANDLE_INIT,           (ai_handler_func_t)_functionality_init},
        {API_DATA_INIT,             (ai_handler_func_t)_recording_init},
        {API_DATA_SEND,             (ai_handler_func_t)_encoded_data_come},
        {CALLBACK_CMD_RECEIVE,      (ai_handler_func_t)_cmd_come},
        {CALLBACK_AI_CONNECT,       (ai_handler_func_t)app_recording_connected},
        {CALLBACK_AI_DISCONNECT,    (ai_handler_func_t)app_recording_disconnected},
        {CALLBACK_STORE_SLAVE_DATA, (ai_handler_func_t)app_recording_store_peer_data},
        {CALLBACK_UPDATE_MTU,       (ai_handler_func_t)NULL},
        {CALLBACK_WAKE_UP,          (ai_handler_func_t)NULL},
        {CALLBACK_AI_ENABLE,        (ai_handler_func_t)NULL},
        {CALLBACK_START_SPEECH,     (ai_handler_func_t)NULL},
        {CALLBACK_ENDPOINT_SPEECH,  (ai_handler_func_t)NULL},
        {CALLBACK_STOP_SPEECH,      (ai_handler_func_t)NULL},
        {CALLBACK_DATA_SEND_DONE,   (ai_handler_func_t)app_recording_send_done},
        {CALLBACK_KEY_EVENT_HANDLE, (ai_handler_func_t)NULL},
};

const struct ai_handler_func_table recording_handler_function_table =
    {&recording_handler_function_list[0], (sizeof(recording_handler_function_list) / sizeof(struct ai_func_handler))};

AI_HANDLER_FUNCTION_TABLE_TO_ADD(AI_SPEC_RECORDING, &recording_handler_function_table)
