#if defined(__XSPACE_FLASH_ACCESS__)
#include "cmsis.h"
#include "plat_types.h"
#include "tgt_hardware.h"
#include "string.h"
#include "export_fn_rom.h"
#include "hal_trace.h"
#include "hal_norflash.h"
#include "norflash_api.h"
#include "heap_api.h"
#include "reboot_param.h"
#include "xspace_flash_access.h"
#include "cache.h"


#define crc32(crc, buf, len) __export_fn_rom.crc32(crc, buf, len)

extern uint32_t __xspace_parameter_start[];
extern uint32_t __xspace_parameter_end[];
static xspace_section_t *xspace_section_p = NULL;

extern uint32_t __xspace_backup_data_start[];
extern uint32_t __xspace_backup_data_end[];
static xspace_section_t *xspace_section_backup_p = NULL;

static void xspace_section_callback(void* param)
{
    NORFLASH_API_OPERA_RESULT *opera_result = (NORFLASH_API_OPERA_RESULT*)param;

    XFA_TRACE(5, "type=%d, addr=0x%x, len=0x%x, result=%d suspend_num=%d.",
            opera_result->type, opera_result->addr, opera_result->len, opera_result->result, opera_result->suspend_num);
}

int xspace_section_open(void)
{
    uint32_t lock;
    uint8_t *mempool = NULL;
    enum NORFLASH_API_RET_T ret;

    xspace_section_p = (xspace_section_t *)__xspace_parameter_start;

    if (xspace_section_p->head.magic != nvrec_xspace_dev_magic || xspace_section_p->head.version < nvrec_xspace_current_version) {
        XFA_TRACE(0, "xspace section is invalid, need restore");

        syspool_init();
        syspool_get_buff((uint8_t **)&mempool, 0x1000);
        memset(mempool, 0x00, 0x1000);
        ((xspace_section_t *)mempool)->head.magic = nvrec_xspace_dev_magic;
        ((xspace_section_t *)mempool)->head.version = nvrec_xspace_current_version;
        ((xspace_section_t *)mempool)->head.crc = crc32(0,(unsigned char *)(&(((xspace_section_t *)mempool)->head.reserved0)),sizeof(xspace_section_t) - 2 - 2 - 4);

        lock = int_lock_global();
        hal_trace_pause();
        ret = norflash_api_erase(NORFLASH_API_MODULE_ID_XSPACE_DATA,((uint32_t)__xspace_parameter_start), 0x1000, false);
        ASSERT(ret == NORFLASH_API_OK,"xspace_section_p: erase failed! ret = %d.",ret);
        ret = norflash_api_write(NORFLASH_API_MODULE_ID_XSPACE_DATA,((uint32_t)__xspace_parameter_start), mempool, 0x1000, false);
        ASSERT(ret == NORFLASH_API_OK,"xspace_section_p: write failed! ret = %d.",ret);
        hal_trace_continue();
        int_unlock_global(lock);
    }

    xspace_section_backup_p = (xspace_section_t *)__xspace_backup_data_start;
    if (xspace_section_backup_p->head.magic != nvrec_xspace_dev_magic || xspace_section_backup_p->head.version < nvrec_xspace_current_version) {
        XFA_TRACE(0, "xspace_section_backup_p is invalid, need restore");

        syspool_init();
        syspool_get_buff((uint8_t **)&mempool, 0x1000);
        memset(mempool, 0x00, 0x1000);
        ((xspace_section_t *)mempool)->head.magic = nvrec_xspace_dev_magic;
        ((xspace_section_t *)mempool)->head.version = nvrec_xspace_current_version;
        ((xspace_section_t *)mempool)->head.crc = crc32(0,(unsigned char *)(&(((xspace_section_t *)mempool)->head.reserved0)),sizeof(xspace_section_t)-2-2-4);

        lock = int_lock_global();
        hal_trace_pause();
        ret = norflash_api_erase(NORFLASH_API_MODULE_ID_XSPACE_DATA_BACKUP, ((uint32_t)__xspace_backup_data_start),0x1000,false);
        //ASSERT(ret == NORFLASH_API_OK,"xspace_section_backup_p: erase failed! ret = %d.",ret);
        XFA_TRACE(1, "xspace_section_backup_p: erase ! ret = %d.",ret);
        ret = norflash_api_write(NORFLASH_API_MODULE_ID_XSPACE_DATA_BACKUP, ((uint32_t)__xspace_backup_data_start),mempool,0x1000,false);
        //ASSERT(ret == NORFLASH_API_OK,"xspace_section_backup_p: write failed! ret = %d.",ret);
        XFA_TRACE(1, "xspace_section_backup_p: write ! ret = %d.",ret);
        hal_trace_continue();
        int_unlock_global(lock);
    }
    
    if(xspace_section_backup_p->data.touch_back_key != 0x5aa5aa55) {
        if(xspace_section_p->data.touch_back_key != 0x5aa5aa55) {
            XFA_TRACE(0, "Touch main and backup data err!");
            return -1;
        } else {
            XFA_TRACE(0, "xspace_section_p--->xspace_section_backup_p");
            syspool_init();
            syspool_get_buff((uint8_t **)&mempool, 0x1000);
            memset(mempool, 0x00, 0x1000);
            memcpy(((xspace_section_t *)mempool)->data.touch_back_data, xspace_section_p->data.touch_back_data, sizeof(xspace_section_p->data.touch_back_data));
            ((xspace_section_t *)mempool)->data.touch_back_key = xspace_section_p->data.touch_back_key;
            ((xspace_section_t *)mempool)->head.magic = nvrec_xspace_dev_magic;
            ((xspace_section_t *)mempool)->head.version = nvrec_xspace_current_version;
            ((xspace_section_t *)mempool)->head.crc = crc32(0,(unsigned char *)(&(((xspace_section_t *)mempool)->head.reserved0)),sizeof(xspace_section_t) - 2 - 2 - 4);

            lock = int_lock_global();
            hal_trace_pause();
            ret = norflash_api_erase(NORFLASH_API_MODULE_ID_XSPACE_DATA_BACKUP, ((uint32_t)__xspace_backup_data_start), 0x1000, false);
            //ASSERT(ret == NORFLASH_API_OK,"xspace_section_backup_p: erase failed! ret = %d.",ret);
            XFA_TRACE(1, "xspace_section_backup_p: erase ! ret = %d.",ret);
            ret = norflash_api_write(NORFLASH_API_MODULE_ID_XSPACE_DATA_BACKUP,((uint32_t)__xspace_backup_data_start), mempool, 0x1000, false);
            //ASSERT(ret == NORFLASH_API_OK,"xspace_section_backup_p: write failed! ret = %d.",ret);
            XFA_TRACE(1, "xspace_section_backup_p: write ! ret = %d.",ret);
            hal_trace_continue();
            int_unlock_global(lock);
        }
    } else {
        if(xspace_section_p->data.touch_back_key == 0x5aa5aa55) {
            XFA_TRACE(0, "touch main and backup data ok!");
        } else {
            XFA_TRACE(0, "xspace_section_p<---xspace_section_backup_p");
            touch_back_data_write_flash(xspace_section_backup_p->data.touch_back_data,sizeof(xspace_section_backup_p->data.touch_back_data));
        }
    }

    XFA_TRACE(0, "xspace_section_backup_p:");
    DUMP8("0x%02x;", xspace_section_backup_p->data.touch_back_data, sizeof(xspace_section_backup_p->data.touch_back_data));

    return 0;
}

