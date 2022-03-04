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
#if defined(NEW_NV_RECORD_ENABLED)
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "nvrecord_extension.h"
#include "nvrecord_bt.h"
#include "hal_trace.h"
#include "app_a2dp.h"


//#define nv_record_verbose_log

#ifdef FPGA
nvrec_btdevicerecord g_fpga_ram_record;
void ram_record_ddbrec_init(void)
{
    g_fpga_ram_record.record.trusted = false;
}

bt_status_t ram_record_ddbrec_find(const bt_bdaddr_t* bd_ddr, nvrec_btdevicerecord **record)
{
    if (g_fpga_ram_record.record.trusted && \
        !memcmp(&g_fpga_ram_record.record.bdAddr.address[0], &bd_ddr->address[0], 6))
    {
        *record = &g_fpga_ram_record;
        return BT_STS_SUCCESS;
    }
    else
    {
        return BT_STS_FAILED;
    }
}

bt_status_t ram_record_ddbrec_add(const nvrec_btdevicerecord* param_rec)
{
    g_fpga_ram_record = *param_rec;
    g_fpga_ram_record.record.trusted = true;
    return BT_STS_SUCCESS;
}

bt_status_t ram_record_ddbrec_delete(const bt_bdaddr_t *bdaddr)
{
    if (g_fpga_ram_record.record.trusted && \
        !memcmp(&g_fpga_ram_record.record.bdAddr.address[0], &bdaddr->address[0], 6))
    {
        g_fpga_ram_record.record.trusted = false;
    }
    return BT_STS_SUCCESS;
}
#else
void ram_record_ddbrec_init(void)
{
}
#endif
void nvrecord_rebuild_paired_bt_dev_info(NV_RECORD_PAIRED_BT_DEV_INFO_T* pPairedBtInfo)
{
    memset((uint8_t *)pPairedBtInfo, 0, sizeof(NV_RECORD_PAIRED_BT_DEV_INFO_T));

    pPairedBtInfo->pairedDevNum = 0;
}

void nv_record_btdevicerecord_set_a2dp_profile_active_state(btdevice_profile* device_plf, bool isActive)
{
#ifndef FPGA
    uint32_t lock = nv_record_pre_write_operation();
    if (isActive != device_plf->a2dp_act)
    {
        nv_record_update_runtime_userdata();
    }
    device_plf->a2dp_act = isActive;

    nv_record_post_write_operation(lock);
#else
    device_plf->a2dp_act = isActive;
#endif
}

void nv_record_btdevicerecord_set_hfp_profile_active_state(btdevice_profile* device_plf, bool isActive)
{
#ifndef FPGA
    uint32_t lock = nv_record_pre_write_operation();
    if (isActive != device_plf->hfp_act)
    {
        nv_record_update_runtime_userdata();
    }
    device_plf->hfp_act = isActive;

    nv_record_post_write_operation(lock);
#else
    device_plf->hfp_act = isActive;
#endif
}

void nv_record_btdevicerecord_set_a2dp_profile_codec(btdevice_profile* device_plf, uint8_t a2dpCodec)
{
#ifndef FPGA
    uint32_t lock = nv_record_pre_write_operation();
    if (a2dpCodec != device_plf->a2dp_codectype)
    {
        nv_record_update_runtime_userdata();
    }
    device_plf->a2dp_codectype = a2dpCodec;

    nv_record_post_write_operation(lock);
#else
    device_plf->a2dp_codectype = a2dpCodec;
#endif
}

int nv_record_get_paired_dev_count(void)
{
    if (NULL == nvrecord_extension_p)
    {
        return 0;
    }

    return nvrecord_extension_p->bt_pair_info.pairedDevNum;
}

/*
return:
    -1:     enum dev failure.
    0:      without paired dev.
    1:      only 1 paired dev,store@record1.
    2:      get 2 paired dev.notice:record1 is the latest record.
*/
int nv_record_enum_latest_two_paired_dev(btif_device_record_t* record1,btif_device_record_t* record2)
{
    if((NULL == record1) || (NULL == record2) || (NULL == nvrecord_extension_p))
    {
        return -1;
    }

    if (nvrecord_extension_p->bt_pair_info.pairedDevNum > 0)
    {
        if (1 == nvrecord_extension_p->bt_pair_info.pairedDevNum)
        {
            memcpy((uint8_t *)record1, (uint8_t *)&(nvrecord_extension_p->bt_pair_info.pairedBtDevInfo[0]),
                   sizeof(btif_device_record_t));
            return 1;
        }
        else
        {
            memcpy((uint8_t *)record1, (uint8_t *)&(nvrecord_extension_p->bt_pair_info.pairedBtDevInfo[0]),
                   sizeof(btif_device_record_t));
            memcpy((uint8_t *)record2, (uint8_t *)&(nvrecord_extension_p->bt_pair_info.pairedBtDevInfo[1]),
                   sizeof(btif_device_record_t));
            return 2;
        }
    }
    else
    {
        return 0;
    }
}

