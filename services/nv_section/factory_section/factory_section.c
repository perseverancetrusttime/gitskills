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
#include "cmsis.h"
#include "plat_types.h"
#include "tgt_hardware.h"
#include "string.h"
#include "crc32_c.h"
#include "factory_section.h"
#include "pmu.h"
#include "hal_trace.h"
#include "hal_norflash.h"
#include "norflash_api.h"
#include "heap_api.h"

#if defined(__XSPACE_UI__)
#include <string.h>
#include <stdio.h>
#include "xspace_dev_info.h"

static char bt_name[63];
#endif
extern uint32_t __factory_start[];
extern uint32_t __factory_end[];

#if defined(__XSPACE_BT_NAME__)
extern int nvrec_dev_get_xtal_fcap(unsigned int *xtal_fcap);
#endif
static factory_section_t *factory_section_p = NULL;
static uint8_t nv_record_dev_rev = nvrec_current_version;

    

int factory_section_get_buff(uint8_t **buff, uint32_t size)
{
 #if defined(__PROJ_XSPACE__) 
    static uint32_t sec_mem[0x1000/4];
    *buff = (uint8_t *)sec_mem;
    ASSERT (size <= 0x1000, "factory section pool in shortage! To allocate size %d but free size %d.",
        size, 0x1000);
    return 0x1000;
 #else
    syspool_init();
    return syspool_get_buff(buff, size);
#endif    
}

static void factory_callback(void* param)
{
    NORFLASH_API_OPERA_RESULT *opera_result;

    opera_result = (NORFLASH_API_OPERA_RESULT*)param;

    TRACE(5,"%s:type = %d, addr = 0x%x,len = 0x%x,result = %d,suspend_num=%d.",
                __func__,
                opera_result->type,
                opera_result->addr,
                opera_result->len,
                opera_result->result,
                opera_result->suspend_num);
}

void factory_section_init(void)
{
    enum NORFLASH_API_RET_T result;
    uint32_t sector_size = 0;
    uint32_t block_size = 0;
    uint32_t page_size = 0;

    hal_norflash_get_size(HAL_FLASH_ID_0,
               NULL,
               &block_size,
               &sector_size,
               &page_size);
    result = norflash_api_register(NORFLASH_API_MODULE_ID_FACTORY,
                    HAL_FLASH_ID_0,
                    ((uint32_t)__factory_start),
                    (uint32_t)__factory_end - (uint32_t)__factory_start,
                    block_size,
                    sector_size,
                    page_size,
                    FACTORY_SECTOR_SIZE,
                    factory_callback
                    );
    ASSERT(result == NORFLASH_API_OK,"nv_record_init: module register failed! result = %d.",result);
}
#if defined(__XSPACE_UI__)  				
uint8_t g_xpsace_ui_current_eq_index = 0; 
#endif