void xspace_section_init(void)
{
    enum NORFLASH_API_RET_T result;
    uint32_t sector_size = 0;
    uint32_t block_size = 0;
    uint32_t page_size = 0;

    hal_norflash_get_size(HAL_FLASH_ID_0, NULL, &block_size, &sector_size, &page_size);
    result = norflash_api_register(NORFLASH_API_MODULE_ID_XSPACE_DATA,
                    HAL_FLASH_ID_0,
                    (uint32_t)__xspace_parameter_start,
                    (uint32_t)__xspace_parameter_end - (uint32_t)__xspace_parameter_start,
                    block_size,
                    sector_size,
                    page_size,
                    XSPACE_SECTOR_SIZE,
                    xspace_section_callback
                    );
    
    ASSERT(result == NORFLASH_API_OK,"xspace_section_init: module register failed! result = %d.",result);

    result = norflash_api_register(NORFLASH_API_MODULE_ID_XSPACE_DATA_BACKUP,
                    HAL_FLASH_ID_0,
                    (uint32_t)__xspace_backup_data_start,
                    (uint32_t)__xspace_backup_data_end - (uint32_t)__xspace_backup_data_start,
                    block_size,
                    sector_size,
                    page_size,
                    XSPACE_SECTOR_SIZE,
                    xspace_section_callback
                    );

    XFA_TRACE(1, "xspace_section_backup_init: module register ! result = %d.",result);
    xspace_section_open();
}

