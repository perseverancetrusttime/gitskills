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
//#include "mbed.h"

#if defined(IBRT)
#include <stdio.h>
#include <assert.h>

#include "hal_trace.h"
#include "app_ibrt_if.h"
#include "app_tws_ctrl_thread.h"
#include "app_ibrt_rssi.h"
#include "app_custom_api.h"
#include "a2dp_decoder.h"
#include "app_tws_ibrt.h"
#include "app_tws_ibrt_conn.h"

//static struct rssi_window_struct tws_rssi_window = {0};
//static struct rssi_window_struct mobile_rssi_window[BT_DEVICE_NUM + 1] = {0,};

extern float a2dp_audio_get_sample_reference(void);
extern int8_t a2dp_audio_get_current_buf_size(void);


void rssi_window_push(ibrt_rssi_window_t *p, int8_t data)
{
    if(p == NULL)
    {
        return;
    }

    if (p->index < RSSI_WINDOW_SIZE)
    {
        for(uint8_t i=p->index; i>0; i--)
        {
            p->buf[i] = p->buf[i-1];
        }
        p->buf[0] = data;
        p->index++;
    }
    else if (p->index == RSSI_WINDOW_SIZE)
    {
        for(uint8_t i=p->index-1; i>0; i--)
        {
            p->buf[i] = p->buf[i-1];
        }
        p->buf[0] = data;
    }
    else
    {
        ASSERT(0,"%s,invalid index=%x",__func__,p->index);
    }
}


