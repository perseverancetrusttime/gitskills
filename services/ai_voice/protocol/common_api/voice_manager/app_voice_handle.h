#ifndef __APP_VOICE_HANDLE_H
#define __APP_VOICE_HANDLE_H

#ifndef ADV_DATA_LEN
#define ADV_DATA_LEN     0x1F
#endif

#ifndef BLE_DATA_LEN
#define BLE_DATA_LEN     0x1B
#endif

/*
 * GMA VOICE CMD PROFILE ATTRIBUTES
 ****************************************************************************************
 */
typedef struct{
    uint16_t srv_uuid;   //
    uint16_t write_att;  //phone to accessory for data(prop write with resp)
    uint16_t read_att;
    uint16_t indicate_att;//accessory to phone for except mic and ota data
    uint16_t notfy_att; //accessory to phone for ota/mic data
    uint16_t ota_write_att; //phone to accessory for ota(prop write no resp)
}voice_ble_para_t;

typedef struct{
    uint8_t adv_data[ADV_DATA_LEN];
    uint8_t adv_data_len;
}voice_ble_adv_para_t;

typedef struct{
    uint8_t srv_uuid[16];  //uuid
}voice_spp_para_t;

typedef struct{
    voice_ble_para_t gma_ble_para; //ble svr 
    voice_spp_para_t spp_para; //spp uuid svr
}hal_voice_base_t;

typedef enum{
    MIC_STATE_OFF   = 0,
    MIC_STATE_ON    = 1,
} mic_state_e;

typedef enum{
    TICK_TYPE_10MS = 0,
    TICK_TYPE_50MS,
    TICK_TYPE_MAX,
}tick_type_e;

typedef enum{
    BLE_CONNECT = 0,
    BLE_DISCONNECT,
    SPP_CONNECT,
    SPP_DISCONNECT,
    BT_DEFAULT,
    BT_STATE_MAX,
}bt_connect_e;

typedef struct{
    uint16_t value;
    uint8_t status;
}voice_power_value_t;

typedef struct{
    hal_voice_base_t gws_base;
}voice_model_cb;

void voice_recv_proc_cb(uint8_t *buf, uint32_t len);
void voice_ota_recv_proc_cb(uint8_t *buf, uint32_t len);
void voice_mic_recv_data_cb(uint8_t *buf, uint32_t len);
void voice_time_tick_cb(tick_type_e tick_type);
void voice_bt_connect_cb(bt_connect_e bt_state);
void voice_ble_adv_data_set(void *para, uint8_t len);
void voice_ble_adv_start(void);
void voice_ble_adv_stop(void);
void voice_ble_send_data_notify(uint8_t *buf, uint32_t len);
void voice_ble_send_data_indicate(uint8_t *buf, uint32_t len);
void voice_ble_send_cmd_notify(uint8_t *buf, uint32_t len);
void voice_ble_send_cmd_indicate(uint8_t *buf, uint32_t len);
void voice_spp_send_data(uint8_t *buf, uint32_t len);
uint32_t voice_start_advertising(void *param1, uint32_t param2);
uint32_t voice_connected(void *param1, uint32_t param2);
uint32_t voice_disconnected(void *param1, uint32_t param2);
bool a2dp_status_get(void);
void a2dp_play_start(void);
void a2dp_play_pause(void);
void a2dp_play_stop(void);
void a2dp_play_next(void);
void a2dp_play_last(void);
void voice_volume_set(uint16_t type, uint8_t vol_num);
bool hfp_status_get(void);
void dial_phone_num(uint8_t *buf, uint16_t len);
void telephone_call(void);
void telephone_refuse(void);
void voice_mic_set(mic_state_e state);
void power_value_get(voice_power_value_t *gws_power);
//void sal_random_value_get(uint8_t *gws_random, uint8_t size);


#endif