static int xspace_flash_data_copy_to_ram(uint8_t **mempool)
{
    XFA_TRACE(0, "enter! ");
    if(xspace_section_p == NULL) {
        XFA_TRACE(0, "xspace_section_p == NULL ");
        return -1;
    }

    if(mempool == NULL) {
        XFA_TRACE(0, "mempool == NUL!");
        return -2;
    }

    if(get_4k_cache_pool_used_flag() == true)
    {
        XFA_TRACE(0, "cache bufffer is busy!");
        return -3;
    }

    get_4k_cache_pool(mempool);

    memcpy(*mempool, xspace_section_p, 0x1000);
    return 0;
    
}

static int xspace_ram_data_copy_to_flash(uint8_t *mempool)
{
    enum NORFLASH_API_RET_T ret;
    uint32_t lock;
    

    ((xspace_section_t *)mempool)->head.crc = \
        crc32(0,(unsigned char *)(&(((xspace_section_t *)mempool)->head.reserved0)),sizeof(xspace_section_t)-2-2-4);

    lock = int_lock_global();
    hal_trace_pause();

    ret = norflash_api_erase(NORFLASH_API_MODULE_ID_XSPACE_DATA,((uint32_t)__xspace_parameter_start),0x1000,false);

    ASSERT(ret == NORFLASH_API_OK,"xspace_section_set_ir_calib_value: erase failed! ret = %d.",ret);
    ret = norflash_api_write(NORFLASH_API_MODULE_ID_XSPACE_DATA,((uint32_t)__xspace_parameter_start),mempool,0x1000,false);

    ASSERT(ret == NORFLASH_API_OK,"xspace_section_set_ir_calib_value: write failed! ret = %d.",ret);
    hal_trace_continue();
    int_unlock_global(lock);

    return 0;
}
///////////////touch//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int touch_data_write_flash(uint8_t *buf, uint16_t len)
{
    uint8_t *mempool = NULL;
    uint32_t key = 0x5aa5aa55;
    int ret = 0;

    XFA_TRACE(0, "enter");

    if (xspace_flash_data_copy_to_ram(&mempool) == 0) {
        if (len <= sizeof(xspace_section_p->data.touch_data)) {
            memcpy(((xspace_section_t *)mempool)->data.touch_data, buf, len);
            ((xspace_section_t *)mempool)->data.touch_key = key;

            if (xspace_ram_data_copy_to_flash(mempool) == 0) {
                memcpy(mempool, xspace_section_p->data.touch_data, len);
                XFA_TRACE(1, "write addr=0x%x,data:", (uint32_t)xspace_section_p->data.touch_data);
                DUMP8("%x,", mempool, len);
            } else {
                XFA_TRACE(0, "write fail! ");
                ret = -3;
            }
        } else {
            XFA_TRACE(0, "len(%d) > touch data size(%d) !", len, sizeof(xspace_section_p->data.touch_data));
            ret = -2;
        }
        _4k_cache_pool_free();
    } else {
        XFA_TRACE(0, "mempool error !");
        ret = -1;
    }

    XFA_TRACE(0, "exit");
    return ret;
}

int touch_data_read_flash(uint8_t *buf, uint16_t len)
{
    xspace_section_data_t temp_xspace_section;
    uint32_t key = 0x5aa5aa55;

    XFA_TRACE(0, "enter");
    if (xspace_section_p) {
        memcpy(temp_xspace_section.touch_data, xspace_section_p->data.touch_data, sizeof(temp_xspace_section.touch_data));
        memcpy(&temp_xspace_section.touch_key, &xspace_section_p->data.touch_key, sizeof(temp_xspace_section.touch_key));
        if (len <= sizeof(xspace_section_p->data.touch_data)) {
            if (temp_xspace_section.touch_key == key) {
                memcpy(buf, temp_xspace_section.touch_data, len);
            } else {
                memset(buf, 0, len);
            }
            XFA_TRACE(1, "read addr=0x%x,data:", (uint32_t)xspace_section_p->data.touch_data);
            DUMP8("%x,", buf, len);
            return 0;
        } else {
            XFA_TRACE(0, "len(%d) > touch data size(%d) !", len, sizeof(xspace_section_p->data.touch_data));
            return -1;
        }
    } else {
        XFA_TRACE(0, "xspace_section_p addr fail! \r\n");
        return -2;
    }

    return 0;
}

