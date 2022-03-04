#include <stdio.h>
#include <string.h>
#include "cmsis_os.h"
#include "hal_trace.h"
#include "cqueue.h"
#include "cmsis_os.h"
#include "cqueue.h"
#include "spp_api.h"
#include "app_spp.h"
#include "umm_malloc.h"
#include "app_ai_tws.h"

#include "ai_spp.h"
#include "ai_transport.h"
#include "ai_thread.h"
#include "ai_control.h"
#include "ai_manager.h"
#include "app_bt.h"
#include "app_spp.h"

#ifdef IOS_IAP2_PROTOCOL
#include "bluetooth_iap2_link.h"
#endif

typedef struct
{
    uint8_t ai_index;
    struct spp_device *spp_dev;
} AI_SPP_DEVICE_T;

static uint8_t ai_spp_rx_buf[BT_DEVICE_NUM*SPP_RECV_BUFFER_SIZE];

static AI_SPP_DEVICE_T ai_spp_dev[MUTLI_AI_NUM][AI_CONNECT_COUNT];

#if BTIF_SPP_SERVER == BTIF_ENABLED
osMutexDef(ai_server_spp_mutex);
#endif

#if BTIF_SPP_CLIENT == BTIF_ENABLED
osMutexDef(ai_client_spp_mutex);
#endif

osMutexDef(ai_credit_mutex_1);
#if BT_DEVICE_NUM > 1
osMutexDef(ai_credit_mutex_2);
#endif

void app_ai_spp_deinit_tx_buf(void)
{
    //umm_init();
}

/****************************** IAP2 SPP Read data Frame Integral Check ********************************/
#ifdef IOS_IAP2_PROTOCOL
#define SPP_PARSE_STATE_HEADER      0
#define SPP_PARSE_STATE_DATA        1
#define SPP_READ_BUF_MAX_LEN        600

static uint8_t spp_parse_state = SPP_PARSE_STATE_HEADER;
static uint8_t read_cmd_buf[SPP_READ_BUF_MAX_LEN] = {0};
static uint16_t read_cmd_len = 0;

const uint8_t   kIap2PacketDetectData[]     = { 0xFF, 0x55, 0x02, 0x00, 0xEE, 0x10 };
uint32_t        kIap2PacketDetectDataLen    = 6;
const uint8_t   kIap2PacketDetectBadData[]  = { 0xFF, 0x55, 0x04, 0x00, 0x02, 0x04, 0xEE, 0x08 };
uint32_t        kIap2PacketDetectBadDataLen = 8;

void iap2_integral_read_buffer_reset(void)
{
    spp_parse_state = SPP_PARSE_STATE_HEADER;
    read_cmd_len = 0;
    memset(read_cmd_buf,0,SPP_READ_BUF_MAX_LEN);
    TRACE(1,"%s", __func__);
}
int iap2_integral_read_parse(uint8_t read_data)
{
    uint16_t cmd_len = 0;
    switch(spp_parse_state)
    {
        case SPP_PARSE_STATE_HEADER:
            read_cmd_buf[read_cmd_len++] = read_data;
            if(read_cmd_len == kIap2PacketDetectDataLen)
            {
                if (memcmp(read_cmd_buf, kIap2PacketDetectData, kIap2PacketDetectDataLen) == 0)
                {
                    iap2_buf_parse(read_cmd_buf, read_cmd_len);
                    spp_parse_state = SPP_PARSE_STATE_HEADER;
                    read_cmd_len = 0;
                    memset(read_cmd_buf, 0, SPP_READ_BUF_MAX_LEN);
                    break;
                }
                spp_parse_state = SPP_PARSE_STATE_DATA;
            }
            break;
        case SPP_PARSE_STATE_DATA:
            read_cmd_buf[read_cmd_len++] = read_data;
            cmd_len = read_cmd_buf[2];
            cmd_len = cmd_len<<8|read_cmd_buf[3];
            if(read_cmd_len == cmd_len)
            {
                //DUMP8("%02x ", read_cmd_buf, read_cmd_len);
                iap2_buf_parse(read_cmd_buf, read_cmd_len);
                spp_parse_state = SPP_PARSE_STATE_HEADER;
                read_cmd_len = 0;
                memset(read_cmd_buf,0,SPP_READ_BUF_MAX_LEN);
            }
            break;
        default:
            break;
     }  
     return 0;
}
#endif