/*****************************************************************************
 Prototype    : app_ibrt_ui_rssi_reset
 Description  : reset mobile link's rssi parameters
 Input        : void
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/4/17
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_ibrt_ui_rssi_reset(void)
{
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    ibrt_mobile_info_t *p_mobile_info_array[BT_DEVICE_NUM + 1];
    ibrt_mobile_info_t *p_mobile_info =  NULL;
    uint8_t count = 0;

    //clear tws rssi window
    memset((uint8_t*)&p_ibrt_ctrl->tws_rssi_window,0,sizeof(ibrt_rssi_window_t));
    p_ibrt_ctrl->tws_rssi_window.index = 0;

    //get all mobile connected infor
    count = app_ibrt_conn_get_all_valid_mobile_info(p_mobile_info_array,BT_DEVICE_NUM + 1);
    
    for (uint8_t i = 0; i < count; i++)
    {
        p_mobile_info = p_mobile_info_array[i];

        //local rssi paramters
        p_mobile_info->raw_rssi.agc_idx0 = 0;
        p_mobile_info->raw_rssi.rssi0 = 0;
        p_mobile_info->raw_rssi.rssi0_max = 0x7f;
        p_mobile_info->raw_rssi.rssi0_min = 0x80;
        p_mobile_info->raw_rssi.agc_idx1 = 0;
        p_mobile_info->raw_rssi.rssi1 = 0;
        p_mobile_info->raw_rssi.rssi1_max = 0x7f;
        p_mobile_info->raw_rssi.rssi1_min = 0x80;
        p_mobile_info->raw_rssi.rssi2 = 0;
        p_mobile_info->raw_rssi.rssi2_max = 0x7f;
        p_mobile_info->raw_rssi.rssi2_min = 0x80;
        p_mobile_info->raw_rssi.ser = 0;
        p_mobile_info->raw_rssi.rx_data_sum = 0;

        //TODO...,peer rssi parameters,used for tws role switch triggered by rssi
        p_mobile_info->peer_raw_rssi.agc_idx0 = 0;
        p_mobile_info->peer_raw_rssi.rssi0 = 0;
        p_mobile_info->peer_raw_rssi.rssi0_max = 0x7f;
        p_mobile_info->peer_raw_rssi.rssi0_min = 0x80;
        p_mobile_info->peer_raw_rssi.agc_idx1 = 0;
        p_mobile_info->peer_raw_rssi.rssi1 = 0;
        p_mobile_info->peer_raw_rssi.rssi1_max = 0x7f;
        p_mobile_info->peer_raw_rssi.rssi1_min =0x80;
        p_mobile_info->peer_raw_rssi.rssi2 = 0;
        p_mobile_info->peer_raw_rssi.rssi2_max = 0x7f;
        p_mobile_info->peer_raw_rssi.rssi2_min = 0x80;
        p_mobile_info->peer_raw_rssi.ser = 0;
        p_mobile_info->peer_raw_rssi.rx_data_sum = 0;    

        //clear mobile rssi window
         memset((uint8_t*)&p_mobile_info->rssi_window,0,sizeof(ibrt_rssi_window_t));
         p_mobile_info->rssi_window.index = 0;
    }

}

static void app_ibrt_ui_rssi_dump(const char * str,const rssi_t * local_rssi, const rssi_t* peer_rssi)
{
    if(str){
        TRACE(1,"%s",str);
    }
    if(local_rssi){
        TRACE(5,"local:%d %d %d %d %d",local_rssi->rssi0, local_rssi->agc_idx0,local_rssi->rssi0_max,local_rssi->rssi0_min,local_rssi->ser);
    }
    if(peer_rssi){
        TRACE(5,"peer:%d %d %d %d %d",peer_rssi->rssi0, peer_rssi->agc_idx0,peer_rssi->rssi0_max,peer_rssi->rssi0_min,peer_rssi->ser);
    }
}

/*****************************************************************************
 Prototype    : app_ibrt_ui_rssi_process
 Description  : read rssi and save it
 Input        : void
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/4/17
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_ibrt_ui_rssi_process(void)
{
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    ibrt_mobile_info_t *p_mobile_info_array[BT_DEVICE_NUM + 1];
    ibrt_mobile_info_t *p_mobile_info =  NULL;
    uint8_t count = 0;

    rx_agc_t tws_agc = {0};
    
    int32_t tws_rssi_sum = 0;
    bool tws_rssi_influence = false;
    if (app_tws_ibrt_tws_link_connected())
    {   
        //read and save rssi
        bt_drv_reg_op_read_rssi_in_dbm(p_ibrt_ctrl->tws_conhandle,&tws_agc);
        rssi_window_push(&p_ibrt_ctrl->tws_rssi_window, tws_agc.rssi);
        
        if (p_ibrt_ctrl->tws_rssi_window.index >= RSSI_WINDOW_SIZE)
        {
            for (uint8_t i = 0; i < RSSI_WINDOW_SIZE; i++)
            {
                tws_rssi_sum += p_ibrt_ctrl->tws_rssi_window.buf[i];
            }
            tws_rssi_influence = true;
        }
    }

    //update tws rssi influence
    if (tws_rssi_influence == true)
    {
        p_ibrt_ctrl->raw_rssi.rssi0 = tws_rssi_sum/RSSI_WINDOW_SIZE;
        p_ibrt_ctrl->raw_rssi.agc_idx0 = tws_agc.rxgain;
        
        if (p_ibrt_ctrl->raw_rssi.rssi0 >= p_ibrt_ctrl->raw_rssi.rssi0_max)
        {
            p_ibrt_ctrl->raw_rssi.rssi0_max = p_ibrt_ctrl->raw_rssi.rssi0;
        }
        
        if (p_ibrt_ctrl->raw_rssi.rssi0 <= p_ibrt_ctrl->raw_rssi.rssi0_min)
        {
            p_ibrt_ctrl->raw_rssi.rssi0_min = p_ibrt_ctrl->raw_rssi.rssi0;
        }
        app_ibrt_ui_rssi_dump("tws:",(const rssi_t *)&(p_ibrt_ctrl->raw_rssi),NULL);
    }
    else
    {
        p_ibrt_ctrl->raw_rssi.rssi0 = 0;
        p_ibrt_ctrl->raw_rssi.rssi0_max = 0x80;
        p_ibrt_ctrl->raw_rssi.rssi0_min = 0x7f;
        p_ibrt_ctrl->raw_rssi.agc_idx0 = 0;
    }


    //get all mobile connected infor
    count = app_ibrt_conn_get_all_valid_mobile_info(p_mobile_info_array,BT_DEVICE_NUM + 1);

    for (uint8_t i = 0; i < count; i++)
    {
        rx_agc_t mobile_agc= {0};

        p_mobile_info = p_mobile_info_array[i];

        //read and save rssi
        if (INVALID_HANDLE != p_mobile_info->ibrt_conhandle)
        {
            bt_drv_reg_op_read_rssi_in_dbm(p_mobile_info->ibrt_conhandle,&mobile_agc);
        }
        else if (INVALID_HANDLE != p_mobile_info->mobile_conhandle)
        {
            bt_drv_reg_op_read_rssi_in_dbm(p_mobile_info->mobile_conhandle,&mobile_agc);
        }
        rssi_window_push(&p_mobile_info->rssi_window, mobile_agc.rssi);

        if (p_mobile_info->rssi_window.index >= RSSI_WINDOW_SIZE)
        {
            int32_t mobile_rssi_sum = 0;
            for (uint8_t i = 0; i < RSSI_WINDOW_SIZE; i++)
            {
                mobile_rssi_sum += p_mobile_info->rssi_window.buf[i];
            }

            p_mobile_info->raw_rssi.rssi0 = mobile_rssi_sum/RSSI_WINDOW_SIZE;
            p_mobile_info->raw_rssi.agc_idx0 = mobile_agc.rxgain;

            if (p_mobile_info->raw_rssi.rssi0 >= p_mobile_info->raw_rssi.rssi0_max)
            {
                p_mobile_info->raw_rssi.rssi0_max = p_mobile_info->raw_rssi.rssi0;
            }
            
            if (p_mobile_info->raw_rssi.rssi0 <= p_mobile_info->raw_rssi.rssi0_min)
            {
                p_mobile_info->raw_rssi.rssi0_min = p_mobile_info->raw_rssi.rssi0;
            }
            app_ibrt_ui_rssi_dump("mobile:",(const rssi_t *)&(p_mobile_info->raw_rssi),NULL);
        }
        else
        {
            p_mobile_info->raw_rssi.rssi0 = 0;
            p_mobile_info->raw_rssi.rssi0_max = 0x80;
            p_mobile_info->raw_rssi.rssi0_min = 0x7f;
            p_mobile_info->raw_rssi.agc_idx0 = 0;
        }
    }

}


//static tota_stutter_t g_stutter = {0,};
// TODO:how to get peer rssi info
void app_ibrt_rssi_get_stutter(uint8_t * data,uint32_t * data_len)
{
    // header + tws rssi + mobile1 rssi + mobile2 rssi + ... + mobilen rssi
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    ibrt_mobile_info_t *p_mobile_info_array[BT_DEVICE_NUM + 1] = {NULL};
    ibrt_mobile_info_t *p_mobile_info =  NULL;
    uint8_t count = 0;
    uint8_t dataPos = 0;
    //get all mobile connected infor
    count = app_ibrt_conn_get_all_valid_mobile_info(p_mobile_info_array,BT_DEVICE_NUM + 1);

    /* header */
    data[dataPos++] = 0x0b;
    data[dataPos++] = 0x90;
    data[dataPos++] = 0x01;
    data[dataPos++] = 0x00;
    data[dataPos++] = 0x22;
    data[dataPos++] = 0x00;
    data[dataPos++] = count;
    /* mobile a rssi 7 */
    p_mobile_info = p_mobile_info_array[0];
    data[dataPos++] = p_mobile_info==NULL?0:p_mobile_info->raw_rssi.agc_idx0;
    data[dataPos++] = p_mobile_info==NULL?0:p_mobile_info->raw_rssi.rssi0;
    data[dataPos++] = p_mobile_info==NULL?0:p_mobile_info->raw_rssi.rssi0_max;
    data[dataPos++] = p_mobile_info==NULL?0:p_mobile_info->raw_rssi.rssi0_min;

    /* tws rssi - local 11 */
    data[dataPos++] = p_ibrt_ctrl->raw_rssi.agc_idx0;
    data[dataPos++] = p_ibrt_ctrl->raw_rssi.rssi0;
    data[dataPos++] = p_ibrt_ctrl->raw_rssi.rssi0_max;
    data[dataPos++] = p_ibrt_ctrl->raw_rssi.rssi0_min;
    data[dataPos++] = p_ibrt_ctrl->raw_rssi.ser;
    data[dataPos++] = 0;//faidx;

    /* mobile b rssi 17*/
    p_mobile_info = p_mobile_info_array[1];
    data[dataPos++] = p_mobile_info==NULL?0:p_mobile_info->raw_rssi.agc_idx0;
    data[dataPos++] = p_mobile_info==NULL?0:p_mobile_info->raw_rssi.rssi0;
    data[dataPos++] = p_mobile_info==NULL?0:p_mobile_info->raw_rssi.rssi0_max;
    data[dataPos++] = p_mobile_info==NULL?0:p_mobile_info->raw_rssi.rssi0_min;

    /* mobile a rssi peer 21 */
    p_mobile_info = p_mobile_info_array[0];
    data[dataPos++] = p_mobile_info==NULL?0:p_mobile_info->peer_raw_rssi.agc_idx0;
    data[dataPos++] = p_mobile_info==NULL?0:p_mobile_info->peer_raw_rssi.rssi0;
    data[dataPos++] = p_mobile_info==NULL?0:p_mobile_info->peer_raw_rssi.rssi0_max;
    data[dataPos++] = p_mobile_info==NULL?0:p_mobile_info->peer_raw_rssi.rssi0_min;

    /* tws rssi - peer 25 */
    data[dataPos++] = p_ibrt_ctrl->raw_rssi.agc_idx0;
    data[dataPos++] = p_ibrt_ctrl->raw_rssi.rssi0;
    data[dataPos++] = p_ibrt_ctrl->raw_rssi.rssi0_max;
    data[dataPos++] = p_ibrt_ctrl->raw_rssi.rssi0_min;
    data[dataPos++] = p_ibrt_ctrl->raw_rssi.ser;
    data[dataPos++] = 0;//faidx;

    /* mobile b rssi peer */
    p_mobile_info = p_mobile_info_array[1];
    data[dataPos++] = p_mobile_info==NULL?0:p_mobile_info->peer_raw_rssi.agc_idx0;
    data[dataPos++] = p_mobile_info==NULL?0:p_mobile_info->peer_raw_rssi.rssi0;
    data[dataPos++] = p_mobile_info==NULL?0:p_mobile_info->peer_raw_rssi.rssi0_max;
    data[dataPos++] = p_mobile_info==NULL?0:p_mobile_info->peer_raw_rssi.rssi0_min;

    TRACE(10,"tws rssi: %d %d %d %d %d : %d %d %d %d %d",
        data[11], data[12], data[13], data[14], data[15],
        data[25], data[26], data[27], data[28], data[29]);

    TRACE(9,"count(%d) mobile(%d) rssi: %d %d %d %d : %d %d %d %d", data[6], 0,
        data[7],  data[8],  data[9],  data[10],
        data[21], data[22], data[23], data[24]);
    TRACE(9,"mobile(%d) rssi: %d %d %d %d : %d %d %d %d", 1,
        data[17], data[18], data[19], data[20],
        data[31], data[32], data[33], data[34]);

    data[dataPos++] = p_ibrt_ctrl->nv_role;
    *data_len = dataPos;
}