////////////////touch///////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////touch//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int touch_back_data_write_flash(uint8_t *buf, uint16_t len)
{
    uint8_t *mempool = NULL;
    uint32_t key = 0x5aa5aa55;
    int ret = 0;

    XFA_TRACE(0, "enter");

    if (xspace_flash_data_copy_to_ram(&mempool) == 0) {
        if (len <= sizeof(xspace_section_p->data.touch_back_data)) {
            memcpy(((xspace_section_t *)mempool)->data.touch_back_data, buf, len);
            ((xspace_section_t *)mempool)->data.touch_back_key = key;

            if (xspace_ram_data_copy_to_flash(mempool) == 0) {
                memcpy(mempool, xspace_section_p->data.touch_back_data, len);
                XFA_TRACE(1, "write addr=0x%x,data:", (uint32_t)xspace_section_p->data.touch_back_data);
                DUMP8("%x,", mempool, len);
            } else {
                XFA_TRACE(0, "write fail! ");
                ret = -3;
            }
        } else {
            XFA_TRACE(0, "len(%d) > touch_back_data size(%d) !", len, sizeof(xspace_section_p->data.touch_back_data));
            ret = -2;
        }
        _4k_cache_pool_free();
    } else {
        XFA_TRACE(0, "mempool fail! ");
        ret = -1;
    }

    return ret;
}

int touch_back_data_read_flash(uint8_t *buf, uint16_t len)
{
    xspace_section_data_t temp_xspace_section;
    uint32_t key = 0x5aa5aa55;

    XFA_TRACE(0, "enter");
    if (xspace_section_p) {
        memcpy(temp_xspace_section.touch_back_data, xspace_section_p->data.touch_back_data, sizeof(temp_xspace_section.touch_back_data));
        memcpy(&temp_xspace_section.touch_back_key, &xspace_section_p->data.touch_back_key, sizeof(temp_xspace_section.touch_back_key));

        if (len <= sizeof(xspace_section_p->data.touch_back_data)) {
            if (temp_xspace_section.touch_back_key == key) {
                memcpy(buf, temp_xspace_section.touch_back_data, len);
            } else {
                memset(buf, 0, len);
            }
            XFA_TRACE(1, "read addr=0x%x,data:", (uint32_t)xspace_section_p->data.touch_back_data);
            DUMP8("%x,", buf, len);
            return 0;
        } else {
            XFA_TRACE(0, "len(%d) > touch_back_data size(%d) !", len, sizeof(xspace_section_p->data.touch_back_data));
            return -1;
        }
    } else {
        XFA_TRACE(0, "xspace_section_p addr fail! \r\n");
        return -2;
    }

    return 0;
}

////////////////touch///////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////xspace_backup touch backup//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static int xspace_backup_flash_data_copy_to_ram(uint8_t **mempool)
{
    XFA_TRACE(0, "enter! ");
    if (xspace_section_backup_p == NULL) {
        XFA_TRACE(0, "xspace_section_backup_p addr err! ");
        return -1;
    }

    if (mempool == NULL) {
        XFA_TRACE(0, "mempool addr err!");
        return -2;
    }

    if (get_4k_cache_pool_used_flag() == true) {
        XFA_TRACE(0, "cache bufffer is busy!");
        return -3;
    }

    get_4k_cache_pool(mempool);
    memcpy(*mempool, xspace_section_backup_p, 0x1000);
    return 0;
}

