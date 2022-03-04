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
#include <string.h>
#include "hal_trace.h"
#include "factory_section.h"
#include "nvrecord.h"
#include "nvrecord_env.h"
#include "app_ibrt_nvrecord.h"
#include "btapp.h"

extern "C"
{
#include "ddbif.h"
}

/*****************************************************************************

Prototype    : app_ibrt_nvrecord_config_load
Description  :
Input        : void
Output       : None
Return Value :
Calls        :
Called By    :

History        :
Date         : 2019/3/29
Author       : bestechnic
Modification : Created function

*****************************************************************************/
void app_ibrt_nvrecord_config_load(void *config)
{
    struct nvrecord_env_t *nvrecord_env;
    ibrt_config_t *ibrt_config = (ibrt_config_t *)config;
    //factory_section_original_btaddr_get(ibrt_config->local_addr.address);
    nv_record_env_get(&nvrecord_env);
    if(nvrecord_env->ibrt_mode.mode != IBRT_UNKNOW)
    {
        ibrt_config->nv_role=nvrecord_env->ibrt_mode.mode;
        ibrt_config->peer_addr=nvrecord_env->ibrt_mode.record.bdAddr;
        ibrt_config->local_addr=nvrecord_env->ibrt_mode.record.bdAddr;
    }
    else
    {
        ibrt_config->nv_role=IBRT_UNKNOW;
    }
}
/*****************************************************************************

Prototype    : app_ibrt_nvrecord_find
Description  :
Input        : void
Output       : None
Return Value :
Calls        :
Called By    :

History        :
Date         : 2019/3/29
Author       : bestechnic
Modification : Created function

*****************************************************************************/
int app_ibrt_nvrecord_find(const bt_bdaddr_t *bd_ddr, nvrec_btdevicerecord **record)
{
    return nv_record_btdevicerecord_find(bd_ddr, record);
}
/*****************************************************************************

Prototype    : app_ibrt_nvrecord_update_ibrt_mode_tws
Description  : app_ibrt_nvrecord_update_ibrt_mode_tws
Input        : status
Output       : None
Return Value :
Calls        :
Called By    :

History        :
Date         : 2019/3/29
Author       : bestechnic
Modification : Created function

*****************************************************************************/
void app_ibrt_nvrecord_update_ibrt_mode_tws(bool status)
{
    struct nvrecord_env_t *nvrecord_env;

    TRACE(2,"##%s,%d",__func__,status);
    nv_record_env_get(&nvrecord_env);
    nvrecord_env->ibrt_mode.tws_connect_success = status;
    nv_record_env_set(nvrecord_env);

}
/*****************************************************************************
 Prototype    : app_ibrt_nvrecord_get_latest_mobiles_addr
 Description  : get latest mobile addr from nv
 Input        : bt_bdaddr_t *mobile_addr
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/4/25
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
int app_ibrt_nvrecord_get_latest_mobiles_addr(bt_bdaddr_t *mobile_addr1, bt_bdaddr_t* mobile_addr2)
{
    btif_device_record_t record;
    int found_mobile_addr_count = 0;
    int paired_dev_count = nv_record_get_paired_dev_count();

    /* the tail is latest, the head is oldest */
    for (int i = paired_dev_count - 1; i >= 0; --i)
    {
        if (BT_STS_SUCCESS != nv_record_enum_dev_records(i, &record))
        {
            break;
        }

        if (MOBILE_LINK == app_tws_ibrt_get_link_type_by_addr(&record.bdAddr))
        {
            found_mobile_addr_count += 1;

            if (found_mobile_addr_count == 1)
            {
                *mobile_addr1 = record.bdAddr;
            }
            else if (found_mobile_addr_count == 2)
            {
                *mobile_addr2 = record.bdAddr;
                break;
            }
        }
    }

    return found_mobile_addr_count;
}

bt_status_t app_ibrt_nvrecord_find_mobile_nv_record(const bt_bdaddr_t *mobile_addr,nvrec_btdevicerecord *nv_record)
{
    nvrec_btdevicerecord* pNvRecord;
    ibrt_link_type_e link_type;
    int record_num = nv_record_get_paired_dev_list(&pNvRecord);
    uint8_t paired_mobile_num = 0;

    if (record_num > 0)
    {
        for(uint8_t i = 0; i < record_num; i++)
        {
            link_type = app_tws_ibrt_get_link_type_by_addr(&pNvRecord[i].record.bdAddr);
            if ((link_type == MOBILE_LINK) 
                && (paired_mobile_num < app_tws_ibrt_support_max_remote_link()))
            {
                if(!memcmp((uint8_t*)&pNvRecord[i].record.bdAddr.address[0],
                    (uint8_t*)&mobile_addr->address[0],sizeof(mobile_addr->address)))
                {
                    memcpy((uint8_t*)&nv_record[paired_mobile_num],(uint8_t*)&pNvRecord[i],sizeof(nvrec_btdevicerecord));

                    return BT_STS_SUCCESS;
                }
            }
        }

        return BT_STS_NOT_FOUND;
    }
    else
    {
        return BT_STS_NOT_FOUND;
    }
}