uint8_t ai_app_find_spec_from_service_id(uint8_t service_id)
{
    TRACE(2, "%s, service_id %d", __func__, service_id);
    uint8_t ai_spec = 0;

    switch(service_id)
    {
        case RFCOMM_CHANNEL_AMA:
           ai_spec = AI_SPEC_AMA;
           break;
        case RFCOMM_CHANNEL_BES:
           ai_spec = AI_SPEC_BES;
           break;
        case RFCOMM_CHANNEL_BAIDU:
           ai_spec = AI_SPEC_BAIDU;
           break;
        case RFCOMM_CHANNEL_TENCENT:
           ai_spec = AI_SPEC_TENCENT;
           break;
        case RFCOMM_CHANNEL_ALI:
           ai_spec = AI_SPEC_ALI;
           break;
        case RFCOMM_CHANNEL_COMMON:
           ai_spec = AI_SPEC_COMMON;
           break;
        case RFCOMM_CHANNEL_COMMON_RECORD:
            ai_spec = AI_SPEC_RECORDING;
            break;
        default:
            ASSERT(false, "Invalid service id %d.", service_id);
            break;
    }

    return ai_spec;
}

int ai_spp_data_event_callback(uint8_t device_id, void *pDev, uint8_t process, uint8_t *pData, uint16_t dataLen)
{
    TRACE(2,"%s len %d", __func__, dataLen);
    DUMP8("%02x ", pData, dataLen);

#ifdef IOS_IAP2_PROTOCOL
    if(app_ai_connect_mobile_has_ios()) 
    {
        for(uint16_t i = 0; i < dataLen; i++)
        {
            iap2_integral_read_parse(*(pData+i));
        }
        return 0;
    }
#endif

    struct spp_device *locDev = (spp_device *)pDev;
    uint8_t ai_index = ai_app_find_spec_from_service_id(locDev->sppDev.type.sppService->rf_service.serviceId);
    TRACE(2,"%s ai_index %d", __func__, ai_index);

    ai_function_handle(CALLBACK_CMD_RECEIVE, pData, dataLen, ai_index, device_id);

    return 0;
}

static struct spp_device *ai_spp_find_device_from_id(uint8_t ai_index, uint8_t device_id)
{
    struct BT_DEVICE_T *curr_device = app_bt_get_device(device_id);
    int i = 0;
    struct spp_device *_ai_spp_dev = NULL;

    ASSERT(ai_index<MUTLI_AI_NUM, "%s ai_index %d", __func__, ai_index);

    for (; i < BT_DEVICE_NUM; i += 1)
    {
        if (app_ai_spp_is_connected(ai_index, i))
        {
            _ai_spp_dev = ai_spp_dev[ai_index][i].spp_dev;
            if (memcmp(&_ai_spp_dev->btaddr, &curr_device->remote, sizeof(bt_bdaddr_t)) == 0)
            {
                return _ai_spp_dev;
            }
        }
    }

    return NULL;
    
}
uint8_t app_ai_spp_get_device_connected_index(uint8_t ai_index, bt_bdaddr_t* bdaddr)
{
    for (uint8_t i = 0 ; i < AI_CONNECT_NUM_MAX; i += 1)
    {
        if(ai_spp_dev[ai_index][i].spp_dev != NULL)
        {
            if(memcmp(&ai_spp_dev[ai_index][i].spp_dev->btaddr, bdaddr, sizeof(bt_bdaddr_t)) == 0)
            {
                return i;
            }
        }
    }
    return AI_CONNECT_NUM_MAX;
}