int factory_section_open(void)
{
    factory_section_p = (factory_section_t *)__factory_start;

    if (factory_section_p->head.magic != nvrec_dev_magic){
        factory_section_p = NULL;
        return -1;
    }
    if ((factory_section_p->head.version < nvrec_mini_version) ||
    	(factory_section_p->head.version > nvrec_current_version))
    {
        factory_section_p = NULL;
        return -1;
    }

    nv_record_dev_rev = factory_section_p->head.version;

	if (1 == nv_record_dev_rev)
	{
#if defined(__XSPACE_UI__)
        uint32_t len = sizeof(factory_section_t)-2-2-4-(5+63+2+2+2+1+8+1+1)*sizeof(int);
#else
        uint32_t len = sizeof(factory_section_t)-2-2-4-(5+63+2+2+2+1+8)*sizeof(int);
#endif
	    if (factory_section_p->head.crc !=
		   	crc32_c(0,(unsigned char *)(&(factory_section_p->head.reserved0)), len)){

	        factory_section_p = NULL;
	        return -1;
	    }

		memcpy(bt_addr, factory_section_p->data.bt_address, 6);
		memcpy(ble_addr, factory_section_p->data.ble_address, 6);
		TRACE(2,"%s sucess btname:%s", __func__, factory_section_p->data.device_name);
    }
    else
	{
		// check the data length
		if (((uint32_t)(&((factory_section_t *)0)->data.rev2_reserved0)+
			factory_section_p->data.rev2_data_len) > 4096)
		{
			TRACE(1,"nv rec dev data len %d has exceeds the facory sector size!.",
				factory_section_p->data.rev2_data_len);
			return -1;
		}

		if (factory_section_p->data.rev2_crc !=
			crc32_c(0,(unsigned char *)(&(factory_section_p->data.rev2_reserved0)),
			factory_section_p->data.rev2_data_len)){
	        factory_section_p = NULL;
	        return -1;
	    }


		memcpy(bt_addr, factory_section_p->data.rev2_bt_addr, 6);
		memcpy(ble_addr, factory_section_p->data.rev2_ble_addr, 6);
		TRACE(2,"%s sucess btname:%s", __func__, (char *)factory_section_p->data.rev2_bt_name);
	}

#if defined(__XSPACE_UI__)
    unsigned int ear_local_value = 0;
    factory_section_ear_india_get(&ear_local_value);

    if(0 == ear_local_value)  //other local eq
    {
        g_xpsace_ui_current_eq_index = 0;  
    }
    else if(1 == ear_local_value)	//India local eq
    {
        g_xpsace_ui_current_eq_index = 1;  
    }
    else							//0xff
    {
        g_xpsace_ui_current_eq_index = 0;   //default India eq 
    }   
#endif

#if defined(__XSPACE_BT_NAME__)
	unsigned int xtal_fcap = 0;
	nvrec_dev_get_xtal_fcap(&xtal_fcap);
    memset(bt_name, 0x00, sizeof(bt_name));
    memcpy(bt_name, xui_bt_init_name, strlen(xui_bt_init_name));

    sprintf(bt_name + strlen(bt_name),
            "_V%d.%d.%d.%d_%02x%02x%02x_C%02x",
            (int)xui_dev_sw_version[0],
            (int)xui_dev_sw_version[1],
            (int)xui_dev_sw_version[2],
            (int)xui_dev_sw_version[3],
            (int)bt_addr[0],
            (int)bt_addr[1],
            (int)bt_addr[2],
            (int)xtal_fcap);

    TRACE(2, "%s success bt name:%s", __func__, bt_name);
    TRACE(1, "%s, firmware software version:", __func__);

#else
    memset(bt_name, 0x00, sizeof(bt_name));
    memcpy(bt_name, xui_bt_init_name, strlen(xui_bt_init_name));
#endif

#if defined(__XSPACE_UI__)
    memcpy((void *)ble_addr, (void *)bt_addr, 6);
#endif


    TRACE(1, "%s,local bt addr:", __func__);
    DUMP8("0x%02x,", bt_addr, 6);
    TRACE(1, "%s,local ble addr:", __func__);
    DUMP8("0x%02x,", ble_addr, 6);

    return 0;
}

uint8_t* factory_section_get_bt_address(void)
{
    if (factory_section_p)
    {
        if (1 == nv_record_dev_rev)
        {
            return (uint8_t *)&(factory_section_p->data.bt_address);
        }
        else
        {
            return (uint8_t *)&(factory_section_p->data.rev2_bt_addr);
        }
    }
    else
    {
        return NULL;
    }
}