/*
return:
    -1:     enum dev failure.
    0:      without paired dev.
    x:      paired device number.
*/
int nv_record_get_paired_dev_list(nvrec_btdevicerecord** record)
{
    if((NULL == record) || (NULL == nvrecord_extension_p))
    {
        return -1;
    }
    if (nvrecord_extension_p->bt_pair_info.pairedDevNum > 0)
    {
        *record = nvrecord_extension_p->bt_pair_info.pairedBtDevInfo;
        return nvrecord_extension_p->bt_pair_info.pairedDevNum;
    }
    else
    {
        return 0;
    }
}

static void nv_record_print_dev_record(const btif_device_record_t* record)
{
    TRACE(0,"nv record bdAddr = ");
    DUMP8("%02x ",record->bdAddr.address,BT_ADDR_OUTPUT_PRINT_NUM);
#ifdef nv_record_verbose_log
    TRACE(0,"record_trusted = ");
    DUMP8("%d ",&record->trusted,sizeof((uint8_t)record->trusted));
    TRACE(0,"record_for_bt_source = ");
    DUMP8("%d ",&record->trusted,sizeof((uint8_t)record->for_bt_source));
#endif
    TRACE(0,"record_linkKey = ");
    DUMP8("%02x ",record->linkKey,sizeof(record->linkKey));
#ifdef nv_record_verbose_log
    TRACE(0,"record_keyType = ");
    DUMP8("%x ",&record->keyType,sizeof(record->keyType));
    TRACE(0,"record_pinLen = ");
    DUMP8("%x ",&record->pinLen,sizeof(record->pinLen));
#endif
}

void nv_record_all_ddbrec_print(void)
{
    if (NULL == nvrecord_extension_p)
    {
        TRACE(0,"No BT paired dev.");
        return;
    }

    if (nvrecord_extension_p->bt_pair_info.pairedDevNum > 0)
    {
        for(uint8_t tmp_i=0; tmp_i < nvrecord_extension_p->bt_pair_info.pairedDevNum; tmp_i++)
        {
            btif_device_record_t record;
            bt_status_t ret_status;
            ret_status = nv_record_enum_dev_records(tmp_i, &record);
            if (BT_STS_SUCCESS == ret_status)
            {
                nv_record_print_dev_record(&record);
            }
        }
    }
    else
    {
        TRACE(0,"No BT paired dev.");
    }
}

/*
this function should be surrounded by OS_LockStack and OS_UnlockStack when call.
*/
bt_status_t nv_record_enum_dev_records(unsigned short index,btif_device_record_t* record)
{
    btif_device_record_t *recaddr = NULL;

    if((index >= nvrecord_extension_p->bt_pair_info.pairedDevNum) || (NULL == nvrecord_extension_p))
    {
        return BT_STS_FAILED;
    }

    recaddr = (btif_device_record_t *)&(nvrecord_extension_p->bt_pair_info.pairedBtDevInfo[index].record);
    memcpy(record, recaddr, sizeof(btif_device_record_t));
    //nv_record_print_dev_record(record);
    return BT_STS_SUCCESS;
}

static int8_t nv_record_get_bt_pairing_info_index(const uint8_t* btAddr)
{
    NV_RECORD_PAIRED_BT_DEV_INFO_T* pBtDevInfo =
        (NV_RECORD_PAIRED_BT_DEV_INFO_T *)(&(nvrecord_extension_p->bt_pair_info));

    for (uint8_t index = 0; index < pBtDevInfo->pairedDevNum; index++)
    {
        if (!memcmp(pBtDevInfo->pairedBtDevInfo[index].record.bdAddr.address,
                    btAddr, BTIF_BD_ADDR_SIZE))
        {
            return (int8_t)index;
        }
    }

    return -1;
}

static uint8_t LatestNewlyPairedDeviceBtAddr[BTIF_BD_ADDR_SIZE];