#if 0
void app_ibrt_rssi_get_stutter(uint8_t * data,uint32_t * data_len)
{
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    ibrt_mobile_info_t *p_mobile_info_array[BT_DEVICE_NUM + 1];
    ibrt_mobile_info_t *p_mobile_info =  NULL;
    uint8_t count = 0;

    //get all mobile connected infor
    count = app_ibrt_conn_get_all_valid_mobile_info(p_mobile_info_array,BT_DEVICE_NUM + 1);
    if ( count == 0 )
    {
        TRACE(0, "app_ibrt_rssi_get_stutter: no mobile info");
    }
    else
    {
        p_mobile_info = p_mobile_info_array[0];
    }
    uint8_t temp;
    /* mobile rssi */
    for (uint8_t i = 0; i < count; i ++)
    {
        p_mobile_info = p_mobile_info_array[i];
        data[0] = p_mobile_info->raw_rssi.agc_idx0;
        data[1] = p_mobile_info->raw_rssi.rssi0;
        data[2] = p_mobile_info->raw_rssi.rssi0_max;
        data[3] = p_mobile_info->raw_rssi.rssi0_min;
        
        data[4] = p_mobile_info->raw_rssi.agc_idx1;
        data[5] = p_mobile_info->raw_rssi.rssi1;
        data[6] = p_mobile_info->raw_rssi.rssi1_max;
        data[7] = p_mobile_info->raw_rssi.rssi1_min;
        data[8] = p_mobile_info->raw_rssi.ser;
        
        data[9] = p_mobile_info->peer_raw_rssi.agc_idx0;
        data[10] = p_mobile_info->peer_raw_rssi.rssi0;
        data[11] = p_mobile_info->peer_raw_rssi.rssi0_max;
        data[12] = p_mobile_info->peer_raw_rssi.rssi0_min;
        
        data[13] = p_mobile_info->peer_raw_rssi.agc_idx1;
        data[14] = p_mobile_info->peer_raw_rssi.rssi1;
        data[15] = p_mobile_info->peer_raw_rssi.rssi1_max;
        data[16] = p_mobile_info->peer_raw_rssi.rssi1_min;
        data[17] = p_mobile_info->peer_raw_rssi.ser;
    }

    g_stutter.sample_ref = a2dp_audio_get_sample_reference();
    g_stutter.cur_buf_size_l = a2dp_audio_get_current_buf_size();

    //TRACE(2,"diff us: mobile:%d,tws:%d.", p_mobile_info->mobile_diff_us, p_mobile_info->tws_diff_us);
    TRACE(2,"diff us: mobile:%d,tws:%d. not supprt now", 0, 0);

    TRACE(19,"%d %d %d %d : %d %d %d %d %d/100 <> %d %d %d %d : %d %d %d %d %d/100 %d",
          p_mobile_info->raw_rssi.agc_idx0,
          p_mobile_info->raw_rssi.rssi0,
          p_mobile_info->raw_rssi.rssi0_max,
          p_mobile_info->raw_rssi.rssi0_min,
          p_mobile_info->raw_rssi.agc_idx1,
          p_mobile_info->raw_rssi.rssi1,
          p_mobile_info->raw_rssi.rssi1_max,
          p_mobile_info->raw_rssi.rssi1_min,
          p_mobile_info->raw_rssi.ser,
          p_mobile_info->peer_raw_rssi.agc_idx0,
          p_mobile_info->peer_raw_rssi.rssi0,
          p_mobile_info->peer_raw_rssi.rssi0_max,
          p_mobile_info->peer_raw_rssi.rssi0_min,
          p_mobile_info->peer_raw_rssi.agc_idx1,
          p_mobile_info->peer_raw_rssi.rssi1,
          p_mobile_info->peer_raw_rssi.rssi1_max,
          p_mobile_info->peer_raw_rssi.rssi1_min,
          p_mobile_info->peer_raw_rssi.ser,
          p_mobile_info->current_role);

    if (p_mobile_info->current_role == IBRT_SLAVE)
    {
        for (int i = 0; i < 9; i++)
        {
            temp = data[i + 9];
            data[i + 9] = data[i];
            data[i] = temp;
        }
    }
    data[18] = p_mobile_info->current_role;

    data[19] = bt_drv_reg_op_fa_gain_direct_get();
    data[20] = g_stutter.cur_buf_size_l;  //L buffer size
    data[21] = 0;//p_mobile_info->cur_buf_size; //R buffer size
    //sample_reference
    data[22] = *((int8_t *)&g_stutter.sample_ref);
    data[23] = *((int8_t *)&g_stutter.sample_ref + 1);
    data[24] = *((int8_t *)&g_stutter.sample_ref + 2);
    data[25] = *((int8_t *)&g_stutter.sample_ref + 3);
    //mobile diff / us
    data[26] = 0;//*((int8_t *)&p_ibrt_ctrl->mobile_diff_us);
    data[27] = 0;//*((int8_t *)&p_ibrt_ctrl->mobile_diff_us + 1);
    data[28] = 0;//*((int8_t *)&p_ibrt_ctrl->mobile_diff_us + 2);
    data[29] = 0;//*((int8_t *)&p_ibrt_ctrl->mobile_diff_us + 3);
    //tws diff / us
    data[30] = 0;//*((int8_t *)&p_ibrt_ctrl->tws_diff_us);
    data[31] = 0;//*((int8_t *)&p_ibrt_ctrl->tws_diff_us + 1);
    data[32] = 0;//*((int8_t *)&p_ibrt_ctrl->tws_diff_us + 2);
    data[33] = 0;//*((int8_t *)&p_ibrt_ctrl->tws_diff_us + 3);

    //L error_sum
    data[34] = *((int8_t *)&p_mobile_info->raw_rssi.ser);
    data[35] = *((int8_t *)&p_mobile_info->raw_rssi.ser+1);
    //L Total_sum
    data[36] = *((int8_t *)&p_mobile_info->raw_rssi.rx_data_sum);
    data[37] = *((int8_t *)&p_mobile_info->raw_rssi.rx_data_sum+1);
    //R error_sum
    data[38] = *((int8_t *)&p_mobile_info->peer_raw_rssi.ser);
    data[39] = *((int8_t *)&p_mobile_info->peer_raw_rssi.ser+1);
    //R Total_sum
    data[40] = *((int8_t *)&p_mobile_info->peer_raw_rssi.rx_data_sum);
    data[41] = *((int8_t *)&p_mobile_info->peer_raw_rssi.rx_data_sum+1);

    //TRACE(1,"0x%08x", p_mobile_info->mobile_diff_us);
    //TRACE(1,"0x%08x", p_mobile_info->tws_diff_us);
    TRACE(1,"0x%08x", 0);
    TRACE(1,"0x%08x", 0);
    TRACE(1,"0x%08x", p_mobile_info->raw_rssi.ser);
    TRACE(1,"0x%08x", p_mobile_info->raw_rssi.rx_data_sum);
    TRACE(1,"0x%08x", p_mobile_info->peer_raw_rssi.ser);
    TRACE(1,"0x%08x", p_mobile_info->peer_raw_rssi.rx_data_sum);

    TRACE(23,"%d %d %d %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
          data[19],
          data[20],
          data[21],
          data[22],
          data[23],
          data[24],
          data[25],
          data[26],
          data[27],
          data[28],
          data[29],
          data[30],
          data[31],
          data[32],
          data[33],
          data[34],
          data[35],
          data[36],
          data[37],
          data[38],
          data[39],
          data[40],
          data[41]);
    *data_len = 42;
}