static int xspace_backup_ram_data_copy_to_flash(uint8_t *mempool)
{
    enum NORFLASH_API_RET_T ret;
    uint32_t lock;

    ((xspace_section_t *)mempool)->head.crc = \
        crc32(0,(unsigned char *)(&(((xspace_section_t *)mempool)->head.reserved0)),sizeof(xspace_section_t)-2-2-4);

    lock = int_lock_global();
    hal_trace_pause();
    ret = norflash_api_erase(NORFLASH_API_MODULE_ID_XSPACE_DATA_BACKUP, ((uint32_t)__xspace_backup_data_start), 0x1000, false);
    XFA_TRACE(1, "xspace_section_backup_p: erase ! ret = %d.",ret);
    ret = norflash_api_write(NORFLASH_API_MODULE_ID_XSPACE_DATA_BACKUP, ((uint32_t)__xspace_backup_data_start), mempool, 0x1000,false);
    XFA_TRACE(1, "xspace_section_backup_p: write ! ret = %d.",ret);
    hal_trace_continue();
    int_unlock_global(lock);

    return 0;
}

int xspace_backup_touch_back_data_write_flash(uint8_t *buf,uint16_t len)
{
    uint8_t *mempool = NULL;
    uint32_t key=0x5aa5aa55;
    int ret=0;
    
    XFA_TRACE(0, "enter");

    if(xspace_backup_flash_data_copy_to_ram(&mempool) == 0) {
        if(len <= sizeof(xspace_section_backup_p->data.touch_back_data)) {
            memcpy(((xspace_section_t *)mempool)->data.touch_back_data, buf, len);
            ((xspace_section_t *)mempool)->data.touch_back_key = key;
            
            if(xspace_backup_ram_data_copy_to_flash(mempool) == 0) {
                memcpy(mempool, xspace_section_backup_p->data.touch_back_data, len);
                XFA_TRACE(1, "write addr=0x%x,data:", (uint32_t)xspace_section_backup_p->data.touch_back_data);
                DUMP8("%x,", mempool, len);
            } else {
                XFA_TRACE(0, "write fail! ");
                ret = -3;
            }
        } else {
            XFA_TRACE(0, "len(%d) > touch_back_data size(%d) !", len, sizeof(xspace_section_p->data.touch_back_data));
            ret = -2;
        }
        _4k_cache_pool_free();
    } else {
        XFA_TRACE(0, "mempool fail! ");
        ret = -1;
    }

    return ret;
}


int xspace_backup_touch_back_data_read_flash(uint8_t *buf,uint16_t len)
{
    xspace_section_data_t temp_xspace_section;
    uint32_t key=0x5aa5aa55;
    
    XFA_TRACE(0, "enter");
    if (xspace_section_backup_p) {
        memcpy(temp_xspace_section.touch_back_data, xspace_section_backup_p->data.touch_back_data, sizeof(temp_xspace_section.touch_back_data));
        memcpy(&temp_xspace_section.touch_back_key, &xspace_section_backup_p->data.touch_back_key, sizeof(temp_xspace_section.touch_back_key));
    
        if(len <= sizeof(xspace_section_backup_p->data.touch_back_data)) {
            if(buf) {
                if(temp_xspace_section.touch_back_key == key) {
                    memcpy(buf, temp_xspace_section.touch_back_data, len);
                } else {
                    memset(buf, 0, len);
                }
                XFA_TRACE(1, "read addr=0x%x,data:", (uint32_t)xspace_section_backup_p->data.touch_back_data);
                DUMP8("%x,",buf,len);
                return 0;
            } else {
                XFA_TRACE(0, "buf addr fail!");
                return -3;
            }
        } else {
            XFA_TRACE(0, "len(%d) > touch_back_data size(%d) !", len, sizeof(xspace_section_p->data.touch_back_data));
            return -1;
        }
    } else {
        XFA_TRACE(0, "xspace_section_backup_p addr fail! \r\n");
        return -2;
    }

    return 0;
}

////////////////touch///////////////////////////////////////////////////////////////////////////////////////////////////////////