static void nv_record_btdevice_new_device_paired_default_callback(const uint8_t* btAddr)
{
    TRACE(0, "New device paired:");
    DUMP8("%02x ", btAddr, BT_ADDR_OUTPUT_PRINT_NUM);
    memcpy(LatestNewlyPairedDeviceBtAddr, btAddr, sizeof(LatestNewlyPairedDeviceBtAddr));
}

uint8_t* nv_record_btdevice_get_latest_paired_device_bt_addr(void)
{
    return LatestNewlyPairedDeviceBtAddr;
}

static nv_record_btdevice_new_device_paired_func_t newDevicePairedCallback = 
    nv_record_btdevice_new_device_paired_default_callback;

void nv_record_bt_device_register_newly_paired_device_callback(nv_record_btdevice_new_device_paired_func_t func)
{
    newDevicePairedCallback = func;
}

/**********************************************
this function should be surrounded by OS_LockStack and OS_UnlockStack when call.
**********************************************/
static void nv_record_pairing_info_reset(nvrec_btdevicerecord* pPairingInfo)
{
    pPairingInfo->device_vol.a2dp_vol = NVRAM_ENV_STREAM_VOLUME_A2DP_VOL_DEFAULT;
    pPairingInfo->device_vol.hfp_vol = NVRAM_ENV_STREAM_VOLUME_HFP_VOL_DEFAULT;
    pPairingInfo->device_plf.hfp_act = false;
    pPairingInfo->device_plf.a2dp_abs_vol =
        a2dp_convert_local_vol_to_bt_vol(NVRAM_ENV_STREAM_VOLUME_A2DP_VOL_DEFAULT);
    pPairingInfo->device_plf.a2dp_act = false;
    pPairingInfo->pnp_info.vend_id = 0;
    pPairingInfo->pnp_info.vend_id_source = 0;
}

