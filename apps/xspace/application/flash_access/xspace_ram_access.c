#if defined(__XSPACE_RAM_ACCESS__)


#include "cmsis_os.h"
#include <string.h>
#include "hal_trace.h"
#include "hal_location.h"

#include "xspace_ram_access.h"
#include "export_fn_rom.h"



#if 1
extern struct REBOOT_PARAM_T reboot_param;
#define noInit_spp_mode reboot_param.noInit_backup[0]
#else
static uint32_t nv_reboot_param[2];
#define noInit_spp_mode nv_reboot_param[0]
#endif



uint32_t REBOOT_CUSTOM_PARAM_LOC noInit_backup[8];
xspace_data_cache_t REBOOT_CUSTOM_PARAM_LOC xspace_data_cache;


int xspace_data_keeper_flush(void)
{
    xspace_data_cache.crc = __export_fn_rom.crc32(0, (uint8_t *)(&xspace_data_cache), (sizeof(xspace_data_cache_t) - sizeof(uint32_t)));
    return 0;
}


#define TOUCH_KEY_VAL       (0x5aa5aa55)
int xra_write_touch_data_to_ram(uint8_t *buf,uint16_t len)
{
    int ret = 0;
    uint32_t key = TOUCH_KEY_VAL;
    xspace_data_cache_t *p_cache = &xspace_data_cache;
    uint16_t touch_data_size = sizeof(xspace_data_cache.touch_data);
    
    if(len <= touch_data_size) {
        memcpy(p_cache->touch_data, buf, len);
        p_cache->touch_key = key;
        
        XRA_TRACE(1, "write addr=0x%x, data:", (uint32_t)p_cache->touch_data);
        DUMP8("%x,", p_cache->touch_data,len);
        
        ret = xspace_data_keeper_flush();
    } else {
        XRA_TRACE(0, "len err");
        ret = -1;
    }
    
    return ret;
}

int xra_read_touch_data_from_ram(uint8_t *buf,uint16_t len)
{
    uint32_t crc;
    uint32_t key = TOUCH_KEY_VAL;
    xspace_data_cache_t *p_cache = &xspace_data_cache;
    
    crc = __export_fn_rom.crc32(0, (uint8_t *)p_cache, (sizeof(xspace_data_cache_t) - sizeof(uint32_t)));
    
    if (crc == p_cache->crc) {
        if(p_cache->touch_key == key) {
            memcpy(buf, p_cache->touch_data, len);
            XRA_TRACE(1, "read addr=0x%x, data:", (uint32_t)p_cache->touch_data);
            DUMP8("%x,",buf,len);
            return 0;
        } else {
            XRA_TRACE(1, " key fail->%x", p_cache->touch_key);
            return -1;
        }
    } else {
        XRA_TRACE(2, " start crc:%08x/%08x", p_cache->crc, crc);
    }
    return -1;
}


int xra_write_touch_back_data_to_ram(uint8_t *buf,uint16_t len)
{
    int ret = 0;
    uint32_t key = TOUCH_KEY_VAL;
    xspace_data_cache_t *p_cache = &xspace_data_cache;
 
    if(len <= sizeof(p_cache->touch_back_data)) {
        memcpy(p_cache->touch_back_data, buf, len);
        p_cache->touch_back_key = key;
        
        XRA_TRACE(1, "write addr=0x%x,data:", (uint32_t)p_cache->touch_back_data);
        DUMP8("%x,", p_cache->touch_back_data,len);
        
        ret = xspace_data_keeper_flush();
    } else {
        XRA_TRACE(0, "len err");
        ret = -1;
    }
    
    return ret;
}


int xra_get_touch_back_data_from_ram(uint8_t *buf,uint16_t len)
{
    uint32_t crc;
    uint32_t key = TOUCH_KEY_VAL;
    xspace_data_cache_t *p_cache = &xspace_data_cache;
    
    crc = __export_fn_rom.crc32(0, (uint8_t *)p_cache,(sizeof(xspace_data_cache_t) - sizeof(uint32_t)));
    if (crc == p_cache->crc) {
        if(p_cache->touch_back_key == key) {
            memcpy(buf, p_cache->touch_back_data, len);
            XRA_TRACE(1, "read addr=0x%x,data:",(uint32_t)p_cache->touch_back_data);
            DUMP8("%x,",buf,len);
            return 0;
        } else {
            XRA_TRACE(1, " key fail->%x", p_cache->touch_back_key);
            return -1;
        }
    } else {
        XRA_TRACE(2, " start crc:%08x/%08x", p_cache->crc, crc);
    }
    return -1;
}