/////////////////////////batterry_data/////////////////////////////////////////////
int batterry_data_write_flash(uint32_t cuur_percent,uint32_t dump_energy)
{
    uint8_t *mempool = NULL;
    uint32_t key=0x5aa5aa55;
    int ret=0;

    if(xspace_flash_data_copy_to_ram(&mempool) == 0)
    {
        ((xspace_section_t *)mempool)->data.batterry_data[0] = cuur_percent;
        ((xspace_section_t *)mempool)->data.batterry_data[1] = dump_energy;
        ((xspace_section_t *)mempool)->data.batterry_data[2] = key;
        
        if(xspace_ram_data_copy_to_flash(mempool) == 0) {
            memcpy(mempool, xspace_section_p->data.batterry_data, sizeof(xspace_section_p->data.batterry_data));
            XFA_TRACE(0, "data is:");
            DUMP8("%x,",mempool,sizeof(xspace_section_p->data.batterry_data));
        } else {
            XFA_TRACE(0, "write error");
            ret = -2;
        }
        _4k_cache_pool_free();
    } else {
        XFA_TRACE(0, "mempool error");
        ret = -1;
    }
   
    return ret;
}


int batterry_data_read_flash(uint32_t *curr_percent,uint32_t *dump_energy)
{
    xspace_section_data_t temp_xspace_section;
    uint32_t key=0x5aa5aa55;
    
    if (xspace_section_p) {
        XFA_TRACE(0, "enter!");
        memcpy(temp_xspace_section.batterry_data, xspace_section_p->data.batterry_data, sizeof(temp_xspace_section.batterry_data));
		XFA_TRACE(3, "data[0]:%d data[1]:%d data[2]:0x%08x\n",temp_xspace_section.batterry_data[0],
						 temp_xspace_section.batterry_data[1],temp_xspace_section.batterry_data[2]);
        if((temp_xspace_section.batterry_data[2] == key)
            && (temp_xspace_section.batterry_data[1] < 1000001)
            && (temp_xspace_section.batterry_data[0] < 101))
        {
            *curr_percent = temp_xspace_section.batterry_data[0];
            *dump_energy = temp_xspace_section.batterry_data[1];

            XFA_TRACE(2, "curr_percent=%d,dump_energy=%d", *curr_percent, *dump_energy);            
            batterry_data_write_flash(200, 2000000);
            return 0;
        } else {
            XFA_TRACE(0, "flash no updata!");
            return -2;
        }
    } else {
        XFA_TRACE(0, " len fail!");
        return -1;
    }
    
    return 0;
}

int hw_rst_use_bat_data_flag_write_flash(uint8_t flag)
{
    uint8_t *mempool = NULL;
    int ret = 0;

    if (xspace_flash_data_copy_to_ram(&mempool) == 0) {
        ((xspace_section_t *)mempool)->data.hw_rst_use_bat_data_flag = flag;
        if (xspace_ram_data_copy_to_flash(mempool) == 0) {
            memcpy(mempool, &xspace_section_p->data.hw_rst_use_bat_data_flag, sizeof(xspace_section_p->data.hw_rst_use_bat_data_flag));
            XFA_TRACE(0, "data is:");
            DUMP8("0x%x,", mempool, sizeof(xspace_section_p->data.hw_rst_use_bat_data_flag));
        } else {
            XFA_TRACE(0, "write err");
            ret = -2;
        }
        _4k_cache_pool_free();
    } else {
        XFA_TRACE(0, "mempool err");
        ret = -1;
    }
    return ret;
}

int hw_rst_use_bat_data_flag_read_flash(uint8_t *flag)
{
    xspace_section_data_t temp_xspace_section;

    if (xspace_section_p) {
        XFA_TRACE(0, "enter!");
        memcpy(&temp_xspace_section.hw_rst_use_bat_data_flag, &xspace_section_p->data.hw_rst_use_bat_data_flag, sizeof(temp_xspace_section.hw_rst_use_bat_data_flag));
        *flag = temp_xspace_section.hw_rst_use_bat_data_flag;
        XFA_TRACE(0, "flag=%d", *flag);
        return 0;
    } else {
        XFA_TRACE(0, "xspace_section_p fail!");
        return -1;
    }
    return 0;
}

/////////////////////////batterry_data/////////////////////////////////////////////