static bt_status_t POSSIBLY_UNUSED nv_record_ddbrec_add(const btif_device_record_t* param_rec)
{
    if ((NULL == param_rec) || (NULL == nvrecord_extension_p))
    {
        return BT_STS_FAILED;
    }

    uint32_t lock = nv_record_pre_write_operation();

    bool isUpdateNv = false;
    bool isFlushNv = false;

    // try to find the entry
    int8_t indexOfEntry = -1;
    NV_RECORD_PAIRED_BT_DEV_INFO_T* pBtDevInfo =
        (NV_RECORD_PAIRED_BT_DEV_INFO_T *)(&(nvrecord_extension_p->bt_pair_info));
    indexOfEntry = nv_record_get_bt_pairing_info_index(param_rec->bdAddr.address);

    if (-1 == indexOfEntry)
    {
        // don't exist,  need to add to the head of the entry list
        if (MAX_BT_PAIRED_DEVICE_COUNT == pBtDevInfo->pairedDevNum)
        {
            for (uint8_t k = 0; k < MAX_BT_PAIRED_DEVICE_COUNT - 1; k++)
            {
                memcpy((uint8_t *)&(pBtDevInfo->pairedBtDevInfo[MAX_BT_PAIRED_DEVICE_COUNT - 1 - k]),
                       (uint8_t *)&(pBtDevInfo->pairedBtDevInfo[MAX_BT_PAIRED_DEVICE_COUNT - 2 - k]),
                       sizeof(nvrec_btdevicerecord));
            }
            pBtDevInfo->pairedDevNum--;
        }
        else
        {
            for (uint8_t k = 0; k < pBtDevInfo->pairedDevNum; k++)
            {
                memcpy((uint8_t *)&(pBtDevInfo->pairedBtDevInfo[pBtDevInfo->pairedDevNum - k]),
                       (uint8_t *)&(pBtDevInfo->pairedBtDevInfo[pBtDevInfo->pairedDevNum - 1 - k]),
                       sizeof(nvrec_btdevicerecord));
            }
        }

        // fill the default value
        nvrec_btdevicerecord* nvrec_pool_record = &(pBtDevInfo->pairedBtDevInfo[0]);
        memcpy((uint8_t *)&(nvrec_pool_record->record), (uint8_t *)param_rec,
               sizeof(btif_device_record_t));
        nv_record_pairing_info_reset(nvrec_pool_record);

        pBtDevInfo->pairedDevNum++;

        TRACE(0, "%s new added", __FUNCTION__);

        newDevicePairedCallback(nvrec_pool_record->record.bdAddr.address);

        isUpdateNv = true;
        // need to flush the nv record
        isFlushNv = true;
    }
    else
    {
        // exist

        bool isRecordChanged = false;
        
        // check whether the record is changed, if so, do flush immediately
        if (memcmp((uint8_t *)&(pBtDevInfo->pairedBtDevInfo[indexOfEntry].record),
                       (uint8_t *)param_rec, sizeof(btif_device_record_t)))
        {
            newDevicePairedCallback(pBtDevInfo->pairedBtDevInfo[indexOfEntry].record.bdAddr.address);
            TRACE(0, "%s used to be paired, link changed", __FUNCTION__);
            isFlushNv = true;
            isRecordChanged = true;
        }

        // check whether it's already at the head
        // if not, move it to the head
        if (indexOfEntry > 0)
        {
            nvrec_btdevicerecord record;
            memcpy((uint8_t *)&record, (uint8_t *)&(pBtDevInfo->pairedBtDevInfo[indexOfEntry]),
                   sizeof(record));

            // if not, move it to the head
            for (uint8_t k = 0; k < indexOfEntry; k++)
            {
                memcpy((uint8_t *)&(pBtDevInfo->pairedBtDevInfo[indexOfEntry - k]),
                       (uint8_t *)&(pBtDevInfo->pairedBtDevInfo[indexOfEntry - 1 - k]),
                       sizeof(nvrec_btdevicerecord));
            }

            memcpy((uint8_t *)&(pBtDevInfo->pairedBtDevInfo[0]), (uint8_t *)&record,
                   sizeof(record));

            // update the link info
            memcpy((uint8_t *)&(pBtDevInfo->pairedBtDevInfo[0].record), (uint8_t *)param_rec,
                   sizeof(btif_device_record_t));

            // need to flush the nv record
            isUpdateNv = true;
        }
        // else, check whether the link info needs to be updated
        else
        {
            if (memcmp((uint8_t *)&(pBtDevInfo->pairedBtDevInfo[0].record),
                       (uint8_t *)param_rec, sizeof(btif_device_record_t)))
            {
                // update the link info
                memcpy((uint8_t *)&(pBtDevInfo->pairedBtDevInfo[0].record), (uint8_t *)param_rec,
                       sizeof(btif_device_record_t));

                // need to flush the nv record
                isUpdateNv = true;
            }
        }

        if (isRecordChanged)
        {
            nv_record_pairing_info_reset(&(pBtDevInfo->pairedBtDevInfo[0]));
        }
    }

    if (isUpdateNv)
    {
        nv_record_update_runtime_userdata();
    }

    nv_record_post_write_operation(lock);

    if (isFlushNv)
    {
        nv_record_flash_flush();
    }

    TRACE(1,"paired Bt dev:%d", pBtDevInfo->pairedDevNum);
    TRACE(1,"isUpdateNv: %d isFlushNv: %d", isUpdateNv, isFlushNv);
    nv_record_all_ddbrec_print();

    return BT_STS_SUCCESS;
}

/*
this function should be surrounded by OS_LockStack and OS_UnlockStack when call.
*/
bt_status_t nv_record_add(SECTIONS_ADP_ENUM type, void *record)
{
    bt_status_t retstatus = BT_STS_FAILED;

    if ((NULL == record) || (section_none == type))
    {
        return BT_STS_FAILED;
    }

    switch(type)
    {
        case section_usrdata_ddbrecord:
#ifndef FPGA
            retstatus = nv_record_ddbrec_add(record);
#else
            retstatus = ram_record_ddbrec_add(record);
#endif
            break;
        default:
            break;
    }

    return retstatus;
}

/*
this function should be surrounded by OS_LockStack and OS_UnlockStack when call.
*/
bt_status_t nv_record_ddbrec_find(const bt_bdaddr_t* bd_ddr, btif_device_record_t *record)
{
    if ((NULL == bd_ddr) || (NULL == record) || (NULL == nvrecord_extension_p))
    {
        return BT_STS_FAILED;
    }

    int8_t indexOfEntry = -1;
    NV_RECORD_PAIRED_BT_DEV_INFO_T* pBtDevInfo =
        (NV_RECORD_PAIRED_BT_DEV_INFO_T *)(&(nvrecord_extension_p->bt_pair_info));
    indexOfEntry = nv_record_get_bt_pairing_info_index(bd_ddr->address);

    if (-1 == indexOfEntry)
    {
        return BT_STS_FAILED;
    }
    else
    {
        memcpy((uint8_t *)record, (uint8_t *)&(pBtDevInfo->pairedBtDevInfo[indexOfEntry].record),
               sizeof(btif_device_record_t));
        return BT_STS_SUCCESS;
    }
}