/*****************************************************************************
 Prototype    : app_ibrt_nvrecord_get_mobile_addr
 Description  : get mobile addr from nv
 Input        : bt_bdaddr_t *mobile_addr
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/4/25
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
bt_status_t app_ibrt_nvrecord_get_mobile_addr(bt_bdaddr_t mobile_addr_list[],uint8_t *count)
{
#if !defined(FPGA)
    nvrec_btdevicerecord* pNvRecord;
    ibrt_link_type_e link_type;
    int record_num = nv_record_get_paired_dev_list(&pNvRecord);
    uint8_t pairedMobileNum = 0;

    if (record_num > 0)
    {
        for(uint8_t i = 0; i < record_num; i++)
        {
            link_type = app_tws_ibrt_get_link_type_by_addr(&pNvRecord[i].record.bdAddr);
            if ((link_type == MOBILE_LINK) 
                && (pairedMobileNum < app_tws_ibrt_support_max_remote_link()))
            {
                memcpy((uint8_t*)&mobile_addr_list[pairedMobileNum],(uint8_t*)&pNvRecord[i].record.bdAddr,BTIF_BD_ADDR_SIZE);
                pairedMobileNum++;
            }
        }
        *count = pairedMobileNum;
        return BT_STS_SUCCESS;
    }
    else
    {
        return BT_STS_NOT_FOUND;
    }
#else
    unsigned char POSSIBLY_UNUSED stub_addr_1[6] = {0x6C, 0x04, 0x5E, 0x6C, 0x66, 0x5C};
    unsigned char POSSIBLY_UNUSED stub_addr_2[6] = {0x3E, 0x44, 0x82, 0x73, 0x4D, 0x6C};
    unsigned char POSSIBLY_UNUSED stub_addr_3[6] = {0x26, 0x88, 0x2D, 0x43, 0xE1, 0xBC};

    memcpy((uint8_t*)&mobile_addr_list[0].address[0],(uint8_t*)stub_addr_1,BTIF_BD_ADDR_SIZE);
    memcpy((uint8_t*)&mobile_addr_list[1].address[0],(uint8_t*)stub_addr_2,BTIF_BD_ADDR_SIZE);
    //memcpy((uint8_t*)&mobile_addr_list[2].address[0],(uint8_t*)stub_addr_3,BTIF_BD_ADDR_SIZE);

    *count = 2;

    return BT_STS_SUCCESS;
#endif

        return BT_STS_FAILED;
}

#if defined(FPGA)
/*
 * the order of address and count is not the same in both side
*/
bt_status_t app_ibrt_nvrecord_get_mobile_addr_on_fpga(bt_bdaddr_t mobile_addr_list[],uint8_t *count,bool master)
{
    unsigned char POSSIBLY_UNUSED stub_addr_1[6] = {0x6C, 0x04, 0x5E, 0x6C, 0x66, 0x5C};
    unsigned char POSSIBLY_UNUSED stub_addr_2[6] = {0x3E, 0x44, 0x82, 0x73, 0x4D, 0x6C};
    unsigned char POSSIBLY_UNUSED stub_addr_3[6] = {0x26, 0x88, 0x2D, 0x43, 0xE1, 0xBC};

    if(master)
    {
        memcpy((uint8_t*)&mobile_addr_list[0].address[0],(uint8_t*)stub_addr_1,BTIF_BD_ADDR_SIZE);
        memcpy((uint8_t*)&mobile_addr_list[1].address[0],(uint8_t*)stub_addr_2,BTIF_BD_ADDR_SIZE);
        //memcpy((uint8_t*)&mobile_addr_list[2].address[0],(uint8_t*)stub_addr_3,BTIF_BD_ADDR_SIZE);
    }
    else
    {
        memcpy((uint8_t*)&mobile_addr_list[1].address[0],(uint8_t*)stub_addr_1,BTIF_BD_ADDR_SIZE);
        memcpy((uint8_t*)&mobile_addr_list[0].address[0],(uint8_t*)stub_addr_2,BTIF_BD_ADDR_SIZE);
        //memcpy((uint8_t*)&mobile_addr_list[2].address[0],(uint8_t*)stub_addr_3,BTIF_BD_ADDR_SIZE);
    }

    *count = 2;

    return BT_STS_SUCCESS;
}
#endif