int xra_write_box_version_data_to_ram(uint8_t *ver, uint32_t len)
{
    int ret = 0;
    xspace_data_cache_t *p_cache = &xspace_data_cache;
    
    XRA_TRACE(1, "%s\n",__func__);
    p_cache->box_ver_data[0] = 0x5a;
    memcpy(&p_cache->box_ver_data[1], ver, len);
    ret = xspace_data_keeper_flush();
    return ret;
}

int xra_get_box_version_data_from_ram(uint8_t *ver, uint32_t len)
{    
    uint32_t crc;
    xspace_data_cache_t *p_cache = &xspace_data_cache;
    
    crc = __export_fn_rom.crc32(0, (uint8_t *)p_cache, (sizeof(xspace_data_cache_t) - sizeof(uint32_t)));
    XRA_TRACE(2, " start crc:%08x/%08x", p_cache->crc, crc);
    
    if (crc == p_cache->crc) {
        if(p_cache->box_ver_data[0] == 0x5a) {
             memcpy(ver,&p_cache->box_ver_data[1],len);
             XRA_TRACE(1, " ver len=%x",len);
             return 0;
         }  else {
             XRA_TRACE(1, " data fail->%x", p_cache->box_ver_data[0]);
             return -1;
         }
    }
    return -1;

}


int xra_write_earphone_battery_data_to_ram(uint32_t cur_percent,uint32_t dump_energy)
{
    int ret = 0;
    xspace_data_cache_t *p_cache = &xspace_data_cache;
    
    XRA_TRACE(1, "%s\n",__func__);
    p_cache->batterry_data[0] = cur_percent;
    p_cache->batterry_data[1] = dump_energy;
    p_cache->batterry_data[2] = 0x5aa5aa55;
    ret = xspace_data_keeper_flush();

    XRA_TRACE(3, "data[0]:%d data[1]:%d data[2]:0x%08x\n",
                            p_cache->batterry_data[0], p_cache->batterry_data[1], p_cache->batterry_data[2]);
    
    return ret;
}


int xra_get_earphone_battery_data_from_ram(uint32_t *cuur_percent,uint32_t *dump_energy)
{
    xspace_data_cache_t *p_cache = &xspace_data_cache;
    uint32_t crc;
    crc = __export_fn_rom.crc32(0, (uint8_t *)p_cache, (sizeof(xspace_data_cache_t) - sizeof(uint32_t)));
    XRA_TRACE(2, " start crc:%08x/%08x", p_cache->crc, crc);
    if (crc == p_cache->crc) {
        if(p_cache->batterry_data[2] == 0x5aa5aa55) {
            *cuur_percent=p_cache->batterry_data[0];
            *dump_energy=p_cache->batterry_data[1];
            XRA_TRACE(2, " cuur_percent=%d,dump_energy=%d", *cuur_percent,*dump_energy);
            return 0;
        } else {
            XRA_TRACE(1, " data fail->%x",p_cache->batterry_data[2]);
            return -1;
        }
    }
    return -1;
}

int xra_write_box_battery_data_to_ram(uint32_t bat,uint32_t time)
{
    int ret = 0;
    xspace_data_cache_t *p_cache = &xspace_data_cache;
    
    
    XRA_TRACE(1, "%s\n",__func__);
    p_cache->box_batterry_data[0] = bat;
    p_cache->box_batterry_data[1] = time;
    p_cache->box_batterry_data[2]= 0x5aa5aa55;
    ret = xspace_data_keeper_flush();
    
    return ret;
}

int xra_get_box_battery_data_from_ram(uint32_t *bat,uint32_t *time)
{
    xspace_data_cache_t *p_cache = &xspace_data_cache;
    uint32_t crc;
    crc = __export_fn_rom.crc32(0,(uint8_t *)p_cache, (sizeof(xspace_data_cache_t) - sizeof(uint32_t)));
    XRA_TRACE(2, " start crc:%08x/%08x", p_cache->crc, crc);
    
    if (crc == p_cache->crc) {
        if(p_cache->box_batterry_data[2] == 0x5aa5aa55) {
            *bat=p_cache->box_batterry_data[0];
            *time=p_cache->box_batterry_data[1];
            XRA_TRACE(2, " :bat=%d,time=%d", *bat,*time);
            return 0;
        } else {
            XRA_TRACE(1, " data fail->%x", p_cache->batterry_data[2]);
            return -1;
        }
    }
    return -1;
}