void ai_spp_enumerate_disconnect_service(uint8_t *address,uint8_t deviceID)
{
    for(uint8_t ai_index = AI_SPEC_AMA; ai_index < AI_SPEC_COUNT; ai_index++){
        uint8_t connect_index = app_ai_get_connect_index(address, ai_index);
        if(connect_index != AI_CONNECT_NUM_MAX){
            if (app_ai_get_transport_type(ai_index, connect_index) == AI_TRANSPORT_SPP){
                connect_index = app_ai_spp_get_device_connected_index(ai_index, (bt_bdaddr_t*)address);
                if(connect_index != AI_CONNECT_NUM_MAX)
                    app_ai_spp_disconnlink(ai_index, connect_index);
            }
        }
    }
}

static void ai_spp_callback(uint8_t device_id, struct spp_device *locDev, spp_callback_parms *Info)
{
    uint8_t ai_index = ai_app_find_spec_from_service_id(locDev->sppDev.type.sppService->rf_service.serviceId);

    if (BTIF_SPP_EVENT_REMDEV_CONNECTED == Info->event)
    {
        TRACE(1,"::AI_SPP_CONNECTED ai_index:%d, mobileAddr:", ai_index);
        DUMP8("%02x ", locDev->btaddr.address, BT_ADDR_OUTPUT_PRINT_NUM);
        locDev->spp_connected_flag = true;
        umm_init();
#ifdef IOS_IAP2_PROTOCOL
        if(app_ai_connect_mobile_has_ios())
            bt_iap2_packet_init();
#endif

        if(app_ai_get_connected_mun() > AI_CONNECT_NUM_MAX)
        {
            TRACE(1,"%s, Already connected max num ai !!!", __func__);

        }
        else
        {
            ai_function_handle(CALLBACK_AI_CONNECT, (void *)&locDev->btaddr, AI_TRANSPORT_SPP, ai_index, device_id);
        }
#ifdef IBRT
        if ((app_bt_is_a2dp_connected(device_id) == false) && (app_bt_is_hfp_connected(device_id) == false))
        {
            if(IBRT_MASTER == app_tws_get_ibrt_role((bt_bdaddr_t *)&locDev->btaddr))
            {
                TRACE(1, "manual reconnect mobile profiles");
                app_bt_ibrt_reconnect_mobile_profile((bt_bdaddr_t *)&locDev->btaddr);
            }
        }
#endif        
    }
    else if (BTIF_SPP_EVENT_REMDEV_DISCONNECTED == Info->event)
    {
        uint8_t connect_index = app_ai_get_connect_index(locDev->btaddr.address, ai_index);
        TRACE(2,"::AI_SPP_DISCONNECTED, connIdx:%d ai_index:%d, mobileAddr:", connect_index, ai_index);
        DUMP8("%02x ", locDev->btaddr.address, BT_ADDR_OUTPUT_PRINT_NUM);
        locDev->spp_connected_flag = false;

#ifdef IOS_IAP2_PROTOCOL
        if(app_ai_connect_mobile_has_ios())
        {
            iap2_integral_read_buffer_reset();
            bt_iap2_packet_deinit();
        }
#endif

        if(AI_CONNECT_NUM_MAX == connect_index)
        {
            TRACE(2, "%s, disconnect by self, because connected num is %d", __func__, connect_index);
        }
        else
        {
            if (app_ai_get_transport_type(ai_index, connect_index) == AI_TRANSPORT_SPP)
            {
                app_ai_update_connect_state(AI_IS_DISCONNECTING, connect_index);
                ai_function_handle(CALLBACK_AI_DISCONNECT, NULL, AI_TRANSPORT_SPP, ai_index, device_id);
            }
            app_ai_clear_connect_info(connect_index);
        }
    }
    else if (BTIF_SPP_EVENT_DATA_SENT == Info->event)
    {
        umm_free(((struct spp_tx_done *)(Info->p.other))->tx_buf);
        ai_function_handle(CALLBACK_DATA_SEND_DONE, NULL, 0, ai_index, device_id);
    }
    else
    {
        TRACE(1,"::unknown event %d\n", Info->event);
    }
}