bt_status_t app_ibrt_nvrecord_get_mobile_paired_dev_list(nvrec_btdevicerecord *nv_record,uint8_t *count)
{
    nvrec_btdevicerecord* pNvRecord;
    ibrt_link_type_e link_type;
    int record_num = nv_record_get_paired_dev_list(&pNvRecord);
    uint8_t paired_mobile_num = 0;

    if (record_num > 0)
    {
        for(uint8_t i = 0; i < record_num; i++)
        {
            link_type = app_tws_ibrt_get_link_type_by_addr(&pNvRecord[i].record.bdAddr);
            if ((link_type == MOBILE_LINK) 
                && (paired_mobile_num < app_tws_ibrt_support_max_remote_link()))
            {
                memcpy((uint8_t*)&nv_record[paired_mobile_num],(uint8_t*)&pNvRecord[i],sizeof(nvrec_btdevicerecord));
                paired_mobile_num++;
            }
        }
        TRACE(2,"paired_dev_list size = %d,%d",record_num,*count);
        *count = paired_mobile_num;
        return BT_STS_SUCCESS;
    }
    else
    {
        return BT_STS_NOT_FOUND;
    }
}

#if defined(FPGA)
bt_status_t app_ibrt_nvrecord_get_paired_dev_list_on_fpga(nvrec_btdevicerecord* record,uint8_t *count,bool master)
{
    unsigned char POSSIBLY_UNUSED stub_addr_1[6] = {0x6C, 0x04, 0x5E, 0x6C, 0x66, 0x5C};
    unsigned char POSSIBLY_UNUSED stub_addr_2[6] = {0x3E, 0x44, 0x82, 0x73, 0x4D, 0x6C};
    unsigned char POSSIBLY_UNUSED stub_addr_3[6] = {0x26, 0x88, 0x2D, 0x43, 0xE1, 0xBC};

    if(master)
    {
        memcpy((uint8_t*)&record[0].record.bdAddr.address[0],(uint8_t*)stub_addr_1,BTIF_BD_ADDR_SIZE);
        memcpy((uint8_t*)&record[1].record.bdAddr.address[0],(uint8_t*)stub_addr_2,BTIF_BD_ADDR_SIZE);
        //memcpy((uint8_t*)&record[2].record.bdAddr.address[0],(uint8_t*)stub_addr_3,BTIF_BD_ADDR_SIZE);
    }
    else
    {
        memcpy((uint8_t*)&record[1].record.bdAddr.address[0],(uint8_t*)stub_addr_1,BTIF_BD_ADDR_SIZE);
        memcpy((uint8_t*)&record[0].record.bdAddr.address[0],(uint8_t*)stub_addr_2,BTIF_BD_ADDR_SIZE);
        //memcpy((uint8_t*)&record[0].record.bdAddr.address[0],(uint8_t*)stub_addr_3,BTIF_BD_ADDR_SIZE);
    }

    *count = 2;
    return BT_STS_SUCCESS;
}
#endif


/*****************************************************************************
 Prototype    : app_ibrt_nvrecord_delete_all_mobile_record
 Description  : app_ibrt_nvrecord_delete_all_mobile_record
 Input        : None
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/4/25
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_ibrt_nvrecord_delete_all_mobile_record(void)
{
    btif_device_record_t record = {0};
    int paired_dev_count = nv_record_get_paired_dev_count();

    for (int i = paired_dev_count - 1; i >= 0; --i)
    {
        nv_record_enum_dev_records(i, &record);

        if (MOBILE_LINK == app_tws_ibrt_get_link_type_by_addr(&record.bdAddr))
        {
            nv_record_ddbrec_delete(&record.bdAddr);
        }
    }
}

int app_ibrt_nvrecord_choice_mobile_addr(bt_bdaddr_t *mobile_addr, uint8_t index)
{
    btif_device_record_t record;
    int paired_dev_count = nv_record_get_paired_dev_count();
    int mobile_found_count = 0;
    if (paired_dev_count < index + 1)
    {
        return 0;
    }

    /* the tail is latest, the head is oldest */
    for (int i = paired_dev_count - 1; i >= 0; --i)
    {
        if (BT_STS_SUCCESS != nv_record_enum_dev_records(i, &record))
        {
            break;
        }

        if (MOBILE_LINK == app_tws_ibrt_get_link_type_by_addr(&record.bdAddr))
        {
            if (mobile_found_count == index)
            {
                *mobile_addr = record.bdAddr;
                return 1;
            }

            mobile_found_count += 1;
        }
    }

    return 0;
}