void app_ibrt_debug_parse(uint8_t *data, uint32_t data_len)
{
    bool do_it = false;
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    ibrt_mobile_info_t *p_mobile_info = NULL;
    uint8_t *parse_p = data;

    TRACE(2,"%s len:%d", __func__, data_len);
    DUMP8("%02x ",data, data_len);

    if (p_ibrt_ctrl->audio_chnl_sel == A2DP_AUDIO_CHANNEL_SELECT_LCHNL){
        if (*parse_p & 0x01){
            do_it = true;
        }
    }
    if (p_ibrt_ctrl->audio_chnl_sel == A2DP_AUDIO_CHANNEL_SELECT_RCHNL){
        if (*parse_p & 0x02){
            do_it = true;
        }
    }
    parse_p++;
    if (do_it){
        uint8_t funcCode = *parse_p;
        parse_p++;
        switch (funcCode)
        {
            case 1:
                if (app_tws_ibrt_mobile_link_connected()){
                    bt_drv_reg_op_dgb_link_gain_ctrl_set(p_ibrt_ctrl->mobile_conhandle, 0, *parse_p, *parse_p == 0xff ? 0 : 1);
                }else if (app_tws_ibrt_slave_ibrt_link_connected()){
                    bt_drv_reg_op_dgb_link_gain_ctrl_set(p_ibrt_ctrl->ibrt_conhandle, 0, *parse_p, *parse_p == 0xff ? 0 : 1);
                }
                break;
            case 2:
                bt_drv_reg_op_dgb_link_gain_ctrl_set(p_ibrt_ctrl->tws_conhandle, 0, *parse_p, *parse_p == 0xff ? 0 : 1);
                break;
            case 3:
                bt_drv_reg_op_fa_gain_direct_set(*parse_p);
                break;
            case 4:
                {
                    uint32_t data_format = 0;
                    data_format = be_to_host32(parse_p);
                    TRACE(1,"lowlayer_monitor len:%d", data_format);
                    if (app_tws_ibrt_mobile_link_connected()){
                        btif_me_set_link_lowlayer_monitor(btif_me_get_remote_device_by_handle(p_ibrt_ctrl->mobile_conhandle), FLAG_START_DATA, REP_FORMAT_PACKET, data_format, 0);
                    }else if (app_tws_ibrt_slave_ibrt_link_connected()){
                        btif_me_set_link_lowlayer_monitor(btif_me_get_remote_device_by_handle(p_ibrt_ctrl->ibrt_conhandle), FLAG_START_DATA, REP_FORMAT_PACKET, data_format, 0);
                    }
                }
                break;
            default:
                TRACE(1,"wrong cmd 0x%x",funcCode);
                break;
        }
    }
}
#endif

#endif