bool app_ai_spp_send(uint8_t* ptrData, uint32_t length, uint8_t ai_index, uint8_t device_id)
{
    bool ret = FALSE;
    TRACE(2,"%s length %d", __func__, length);

    if (app_ai_is_in_tws_mode(0))
    {
        if (app_ai_tws_link_connected() && (app_ai_tws_get_local_role() != APP_AI_TWS_MASTER))
        {
            TRACE(2,"%s role %d isn't MASTER", __func__, app_ai_tws_get_local_role());
            return FALSE;
        }
    }

    struct spp_device *_ai_spp_dev = ai_spp_find_device_from_id(ai_index, device_id);
    if(_ai_spp_dev)
    {
        ret = (app_spp_send_data(_ai_spp_dev, ptrData, (uint16_t *)&length)==BT_STS_SUCCESS)?TRUE:FALSE;
    }

    return ret;
}

AI_SPP__REGISTER_INSTANCE_T* ai_spp_register_get_entry_pointer_from_ai_code(uint32_t ai_code)
{
    for (uint32_t index = 0;
         index < ((uint32_t)__ai_spp_register_table_end-(uint32_t)__ai_spp_register_table_start)/sizeof(AI_SPP__REGISTER_INSTANCE_T); index++)
    {
        if (AI_SPP_PTR_FROM_ENTRY_INDEX(index)->ai_code == ai_code)
        {
            TRACE(3,"%s ai_code %d index %d", __func__, ai_code, index);
            return AI_SPP_PTR_FROM_ENTRY_INDEX(index);
        }
    }

    ASSERT(0, "%s fail ai_code %d", __func__, ai_code);
    return NULL;
}

uint32_t ai_spp_register_get_entry_index_from_ai_code(uint32_t ai_code)
{
    for (uint32_t index = 0;
         index < ((uint32_t)__ai_spp_register_table_end-(uint32_t)__ai_spp_register_table_start)/sizeof(AI_SPP__REGISTER_INSTANCE_T); index++)
    {
        if (AI_SPP_PTR_FROM_ENTRY_INDEX(index)->ai_code == ai_code)
        {
            TRACE(3,"%s ai_code %d index %d", __func__, ai_code, index);
            return index;
        }
    }

    ASSERT(0, "%s fail ai_code %d", __func__, ai_code);
    return INVALID_AI_SPP_ENTRY_INDEX;
}

void ai_spp_register_uuid_sdp_services(uint8_t *service_id, btif_sdp_record_param_t *param, uint8_t ai_index)
{
    AI_SPP__REGISTER_INSTANCE_T *pInstance = NULL;

    TRACE(1,"%s", __func__);
    pInstance = ai_spp_register_get_entry_pointer_from_ai_code(ai_index);
    if(pInstance)
        pInstance->ai_handler(service_id, param);
}