/////////////////////////box_batterry/////////////////////////////////////////////
int box_batterry_data_write_flash(uint32_t bat, uint32_t time)
{
    uint8_t *mempool = NULL;
    uint32_t key = 0x5aa5aa55;
    int ret = 0;

    if (xspace_flash_data_copy_to_ram(&mempool) == 0) {
        ((xspace_section_t *)mempool)->data.box_batterry_data[0] = bat;
        ((xspace_section_t *)mempool)->data.box_batterry_data[1] = time;
        ((xspace_section_t *)mempool)->data.box_batterry_data[2] = key;

        if (xspace_ram_data_copy_to_flash(mempool) == 0) {
            memcpy(mempool, xspace_section_p->data.box_batterry_data, sizeof(xspace_section_p->data.box_batterry_data));
            XFA_TRACE(0, "data is:");
            DUMP8("%x,", mempool, sizeof(xspace_section_p->data.box_batterry_data));
        } else {
            XFA_TRACE(0, "write err");
            ret = -2;
        }
        _4k_cache_pool_free();
    } else {
        XFA_TRACE(0, "mempool err");
        ret = -1;
    }

    return ret;
}

int box_batterry_data_read_flash(uint32_t *bat, uint32_t *time)
{
    xspace_section_data_t temp_xspace_section;
    uint32_t key = 0x5aa5aa55;

    if (xspace_section_p) {
        XFA_TRACE(0, "enter!");
        memcpy(temp_xspace_section.box_batterry_data, xspace_section_p->data.box_batterry_data, sizeof(temp_xspace_section.box_batterry_data));
        if (temp_xspace_section.box_batterry_data[2] == key) {
            *bat = temp_xspace_section.box_batterry_data[0];
            *time = temp_xspace_section.box_batterry_data[1];
            XFA_TRACE(2, "bat=%d,time=%d", *bat, *time);
            return 0;
        } else {
            XFA_TRACE(0, "flash no updata!");
            return -2;
        }
    } else {
        XFA_TRACE(0, " len fail!");
        return -1;
    }

    return 0;
}
/////////////////////////box_batterry/////////////////////////////////////////////

///////////////////box_version///////////////////////////////////////////////////////////////////////////////
int box_version_data_write_flash(uint8_t *ver, uint32_t len)
{
    uint8_t *mempool = NULL;
    uint32_t key = 0x5aa5aa55;
    int ret = 0;

    if (xspace_flash_data_copy_to_ram(&mempool) == 0) {
        if (len <= sizeof(xspace_section_p->data.box_ver_data)) {
            ((xspace_section_t *)mempool)->data.box_ver_key = key;
            memcpy(((xspace_section_t *)mempool)->data.box_ver_data, ver, len);

            if (xspace_ram_data_copy_to_flash(mempool) == 0) {
                memcpy(mempool, xspace_section_p->data.box_ver_data, sizeof(xspace_section_p->data.box_ver_data));
                XFA_TRACE(0, "data is:");
                DUMP8("%x,", mempool, sizeof(xspace_section_p->data.box_ver_data));
            } else {
                _4k_cache_pool_free();
                ret = -2;
            }
        } else {
            XFA_TRACE(0, "len err");
            ret = -3;
        }
        _4k_cache_pool_free();
    } else {
        XFA_TRACE(0, "mempool err");
        ret = -1;
    }

    return ret;
}

int box_version_data_read_flash(uint8_t *ver, uint32_t len)
{
    xspace_section_data_t temp_xspace_section;
    uint32_t key = 0x5aa5aa55;

    XFA_TRACE(0, "enter!");
    if (xspace_section_p) {
        memcpy(&temp_xspace_section.box_ver_key, &xspace_section_p->data.box_ver_key, sizeof(temp_xspace_section.box_ver_key));
        if (len <= sizeof(xspace_section_p->data.box_ver_data)) {
            if (temp_xspace_section.box_ver_key != key) {
                XFA_TRACE(0, "flash no updata!");
                return -3;
            }
            memcpy(ver, xspace_section_p->data.box_ver_data, len);
            XFA_TRACE(0, "data is:");
            DUMP8("%x,", ver, len);
        } else {
            XFA_TRACE(0, "len err!");
            return -2;
        }
    } else {
        XFA_TRACE(0, " xspace_section_p fail!");
        return -1;
    }

    return 0;
}
///////////////////box_version///////////////////////////////////////////////////////////////////////////////