#if defined(__XSPACE_UI__)
int factory_section_set_bt_addr(uint8_t *bt_addr)
{
    uint8_t *mempool = NULL;
    uint32_t lock;
    enum NORFLASH_API_RET_T ret;

    if (!factory_section_p)
    {
        TRACE(1, "%s,factory_section_p == NULL!", __func__);
        return -1;
    }


    factory_section_get_buff((uint8_t **)&mempool, 0x1000);//modify by tanfx 20210518
	
    lock = int_lock_global();
    hal_trace_pause();	
	
    memcpy(mempool, factory_section_p, 0x1000);

    TRACE(1, "%s,config dest bt addr:", __func__);
    DUMP8("0x%02x,",bt_addr,6);

    if (1 == nv_record_dev_rev)
    {
        TRACE(1, "%s,local bt addr:", __func__);
        DUMP8("0x%02x,", ((factory_section_t *)mempool)->data.rev2_bt_addr, 6);

        memcpy(((factory_section_t *)mempool)->data.bt_address, bt_addr, 6);
        ((factory_section_t *)mempool)->head.crc = crc32_c(0,
                                                (unsigned char *)(&(((factory_section_t *)mempool)->head.reserved0)),
                                                sizeof(factory_section_t)-2-2-4);
    }
    else
    {
        TRACE(1, "%s,local bt addr:", __func__);
        DUMP8("0x%02x,", ((factory_section_t *)mempool)->data.rev2_bt_addr, 6);
        memcpy(((factory_section_t *)mempool)->data.rev2_bt_addr, bt_addr, 6);
    	((factory_section_t *)mempool)->data.rev2_crc =
    		crc32_c(0,(unsigned char *)(&(((factory_section_t *)mempool)->data.rev2_reserved0)),
			((factory_section_t *)mempool)->data.rev2_data_len);
	}

    lock = int_lock_global();
    ret = norflash_api_erase(NORFLASH_API_MODULE_ID_FACTORY,((uint32_t)__factory_start),0x1000,false);
    ASSERT(ret == NORFLASH_API_OK,"factory_section_set_bt_addr: erase failed! ret = %d.",ret);
    ret = norflash_api_write(NORFLASH_API_MODULE_ID_FACTORY,((uint32_t)__factory_start),mempool,0x1000,false);
    ASSERT(ret == NORFLASH_API_OK,"factory_section_set_bt_addr: write failed! ret = %d.",ret);

	hal_trace_continue();
    int_unlock_global(lock);

    if(memcmp(factory_section_p->data.rev2_bt_addr, bt_addr, 6) != 0)
    {
        DUMP8("%x",factory_section_p->data.rev2_bt_addr,6);
        TRACE(0, "factory_section_set_bt_addr fail");
        return -2;
    }

    TRACE(0, "factory_section_set_bt_addr success!");

    return 0;
}
#else
int factory_section_set_bt_address(uint8_t* btAddr)
{
    uint32_t lock;
    enum NORFLASH_API_RET_T ret;

    if (factory_section_p){
        TRACE(0, "Update bt addr as:");
        DUMP8("%02x ", btAddr, 6);

        uint32_t heap_size = syspool_original_size();
        uint8_t* tmpBuf = 
            (uint8_t *)(((uint32_t)syspool_start_addr()+heap_size-0x3000+FACTORY_SECTOR_SIZE-1)/FACTORY_SECTOR_SIZE*FACTORY_SECTOR_SIZE);
    
        lock = int_lock_global();
        hal_trace_pause();
        memcpy(tmpBuf, factory_section_p, FACTORY_SECTOR_SIZE);
        if (1 == nv_record_dev_rev)
        {
            memcpy(((factory_section_t *)tmpBuf)->data.bt_address,
                btAddr, 6);
            ((factory_section_t *)tmpBuf)->head.crc = 
                crc32_c(0,(unsigned char *)(&(((factory_section_t *)tmpBuf)->head.reserved0)),sizeof(factory_section_t)-2-2-4);
        }
        else
        {
            memcpy((uint8_t *)&(((factory_section_t *)tmpBuf)->data.rev2_bt_addr),
                btAddr, 6);
            ((factory_section_t *)tmpBuf)->data.rev2_crc =
                crc32_c(0,(unsigned char *)(&(((factory_section_t *)tmpBuf)->data.rev2_reserved0)),
                factory_section_p->data.rev2_data_len);
        }

        ret = norflash_api_erase(NORFLASH_API_MODULE_ID_FACTORY,(uint32_t)(__factory_start)&0x00FFFFFF,FACTORY_SECTOR_SIZE,false);
        ASSERT(ret == NORFLASH_API_OK,"%s: erase failed! ret = %d.",__FUNCTION__,ret);
        ret = norflash_api_write(NORFLASH_API_MODULE_ID_FACTORY,(uint32_t)(__factory_start)&0x00FFFFFF,(uint8_t *)tmpBuf,FACTORY_SECTOR_SIZE,false);
        ASSERT(ret == NORFLASH_API_OK,"%s: write failed! ret = %d.",__FUNCTION__,ret);

        hal_trace_continue();
        int_unlock_global(lock);

        return 0;
    }else{
        return -1;
    }
}
#endif