bool nv_record_get_pnp_info(bt_bdaddr_t *bdAddr, bt_dip_pnp_info_t *pPnpInfo)
{
    nvrec_btdevicerecord *record = NULL;

    TRACE(1,"%s", __func__);
    DUMP8("%x ", bdAddr->address, BTIF_BD_ADDR_SIZE);

    if (!nv_record_btdevicerecord_find(bdAddr, &record))
    {
        if (record->pnp_info.vend_id)
        {
            TRACE(2, "has the vend_id 0x%x vend_id_source 0x%x", 
                record->pnp_info.vend_id, record->pnp_info.vend_id_source);
            *pPnpInfo = record->pnp_info;
            return true;
        }
    }

    pPnpInfo->vend_id = 0;
    pPnpInfo->vend_id_source = 0;
    return false;
}

/*
this function should be surrounded by OS_LockStack and OS_UnlockStack when call.
*/
bt_status_t nv_record_ddbrec_delete(const bt_bdaddr_t *bdaddr)
{
#ifndef FPGA
    if (NULL == nvrecord_extension_p)
    {
        return BT_STS_FAILED;
    }

    int8_t indexOfEntry = -1;
    NV_RECORD_PAIRED_BT_DEV_INFO_T* pBtDevInfo =
        (NV_RECORD_PAIRED_BT_DEV_INFO_T *)(&(nvrecord_extension_p->bt_pair_info));
    indexOfEntry = nv_record_get_bt_pairing_info_index(bdaddr->address);
    if (-1 == indexOfEntry)
    {
        return BT_STS_FAILED;
    }

    uint32_t lock = nv_record_pre_write_operation();

    for (uint8_t k = 0; k < pBtDevInfo->pairedDevNum - indexOfEntry - 1; k++)
    {
        memcpy((uint8_t *)&(pBtDevInfo->pairedBtDevInfo[indexOfEntry + k]),
               (uint8_t *)&(pBtDevInfo->pairedBtDevInfo[indexOfEntry + 1 + k]),
               sizeof(nvrec_btdevicerecord));
    }

    memset((uint8_t *)&(pBtDevInfo->pairedBtDevInfo[pBtDevInfo->pairedDevNum-1]), 0,
           sizeof(nvrec_btdevicerecord));
    pBtDevInfo->pairedDevNum--;

    nv_record_update_runtime_userdata();

    nv_record_post_write_operation(lock);
#else
    ram_record_ddbrec_delete(bdaddr);
#endif
    return BT_STS_SUCCESS;
}

int nv_record_btdevicerecord_find(const bt_bdaddr_t *bd_ddr, nvrec_btdevicerecord **record)
{
#ifndef FPGA
    if ((NULL == bd_ddr) || (NULL == record) || (NULL == nvrecord_extension_p))
    {
        return -1;
    }

    int8_t indexOfEntry = -1;
    NV_RECORD_PAIRED_BT_DEV_INFO_T* pBtDevInfo =
        (NV_RECORD_PAIRED_BT_DEV_INFO_T *)(&(nvrecord_extension_p->bt_pair_info));
    indexOfEntry = nv_record_get_bt_pairing_info_index(bd_ddr->address);

    if (-1 == indexOfEntry)
    {
        return -1;
    }

    *record =
        (nvrec_btdevicerecord *)&(pBtDevInfo->pairedBtDevInfo[indexOfEntry]);
#else
    ram_record_ddbrec_find(bd_ddr, record);
#endif		
    return 0;
}

void nv_record_btdevicerecord_set_a2dp_vol(nvrec_btdevicerecord* pRecord, int8_t vol)
{
    uint32_t lock = nv_record_pre_write_operation();
    if (vol != pRecord->device_vol.a2dp_vol)
    {
        nv_record_update_runtime_userdata();
        pRecord->device_vol.a2dp_vol = vol;
    }

    nv_record_post_write_operation(lock);
}

void nv_record_btdevicerecord_set_a2dp_abs_vol(nvrec_btdevicerecord* pRecord, int8_t vol)
{
    uint32_t lock = nv_record_pre_write_operation();
    if (vol != pRecord->device_plf.a2dp_abs_vol)
    {
        nv_record_update_runtime_userdata();
        pRecord->device_plf.a2dp_abs_vol = vol;
    }

    nv_record_post_write_operation(lock);
}