//////////////////////peer_version//////////////////////////////////////////////////////////////////////////
int peer_version_data_clear_flash(void)
{
    uint8_t *mempool = NULL;
    uint32_t key = 0xFFFFFFFF;
    int ret = 0;

    if (xspace_flash_data_copy_to_ram(&mempool) == 0) {
        ((xspace_section_t *)mempool)->data.peer_ver_key = key;
        memset(((xspace_section_t *)mempool)->data.peer_ver_data, 0xFF, sizeof(xspace_section_p->data.peer_ver_data));

        if (xspace_ram_data_copy_to_flash(mempool) == 0) {
            memcpy(mempool, xspace_section_p->data.peer_ver_data, sizeof(xspace_section_p->data.peer_ver_data));
            XFA_TRACE(0, "data is:");
            DUMP8("%x,", mempool, sizeof(xspace_section_p->data.peer_ver_data));
        } else {
            XFA_TRACE(0, "write err");
            ret = -2;
        }
        _4k_cache_pool_free();
    } else {
        XFA_TRACE(0, "mempool err");
        ret = -1;
    }

    return ret;
}

int peer_version_data_write_flash(uint8_t *ver, uint32_t len)
{
    uint8_t *mempool = NULL;
    uint32_t key = 0x5aa5aa55;
    int ret = 0;

    if (xspace_flash_data_copy_to_ram(&mempool) == 0) {
        if (len <= sizeof(xspace_section_p->data.peer_ver_data)) {
            ((xspace_section_t *)mempool)->data.peer_ver_key = key;
            memcpy(((xspace_section_t *)mempool)->data.peer_ver_data, ver, len);

            if (xspace_ram_data_copy_to_flash(mempool) == 0) {
                memcpy(mempool, xspace_section_p->data.peer_ver_data, len);
                XFA_TRACE(0, "data is:");
                DUMP8("%x,", mempool, len);
            } else {
                XFA_TRACE(0, "write err");
                ret = -2;
            }
        } else {
            XFA_TRACE(0, "len err");
            ret = -3;
        }
        _4k_cache_pool_free();
    } else {
        XFA_TRACE(0, "mempool err");
        ret = -1;
    }
    return ret;
}

int peer_version_data_read_flash(uint8_t *ver, uint32_t len)
{
    xspace_section_data_t temp_xspace_section;
    uint32_t key = 0x5aa5aa55;

    if (xspace_section_p) {
        XFA_TRACE(0, "enter!");

        memcpy(&temp_xspace_section.peer_ver_key, &xspace_section_p->data.peer_ver_key, sizeof(temp_xspace_section.peer_ver_key));
        if (len <= sizeof(xspace_section_p->data.peer_ver_data)) {
            if (temp_xspace_section.peer_ver_key != key) {
                XFA_TRACE(0, "flash no updata!");
                return -3;
            }
            memcpy(ver, xspace_section_p->data.peer_ver_data, len);
            XFA_TRACE(0, "data is:");
            DUMP8("%x,", ver, len);
        } else {
            XFA_TRACE(0, "len err!");
            return -2;
        }
    } else {
        XFA_TRACE(0, " xspace_section_p fail!");
        return -1;
    }

    return 0;
}
//////////////////////peer_version//////////////////////////////////////////////////////////////////////////

#endif   // #if defined(__XSPACE_FLASH_ACCESS__)
#if defined(__GOODIX_DEBUG__)
extern uint16_t ustBuffer2Flash[25];
void app_gh621x_debug_info_write_flash(void)
{
	uint8_t *mempool = NULL;
	uint8_t len = 25 * 2;
	XFA_TRACE(0, "app_gh621x_debug_info_write_flash");
	if(xspace_flash_data_copy_to_ram(&mempool) == 0) 
	{
		memcpy(((xspace_section_t *)mempool)->data.gh621x_debug_buff, ustBuffer2Flash, len);
		if(xspace_ram_data_copy_to_flash(mempool) != 0)
		{
			XFA_TRACE(0, "app_gh621x_debug_info_write_flash write err");
		}
		_4k_cache_pool_free();
	}
	else
	{
		XFA_TRACE(0, "app_gh621x_debug_info_write_flash mempool err");
	}

}

void app_gh621x_debug_info_read_flash(uint16_t* param, uint8_t len)
{
	XFA_TRACE(0, "app_gh621x_debug_info_read_flash");	
	if (xspace_section_p) 
	{
		memcpy(param, xspace_section_p->data.gh621x_debug_buff, len);
	}
	else
	{
		XFA_TRACE(0, " app_gh621x_debug_info_read_flash fail!");
	}

}
#endif