uint8_t* factory_section_get_bt_name(void)
{
    if (factory_section_p)
    {
#if defined(__XSPACE_UI__)
        return (uint8_t *)bt_name;   /*modify by sw007 20200506*/
#else
        if (1 == nv_record_dev_rev)
        {
            return (uint8_t *)&(factory_section_p->data.device_name);
        }
        else
        {
            return (uint8_t *)&(factory_section_p->data.rev2_bt_name);
        }
#endif
    }
    else
    {
        return (uint8_t *)BT_LOCAL_NAME;
    }
}

uint8_t* factory_section_get_ble_name(void)
{
#if defined(__XSPACE_UI__)
    return (uint8_t *)BLE_DEFAULT_NAME;
#else
	if (factory_section_p)
	{
		if (1 == nv_record_dev_rev)
		{
			return (uint8_t *)BLE_DEFAULT_NAME;
		}
		else
		{
			return (uint8_t *)&(factory_section_p->data.rev2_ble_name);
		}
	}
	else
	{
		return (uint8_t *)BLE_DEFAULT_NAME;
	}
#endif
}

uint32_t factory_section_get_version(void)
{
	if (factory_section_p)
	{
		return nv_record_dev_rev;
	}

	return 0;
}

int factory_section_xtal_fcap_get(unsigned int *xtal_fcap)
{
    if (factory_section_p){
    	if (1 == nv_record_dev_rev)
    	{
        	*xtal_fcap = factory_section_p->data.xtal_fcap;
        }
        else
        {
        	*xtal_fcap = factory_section_p->data.rev2_xtal_fcap;
        }
        return 0;
    }else{
        return -1;
    }
}

int factory_section_xtal_fcap_set(unsigned int xtal_fcap)
{
    uint8_t *mempool = NULL;
    uint32_t lock;
    enum NORFLASH_API_RET_T ret;

    if (factory_section_p){
        TRACE(1,"factory_section_xtal_fcap_set:%d", xtal_fcap);
        //syspool_init();
        //syspool_get_buff((uint8_t **)&mempool, 0x1000);
        factory_section_get_buff((uint8_t **)&mempool, 0x1000);
        memcpy(mempool, factory_section_p, 0x1000);
        if (1 == nv_record_dev_rev)
        {
        	((factory_section_t *)mempool)->data.xtal_fcap = xtal_fcap;
        	((factory_section_t *)mempool)->head.crc = crc32_c(0,(unsigned char *)(&(((factory_section_t *)mempool)->head.reserved0)),sizeof(factory_section_t)-2-2-4);
		}
		else
		{
        	((factory_section_t *)mempool)->data.rev2_xtal_fcap = xtal_fcap;
        	((factory_section_t *)mempool)->data.rev2_crc =
        		crc32_c(0,(unsigned char *)(&(factory_section_p->data.rev2_reserved0)),
				factory_section_p->data.rev2_data_len);
		}
        lock = int_lock_global();
        hal_trace_pause();

        ret = norflash_api_erase(NORFLASH_API_MODULE_ID_FACTORY,(uint32_t)(__factory_start),0x1000,false);
        ASSERT(ret == NORFLASH_API_OK,"factory_section_xtal_fcap_set: erase failed! ret = %d.",ret);
        ret = norflash_api_write(NORFLASH_API_MODULE_ID_FACTORY,(uint32_t)(__factory_start),(uint8_t *)mempool,0x1000,false);
        ASSERT(ret == NORFLASH_API_OK,"factory_section_xtal_fcap_set: write failed! ret = %d.",ret);

        hal_trace_continue();
        int_unlock_global(lock);

        return 0;
    }else{
        return -1;
    }
}