int xra_clear_peer_version_data_in_ram( void )
{
    int ret=0;
    xspace_data_cache_t *p_cache = &xspace_data_cache;

    XRA_TRACE(1, "%s",__func__);

    memset(p_cache->peer_ver_data, 0xFF, sizeof(p_cache->peer_ver_data));
    ret = xspace_data_keeper_flush();

    return ret;
}

int xra_write_peer_version_data_to_ram( uint8_t *ver, uint32_t len)
{
    int ret=0;
    xspace_data_cache_t *p_cache = &xspace_data_cache;
    
    XRA_TRACE(1, "%s",__func__);

    if(len>=10)
        return -2;

    p_cache->peer_ver_data[0] = 0x5a;
    memcpy(&p_cache->peer_ver_data[1],ver,len);
    
    ret = xspace_data_keeper_flush();
    
    return ret;
}

int xra_get_peer_version_data_from_ram( uint8_t *ver, uint32_t len)
{
    xspace_data_cache_t *p_cache = &xspace_data_cache;
    uint32_t crc;
    
    XRA_TRACE(1, "%s", __func__);
    if(len>=10)
        return -2;
    
    crc = __export_fn_rom.crc32(0, (uint8_t *)p_cache, (sizeof(xspace_data_cache_t) - sizeof(uint32_t)));
    XRA_TRACE(2, " start crc:%08x/%08x", p_cache->crc, crc);
    
    if (crc == p_cache->crc) {
        if(p_cache->peer_ver_data[0] == 0x5a) {
            memcpy(ver,&p_cache->peer_ver_data[1],len);
            XRA_TRACE(1, " :ver len=%x", len);
            return 0;
        } else {
            XRA_TRACE(1, " data fail->%x", p_cache->peer_ver_data[0]);
            return -1;
        }
    }
    return -1;
}

int xra_clear_key_func_data_in_ram( void )
{
    int ret = -1;
    xspace_data_cache_t *p_cache = &xspace_data_cache;

    XRA_TRACE(1, "%s",__func__);

    memset(p_cache->key_func_data, 0xFF, sizeof(p_cache->key_func_data));
    ret = xspace_data_keeper_flush();
    return ret;
}

int xra_write_key_func_data_to_ram( uint8_t left, uint8_t right,uint8_t update)
{
    int ret = -1;
    xspace_data_cache_t *p_cache = &xspace_data_cache;
    
    XRA_TRACE(1, "%s",__func__);

    p_cache->key_func_data[0] = 0x5a;
    p_cache->key_func_data[1] = left;
    p_cache->key_func_data[2] = right;
    p_cache->key_func_data[3] = update;
    ret = xspace_data_keeper_flush();
    return ret;
}

int xra_get_key_func_data_form_ram( uint8_t *left, uint8_t *right,uint8_t *update)
{
    xspace_data_cache_t *p_cache = &xspace_data_cache;    
    uint32_t crc;

    XRA_TRACE(1, "%s",__func__);

    
    crc = __export_fn_rom.crc32(0, (uint8_t *)p_cache, (sizeof(xspace_data_cache_t) - sizeof(uint32_t)));
    XRA_TRACE(2, " start crc:%08x/%08x", p_cache->crc, crc);
    if (crc == p_cache->crc) {
        if(p_cache->key_func_data[0] == 0x5a) {
            *left = p_cache->key_func_data[1];
            *right = p_cache->key_func_data[2];
            *update = p_cache->key_func_data[3];
            XRA_TRACE(3, " left:%d,right:%d,update:%d", *left,*right,*update);
            return 0;
        } else {
            XRA_TRACE(1, " data fail->%x", p_cache->key_func_data[0]);
            return -1;
        }
    }
    return -1;
}


/////////////////////////boot ram/////////////////////////////////////////////
int xra_spp_mode_write_noInit_data(uint32_t spp_mod)
{
    int ret=0;
    xspace_data_cache_t *p_cache = &xspace_data_cache;    
    
    
    XRA_TRACE(1, "%s", __func__);
    p_cache->spp_mode = spp_mod;    
    ret = xspace_data_keeper_flush();
    XRA_TRACE(1, " spp_mode=%x", p_cache->spp_mode);
    
    return ret;
}

uint32_t xra_spp_mode_read_noInit_data(void)
{
    xspace_data_cache_t *p_cache = &xspace_data_cache;    

    XRA_TRACE(1, " spp_mode%x", p_cache->spp_mode);
    return p_cache->spp_mode;
}
/////////////////////////boot ram/////////////////////////////////////////////




#endif  /* __XSPACE_RAM_ACCESS__ */