void nv_record_btdevicerecord_set_hfp_vol(nvrec_btdevicerecord* pRecord, int8_t vol)
{
    uint32_t lock = nv_record_pre_write_operation();
    if (vol != pRecord->device_vol.hfp_vol)
    {
        nv_record_update_runtime_userdata();
        pRecord->device_vol.hfp_vol = vol;
    }

    nv_record_post_write_operation(lock);
}

void nv_record_btdevicevolume_set_a2dp_vol(btdevice_volume* device_vol, int8_t vol)
{
    uint32_t lock = nv_record_pre_write_operation();
    if (vol != device_vol->a2dp_vol)
    {
        nv_record_update_runtime_userdata();
        device_vol->a2dp_vol = vol;
    }

    nv_record_post_write_operation(lock);

}

void nv_record_btdevicevolume_set_hfp_vol(btdevice_volume* device_vol, int8_t vol)
{
    uint32_t lock = nv_record_pre_write_operation();
    if (vol != device_vol->hfp_vol)
    {
        nv_record_update_runtime_userdata();
        device_vol->hfp_vol = vol;
    }

    nv_record_post_write_operation(lock);

}

void nv_record_btdevicerecord_set_pnp_info(nvrec_btdevicerecord* pRecord, bt_dip_pnp_info_t* pPnpInfo)
{
    TRACE(2, "%s vend id 0x%x", __func__, pPnpInfo->vend_id);
    uint32_t lock = nv_record_pre_write_operation();
    if (pPnpInfo->vend_id != pRecord->pnp_info.vend_id)
    {
        nv_record_update_runtime_userdata();
        pRecord->pnp_info = *pPnpInfo;
    }

    nv_record_post_write_operation(lock);
}

#ifdef TOTA_CRASH_DUMP_TOOL_ENABLE
void nv_record_btdevicerecord_crash_dump(void)
{
    if (NULL == nvrecord_extension_p)
    {
        return;
    }

    if (nvrecord_extension_p->bt_pair_info.pairedDevNum > 0)
    {
        for(uint8_t tmp_i=0; tmp_i < nvrecord_extension_p->bt_pair_info.pairedDevNum; tmp_i++)
        {
            btif_device_record_t record;
            bt_status_t ret_status;
            ret_status = nv_record_enum_dev_records(tmp_i, &record);
            if (BT_STS_SUCCESS == ret_status)
            {
                CRASH_DUMP_DEBUG_TRACE(0,"nv record bdAddr = \n\r");
                DUMP8("%02x ",record.bdAddr.address,BT_ADDR_OUTPUT_PRINT_NUM);
                CRASH_DUMP_DEBUG_TRACE(0,"record_trusted = ");
                DUMP8("%d ",&record.trusted,sizeof((uint8_t)record.trusted));
                CRASH_DUMP_DEBUG_TRACE(0,"record_linkKey = ");
                DUMP8("%02x ",record.linkKey,sizeof(record.linkKey));
                CRASH_DUMP_DEBUG_TRACE(0,"record_keyType = ");
                DUMP8("%x ",&record.keyType,sizeof(record.keyType));
                CRASH_DUMP_DEBUG_TRACE(0,"record_pinLen = ");
                DUMP8("%x ",&record.pinLen,sizeof(record.pinLen));
                CRASH_DUMP_DEBUG_TRACE(1,"record_a2dp_vol = %d",nvrecord_extension_p->bt_pair_info.pairedBtDevInfo[tmp_i].device_vol.a2dp_vol);
                CRASH_DUMP_DEBUG_TRACE(1,"record_hfp_vol = %d",nvrecord_extension_p->bt_pair_info.pairedBtDevInfo[tmp_i].device_vol.hfp_vol);
                CRASH_DUMP_DEBUG_TRACE(1,"record_a2dp_codec = %d",nvrecord_extension_p->bt_pair_info.pairedBtDevInfo[tmp_i].device_plf.a2dp_codectype);
                CRASH_DUMP_DEBUG_TRACE(1,"record_a2dp_act = %d",nvrecord_extension_p->bt_pair_info.pairedBtDevInfo[tmp_i].device_plf.a2dp_act);
                CRASH_DUMP_DEBUG_TRACE(1,"record_hfp_act = %d",nvrecord_extension_p->bt_pair_info.pairedBtDevInfo[tmp_i].device_plf.hfp_act);
            }
        }
    }
}
#endif

#endif //#if defined(NEW_NV_RECORD_ENABLED)