void factory_section_original_btaddr_get(uint8_t *btAddr)
{
    if(factory_section_p){
        TRACE(0,"get factory_section_p");
        if (1 == nv_record_dev_rev)
        {
            memcpy(btAddr, factory_section_p->data.bt_address, 6);
        }
        else
        {
            memcpy(btAddr, factory_section_p->data.rev2_bt_addr, 6);
        }
    }else{
        TRACE(0,"get bt_addr");
        memcpy(btAddr, bt_addr, 6);
    }
}

void factory_section_original_bleaddr_get(uint8_t *bleAddr)
{
    if (factory_section_p){
        TRACE(0, "get factory_section_p");
        if (1 == nv_record_dev_rev)
        {
        	memcpy(bleAddr, factory_section_p->data.ble_address, 6);
        }
        else
        {
        	memcpy(bleAddr, factory_section_p->data.rev2_ble_addr, 6);
        }
    } else {
        TRACE(0, "get ble_addr");
        memcpy(bleAddr, ble_addr, 6);
    }
}

int factory_section_ble_addr_set(uint8_t *ble_addr)
{
    uint8_t *mempool = NULL;
	enum NORFLASH_API_RET_T ret;
    uint32_t lock;
    
    if (factory_section_p){
        TRACE(0, "factory_section_ble_addr_set");
        factory_section_get_buff((uint8_t **)&mempool, 0x1000);
        memcpy(mempool, factory_section_p, 0x1000);

    	if (1 == nv_record_dev_rev)
        {
            memcpy(((factory_section_t *)mempool)->data.ble_address, ble_addr, 6);
            ((factory_section_t *)mempool)->head.crc = 
				crc32_c(0, (unsigned char *)(&(((factory_section_t *)mempool)->head.reserved0)), sizeof(factory_section_t)-2-2-4);

		}
		else
		{
        	 memcpy(((factory_section_t *)mempool)->data.rev2_ble_addr, ble_addr, 6);
        	((factory_section_t *)mempool)->data.rev2_crc =
        		crc32_c(0, (unsigned char *)(&(((factory_section_t *)mempool)->data.rev2_reserved0)), ((factory_section_t *)mempool)->data.rev2_data_len);
		}
		
        lock = int_lock_global();
        hal_trace_pause();  
        ret = norflash_api_erase(NORFLASH_API_MODULE_ID_FACTORY,((uint32_t)__factory_start),0x1000,false);
        ASSERT(ret == NORFLASH_API_OK,"factory_section_set_ble_addr: erase failed! ret = %d.",ret);
        ret = norflash_api_write(NORFLASH_API_MODULE_ID_FACTORY,((uint32_t)__factory_start),mempool,0x1000,false);
        ASSERT(ret == NORFLASH_API_OK,"factory_section_set_ble_addr: write failed! ret = %d.",ret);
        hal_trace_continue();
        int_unlock_global(lock);

        return 0;
    }else {
        return -1;
    }
}

// Add by alex,20200723,<end>

#if defined(__XSPACE_UI__)
int factory_section_ear_color_get(unsigned int *data)
{
    if (factory_section_p){
    	if (1 == nv_record_dev_rev)
    	{
        	*data = factory_section_p->data.rev2_ear_color;
        }
        else
        {
        	*data = factory_section_p->data.rev2_ear_color;
        }
        return 0;
    }else{
        return -1;
    }
}