void app_ai_spp_init(spp_port_t portType, uint8_t ai_index)
{

    TRACE(1,"%s",__func__);
    btif_sdp_record_param_t param;
    osMutexId mid = 0;
    uint8_t service_id = 0;
    uint64_t app_id = 0;
    struct spp_device *_ai_spp_dev = NULL;

    if (BTIF_SPP_SERVER_PORT == portType)
    {
        app_id = app_spp_alloc_server_id();
    }
    else
    {
        app_id = app_spp_alloc_client_id();
    }

    for (uint8_t i = 0 ; i < AI_CONNECT_NUM_MAX; i += 1)
    {
        if (NULL == ai_spp_dev[ai_index][i].spp_dev)
        {
            ai_spp_dev[ai_index][i].spp_dev = app_create_spp_device();
            _ai_spp_dev = ai_spp_dev[ai_index][i].spp_dev;
        }

        ai_spp_dev[ai_index][i].ai_index = ai_index;

        if (i == AI_CONNECT_1)
        {
            mid = osMutexCreate(osMutex(ai_credit_mutex_1));
        }
#if AI_CONNECT_NUM_MAX > 1
        else if (i == AI_CONNECT_2)
        {
            mid = osMutexCreate(osMutex(ai_credit_mutex_2));
        }
#endif

        if (!mid)
        {
            ASSERT(0, "cannot create mutex");
        }

        _ai_spp_dev->creditMutex = mid;

        btif_spp_init_rx_buf(_ai_spp_dev, ai_spp_rx_buf + i * SPP_RECV_BUFFER_SIZE, SPP_RECV_BUFFER_SIZE);
        _ai_spp_dev->portType = portType;
        _ai_spp_dev->app_id = app_id;
        _ai_spp_dev->spp_handle_data_event_func = ai_spp_data_event_callback;

        ai_spp_register_uuid_sdp_services(&service_id, &param, ai_index);
        TRACE(2,"%s %d ",__func__, service_id);
        app_spp_open(_ai_spp_dev, NULL, &param, mid, service_id, ai_spp_callback);
    }
}


void app_ai_spp_client_init(uint8_t ai_index)
{
    TRACE(2,"%s AI index:%d",__func__, ai_index);

#if BTIF_SPP_CLIENT == BTIF_ENABLED
    app_ai_spp_init(BTIF_SPP_CLIENT_PORT, ai_index);
#else
    TRACE(0,"!!!SPP_CLIENT is no support ");
#endif
}


void app_ai_spp_server_init(uint8_t ai_index)
{
    TRACE(2,"%s AI index:%d", __func__, ai_index);

#if BTIF_SPP_SERVER == BTIF_ENABLED
    app_ai_spp_init(BTIF_SPP_SERVER_PORT, ai_index);
#else
    TRACE(0,"!!!SPP_SERVER is no support ");
#endif
}

bool app_ai_spp_is_connected(uint8_t ai_index, uint8_t connected_index)
{
    bool connected = false;
    struct spp_device *sppDev = ai_spp_dev[ai_index][connected_index].spp_dev;
    if (sppDev)
    {
        connected = sppDev->spp_connected_flag;
    }

    return connected;
}

void *app_ai_spp_get_remdev_btaddr(uint8_t ai_index, uint8_t connected_index)
{
    return &ai_spp_dev[ai_index][connected_index].spp_dev->btaddr;
}

void app_ai_spp_disconnlink(uint8_t ai_index, uint8_t connected_index)//only used for device force disconnect
{
    TRACE(2,"%s ai_index:%d, connection_index:%d", __func__, ai_index, connected_index);
    struct spp_device *_ai_spp_dev = ai_spp_dev[ai_index][connected_index].spp_dev;

    if(_ai_spp_dev->spp_connected_flag == true)
    {
        btif_spp_disconnect(_ai_spp_dev, BTIF_BEC_LOCAL_TERMINATED);
    }
    return;
}

void app_ai_link_free_after_spp_dis(uint8_t ai_index, uint8_t connected_index)
{
    TRACE(1,"%s", __func__);
    struct spp_device *_ai_spp_dev = ai_spp_dev[ai_index][connected_index].spp_dev;
    if ((_ai_spp_dev->portType != BTIF_SPP_SERVER_PORT) && (_ai_spp_dev->portType != BTIF_SPP_CLIENT_PORT))
    {
        TRACE(1,"%s spp not open", __func__);
        return;
    }
    
    if (false == _ai_spp_dev->spp_connected_flag)
    {
        btif_spp_close(_ai_spp_dev);
    }
    else if ((btif_me_get_activeCons()) && (true == _ai_spp_dev->spp_connected_flag))
    {
        btif_spp_close(_ai_spp_dev); //send dis cmd to app
    }
}