int factory_section_ear_color_set(unsigned int color_data)
{
    uint8_t *mempool = NULL;
    uint32_t lock;
    enum NORFLASH_API_RET_T ret;

    if (factory_section_p){
		lock = int_lock_global();
		
        TRACE(1,"factory_section_ear_color_set:%d", color_data);
        //syspool_init();
        //syspool_get_buff((uint8_t **)&mempool, 0x1000);
        factory_section_get_buff((uint8_t **)&mempool, 0x1000);//modify by tanfx 20210518
        memcpy(mempool, factory_section_p, 0x1000);
        if (1 == nv_record_dev_rev)
        {
        	((factory_section_t *)mempool)->data.rev2_ear_color = color_data;
        	((factory_section_t *)mempool)->head.crc = crc32_c(0,(unsigned char *)(&(((factory_section_t *)mempool)->head.reserved0)),sizeof(factory_section_t)-2-2-4);
		}
		else
		{
        	((factory_section_t *)mempool)->data.rev2_ear_color = color_data;
        	((factory_section_t *)mempool)->data.rev2_crc =
        		crc32_c(0,(unsigned char *)(&(((factory_section_t *)mempool)->data.rev2_reserved0)),
				((factory_section_t *)mempool)->data.rev2_data_len);
		}
        hal_trace_pause();  

        ret = norflash_api_erase(NORFLASH_API_MODULE_ID_FACTORY,(uint32_t)(__factory_start),0x1000,false);
        ASSERT(ret == NORFLASH_API_OK,"factory_section_ear_color_set: erase failed! ret = %d.",ret);
        ret = norflash_api_write(NORFLASH_API_MODULE_ID_FACTORY,(uint32_t)(__factory_start),(uint8_t *)mempool,0x1000,false);
        ASSERT(ret == NORFLASH_API_OK,"factory_section_ear_color_set: write failed! ret = %d.",ret);
        hal_trace_continue();
        int_unlock_global(lock);

        return 0;
    }else{
        return -1;
    }
}

int factory_section_ear_india_get(unsigned int *data)
{
    if (factory_section_p){
    	if (1 == nv_record_dev_rev)
    	{
        	*data = factory_section_p->data.rev2_ear_india;
        }
        else
        {
        	*data = factory_section_p->data.rev2_ear_india;
        }
        return 0;
    }else{
        return -1;
    }
}

int factory_section_ear_india_set(unsigned int local_data)
{
    uint8_t *mempool = NULL;
    uint32_t lock;
    enum NORFLASH_API_RET_T ret;

    if (factory_section_p){
		lock = int_lock_global();
		
        TRACE(1,"factory_section_ear_india_set:%d", local_data);
        //syspool_init();
        //syspool_get_buff((uint8_t **)&mempool, 0x1000);
        factory_section_get_buff((uint8_t **)&mempool, 0x1000);//modify by tanfx 20210518
        memcpy(mempool, factory_section_p, 0x1000);
        if (1 == nv_record_dev_rev)
        {
        	((factory_section_t *)mempool)->data.rev2_ear_india = local_data;
        	((factory_section_t *)mempool)->head.crc = crc32_c(0,(unsigned char *)(&(((factory_section_t *)mempool)->head.reserved0)),sizeof(factory_section_t)-2-2-4);
		}
		else
		{
        	((factory_section_t *)mempool)->data.rev2_ear_india = local_data;
        	((factory_section_t *)mempool)->data.rev2_crc =
        		crc32_c(0,(unsigned char *)(&(((factory_section_t *)mempool)->data.rev2_reserved0)),
				((factory_section_t *)mempool)->data.rev2_data_len);
		}
		
        hal_trace_pause();  
        ret = norflash_api_erase(NORFLASH_API_MODULE_ID_FACTORY,(uint32_t)(__factory_start),0x1000,false);
        ASSERT(ret == NORFLASH_API_OK,"factory_section_ear_india_set: erase failed! ret = %d.",ret);
        ret = norflash_api_write(NORFLASH_API_MODULE_ID_FACTORY,(uint32_t)(__factory_start),(uint8_t *)mempool,0x1000,false);
        ASSERT(ret == NORFLASH_API_OK,"factory_section_ear_india_set: write failed! ret = %d.",ret);
        hal_trace_continue();
        int_unlock_global(lock);

        return 0;
    }else{
        return -1;
    }
}

uint8_t* xspace_ui_factory_section_get_bt_name(void)
{
	if (factory_section_p)
	{
#if defined(__XSPACE_UI__)
        return (uint8_t *)bt_name;  
#else
		if (1 == nv_record_dev_rev)
		{
			return (uint8_t *)&(factory_section_p->data.device_name);
		}
		else
		{
			return (uint8_t *)&(factory_section_p->data.rev2_bt_name);
		}
#endif
	}
	else
	{
		return (uint8_t *)BT_LOCAL_NAME;
	}
}

#endif
