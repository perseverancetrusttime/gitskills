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
#include "bt_drv_reg_op.h"
#include "bt_drv_internal.h"
#include "bt_drv_1501_internal.h"
#include "bt_drv_interface.h"
#include "bt_drv.h"
#include "hal_sysfreq.h"
#include "hal_chipid.h"
#include "hal_trace.h"
#include "hal_cmu.h"
#include "hal_iomux.h"
#include "hal_psc.h"
#include <string.h>
#include "CrashCatcher.h"
#include "bt_drv_internal.h"
#include "bt_patch_1501_t0.h"
#include "bt_patch_1501_t0_testmode.h"
#include "bt_patch_1501_t1.h"
#include "bt_patch_1501_t1_testmode.h"

#include "bt_patch_1501_t2.h"
#include <besbt_string.h>

#ifdef __BT_RAMRUN__
#include "bt_drv_ramrun_symbol_1501_t0.h"
#else
#include "bt_drv_symbol_1501_t0.h"
#include "bt_drv_symbol_1501_t1.h"
#include "bt_drv_symbol_1501_t2.h"
#endif

/***************************************************************************
*
*Warning: Prohibit reading and writing EM memory in Bluetooth drivers
*
*****************************************************************************/

extern "C" void bes_get_hci_rx_flowctrl_info(void);
extern "C" void bes_get_hci_tx_flowctrl_info(void);
extern "C" void hal_intersys_wakeup_btcore(void);
void bt_drv_i2v_disable_sleep_for_bt_access(void);
void bt_drv_i2v_enable_sleep_for_bt_access(void);
extern bool (*app_bt_is_bt_active_mode_callback)(void);

#ifdef BT_LOG_POWEROFF
static uint32_t reg_op_clk_enb = 0;
extern "C" bool hal_intersys_get_wakeup_flag(void);

#define BT_DRV_REG_OP_CLK_ENB()  do{\
                                            if (!reg_op_clk_enb){ \
                                                if(app_bt_is_bt_active_mode_callback && !app_bt_is_bt_active_mode_callback())  \
                                                {  \
                                                    uint32_t curr_tick,temp_tick; \
                                                    hal_intersys_wakeup_btcore(); \
                                                     curr_tick = hal_sys_timer_get();  \
                                                    do{  \
                                                        hal_sys_timer_delay_us(100);\
                                                        temp_tick = hal_sys_timer_get();\
                                                        if(temp_tick - curr_tick > MS_TO_TICKS(2)) \
                                                        { \
                                                            hal_intersys_wakeup_btcore();  \
                                                            curr_tick = hal_sys_timer_get();  \
                                                }  \
                                                    }while(hal_intersys_get_wakeup_flag());  \
                                            } \
                                            } \
                                            reg_op_clk_enb++;

#define BT_DRV_REG_OP_CLK_DIB()   reg_op_clk_enb--; \
                                            if (!reg_op_clk_enb){ \
                                                bt_drv_i2v_enable_sleep_for_bt_access(); \
                                            } \
                                    }while(0);


#elif defined(__CLK_GATE_DISABLE__)
static uint32_t reg_op_clk_enb = 0;
#define BT_DRV_REG_OP_CLK_ENB()    do{\
                                        if (!reg_op_clk_enb){ \
                                            hal_cmu_bt_sys_clock_force_on(); \
                                        } \
                                        reg_op_clk_enb++;

#define BT_DRV_REG_OP_CLK_DIB()     reg_op_clk_enb--; \
                                    if (!reg_op_clk_enb){ \
                                        hal_cmu_bt_sys_clock_auto(); \
                                    } \
                                    }while(0);
#else
static uint32_t reg_op_clk_enb = 0;
#define BT_DRV_REG_OP_CLK_ENB()    do{\
                                        if (!reg_op_clk_enb){ \
                                            hal_cmu_bt_sys_clock_force_on(); \
                                            BTDIGITAL_REG(0xD0330024) |= (1<<18); \
                                            BTDIGITAL_REG(0xD0330024)&=(~(1<<5)); \
                                        } \
                                        reg_op_clk_enb++;

#define BT_DRV_REG_OP_CLK_DIB()     reg_op_clk_enb--; \
                                    if (!reg_op_clk_enb){ \
                                        BTDIGITAL_REG(0xD0330024) &= ~(1<<18); \
                                        BTDIGITAL_REG(0xD0330024)|=(1<<5); \
                                        hal_cmu_bt_sys_clock_auto(); \
                                    } \
                                    }while(0);
#endif

#define GAIN_TBL_SIZE           0x0F

static __INLINE uint32_t co_mod(uint32_t val, uint32_t div)
{
   ASSERT_ERR(div);
   return ((val) % (div));
}
#define CO_MOD(val, div) co_mod(val, div)

#ifndef __BT_RAMRUN__
struct dbg_set_ebq_test_cmd
{
    uint8_t ssr;
    uint8_t aes_ccm_daycounter;
    uint8_t bb_prot_flh_bv02;
    uint8_t bb_prot_arq_bv43;
    uint8_t enc_mode_req;
    uint8_t lmp_lih_bv126;
    uint8_t lsto_add;
    uint8_t bb_xcb_bv06;
    uint8_t bb_xcb_bv20;
    uint8_t bb_inq_pag_bv01;
    uint8_t sam_status_change_evt;
    uint8_t sam_remap;
    uint8_t ll_con_sla_bi02c;
    int16_t ll_con_sla_bi02c_offset;
    uint8_t ll_pcl_mas_sla_bv01;
    uint8_t ll_pcl_sla_bv29;
    uint8_t bb_prot_arq_bv0608;
    uint8_t lmp_lih_bv48;
    uint8_t hfp_hf_acs_bv17_i;
    uint8_t ll_not_support_phy;
    uint8_t ll_iso_hci_out_buf_num;
    uint8_t ll_iso_hci_in_sdu_len_flag;
    uint8_t ll_iso_hci_in_buf_num;
    uint8_t ll_cis_mic_present;
    uint8_t ll_cig_param_check_disable;
    uint8_t ll_total_num_iso_data_pkts;
    uint8_t ll_isoal_ready_prepare;
    uint8_t framed_cis_param;
    uint32_t cis_offset_min;
    uint8_t ll_bi_alarm_init_distance;//(in half slots)
    uint16_t lld_sync_duration_min;//(in half us)
};
#else
struct dbg_set_ebq_test_cmd
{
    uint8_t ssr;
    uint8_t aes_ccm_daycounter;
    uint8_t bb_prot_flh_bv02;
    uint8_t bb_prot_arq_bv43;
    uint8_t enc_mode_req;
    uint8_t lmp_lih_bv126;
    uint8_t lsto_add;
    uint8_t bb_xcb_bv06;
    uint8_t bb_xcb_bv20;
    uint8_t bb_inq_pag_bv01;
    uint8_t sam_status_change_evt;
    uint8_t sam_remap;
    uint8_t ll_con_sla_bi02c;
    int16_t ll_con_sla_bi02c_offset;
    uint8_t ll_pcl_mas_sla_bv01;
    uint8_t ll_pcl_sla_bv29;
    uint8_t bb_prot_arq_bv0608;
    uint8_t lmp_lih_bv48;
    uint8_t hfp_hf_acs_bv17_i;
    uint8_t ll_not_support_phy;
    uint8_t ll_iso_hci_out_buf_num;
    uint8_t ll_iso_hci_in_sdu_len_flag;
    uint8_t ll_iso_hci_in_buf_num;
    uint8_t ll_cis_mic_present;
    uint8_t ll_cig_param_check_disable;
    uint8_t ll_total_num_iso_data_pkts;
    uint32_t cis_offset_min;
    uint32_t cis_sub_event_duration_config;
    uint8_t ll_bi_alarm_init_distance;//(in half slots)
    uint16_t lld_sync_duration_min;//(in half us)
    uint16_t lld_le_duration_min;//(in half us)
    uint16_t lld_cibi_sync_win_min_distance;//(in half us)
    uint8_t iso_sub_event_duration_config;
    uint8_t ll_bis_snc_bv7_bv11;//ebq bug
};
#endif

struct dbg_set_txpwr_mode_cmd
{
    uint8_t     enable;
    //1: -6dbm, 2:-12dbm
    uint8_t     factor;
    uint8_t     bt_or_ble;//bt_or_ble  0: bt 1:ble
    uint8_t     link_id;
};

#define BT_LINK_ID_MAX  11
#define BLE_LINK_ID_MAX  10

static uint32_t bt_ram_start_addr=0;
static uint32_t hci_fc_env_addr=0;
static uint32_t ld_acl_env_addr=0;
static uint32_t bt_util_buf_env_addr=0;
static uint32_t ble_util_buf_env_addr=0;
static uint32_t ld_bes_bt_env_addr=0;
static uint32_t lc_state_addr=0;
static uint32_t task_message_buffer_addr=0;
static uint32_t lmp_message_buffer_addr=0;
static uint32_t ld_sco_env_addr=0;
static uint32_t rx_monitor_addr=0;
static uint32_t lc_env_addr=0;
static uint32_t lm_nb_sync_active_addr=0;
static uint32_t lm_env_addr=0;
static uint32_t lm_key_env_addr=0;
static uint32_t lc_sco_env_addr=0;
static uint32_t llm_env_addr=0;
static uint32_t rwip_env_addr=0;
static uint32_t g_mem_dump_ctrl_addr=0;
static uint32_t ble_rx_monitor_addr=0;
static uint32_t llc_env_addr=0;
static uint32_t rwip_rf_addr=0;
static uint32_t ld_acl_metrics_addr = 0;
static uint32_t rf_rx_hwgain_tbl_addr = 0;
static uint32_t rf_hwagc_rssi_correct_tbl_addr = 0;
static uint32_t rf_rx_gain_fixed_tbl_addr = 0;
static uint32_t hci_dbg_ebq_test_mode_addr = 0;
static uint32_t host_ref_clk_addr = 0;
static uint32_t dbg_trc_tl_env_addr = 0;
static uint32_t dbg_trc_mem_env_addr = 0;
static uint32_t dbg_bt_common_setting_addr=0;
static uint32_t dbg_bt_sche_setting_addr=0;
static uint32_t dbg_bt_ibrt_setting_addr=0;
static uint32_t dbg_bt_hw_feat_setting_addr=0;
static uint32_t hci_dbg_set_sw_rssi_addr = 0;
static uint32_t lp_clk_addr=0;
static uint32_t rwip_prog_delay_addr=0;
static uint32_t data_backup_cnt_addr=0;
static uint32_t data_backup_addr_ptr_addr=0;
static uint32_t data_backup_val_ptr_addr=0;
static uint32_t sch_multi_ibrt_adjust_env_addr=0;
static uint32_t RF_HWAGC_RSSI_CORRECT_TBL_addr=0;
static uint32_t workmode_patch_version_addr = 0;
static uint32_t rf_rx_gain_ths_tbl_le_addr = 0;
static uint32_t replace_mobile_valid_addr = 0;
static uint32_t replace_mobile_addr_addr = 0;
static uint32_t pcm_need_start_flag_addr = 0;
static uint32_t i2v_val_addr = 0;
static uint32_t rt_sleep_flag_clear_addr = 0;
static uint32_t rf_rx_gain_ths_tbl_bt_3m_addr = 0;
static uint32_t testmode_3m_flag_addr = 0;
static uint32_t normal_tab_en_addr = 0;
static uint32_t normal_iqtab_addr = 0;
static uint32_t testmode_tab_en_addr = 0;
static uint32_t testmode_iqtab_addr = 0;
static uint32_t power_adjust_en_addr = 0;
static uint32_t nosig_sch_flag_addr = 0;
static uint32_t ld_ibrt_env_addr=0;
static uint32_t le_ext_adv_tx_pwr_independent_addr = 0;
static uint32_t llm_local_le_feats_addr = 0;
static uint32_t lld_con_env_addr = 0;
static uint32_t tx_power_val_bkup_addr = 0;
static uint32_t preferr_rate_rate_addr=0;
static uint32_t txpwr_host_set_flg_addr = 0;
static uint32_t txpwr_val_buf_addr = 0;
static uint32_t isoohci_env_addr = 0;
static uint32_t  clkn_sn_buff_addr = 0;
static uint32_t  sync_found_check_hecerror_addr = 0;
static uint32_t unify_testmode_addr = 0;
static uint32_t afh_reporting_interval_addr = 0;

static uint32_t dbg_bt_common_setting_t2_addr=0;
static uint32_t testmode_3m_en_addr = 0;
static uint32_t rf_rx_gain_ths_tbl_bt_addr=0;
static uint32_t fa_agc_cfg_addr = 0;

void bt_drv_reg_op_global_symbols_init(void)
{
    bt_ram_start_addr = 0xc0000000;
#ifndef __BT_RAMRUN__
    enum HAL_CHIP_METAL_ID_T metal_id = hal_get_chip_metal_id();
    if (metal_id == HAL_CHIP_METAL_ID_0 || metal_id == HAL_CHIP_METAL_ID_1)
    {
        hci_fc_env_addr = HCI_FC_ENV_T0_ADDR;
        ld_acl_env_addr = LD_ACL_ENV_T0_ADDR;
        bt_util_buf_env_addr = BT_UTIL_BUF_ENV_T0_ADDR;
        ble_util_buf_env_addr = BLE_UTIL_BUF_ENV_T0_ADDR;
        lc_state_addr = LC_STATE_T0_ADDR;
        ld_sco_env_addr = LD_SCO_ENV_T0_ADDR;
        rx_monitor_addr = RX_MONITOR_T0_ADDR;
        lc_env_addr = LC_ENV_T0_ADDR;
        lm_nb_sync_active_addr = LM_NB_SYNC_ACTIVE_T0_ADDR;
        lm_env_addr = LM_ENV_T0_ADDR;
        lm_key_env_addr = LM_KEY_ENV_T0_ADDR;
        lc_sco_env_addr = LC_SCO_ENV_T0_ADDR;
        llm_env_addr = LLM_ENV_T0_ADDR;
        rwip_env_addr = RWIP_ENV_T0_ADDR;
        ble_rx_monitor_addr = BLE_RX_MONITOR_T0_ADDR;
        llc_env_addr = LLC_ENV_T0_ADDR;
        rwip_rf_addr = RWIP_RF_T0_ADDR;
        ld_acl_metrics_addr = LD_ACL_METRICS_T0_ADDR;
        rf_rx_hwgain_tbl_addr =  RF_RX_HWGAIN_TBL_T0_ADDR;
        rf_hwagc_rssi_correct_tbl_addr = RF_HWAGC_RSSI_CORRECT_TBL_T0_ADDR;
        ld_bes_bt_env_addr = LD_BES_BT_ENV_T0_ADDR;
        rf_rx_gain_fixed_tbl_addr = RF_RX_GAIN_FIXED_TBL_T0_ADDR;
        hci_dbg_ebq_test_mode_addr = HCI_DBG_EBQ_TEST_MODE_T0_ADDR;
        host_ref_clk_addr = HOST_REF_CLK_T0_ADDR;
        dbg_trc_tl_env_addr = DBG_TRC_TL_ENV_T0_ADDR;
        dbg_trc_mem_env_addr = DBG_TRC_MEM_ENV_T0_ADDR;
        dbg_bt_common_setting_addr = DBG_BT_COMMON_SETTING_T0_ADDR;
        dbg_bt_sche_setting_addr = DBG_BT_SCHE_SETTING_T0_ADDR;
        dbg_bt_ibrt_setting_addr = DBG_BT_IBRT_SETTING_T0_ADDR;
        dbg_bt_hw_feat_setting_addr = DBG_BT_HW_FEAT_SETTING_T0_ADDR;
        hci_dbg_set_sw_rssi_addr = HCI_DBG_SET_SW_RSSI_T0_ADDR;
        lp_clk_addr = LP_CLK_T0_ADDR;
        rwip_prog_delay_addr = RWIP_PROG_DELAY_T0_ADDR;
        data_backup_cnt_addr = DATA_BACKUP_CNT_T0_ADDR;
        data_backup_addr_ptr_addr = DATA_BACKUP_ADDR_PTR_T0_ADDR;
        data_backup_val_ptr_addr = DATA_BACKUP_VAL_PTR_T0_ADDR;
        sch_multi_ibrt_adjust_env_addr = SCH_MULTI_IBRT_ADJUST_ENV_T0_ADDR;
        RF_HWAGC_RSSI_CORRECT_TBL_addr = RF_HWAGC_RSSI_CORRECT_TBL_T0_ADDR;
        workmode_patch_version_addr = WORKMODE_PATCH_VERSION_T0_ADDR;
        rf_rx_gain_ths_tbl_le_addr = RF_RX_GAIN_THS_TBL_LE_T0_ADDR;
        replace_mobile_valid_addr = REPLACE_ADDR_VALID_T0_ADDR;
        replace_mobile_addr_addr = REPLACE_MOBILE_ADDR_T0_ADDR;
        pcm_need_start_flag_addr = PCM_NEED_START_FLAG_T0_ADDR;
        i2v_val_addr = TESTMODE_TEST_I2V_VAL_T0_TESTMODE_ADDR;
        rt_sleep_flag_clear_addr = RT_SLEEP_FLAG_CLEAR_T0_ADDR;
        rf_rx_gain_ths_tbl_bt_3m_addr = TESTMODE_RF_RX_GAIN_THS_TBL_BT_PATCH_T0_TESTMODE_ADDR;
        testmode_3m_flag_addr = TESTMODE_TESTMODE_3M_FLAG_T0_TESTMODE_ADDR;
        normal_tab_en_addr = NORMAL_IQTAB_EN_T0_ADDR;
        normal_iqtab_addr = NORMAL_IQTAB_T0_ADDR;
        testmode_tab_en_addr = TESTMODE_IQTAB_EN_T0_TESTMODE_ADDR;
        testmode_iqtab_addr = TESTMODE_IQTAB_T0_TESTMODE_ADDR;
        power_adjust_en_addr = TESTMODE_POWER_ADJUST_EN_T0_TESTMODE_ADDR;
        nosig_sch_flag_addr = TESTMODE_NOSIG_SCH_FLAG_T0_TESTMODE_ADDR;
        ld_ibrt_env_addr = LD_IBRT_ENV_T0_ADDR;
        llm_local_le_feats_addr = LLM_LOCAL_LE_FEATS_T0_ADDR;
    }
    else if (metal_id == HAL_CHIP_METAL_ID_2 || metal_id == HAL_CHIP_METAL_ID_3)
    {
        hci_fc_env_addr = HCI_FC_ENV_T1_ADDR;
        ld_acl_env_addr = LD_ACL_ENV_T1_ADDR;
        bt_util_buf_env_addr = BT_UTIL_BUF_ENV_T1_ADDR;
        ble_util_buf_env_addr = BLE_UTIL_BUF_ENV_T1_ADDR;
        lc_state_addr = LC_STATE_T1_ADDR;
        ld_sco_env_addr = LD_SCO_ENV_T1_ADDR;
        rx_monitor_addr = RX_MONITOR_T1_ADDR;
        lc_env_addr = LC_ENV_T1_ADDR;
        lm_nb_sync_active_addr = LM_NB_SYNC_ACTIVE_T1_ADDR;
        lm_env_addr = LM_ENV_T1_ADDR;
        lm_key_env_addr = LM_KEY_ENV_T1_ADDR;
        lc_sco_env_addr = LC_SCO_ENV_T1_ADDR;
        llm_env_addr = LLM_ENV_T1_ADDR;
        rwip_env_addr = RWIP_ENV_T1_ADDR;
        ble_rx_monitor_addr = BLE_RX_MONITOR_T1_ADDR;
        llc_env_addr = LLC_ENV_T1_ADDR;
        rwip_rf_addr = RWIP_RF_T1_ADDR;
        ld_acl_metrics_addr = LD_ACL_METRICS_T1_ADDR;
        rf_rx_hwgain_tbl_addr =  RF_RX_HWGAIN_TBL_T1_ADDR;
        rf_hwagc_rssi_correct_tbl_addr = RF_HWAGC_RSSI_CORRECT_TBL_T1_ADDR;
        ld_bes_bt_env_addr = LD_BES_BT_ENV_T1_ADDR;
        rf_rx_gain_fixed_tbl_addr = RF_RX_GAIN_FIXED_TBL_T1_ADDR;
        hci_dbg_ebq_test_mode_addr = HCI_DBG_EBQ_TEST_MODE_T1_ADDR;
        host_ref_clk_addr = HOST_REF_CLK_T1_ADDR;
        dbg_trc_tl_env_addr = DBG_TRC_TL_ENV_T1_ADDR;
        dbg_trc_mem_env_addr = DBG_TRC_MEM_ENV_T1_ADDR;
        dbg_bt_common_setting_addr = DBG_BT_COMMON_SETTING_T1_ADDR;
        dbg_bt_sche_setting_addr = DBG_BT_SCHE_SETTING_T1_ADDR;
        dbg_bt_ibrt_setting_addr = DBG_BT_IBRT_SETTING_T1_ADDR;
        dbg_bt_hw_feat_setting_addr = DBG_BT_HW_FEAT_SETTING_T1_ADDR;
        hci_dbg_set_sw_rssi_addr = HCI_DBG_SET_SW_RSSI_T1_ADDR;
        lp_clk_addr = LP_CLK_T1_ADDR;
        rwip_prog_delay_addr = RWIP_PROG_DELAY_T1_ADDR;
        data_backup_cnt_addr = DATA_BACKUP_CNT_T1_ADDR;
        data_backup_addr_ptr_addr = DATA_BACKUP_ADDR_PTR_T1_ADDR;
        data_backup_val_ptr_addr = DATA_BACKUP_VAL_PTR_T1_ADDR;
        sch_multi_ibrt_adjust_env_addr = SCH_MULTI_IBRT_ADJUST_ENV_T1_ADDR;
        RF_HWAGC_RSSI_CORRECT_TBL_addr = RF_HWAGC_RSSI_CORRECT_TBL_T1_ADDR;
        workmode_patch_version_addr = WORKMODE_PATCH_VERSION_T1_ADDR;
        rf_rx_gain_ths_tbl_le_addr = RF_RX_GAIN_THS_TBL_LE_T1_ADDR;
        replace_mobile_valid_addr = REPLACE_ADDR_VALID_T1_ADDR;
        replace_mobile_addr_addr = REPLACE_MOBILE_ADDR_T1_ADDR;
        pcm_need_start_flag_addr = PCM_NEED_START_FLAG_T1_ADDR;
        i2v_val_addr = I2V_VAL_T1_ADDR;
        rt_sleep_flag_clear_addr = RT_SLEEP_FLAG_CLEAR_T1_ADDR;
        rf_rx_gain_ths_tbl_bt_3m_addr = RF_RX_GAIN_THS_TBL_BT_3M_T1_ADDR;
        testmode_3m_flag_addr = TESTMODE_3M_FLAG_T1_ADDR;
        normal_tab_en_addr = NORMAL_IQTAB_EN_T1_ADDR;
        normal_iqtab_addr = NORMAL_IQTAB_T1_ADDR;
        testmode_tab_en_addr = NORMAL_IQTAB_EN_T1_ADDR;
        testmode_iqtab_addr = NORMAL_IQTAB_T1_ADDR;
        power_adjust_en_addr = POWER_ADJUST_EN_T1_ADDR;
        nosig_sch_flag_addr = TESTMODE_NOSIG_SCH_FLAG_T1_TESTMODE_ADDR;
        ld_ibrt_env_addr = LD_IBRT_ENV_T1_ADDR;
        le_ext_adv_tx_pwr_independent_addr = LE_EXT_ADV_TX_PWR_INDEPENDENT_T1_ADDR;
        llm_local_le_feats_addr = LLM_LOCAL_LE_FEATS_T1_ADDR;
        tx_power_val_bkup_addr = TX_POWER_VAL_BKUP_T1_ADDR;
        preferr_rate_rate_addr = PREFERR_RATE_RATE_T1_ADDR;
        txpwr_host_set_flg_addr = TXPWR_HOST_SET_FLG_T1_ADDR;
        txpwr_val_buf_addr = TXPWR_VAL_BUF_T1_ADDR;
        isoohci_env_addr = ISOOHCI_ENV_T1_ADDR;
        clkn_sn_buff_addr = CLKN_SN_BUFF_T1_ADDR;
        sync_found_check_hecerror_addr = SYNC_FOUND_CHECK_HECERROR_T1_ADDR;
        unify_testmode_addr = TESTMODE_TESTMODE_NEW_MODE_T1_TESTMODE_ADDR;
        afh_reporting_interval_addr = AFH_REPORTING_INTERVAL_T1_ADDR;
        rf_rx_gain_ths_tbl_bt_addr = RF_RX_GAIN_THS_TBL_BT_T1_ADDR;
        fa_agc_cfg_addr = FA_AGC_CFG_T1_ADDR;
    }
    else if (metal_id >= HAL_CHIP_METAL_ID_4)
    {
        hci_fc_env_addr = HCI_FC_ENV_T2_ADDR;
        ld_acl_env_addr = LD_ACL_ENV_T2_ADDR;
        bt_util_buf_env_addr = BT_UTIL_BUF_ENV_T2_ADDR;
        ble_util_buf_env_addr = BLE_UTIL_BUF_ENV_T2_ADDR;
        lc_state_addr = LC_STATE_T2_ADDR;
        ld_sco_env_addr = LD_SCO_ENV_T2_ADDR;
        rx_monitor_addr = RX_MONITOR_T2_ADDR;
        lc_env_addr = LC_ENV_T2_ADDR;
        lm_nb_sync_active_addr = LM_NB_SYNC_ACTIVE_T2_ADDR;
        lm_env_addr = LM_ENV_T2_ADDR;
        lm_key_env_addr = LM_KEY_ENV_T2_ADDR;
        lc_sco_env_addr = LC_SCO_ENV_T2_ADDR;
        llm_env_addr = LLM_ENV_T2_ADDR;
        rwip_env_addr = RWIP_ENV_T2_ADDR;
        ble_rx_monitor_addr = BLE_RX_MONITOR_T2_ADDR;
        llc_env_addr = LLC_ENV_T2_ADDR;
        rwip_rf_addr = RWIP_RF_T2_ADDR;
        ld_acl_metrics_addr = LD_ACL_METRICS_T2_ADDR;
        rf_rx_hwgain_tbl_addr =  RF_RX_HWGAIN_TBL_T2_ADDR;
        rf_hwagc_rssi_correct_tbl_addr = RF_HWAGC_RSSI_CORRECT_TBL_T2_ADDR;
        ld_bes_bt_env_addr = LD_BES_BT_ENV_T2_ADDR;
        rf_rx_gain_fixed_tbl_addr = RF_RX_GAIN_FIXED_TBL_T2_ADDR;
        hci_dbg_ebq_test_mode_addr = HCI_DBG_EBQ_TEST_MODE_T2_ADDR;
        host_ref_clk_addr = HOST_REF_CLK_T2_ADDR;
        dbg_trc_tl_env_addr = DBG_TRC_TL_ENV_T2_ADDR;
        dbg_trc_mem_env_addr = DBG_TRC_MEM_ENV_T2_ADDR;
        dbg_bt_common_setting_addr = DBG_BT_COMMON_SETTING_T2_ADDR;
        dbg_bt_sche_setting_addr = DBG_BT_SCHE_SETTING_T2_ADDR;
        dbg_bt_ibrt_setting_addr = DBG_BT_IBRT_SETTING_T2_ADDR;
        dbg_bt_hw_feat_setting_addr = DBG_BT_HW_FEAT_SETTING_T2_ADDR;
        hci_dbg_set_sw_rssi_addr = HCI_DBG_SET_SW_RSSI_T2_ADDR;
        lp_clk_addr = LP_CLK_T2_ADDR;
        rwip_prog_delay_addr = RWIP_PROG_DELAY_T2_ADDR;
        data_backup_cnt_addr = DATA_BACKUP_CNT_T2_ADDR;
        data_backup_addr_ptr_addr = DATA_BACKUP_ADDR_PTR_T2_ADDR;
        data_backup_val_ptr_addr = DATA_BACKUP_VAL_PTR_T2_ADDR;
        sch_multi_ibrt_adjust_env_addr = SCH_MULTI_IBRT_ADJUST_ENV_T2_ADDR;
        RF_HWAGC_RSSI_CORRECT_TBL_addr = RF_HWAGC_RSSI_CORRECT_TBL_T2_ADDR;
        workmode_patch_version_addr = WORKMODE_PATCH_VERSION_T2_ADDR;
        rf_rx_gain_ths_tbl_le_addr = RF_RX_GAIN_THS_TBL_LE_T2_ADDR;
        replace_mobile_valid_addr = REPLACE_ADDR_VALID_T2_ADDR;
        replace_mobile_addr_addr = REPLACE_MOBILE_ADDR_T2_ADDR;
        pcm_need_start_flag_addr = PCM_NEED_START_FLAG_T2_ADDR;
        i2v_val_addr = I2V_VAL_T2_ADDR;
        rf_rx_gain_ths_tbl_bt_3m_addr = RF_RX_GAIN_THS_TBL_BT_3M_T2_ADDR;
        testmode_3m_flag_addr = TESTMODE_3M_FLAG_T2_ADDR;
        normal_tab_en_addr = NORMAL_IQTAB_EN_T2_ADDR;
        normal_iqtab_addr = NORMAL_IQTAB_T2_ADDR;
        testmode_tab_en_addr = NORMAL_IQTAB_EN_T2_ADDR;
        testmode_iqtab_addr = NORMAL_IQTAB_T2_ADDR;
        power_adjust_en_addr = POWER_ADJUST_EN_T2_ADDR;
        ld_ibrt_env_addr = LD_IBRT_ENV_T2_ADDR;
        llm_local_le_feats_addr = LLM_LOCAL_LE_FEATS_T2_ADDR;
        isoohci_env_addr = ISOOHCI_ENV_T2_ADDR;
        dbg_bt_common_setting_t2_addr = DBG_BT_COMMON_SETTING_T2_T2_ADDR;
        testmode_3m_en_addr = TESTMODE_3M_EN_T2_ADDR;
        rt_sleep_flag_clear_addr = RT_SLEEP_FLAG_CLEAR_T2_ADDR;
    }
#else
    {
        hci_fc_env_addr = HCI_FC_ENV_ADDR;
        ld_acl_env_addr = LD_ACL_ENV_ADDR;
        bt_util_buf_env_addr = BT_UTIL_BUF_ENV_ADDR;
        ble_util_buf_env_addr = BLE_UTIL_BUF_ENV_ADDR;
        lc_state_addr = LC_STATE_ADDR;
        ld_sco_env_addr = LD_SCO_ENV_ADDR;
        rx_monitor_addr = RX_MONITOR_ADDR;
        lc_env_addr = LC_ENV_ADDR;
        lm_nb_sync_active_addr = LM_NB_SYNC_ACTIVE_ADDR;
        lm_env_addr = LM_ENV_ADDR;
        lm_key_env_addr = LM_KEY_ENV_ADDR;
        lc_sco_env_addr = LC_SCO_ENV_ADDR;
        llm_env_addr = LLM_ENV_ADDR;
        rwip_env_addr = RWIP_ENV_ADDR;
        ble_rx_monitor_addr = BLE_RX_MONITOR_ADDR;
        llc_env_addr = LLC_ENV_ADDR;
        rwip_rf_addr = RWIP_RF_ADDR;
        ld_acl_metrics_addr = LD_ACL_METRICS_ADDR;
        rf_rx_hwgain_tbl_addr =  RF_RX_HWGAIN_TBL_ADDR;
        rf_hwagc_rssi_correct_tbl_addr = RF_HWAGC_RSSI_CORRECT_TBL_ADDR;
        ld_bes_bt_env_addr = LD_BES_BT_ENV_ADDR;
        rf_rx_gain_fixed_tbl_addr = RF_RX_GAIN_FIXED_TBL_ADDR;
        hci_dbg_ebq_test_mode_addr = HCI_DBG_EBQ_TEST_MODE_ADDR;
        host_ref_clk_addr = HOST_REF_CLK_ADDR;
        dbg_trc_tl_env_addr = DBG_TRC_TL_ENV_ADDR;
        dbg_trc_mem_env_addr = DBG_TRC_MEM_ENV_ADDR;
        dbg_bt_common_setting_addr = DBG_BT_COMMON_SETTING_ADDR;
        dbg_bt_sche_setting_addr = DBG_BT_SCHE_SETTING_ADDR;
        dbg_bt_ibrt_setting_addr = DBG_BT_IBRT_SETTING_ADDR;
        dbg_bt_hw_feat_setting_addr = DBG_BT_HW_FEAT_SETTING_ADDR;
        hci_dbg_set_sw_rssi_addr = HCI_DBG_SET_SW_RSSI_ADDR;
        lp_clk_addr = LP_CLK_ADDR;
        rwip_prog_delay_addr = RWIP_PROG_DELAY_ADDR;
        data_backup_cnt_addr = DATA_BACKUP_CNT_ADDR;
        data_backup_addr_ptr_addr = DATA_BACKUP_ADDR_PTR_ADDR;
        data_backup_val_ptr_addr = DATA_BACKUP_VAL_PTR_ADDR;
        sch_multi_ibrt_adjust_env_addr = SCH_MULTI_IBRT_ADJUST_ENV_ADDR;
        RF_HWAGC_RSSI_CORRECT_TBL_addr = RF_HWAGC_RSSI_CORRECT_TBL_ADDR;
        rf_rx_gain_ths_tbl_le_addr = RF_RX_GAIN_THS_TBL_LE_ADDR;
        lld_con_env_addr = LLD_CON_ENV_ADDR;

        txpwr_host_set_flg_addr = 0;
        txpwr_val_buf_addr = 0;
        clkn_sn_buff_addr = 0;

    }
#endif
}

uint32_t bt_drv_reg_op_get_host_ref_clk(void)
{
    uint32_t ret = 0;
    BT_DRV_REG_OP_ENTER();
    BT_DRV_REG_OP_CLK_ENB();
    ret = *(uint32_t *)host_ref_clk_addr;
    BT_DRV_REG_OP_CLK_DIB();
    BT_DRV_REG_OP_EXIT();
    return ret;
}
void bt_drv_reg_op_set_controller_log_buffer(void)
{
    BT_DRV_REG_OP_ENTER();
    BT_DRV_REG_OP_CLK_ENB();

    BT_DRV_REG_OP_CLK_DIB();
    BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_op_config_controller_log(uint32_t log_mask, uint16_t* p_cmd_filter, uint8_t cmd_nb, uint8_t* p_evt_filter, uint8_t evt_nb)
{
    BT_DRV_REG_OP_ENTER();
    BT_DRV_REG_OP_CLK_ENB();

    BT_DRV_REG_OP_CLK_DIB();
    BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_op_set_controller_trace_curr_sw(uint32_t log_mask)
{
    BT_DRV_REG_OP_ENTER();
    BT_DRV_REG_OP_CLK_ENB();

    BT_DRV_REG_OP_CLK_DIB();
    BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_op_config_cotroller_cmd_filter(uint16_t* p_cmd_filter)
{
    BT_DRV_REG_OP_ENTER();
    BT_DRV_REG_OP_CLK_ENB();

    BT_DRV_REG_OP_CLK_DIB();
    BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_op_config_cotroller_evt_filter(uint8_t* p_evt_filter)
{
    BT_DRV_REG_OP_ENTER();
    BT_DRV_REG_OP_CLK_ENB();

    BT_DRV_REG_OP_CLK_DIB();
    BT_DRV_REG_OP_EXIT();
}



#if (LL_MONITOR == 1)

void bt_drv_reg_op_ll_monitor(uint16_t connhdl, uint8_t metric_type, uint32_t* ret_val)
{
    BT_DRV_REG_OP_CLK_ENB();
    uint8_t link_id = btdrv_conhdl_to_linkid(connhdl);
    uint32_t ld_acl_metrics_ptr = 0;
    uint32_t metric_off = 12 + metric_type*4;
    if(ld_acl_metrics_addr != 0)
    {
        ld_acl_metrics_ptr = ld_acl_metrics_addr + link_id * 164;
    }
    *ret_val = *(uint32_t*)(ld_acl_metrics_ptr + metric_off);
    BT_DRV_REG_OP_CLK_DIB();
}

#endif


void bt_drv_reg_op_set_rf_rx_hwgain_tbl(int8_t (*p_tbl)[3])
{
    BT_DRV_REG_OP_CLK_ENB();
    int i, j ;
    int8_t (*p_tbl_addr)[3] = (int8_t (*)[3])rf_rx_hwgain_tbl_addr;

    if(p_tbl != NULL)
    {
        for(i = 0; i < GAIN_TBL_SIZE; i++)
        {
            for(j = 0; j < 3; j++)
            {
                p_tbl_addr[i][j] = p_tbl[i][j];
            }
        }
    }
    BT_DRV_REG_OP_CLK_DIB();
}

void bt_drv_reg_op_set_rf_hwagc_rssi_correct_tbl(int8_t* p_tbl)
{
    BT_DRV_REG_OP_CLK_ENB();
    int i;
    int8_t* p_tbl_addr = (int8_t*)rf_hwagc_rssi_correct_tbl_addr;
    if(p_tbl != NULL)
    {
        for(i = 0; i < 16; i++)
        {
            p_tbl_addr[i] = p_tbl[i];
        }
    }
    BT_DRV_REG_OP_CLK_DIB();
}


#if (defined(ACL_DATA_CRC_TEST))
uint32_t crc32_ieee_update(uint32_t crc, const uint8_t *data, size_t len)
{
    crc = ~crc;
    for (size_t i = 0; i < len; i++)
    {
        crc = crc ^ data[i];

        for (uint8_t j = 0; j < 8; j++)
        {
            crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
        }
    }
    return (~crc);
}

uint32_t crc32_ieee(const uint8_t *data, size_t len)
{
    uint32_t crc = 0;
    BT_DRV_REG_OP_CLK_ENB();
    crc = crc32_ieee_update(0x0, data, len);
    BT_DRV_REG_OP_CLK_DIB();
    return crc;
}
#endif

void bt_drv_reg_op_hci_vender_ibrt_ll_monitor(uint8_t* ptr, uint16_t* p_sum_err,uint16_t* p_rx_total)
{
    const char *monitor_str[METRIC_TYPE_MAX] =
    {
        "TX DM1",
        "TX DH1",
        "TX DM3",
        "TX DH3",
        "TX DM5",
        "TX DH5",
        "TX 2DH1",
        "TX 3DH1",
        "TX 2DH3",
        "TX 3DH3",
        "TX 2DH5",
        "TX 3DH5",  //11

        "RX DM1",
        "RX DH1",
        "RX DM3",
        "RX DH3",
        "RX DM5",
        "RX DH5",
        "RX 2DH1",
        "RX 3DH1",
        "RX 2DH3",
        "RX 3DH3",
        "RX 2DH5",
        "RX 3DH5",  //23
        "hec err",
        "crc err",
        "fec err",
        "gard err",
        "ecc cnt",  //28

        "rf_cnt",
        "slp_duration_cnt",
        "tx_suc_cnt",
        "tx_cnt",
        "sb_suc_cnt",
        "buff err",
        "rx_seq_err_cnt", //35
        "rx_fa_cnt",
        "RX 2EV3",
    };

    uint32_t *p = ( uint32_t * )ptr;
    uint32_t sum_err = 0;
    uint32_t rx_tx_data_sum = 0;

    //BT_DRV_TRACE(0,"ibrt_ui_log:ll_monitor");

    for (uint8_t i = 0; i < METRIC_TYPE_MAX; i++)
    {
        if (*p)
        {
            if((i>= 0 && i<=29)||(i==35)||(i==37))
            {
                rx_tx_data_sum += *p;
            }

            if((i > 23) && (i < 29))
            {
                sum_err += *p;
            }
            BT_DRV_TRACE(2,"%s %d", monitor_str[i], *p);
        }
        p++;
    }
    *p_sum_err = sum_err;
    *p_rx_total = rx_tx_data_sum;
}

int bt_drv_reg_op_currentfreeaclbuf_get(void)
{
    int nRet = 0;

#ifdef BT_INFO_CHECKER
    BT_DRV_REG_OP_ENTER();
    BT_DRV_REG_OP_CLK_ENB();
    //ACL packert number of host set - ACL number of controller has been send to host
    //hci_fc_env.host_set.acl_pkt_nb - hci_fc_env.cntr.acl_pkt_sent
    if(hci_fc_env_addr != 0)
    {
        nRet = (*(volatile uint16_t *)(hci_fc_env_addr+0x2) - *(volatile uint16_t *)(hci_fc_env_addr+0xa));//acl_pkt_nb - acl_pkt_sent
    }
    else
    {
        nRet = 0;
    }
    BT_DRV_REG_OP_CLK_DIB();
    BT_DRV_REG_OP_EXIT();
#endif
    return nRet;
}

void bt_drv_reg_op_lsto_hack(uint16_t hciHandle, uint16_t lsto)
{
    BT_DRV_REG_OP_CLK_ENB();

    uint32_t acl_par_ptr = 0;
    //ld_acl_env

    if(ld_acl_env_addr)
    {
        acl_par_ptr = *(uint32_t *)(ld_acl_env_addr+(hciHandle-0x80)*4);
    }

    if(acl_par_ptr)
    {
        //lsto off:0xbc
        BT_DRV_TRACE(3,"BT_REG_OP:Set the lsto for hciHandle=0x%x, from:0x%x to 0x%x",
                     hciHandle,*(uint16_t *)(acl_par_ptr+0xbc),lsto);

        *(uint16_t *)(acl_par_ptr+0xbc) = lsto;
    }
    else
    {
        BT_DRV_TRACE(0,"BT_REG_OP:ERROR,acl par address error");
    }

    BT_DRV_REG_OP_CLK_DIB();
}

uint16_t bt_drv_reg_op_get_lsto(uint16_t hciHandle)
{
    uint32_t acl_par_ptr = 0;
    uint16_t lsto = 0;
    //ld_acl_env
    BT_DRV_REG_OP_CLK_ENB();
    if(ld_acl_env_addr)
    {
        acl_par_ptr = *(uint32_t *)(ld_acl_env_addr+(hciHandle-0x80)*4);
    }


    if(acl_par_ptr)
    {
        //lsto off:0xbc
        lsto = *(uint16_t *)(acl_par_ptr+0xbc) ;
        BT_DRV_TRACE(2,"BT_REG_OP:lsto=0x%x for hciHandle=0x%x",lsto,hciHandle);
    }
    else
    {
        lsto= 0xffff;
        BT_DRV_TRACE(0,"BT_REG_OP:ERROR,acl par null ptr");
    }

    BT_DRV_REG_OP_CLK_DIB();

    return lsto;
}

int bt_drv_reg_op_acl_chnmap(uint16_t hciHandle, uint8_t *chnmap, uint8_t chnmap_len)
{
    int nRet  = 0;
    uint32_t acl_evt_ptr = 0;
    uint8_t *chnmap_ptr = 0;
    uint8_t link_id = btdrv_conhdl_to_linkid(hciHandle);

    BT_DRV_REG_OP_CLK_ENB();

    if (!btdrv_is_link_index_valid(link_id))
    {
        memset(chnmap, 0, chnmap_len);
        nRet = -1;
        goto exit;
    }
    if (chnmap_len < 10)
    {
        memset(chnmap, 0, chnmap_len);
        nRet = -1;
        goto exit;
    }


    if(ld_acl_env_addr)
    {
        acl_evt_ptr = *(volatile uint32_t *)(ld_acl_env_addr+link_id*4);
    }


    if(acl_evt_ptr != 0)
    {
        //afh_map off:0x40
        chnmap_ptr = (uint8_t *)(acl_evt_ptr+0x40);
    }

    if (!chnmap_ptr)
    {
        memset(chnmap, 0, chnmap_len);
        nRet = -1;
        goto exit;
    }
    else
    {
        memcpy(chnmap, chnmap_ptr, chnmap_len);
    }

exit:
    BT_DRV_REG_OP_CLK_DIB();

    return nRet;
}

extern "C" uint32_t hci_current_left_tx_packets_left(void);
extern "C" uint32_t hci_current_left_rx_packets_left(void);
extern "C" uint32_t hci_current_rx_packet_complete(void);
extern "C" uint8_t hci_current_rx_aclfreelist_cnt(void);
void bt_drv_reg_op_bt_info_checker(void)
{
#ifdef BT_INFO_CHECKER
    uint32_t *rx_buf_ptr=NULL;
    uint32_t *tx_buf_ptr=NULL;
    uint8_t rx_free_buf_count=0;
    uint8_t tx_free_buf_count=0;

    BT_DRV_REG_OP_ENTER();
    BT_DRV_REG_OP_CLK_ENB();

    if(bt_util_buf_env_addr)
    {
        //acl_rx_free off:0x14
        //acl_tx_free off:0x28
        rx_buf_ptr = (uint32_t *)(bt_util_buf_env_addr+0x14); //bt_util_buf_env.acl_rx_free
        tx_buf_ptr = (uint32_t *)(bt_util_buf_env_addr+0x28); //bt_util_buf_env.acl_tx_free
    }

    while(rx_buf_ptr && *rx_buf_ptr)
    {
        rx_free_buf_count++;
        rx_buf_ptr = (uint32_t *)(*rx_buf_ptr);
    }

    BT_DRV_TRACE(3,"BT_REG_OP:acl_rx_free ctrl rxbuff %d, host free %d,%d", \
                 rx_free_buf_count, \
                 hci_current_left_rx_packets_left(), \
                 bt_drv_reg_op_currentfreeaclbuf_get());

    //check tx buff
    while(tx_buf_ptr && *tx_buf_ptr)
    {
        tx_free_buf_count++;
        tx_buf_ptr = (uint32_t *)(*tx_buf_ptr);
    }

    BT_DRV_TRACE(2,"BT_REG_OP:acl_tx_free ctrl txbuff %d, host consider ctrl free %d", \
                 tx_free_buf_count, \
                 hci_current_left_tx_packets_left());
    BT_DRV_REG_OP_CLK_DIB();
    BT_DRV_REG_OP_EXIT();

    bt_drv_reg_op_controller_state_checker();
#endif
}

uint8_t bt_drv_reg_op_get_controller_tx_free_buffer(void)
{
    uint32_t *tx_buf_ptr=NULL;
    uint8_t tx_free_buf_count = 0;

    BT_DRV_REG_OP_CLK_ENB();

    if(bt_util_buf_env_addr)
    {
        //acl_tx_free off:0x28
        tx_buf_ptr = (uint32_t *)(bt_util_buf_env_addr+0x28); //bt_util_buf_env.acl_tx_free
    }
    else
    {
        BT_DRV_TRACE(1, "BT_REG_OP:please fix %s", __func__);
        tx_free_buf_count = 0;
        goto exit;
    }

    //check tx buff
    while(tx_buf_ptr && *tx_buf_ptr)
    {
        tx_free_buf_count++;
        tx_buf_ptr = (uint32_t *)(*tx_buf_ptr);
    }

exit:
    BT_DRV_REG_OP_CLK_DIB();

    return tx_free_buf_count;
}

void bt_drv_reg_op_controller_state_checker(void)
{
    BT_DRV_REG_OP_ENTER();
    BT_DRV_REG_OP_CLK_ENB();

    if(lc_state_addr != 0)
    {
        BT_DRV_TRACE(1,"BT_REG_OP: LC_STATE=0x%x",*(uint32_t *)lc_state_addr);
    }

#ifdef BT_UART_LOG
    BT_DRV_TRACE(1,"current controller clk=%x", bt_drv_reg_op_get_host_ref_clk());
#endif

    BT_DRV_REG_OP_CLK_DIB();
    BT_DRV_REG_OP_EXIT();
}

uint8_t bt_drv_reg_op_force_get_lc_state(uint16_t conhdl)
{
    uint8_t state = 0;

    BT_DRV_REG_OP_CLK_ENB();
    if(lc_state_addr != 0)
    {
        BT_DRV_TRACE(1,"BT_REG_OP: read LC_STATE=0x%x",*(uint32_t *)lc_state_addr);
        uint8_t idx = btdrv_conhdl_to_linkid(conhdl);

        if (btdrv_is_link_index_valid(idx))
        {
            uint8_t *lc_state = (uint8_t *)lc_state_addr;
            state = lc_state[idx];
        }
    }
    BT_DRV_REG_OP_CLK_DIB();

    return state;
}

void bt_drv_reg_op_crash_dump(void)
{
    BT_DRV_REG_OP_CLK_ENB();

    uint8_t *bt_dump_mem_start = (uint8_t*)bt_ram_start_addr;
    uint32_t bt_dump_len_max   = 0x10000;

    uint8_t *em_dump_area_2_start = (uint8_t*)0xd0210000;
    uint32_t em_area_2_len_max    = 0x8000;

    if (workmode_patch_version_addr)
    {
        BT_DRV_TRACE(1,"BT_REG_OP:BT 1501: metal id=%d,patch version=%08x", hal_get_chip_metal_id(), BTDIGITAL_REG(workmode_patch_version_addr));
    }
    //first move R3 to R9, lost R9
    BT_DRV_TRACE(1,"BT controller BusFault_Handler:\nREG:[LR] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE));
    BT_DRV_TRACE(1,"REG:[R0] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +160));
    BT_DRV_TRACE(1,"REG:[R1] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +164));
    BT_DRV_TRACE(1,"REG:[R2] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +168));
    BT_DRV_TRACE(1,"REG:[R3] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +172));
    BT_DRV_TRACE(1,"REG:[R4] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +4));
    BT_DRV_TRACE(1,"REG:[R5] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +8));
    BT_DRV_TRACE(1,"REG:[R6] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +12));
    BT_DRV_TRACE(1,"REG:[R7] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +16));
    BT_DRV_TRACE(1,"REG:[R8] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +20));
    hal_sys_timer_delay(MS_TO_TICKS(100));

    //BT_DRV_TRACE(1,"REG:[R9] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +24));
    BT_DRV_TRACE(1,"REG:[sl] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +28));
    BT_DRV_TRACE(1,"REG:[fp] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +32));
    BT_DRV_TRACE(1,"REG:[ip] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +36));
    BT_DRV_TRACE(1,"REG:[SP,#0] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +40));
    BT_DRV_TRACE(1,"REG:[SP,#4] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +44));
    BT_DRV_TRACE(1,"REG:[SP,#8] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +48));
    BT_DRV_TRACE(1,"REG:[SP,#12] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +52));
    BT_DRV_TRACE(1,"REG:[SP,#16] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +56));
    BT_DRV_TRACE(1,"REG:[SP,#20] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +60));
    BT_DRV_TRACE(1,"REG:[SP,#24] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +64));
    BT_DRV_TRACE(1,"REG:[SP,#28] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +68));
    hal_sys_timer_delay(MS_TO_TICKS(100));

    BT_DRV_TRACE(1,"REG:[SP,#32] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +72));
    BT_DRV_TRACE(1,"REG:[SP,#36] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +76));
    BT_DRV_TRACE(1,"REG:[SP,#40] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +80));
    BT_DRV_TRACE(1,"REG:[SP,#44] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +84));
    BT_DRV_TRACE(1,"REG:[SP,#48] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +88));
    BT_DRV_TRACE(1,"REG:[SP,#52] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +92));
    BT_DRV_TRACE(1,"REG:[SP,#56] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +96));
    BT_DRV_TRACE(1,"REG:[SP,#60] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +100));
    BT_DRV_TRACE(1,"REG:[SP,#64] = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +104));
    BT_DRV_TRACE(1,"REG:SP = 0x%08x",  BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +108));
    BT_DRV_TRACE(1,"REG:MSP = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +112));
    BT_DRV_TRACE(1,"REG:PSP = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +116));
    BT_DRV_TRACE(1,"REG:CFSR = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +120));
    BT_DRV_TRACE(1,"REG:BFAR = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +124));
    BT_DRV_TRACE(1,"REG:HFSR = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +128));
    BT_DRV_TRACE(1,"REG:ICSR = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +132));
    BT_DRV_TRACE(1,"REG:AIRCR = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +136));
    BT_DRV_TRACE(1,"REG:SCR = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +140));
    BT_DRV_TRACE(1,"REG:CCR = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +144));
    BT_DRV_TRACE(1,"REG:SHCSR = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +148));
    BT_DRV_TRACE(1,"REG:AFSR = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +152));
    BT_DRV_TRACE(1,"REG:MMFAR = 0x%08x", BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +156));
    hal_sys_timer_delay(MS_TO_TICKS(100));
    //task_message_buffer
    uint32_t buff_addr = 0;

    if(task_message_buffer_addr)
    {
        buff_addr = task_message_buffer_addr;
    }

    BT_DRV_TRACE(1,"REG:IP_ERRORTYPESTAT: 0x%x",BTDIGITAL_REG(BT_ERRORTYPESTAT_ADDR));
    BT_DRV_TRACE(1,"REG:BT_CURRENTRXDESCPTR: 0x%x",BTDIGITAL_REG(BT_CURRENTRXDESCPTR_ADDR));
    BT_DRV_TRACE(2,"0xd0330050: 0x%x, 54:0x%x",*(uint32_t *)0xd0330050,*(uint32_t *)0xd0330054);
    BT_DRV_TRACE(2,"0x400000a0: 0x%x, a4:0x%x",*(uint32_t *)0x400000a0,*(uint32_t *)0x400000a4);
    bes_get_hci_rx_flowctrl_info();
    bes_get_hci_tx_flowctrl_info();

    if(g_mem_dump_ctrl_addr)
    {
        BT_DRV_TRACE(1,"LMP addr=0x%x",*(uint32_t *)(g_mem_dump_ctrl_addr+4));
        BT_DRV_TRACE(1,"STA addr=0x%x",*(uint32_t *)(g_mem_dump_ctrl_addr+0x10));
        BT_DRV_TRACE(1,"MSG addr=0x%x",*(uint32_t *)(g_mem_dump_ctrl_addr+0x1c));
        BT_DRV_TRACE(1,"SCH addr=0x%x",*(uint32_t *)(g_mem_dump_ctrl_addr+0x28));
        BT_DRV_TRACE(1,"ISR addr=0x%x",*(uint32_t *)(g_mem_dump_ctrl_addr+0x34));
    }

    BT_DRV_TRACE(0,"task msg buff:");
    if(buff_addr != 0)
    {
        for(uint8_t j=0; j<5; j++)
        {
            DUMP8("%02x ", (uint8_t *)(buff_addr+j*20), 20);
        }
    }
    hal_sys_timer_delay(MS_TO_TICKS(100));

    BT_DRV_TRACE(0," ");
    BT_DRV_TRACE(0,"lmp buff:");
    //lmp_message_buffer

    if(lmp_message_buffer_addr)
    {
        buff_addr = lmp_message_buffer_addr;
    }

    if(buff_addr != 0)
    {
        for(uint8_t j=0; j<10; j++)
        {
            DUMP8("%02x ",(uint8_t *)(buff_addr+j*20), 20);
        }
    }
    hal_sys_timer_delay(MS_TO_TICKS(100));

    uint8_t link_id = 0;
    uint32_t evt_ptr = 0;
    uint32_t acl_par_ptr = 0;
    for(link_id=0; link_id<MAX_NB_ACTIVE_ACL; link_id++)
    {
        if(ld_acl_env_addr)
        {
            evt_ptr = *(uint32_t *)(ld_acl_env_addr+link_id*4);
        }

        if (evt_ptr)
        {
            acl_par_ptr = evt_ptr;
            BT_DRV_TRACE(6,"acl_par:link id=%d acl_par_ptr 0x%x, clk off 0x%x, bit off 0x%x, last sync clk off 0x%x, last sync bit off 0x%x",
                         link_id, acl_par_ptr, *(uint32_t *)(acl_par_ptr+0xa8),*(uint16_t *)(acl_par_ptr+0xb8),
                         *(uint32_t *)(acl_par_ptr+0xa4),((*(uint32_t *)(acl_par_ptr+0xb8))&0xFFFF0000)>>16);
        }
    }
    hal_sys_timer_delay(MS_TO_TICKS(100));

    //ld_sco_env
    evt_ptr = 0;

    if(ld_sco_env_addr)
    {
        evt_ptr = *(uint32_t *)ld_sco_env_addr;
    }

    if(evt_ptr != 0)
    {
        BT_DRV_TRACE(1,"esco linkid :%d",*(uint8_t *)(evt_ptr+70));
    }
    btdrv_dump_mem(bt_dump_mem_start, bt_dump_len_max, BT_SUB_SYS_TYPE);
    //btdrv_dump_mem(em_dump_area_1_start, em_area_1_len_max, BT_EM_AREA_1_TYPE);
    btdrv_dump_mem(em_dump_area_2_start, em_area_2_len_max, BT_EM_AREA_2_TYPE);
    //btdrv_dump_mem(mcu_dump_mem_start, mcu_dump_len_max, MCU_SYS_TYPE);

    BT_DRV_TRACE(0,"BT crash dump complete!");

    BT_DRV_REG_OP_CLK_DIB();
}

void bt_drv_reg_op_set_swagc_mode(uint8_t mode)
{
}

bool bt_drv_reg_op_read_rssi_in_dbm(uint16_t connHandle,rx_agc_t* rx_val)
{
    bool ret = false;
#ifdef BT_RSSI_MONITOR
    BT_DRV_REG_OP_ENTER();
    BT_DRV_REG_OP_CLK_ENB();

    if (connHandle == 0xFFFF)
    {
        goto exit;
    }

    do{
        uint8_t idx = btdrv_conhdl_to_linkid(connHandle);
        /// Accumulated RSSI (to compute an average value)
        //  int16_t rssi_acc = 0;
        /// Counter of received packets used in RSSI average
        //     uint8_t rssi_avg_cnt = 1;
        rx_agc_t * rx_monitor_env = NULL;
        uint32_t acl_par_ptr = 0;
        uint32_t rssi_record_ptr=0;

        if (!btdrv_is_link_index_valid(idx))
        {
            goto exit;
        }

        if(ld_acl_env_addr)
        {
            acl_par_ptr = *(uint32_t *)(ld_acl_env_addr+idx*4);
            if(acl_par_ptr)
                rssi_record_ptr = acl_par_ptr+0xd0;
        }

        //rx_monitor
        if(rx_monitor_addr)
        {
            rx_monitor_env = (rx_agc_t*)rx_monitor_addr;
        }

        if(rssi_record_ptr != 0 && rx_monitor_env)
        {
            ret = true;
            // BT_DRV_TRACE(1,"addr=%x,rssi=%d",rssi_record_ptr,*(int8 *)rssi_record_ptr);
#ifdef __NEW_SWAGC_MODE__
            if(bt_drv_reg_op_bt_sync_swagc_en_get())
            {
                rx_val->rssi = *(int8_t *)rssi_record_ptr; // work mode NEW_SYNC_SWAGC_MODE
            }
            else
            {
                rx_val->rssi = rx_monitor_env[idx].rssi; //idle mode OLD_SWAGC_MODE
            }
#else
            rx_val->rssi = rx_monitor_env[idx].rssi;
#endif
            rx_val->rxgain = rx_monitor_env[idx].rxgain;
        }
    }while(0);

exit:
    BT_DRV_REG_OP_CLK_DIB();
    BT_DRV_REG_OP_EXIT();
#endif
    return ret;
}

bool bt_drv_reg_op_read_ble_rssi_in_dbm(uint16_t connHandle,rx_agc_t* rx_val)
{
    bool ret = false;

#ifdef BT_RSSI_MONITOR
    BT_DRV_REG_OP_ENTER();
    BT_DRV_REG_OP_CLK_ENB();

    if (connHandle == 0xFFFF)
    {
        goto exit;
    }

    do{
        uint8_t idx = connHandle;
        /// Accumulated RSSI (to compute an average value)
        int16_t rssi_acc = 0;
        /// Counter of received packets used in RSSI average
        uint8_t rssi_avg_cnt = 1;
        rx_agc_t * rx_monitor_env = NULL;
        if(idx >= MAX_NB_ACTIVE_ACL)
        {
            goto exit;
        }

        //rx_monitor

        if(ble_rx_monitor_addr)
        {
            rx_monitor_env = (rx_agc_t*)ble_rx_monitor_addr;
        }

        if(rx_monitor_env != NULL)
        {
            ret = true;

            for(int i=0; i< rssi_avg_cnt; i++)
            {
                rssi_acc += rx_monitor_env[idx].rssi;
            }
            rx_val->rssi = rssi_acc / rssi_avg_cnt;
            rx_val->rxgain = rx_monitor_env[idx].rxgain;
        }
    }while(0);

exit:
    BT_DRV_REG_OP_CLK_DIB();
    BT_DRV_REG_OP_EXIT();
#endif
    return ret;
}

void bt_drv_reg_op_ibrt_retx_att_nb_set(uint8_t retx_nb)
{
    BT_DRV_REG_OP_ENTER();
    BT_DRV_REG_OP_CLK_ENB();

    int ret = -1;
    uint32_t sco_evt_ptr = 0x0;
    // TODO: [ld_sco_env address] based on CHIP id

    if(ld_sco_env_addr)
    {
        sco_evt_ptr = *(volatile uint32_t *)ld_sco_env_addr;
        ret = 0;
    }

    if(ret == 0)
    {
        uint32_t retx_ptr=0x0;
        if(sco_evt_ptr !=0)
        {
            //offsetof retx_att_nb
            retx_ptr =sco_evt_ptr+0x58;
        }
        else
        {
            BT_DRV_TRACE(0,"BT_REG_OP:Error, ld_sco_env[0].evt ==NULL");
            ret = -2;
        }

        if(ret == 0)
        {
            *(volatile uint8_t *)retx_ptr = retx_nb;
        }
    }

    BT_DRV_TRACE(3,"BT_REG_OP:%s,ret=%d,retx nb=%d",__func__,ret,retx_nb);

    BT_DRV_REG_OP_CLK_DIB();
    BT_DRV_REG_OP_EXIT();
}

uint16_t bt_drv_reg_op_bitoff_getf(int elt_idx)
{
    uint16_t bitoff = 0;
    uint32_t acl_evt_ptr = 0x0;

    BT_DRV_REG_OP_CLK_ENB();
    if(ld_acl_env_addr)
    {
        acl_evt_ptr = *(uint32_t *)(ld_acl_env_addr+elt_idx*4);
    }

    if (acl_evt_ptr != 0)
    {
        //bitoff = acl_evt_ptr + 0xb8;
        bitoff = *(uint16_t *)(acl_evt_ptr + 0xb8);
    }
    else
    {
        BT_DRV_TRACE(1,"BT_REG_OP:ERROR LINK ID FOR RD bitoff %x", elt_idx);
    }
    BT_DRV_REG_OP_CLK_DIB();

    return bitoff;
}

uint8_t  bt_drv_reg_op_get_role(uint8_t linkid)
{
    uint32_t lc_evt_ptr=0;
    uint32_t role_ptr = 0;
    uint8_t role = 0xff;

    BT_DRV_REG_OP_CLK_ENB();
    if(lc_env_addr)
    {
        lc_evt_ptr = *(volatile uint32_t *)(lc_env_addr+linkid*4);//lc_env
    }

    if(lc_evt_ptr !=0)
    {
        //role off:0x3a
        role_ptr = lc_evt_ptr+0x3a;
        role = *(uint8_t *)role_ptr;
    }
    else
    {
        BT_DRV_TRACE(1,"BT_REG_OP:ERROR LINKID =%x",linkid);
        role = 0xff;
    }

    BT_DRV_REG_OP_CLK_DIB();

    return role;
}

void bt_drv_reg_op_set_tpoll(uint8_t linkid,uint16_t poll_interval)
{
    uint32_t acl_evt_ptr = 0x0;
    uint32_t poll_addr;

    BT_DRV_REG_OP_CLK_ENB();
    if(ld_acl_env_addr)
    {
        acl_evt_ptr = *(uint32_t *)(ld_acl_env_addr+linkid*4);
    }

    if (acl_evt_ptr != 0)
    {
        //tpoll off:0xda
        poll_addr = acl_evt_ptr + 0xda;
        *(uint16_t *)poll_addr = poll_interval;
    }
    else
    {
        BT_DRV_TRACE(1,"BT_REG_OP:ERROR LINK ID FOR TPOLL %x", linkid);
    }
    BT_DRV_REG_OP_CLK_DIB();
}

uint16_t bt_drv_reg_op_get_tpoll(uint8_t linkid)
{
    uint32_t acl_evt_ptr = 0x0;
    uint32_t poll_addr;
    uint16_t poll_interval = 0;

    BT_DRV_REG_OP_CLK_ENB();
    if(ld_acl_env_addr)
    {
        acl_evt_ptr = *(uint32_t *)(ld_acl_env_addr+linkid*4);
    }

    if (acl_evt_ptr != 0)
    {
        //tpoll off:0xda
        poll_addr = acl_evt_ptr + 0xda;
        poll_interval = *(uint16_t *)poll_addr;
    }
    else
    {
        BT_DRV_TRACE(1,"BT_REG_OP:ERROR LINK ID FOR TPOLL %x", linkid);
    }
    BT_DRV_REG_OP_CLK_DIB();

    return poll_interval;
}

int8_t  bt_drv_reg_op_rssi_correction(int8_t rssi)
{
    return rssi;
}

void bt_drv_reg_op_set_music_link(uint8_t link_id)
{
    BT_DRV_REG_OP_CLK_ENB();
    if(dbg_bt_sche_setting_addr)
    {
        *(volatile uint8_t *)(dbg_bt_sche_setting_addr+0x2) = link_id;
    }
    BT_DRV_REG_OP_CLK_DIB();
}

void bt_drv_reg_op_set_music_link_duration_extra(uint8_t slot)
{
    BT_DRV_REG_OP_CLK_ENB();
    BT_DRV_REG_OP_CLK_DIB();
}

uint32_t bt_drv_reg_op_get_reconnecting_flag()
{
    uint32_t ret = 0;
    return ret;
}

void bt_drv_reg_op_set_reconnecting_flag()
{

}

void bt_drv_reg_op_clear_reconnecting_flag()
{

}

void bt_drv_reg_op_music_link_config(uint16_t active_link,uint8_t active_role,uint16_t inactive_link,uint8_t inactive_role)
{
    BT_DRV_TRACE(4,"BT_REG_OP:bt_drv_reg_op_music_link_config %x %d %x %d",active_link,active_role,inactive_link,inactive_role);
    BT_DRV_REG_OP_CLK_ENB();
    if (active_role == 0) //MASTER
    {
        bt_drv_reg_op_set_tpoll(active_link-0x80, 0x10);
        if (inactive_role == 0)
        {
            bt_drv_reg_op_set_tpoll(inactive_link-0x80, 0x40);
        }
    }
    else
    {
        bt_drv_reg_op_set_music_link(active_link-0x80);
        bt_drv_reg_op_set_music_link_duration_extra(11);
        if (inactive_role == 0)
        {
            bt_drv_reg_op_set_tpoll(inactive_link-0x80, 0x40);
        }
    }
    BT_DRV_REG_OP_CLK_DIB();
}

bool bt_drv_reg_op_check_bt_controller_state(void)
{
    bool ret=true;
#ifdef BT_INFO_CHECKER
    if(hal_sysfreq_get() <= HAL_CMU_FREQ_32K)
        return ret;

    BT_DRV_REG_OP_ENTER();
    BT_DRV_REG_OP_CLK_ENB();
    uint32_t reg_data = BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +0);
    if((reg_data ==0x42) ||(reg_data==0))
    {
        ret = true;
    }
    else
    {
        ret = false;
    }
    if (false == ret)
    {
        BT_DRV_TRACE(1,"controller dead!!! data=%x",reg_data);
    }
    reg_data = BTDIGITAL_REG(BT_ERRORTYPESTAT_ADDR);
    if((reg_data&(~0x40)) !=0)
    {
        ret = false;
        BT_DRV_TRACE(1,"controller dead!!! BT_ERRORTYPESTAT=%x",reg_data);
    }
    BT_DRV_REG_OP_CLK_DIB();
    BT_DRV_REG_OP_EXIT();
#endif
    return ret;
}

void bt_drv_reg_op_piconet_clk_offset_get(uint16_t connHandle, int32_t *clock_offset, uint16_t *bit_offset)
{
    uint8_t index = 0;

    BT_DRV_REG_OP_ENTER();
    BT_DRV_REG_OP_CLK_ENB();
    if (connHandle)
    {
        index = btdrv_conhdl_to_linkid(connHandle);

        if (btdrv_is_link_index_valid(index))
        {
            *bit_offset = bt_drv_reg_op_bitoff_getf(index);
            *clock_offset = bt_drv_reg_op_get_clkoffset(index);
        }
        else
        {
            *bit_offset = 0;
            *clock_offset = 0;
        }
    }
    else
    {
        *bit_offset = 0;
        *clock_offset = 0;
    }
    BT_DRV_REG_OP_CLK_DIB();
    BT_DRV_REG_OP_EXIT();
}

uint8_t dma_tc_used_index = 0;
#define DMA_TC_MAX_CNT  4
void bt_drv_reg_op_enable_dma_tc(uint8_t adma_ch)
{
    uint32_t val = 0;
    uint8_t i = 0;
    uint32_t lock;
    BT_DRV_REG_OP_CLK_ENB();
    lock = int_lock();
    if(dma_tc_used_index >= DMA_TC_MAX_CNT)
    {
        //  int_unlock(lock);
        TRACE(0,"enable_dma_tc err more than max ch");
        goto exit;
    }
    val = BTDIGITAL_REG(BT_CAP_SEL_ADDR);
    for(i = 0; i < DMA_TC_MAX_CNT; i++)
    {
        if((val>>(i*8)&0x1F)==(uint32_t)(8+adma_ch))
        {
            // int_unlock(lock);
            TRACE(1,"enable_dma_tc err ch already enabled adma ch %d",adma_ch);
            goto exit;
        }
    }
    for(i = 0; i < DMA_TC_MAX_CNT; i++)
    {
        if((val>>(i*8)&0x1F)==0)
            break;
    }
    if(i >= DMA_TC_MAX_CNT)
    {
        //int_unlock(lock);
        TRACE(0,"enable_dma_tc err cannot find unused ch");
        goto exit;
    }
    dma_tc_used_index++;
    BTDIGITAL_REG_SET_FIELD(BT_CAP_SEL_ADDR,1,31,1);//reg_trig_enable
    BTDIGITAL_REG_SET_FIELD(BT_CAP_SEL_ADDR,0x1F,(i*8),(8+adma_ch));//reg_trig_selx
    TRACE(2,"enable_dma_tc succ sel reg 0x%x/%x,used %d",val,BTDIGITAL_REG(BT_CAP_SEL_ADDR),dma_tc_used_index);
exit:
    int_unlock(lock);
    BT_DRV_REG_OP_CLK_DIB();
}

void bt_drv_reg_op_disable_dma_tc(uint8_t adma_ch)
{
    uint32_t val = 0;
    uint8_t i = 0;
    uint32_t lock;
    BT_DRV_REG_OP_CLK_ENB();
    lock = int_lock();
    if(dma_tc_used_index == 0)
    {
        // int_unlock(lock);
        TRACE(0,"disable_dma_tc err no ch enabled");
        goto exit;
    }
    val = BTDIGITAL_REG(BT_CAP_SEL_ADDR);
    for(i = 0; i < DMA_TC_MAX_CNT; i++)
    {
        if((val>>(i*8)&0x1F)==(uint32_t)(8+adma_ch))
            break;
    }
    if(i >= DMA_TC_MAX_CNT)
    {
        // int_unlock(lock);
        TRACE(0,"disable_dma_tc err cannot find same ch id to disable");
        goto exit;
    }
    dma_tc_used_index--;
    BTDIGITAL_REG_SET_FIELD(BT_CAP_SEL_ADDR,0x1F,(i*8),0);//reg_trig_selx
    TRACE(2,"disable_dma_tc succ sel reg 0x%x/%x,used %d",val,BTDIGITAL_REG(BT_CAP_SEL_ADDR),dma_tc_used_index);
exit:
    int_unlock(lock);
    BT_DRV_REG_OP_CLK_DIB();
}

void bt_drv_reg_op_dma_tc_clkcnt_get(uint32_t *btclk, uint16_t *btcnt)
{
    BT_DRV_REG_OP_ENTER();
    BT_DRV_REG_OP_CLK_ENB();
    *btclk = BTDIGITAL_REG(BT_REG_CLKNCNT_CAP4_REG_ADDR);
    *btcnt = (BTDIGITAL_REG(BT_BES_FINECNT_CAP4_REG_ADDR) & 0x3ff);
    TRACE(2, "dma tc clk,cnt %d,%d", *btclk, *btcnt);
    BT_DRV_REG_OP_CLK_DIB();
    BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_op_dma_tc_clkcnt_get_by_ch(uint32_t *btclk, uint16_t *btcnt, uint8_t adma_ch)
{
    uint32_t val = 0;
    uint8_t i = 0;
    uint32_t cap_reg_base = BT_REG_CLKNCNT_CAP4_REG_ADDR;
    BT_DRV_REG_OP_ENTER();
    BT_DRV_REG_OP_CLK_ENB();
    val = BTDIGITAL_REG(BT_CAP_SEL_ADDR);
    for(i = 0; i < DMA_TC_MAX_CNT; i++)
    {
        if((val>>(i*8)&0x1F)==(uint32_t)(8+adma_ch))
            break;
    }
    if(i >= DMA_TC_MAX_CNT)
    {
        TRACE(0,"get_dma_tc err cannot find same ch id, not enabled?");
        goto exit;
        return;
    }
    *btclk = BTDIGITAL_REG(cap_reg_base+8*i);
    *btcnt = (BTDIGITAL_REG(cap_reg_base+4+8*i) & 0x3ff);
exit:
    BT_DRV_REG_OP_CLK_DIB();
    BT_DRV_REG_OP_EXIT();

}

void bt_drv_reg_op_acl_tx_silence_clear(uint16_t connHandle)
{
}

void bt_drv_reg_op_pcm_set(uint8_t en)
{
    BT_DRV_REG_OP_CLK_ENB();
    if(en)
        *(volatile unsigned int *)(BT_BES_PCMCNTL_ADDR) &= ~(0x00000080);
    else
        *(volatile unsigned int *)(BT_BES_PCMCNTL_ADDR) |= 1<<7;
    BT_DRV_REG_OP_CLK_DIB();
}

uint8_t bt_drv_reg_op_pcm_get()
{
    uint8_t ret = 1;
    BT_DRV_REG_OP_CLK_ENB();
    ret = (*(volatile unsigned int *)(BT_BES_PCMCNTL_ADDR) &0x00000080)>>7;
    BT_DRV_REG_OP_CLK_DIB();
    return ~ret;
}

const uint8_t msbc_mute_patten[]=
{
    0x01,0x38,
    0xad, 0x0,  0x0,  0xc5, 0x0,  0x0, 0x0, 0x0,
    0x77, 0x6d, 0xb6, 0xdd, 0xdb, 0x6d, 0xb7,
    0x76, 0xdb, 0x6d, 0xdd, 0xb6, 0xdb, 0x77,
    0x6d, 0xb6, 0xdd, 0xdb, 0x6d, 0xb7, 0x76,
    0xdb, 0x6d, 0xdd, 0xb6, 0xdb, 0x77, 0x6d,
    0xb6, 0xdd, 0xdb, 0x6d, 0xb7, 0x76, 0xdb,
    0x6d, 0xdd, 0xb6, 0xdb, 0x77, 0x6d, 0xb6,
    0xdd, 0xdb, 0x6d, 0xb7, 0x76, 0xdb, 0x6c,
    0x00
};
#define SCO_TX_FIFO_BASE (0xd0210000)

#if defined(CVSD_BYPASS)
#define SCO_TX_MUTE_PATTERN (0x5555)
#else
#define SCO_TX_MUTE_PATTERN (0x0000)
#endif

void bt_drv_reg_op_sco_txfifo_reset(uint16_t codec_id)
{
    BT_DRV_REG_OP_CLK_ENB();
    uint32_t reg_val = BTDIGITAL_REG(BT_E_SCOCURRENTTXPTR_ADDR);
    uint32_t reg_offset0, reg_offset1;
    uint16_t *patten_p = (uint16_t *)msbc_mute_patten;
    BT_DRV_TRACE(2,"BT_REG_OP sco reset=%x", reg_val);

    reg_offset0 = (reg_val & 0x3fff)<<2;
    reg_offset1 = ((reg_val >> 16) & 0x3fff)<<2;
    if (codec_id == 2)
    {
        for (uint8_t i=0; i<60; i+=2)
        {
            BTDIGITAL_BT_EM(SCO_TX_FIFO_BASE+reg_offset0+i) = *patten_p;
            BTDIGITAL_BT_EM(SCO_TX_FIFO_BASE+reg_offset1+i) = *patten_p;
            patten_p++;
        }
    }
    else
    {
        for (uint8_t i=0; i<120; i+=2)
        {
            BTDIGITAL_BT_EM(SCO_TX_FIFO_BASE+reg_offset0+i) = SCO_TX_MUTE_PATTERN;
            BTDIGITAL_BT_EM(SCO_TX_FIFO_BASE+reg_offset1+i) = SCO_TX_MUTE_PATTERN;
        }
    }
    BT_DRV_REG_OP_CLK_DIB();
}

/*****************************************************************************
 Prototype    : btdrv_set_tws_acl_poll_interval
 Description  : in ibrt mode, set tws acl poll interval
 Input        : uint16_t poll_interval
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/4/19
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void btdrv_reg_op_set_private_tws_poll_interval(uint16_t poll_interval, uint16_t poll_interval_in_sco)
{
    //interval_ibrt_estab=poll_interval/2 and interval must be odd
    ASSERT_ERR((poll_interval % 4 == 0));

    BT_DRV_REG_OP_CLK_ENB();
    uint32_t interval_addr = 0;
    uint32_t interval_insco_addr = 0;
    uint32_t interval_ibrt_estab_addr = 0;


    if(dbg_bt_sche_setting_addr != 0)
    {
        interval_ibrt_estab_addr = dbg_bt_sche_setting_addr + 0x7;
        interval_insco_addr =  dbg_bt_sche_setting_addr+0x8;
        interval_addr = dbg_bt_sche_setting_addr+ 0xa;

        do{
            if(BT_DRIVER_GET_U16_REG_VAL(interval_addr) != poll_interval && poll_interval!=0xffff)
            {
                BT_DRIVER_PUT_U16_REG_VAL(interval_addr, poll_interval);
                BT_DRIVER_PUT_U8_REG_VAL(interval_ibrt_estab_addr, (poll_interval / 2));
                BT_DRV_TRACE(2,"BT_REG_OP:Set private tws interval,acl interv=%d", poll_interval);
            }
            if(BT_DRIVER_GET_U16_REG_VAL(interval_insco_addr) != poll_interval_in_sco && poll_interval_in_sco!=0xffff)
            {
                BT_DRIVER_PUT_U16_REG_VAL(interval_insco_addr, poll_interval_in_sco);

                BT_DRV_TRACE(2," acl interv insco =%d", poll_interval_in_sco);
           }
        }while(0);
    }
    BT_DRV_REG_OP_CLK_DIB();
}

void btdrv_reg_op_set_tws_link_duration(uint8_t slot_num)
{
    BT_DRV_REG_OP_ENTER();
    BT_DRV_REG_OP_CLK_ENB();
    //dbg_setting address
    uint32_t op_addr = 0;

    if(dbg_bt_sche_setting_addr)
    {
        op_addr = dbg_bt_sche_setting_addr;
    }

    if(op_addr != 0 && slot_num != 0xff)
    {
        //acl_slot_in_ibrt_mode off:0x6
        uint16_t val = *(volatile uint16_t *)(op_addr+0x6); //acl_slot_in_snoop_mode
        val&=0xff00;
        val|= slot_num;
        *(volatile uint16_t *)(op_addr+0x6) = val;
        BT_DRV_TRACE(1,"BT_REG_OP:Set private tws link duration,val=%d",*((volatile uint16_t*)(op_addr+0x6))&0xff);
    }
    BT_DRV_REG_OP_CLK_DIB();
    BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_op_fastack_status_checker(uint16_t conhdl)
{
    BT_DRV_REG_OP_ENTER();
    BT_DRV_REG_OP_CLK_ENB();

    uint8_t elt_idx = btdrv_conhdl_to_linkid(conhdl);

    if (btdrv_is_link_index_valid(elt_idx))
    {
        BT_DRV_TRACE(2,"BT_DRV_REG:fast ack reg=0x%x",
                     *(volatile uint32_t *)(BT_BES_CNTL2_ADDR));//fast ack reg bit4
    }
    bt_drv_reg_op_bt_info_checker();

    BT_DRV_REG_OP_CLK_DIB();
    BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_op_write_private_public_key(uint8_t* private_key,uint8_t* public_key)
{
    uint8_t* lm_env_ptr = 0;
    uint8_t* lm_private_key_ptr = 0;
    uint8_t* lm_public_key_ptr = 0;

    BT_DRV_REG_OP_CLK_ENB();
    //lm_env
    if(lm_key_env_addr)
    {
        lm_env_ptr = (uint8_t*)lm_key_env_addr;
    }
    else
    {
        goto exit;
    }
    //priv_key_192 off:6c
    lm_private_key_ptr = lm_env_ptr;
    //pub_key_192 off:0x84
    lm_public_key_ptr = lm_private_key_ptr + 0x18;
    memcpy(lm_private_key_ptr,private_key,24);
    memcpy(lm_public_key_ptr,public_key,48);
    BT_DRV_TRACE(0,"BT_REG_OP:private key");
    DUMP8("%02x",lm_private_key_ptr,24);
    BT_DRV_TRACE(0,"public key");
    DUMP8("%02x",lm_public_key_ptr,48);
exit:
    BT_DRV_REG_OP_CLK_DIB();
}

void bt_drv_reg_op_for_test_mode_disable(void)
{
    BT_DRV_REG_OP_CLK_ENB();
    if(dbg_bt_common_setting_addr)
    {
        *(volatile uint8_t *)(dbg_bt_common_setting_addr+0x36) = 0;////sw_seq_filter_en set 0
        *(volatile uint8_t *)(dbg_bt_common_setting_addr) = 0;////dbg_trace_level set 0
    }
    if (dbg_bt_common_setting_t2_addr)
    {
        *(volatile uint8_t *)(dbg_bt_common_setting_t2_addr+9) = 0;///bt_sync_found_hecerror_check
    }
    if (testmode_3m_en_addr)
    {
        *(volatile uint8_t *)(testmode_3m_en_addr) = 1;///testmode_3m use special rx gain table
    }
    if (hci_dbg_set_sw_rssi_addr)
    {
        *(uint16_t *)(hci_dbg_set_sw_rssi_addr+40) = 10;  //bt_link_no_snyc_thd
    }
    BT_DRV_REG_OP_CLK_DIB();
}

uint16_t bt_drv_reg_op_get_ibrt_sco_hdl(uint16_t acl_hdl)
{
    // FIXME
    return acl_hdl|0x100;
}

void bt_drv_reg_op_set_tws_link_id(uint8_t link_id)
{
    BT_DRV_TRACE(1,"set tws link id =%x",link_id);
    ASSERT(link_id <MAX_NB_ACTIVE_ACL || link_id == 0xff,"BT_REG_OP:error tws link id set");
    uint32_t tws_link_id_addr = 0;

    BT_DRV_REG_OP_CLK_ENB();
    if(ld_bes_bt_env_addr != 0)
    {
        tws_link_id_addr = ld_bes_bt_env_addr + 0xc;
    }

    if(tws_link_id_addr != 0)
    {
        if(link_id == 0xff)
        {
            link_id = MAX_NB_ACTIVE_ACL;
        }
        *(uint8_t *)tws_link_id_addr = link_id;
    }
    BT_DRV_REG_OP_CLK_DIB();
}

void bt_drv_reg_op_set_link_policy(uint8_t linkid, uint8_t policy)
{
    BT_DRV_REG_OP_ENTER();
    BT_DRV_REG_OP_CLK_ENB();

    uint32_t lc_evt_ptr = 0x0;
    uint32_t policy_addr;
    if(linkid>=MAX_NB_ACTIVE_ACL)
    {
        goto exit;
    }

    BT_DRV_TRACE(2,"BT_REG_OP: set link=%d, policy=%d",linkid,policy);

    if(lc_env_addr)
    {
        lc_evt_ptr = *(volatile uint32_t *)(lc_env_addr+linkid*4);//lc_env
    }

    if(lc_evt_ptr)
    {
        //LinkPlicySettings off:26
        policy_addr = lc_evt_ptr+0x26;
        *(uint8_t *)policy_addr = policy;
    }
exit:
    BT_DRV_REG_OP_CLK_DIB();
    BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_sco_fifo_dump(void)
{
    BT_DRV_REG_OP_ENTER();
    BT_DRV_REG_OP_CLK_ENB();
    uint32_t reg_val_tx = BTDIGITAL_REG(BT_E_SCOCURRENTTXPTR_ADDR);
    uint32_t reg_tx_offset0, reg_tx_offset1;

    uint32_t reg_val_rx = BTDIGITAL_REG(BT_E_SCOCURRENTRXPTR_ADDR);
    uint32_t reg_rx_offset0, reg_rx_offset1;

    //BT_DRV_TRACE(2,"BT_REG_OP sco tx dump=%x, rx dump = %x", reg_val_tx, reg_val_rx);

    reg_tx_offset0 = (reg_val_tx & 0x3fff)<<2;
    reg_tx_offset1 = ((reg_val_tx >> 16) & 0x3fff)<<2;

    reg_rx_offset0 = (reg_val_rx & 0x3fff)<<2;
    reg_rx_offset1 = ((reg_val_rx >> 16) & 0x3fff)<<2;

    BT_DRV_TRACE(0, "sco tx");
    DUMP8("%02x ", (uint8_t *)(SCO_TX_FIFO_BASE+reg_tx_offset0), 10);
    DUMP8("%02x ", (uint8_t *)(SCO_TX_FIFO_BASE+reg_tx_offset1), 10);
    BT_DRV_TRACE(0, "sco rx");
    DUMP8("%02x ", (uint8_t *)(SCO_TX_FIFO_BASE+reg_rx_offset0), 10);
    DUMP8("%02x ", (uint8_t *)(SCO_TX_FIFO_BASE+reg_rx_offset1), 10);

    BT_DRV_REG_OP_CLK_DIB();
    BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_op_ble_sup_timeout_set(uint16_t ble_conhdl, uint16_t sup_to)
{
    uint32_t llc_env_ptr = 0;

    BT_DRV_REG_OP_CLK_ENB();

    if(llc_env_addr)
    {
        uint32_t llc_env_address = llc_env_addr+ble_conhdl*4;
        llc_env_ptr = *(volatile uint32_t *)llc_env_address;
    }

    if(llc_env_ptr != 0)
    {
        //sup_to off:0x12
        uint32_t llc_env_sup_to_address = llc_env_ptr + 0x12;
        *(volatile uint16_t *)llc_env_sup_to_address = sup_to;
        BT_DRV_TRACE(1,"BT_REG_OP:set ble sup_timeout to %d",sup_to);
    }

    BT_DRV_REG_OP_CLK_DIB();
}

void bt_drv_reg_op_hw_sw_agc_select(uint8_t agc_mode)
{
    uint32_t hw_agc_select_addr = 0;
    BT_DRV_REG_OP_ENTER();
    BT_DRV_REG_OP_CLK_ENB();

    if(rwip_rf_addr)
    {
        //hw_or_sw_agc off:0x3b
        hw_agc_select_addr = rwip_rf_addr+0x3b;
    }
    if(hw_agc_select_addr != 0)
    {
        *(volatile uint8_t *)hw_agc_select_addr = agc_mode;
    }

    BT_DRV_REG_OP_CLK_DIB();
    BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_op_afh_follow_en(bool enable, uint8_t be_followed_link_id, uint8_t follow_link_id)
{
#if BT_AFH_FOLLOW
    BT_DRV_REG_OP_CLK_ENB();
    *(uint8_t *)(lm_env_addr+0xbf) = be_followed_link_id;//be follow follow link id //ok
    *(uint8_t *)(lm_env_addr+0xc0) = follow_link_id;//follow link id
    *(uint8_t *)(lm_env_addr+0xbe) = enable; //afh follow enable
    BT_DRV_REG_OP_CLK_DIB();
#endif
}

#define SNIFF_ATT_OFFSET (0x102)
#define SNIFF_TO_OFFSET (0x106)
void bt_drv_reg_op_force_set_sniff_att(uint16_t conhdle)
{
    BT_DRV_REG_OP_ENTER();
    BT_DRV_REG_OP_CLK_ENB();

    uint8_t linkid = btdrv_conhdl_to_linkid(conhdle);
    uint32_t acl_evt_ptr = 0x0;

    if (ld_acl_env_addr != 0&& btdrv_is_link_index_valid(linkid))
    {
        acl_evt_ptr=*(volatile uint32_t *)(ld_acl_env_addr+linkid*4);//ld_acl_env

        if (acl_evt_ptr != 0)
        {
            uint32_t sniff_att_addr = acl_evt_ptr + SNIFF_ATT_OFFSET;
            if(BT_DRIVER_GET_U16_REG_VAL(sniff_att_addr) < 3)
            {
                BT_DRIVER_PUT_U16_REG_VAL(sniff_att_addr, 3);
            }
            uint32_t sniff_to_addr = acl_evt_ptr + SNIFF_TO_OFFSET;
            if(BT_DRIVER_GET_U16_REG_VAL(sniff_to_addr) > 0)
            {
                BT_DRIVER_PUT_U16_REG_VAL(sniff_to_addr, 0);
            }
            BT_DRV_TRACE(0,"BT_REG_OP:force set sniff att and timeout");
        }
    }
    BT_DRV_REG_OP_CLK_DIB();
    BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_op_fa_gain_direct_set(uint8_t gain_idx)
{
}

uint8_t bt_drv_reg_op_fa_gain_direct_get(void)
{
    return 0;
}

struct rx_gain_fixed_t
{
    uint8_t     enable;//enable or disable
    uint8_t     bt_or_ble;//0:bt 1:ble
    uint8_t     cs_id;//link id
    uint8_t     gain_idx;//gain index
};

void bt_drv_reg_op_dgb_link_gain_ctrl_set(uint16_t connHandle, uint8_t bt_ble_mode, uint8_t gain_idx, uint8_t enable)
{
    BT_DRV_REG_OP_CLK_ENB();
    if(rf_rx_gain_fixed_tbl_addr != 0)
    {
        //struct rx_gain_fixed_t RF_RX_GAIN_FIXED_TBL[HOST_GAIN_TBL_MAX];
        struct rx_gain_fixed_t *rx_gain_fixed_p = (struct rx_gain_fixed_t *)rf_rx_gain_fixed_tbl_addr;
        uint8_t cs_id = btdrv_conhdl_to_linkid(connHandle);

        if (btdrv_is_link_index_valid(cs_id))
        {
            for (uint8_t i=0; i<MAX_NB_ACTIVE_ACL; i++)
            {
                if ((rx_gain_fixed_p->cs_id == cs_id) &&
                    (rx_gain_fixed_p->bt_or_ble == bt_ble_mode))
                {
                    rx_gain_fixed_p->enable    = enable;
                    rx_gain_fixed_p->bt_or_ble = bt_ble_mode;
                    rx_gain_fixed_p->gain_idx = gain_idx;
                    BT_DRV_TRACE(5,"BT_REG_OP:%s hdl:%x/%x mode:%d idx:%d", __func__, connHandle, cs_id, rx_gain_fixed_p->bt_or_ble, gain_idx);
                }
                rx_gain_fixed_p++;
            }
        }
    }
    BT_DRV_REG_OP_CLK_DIB();
}


void bt_drv_reg_op_dgb_link_gain_ctrl_clear(uint16_t connHandle, uint8_t bt_ble_mode)
{
    BT_DRV_REG_OP_CLK_ENB();
    if(rf_rx_gain_fixed_tbl_addr != 0)
    {
        //struct rx_gain_fixed_t RF_RX_GAIN_FIXED_TBL[HOST_GAIN_TBL_MAX];
        struct rx_gain_fixed_t *rx_gain_fixed_p = (struct rx_gain_fixed_t *)rf_rx_gain_fixed_tbl_addr;
        uint8_t cs_id = btdrv_conhdl_to_linkid(connHandle);

        if (btdrv_is_link_index_valid(cs_id))
        {
            for (uint8_t i=0; i<MAX_NB_ACTIVE_ACL; i++)
            {
                if ((rx_gain_fixed_p->cs_id == cs_id) &&
                    (rx_gain_fixed_p->bt_or_ble == bt_ble_mode))
                {
                    rx_gain_fixed_p->enable    = 0;
                    rx_gain_fixed_p->bt_or_ble = bt_ble_mode;
                    rx_gain_fixed_p->gain_idx = 0;
                }
                rx_gain_fixed_p++;
            }
        }
    }
    BT_DRV_REG_OP_CLK_DIB();
}


void bt_drv_reg_op_dgb_link_gain_ctrl_init(void)
{
    BT_DRV_REG_OP_CLK_ENB();
    if(rf_rx_gain_fixed_tbl_addr != 0)
    {
        //struct rx_gain_fixed_t RF_RX_GAIN_FIXED_TBL[HOST_GAIN_TBL_MAX];
        struct rx_gain_fixed_t *rx_gain_fixed_p = (struct rx_gain_fixed_t *)rf_rx_gain_fixed_tbl_addr;

        for (uint8_t i=0; i<MAX_NB_ACTIVE_ACL; i++)
        {
            rx_gain_fixed_p->cs_id = i;
            rx_gain_fixed_p++;
        }
    }
    BT_DRV_REG_OP_CLK_DIB();
}

void bt_drv_reg_op_rx_gain_fix(uint16_t connHandle, uint8_t bt_ble_mode, uint8_t gain_idx, uint8_t enable, uint8_t table_idx)
{
    BT_DRV_REG_OP_CLK_ENB();
    if(rf_rx_gain_fixed_tbl_addr != 0)
    {
        //struct rx_gain_fixed_t RF_RX_GAIN_FIXED_TBL[HOST_GAIN_TBL_MAX];
        struct rx_gain_fixed_t *rx_gain_fixed_p = (struct rx_gain_fixed_t *)rf_rx_gain_fixed_tbl_addr;
        uint8_t cs_id;

        if(bt_ble_mode == 0)//bt
        {
            cs_id = btdrv_conhdl_to_linkid(connHandle);
        }
        else if(bt_ble_mode == 1)//ble
        {
            cs_id = connHandle;
        }
        else
        {
            BT_DRV_TRACE(1,"BT_REG_OP:%s:fix gain fail",__func__);
            goto exit;
        }

        if(table_idx < MAX_NB_ACTIVE_ACL)
        {
            rx_gain_fixed_p[table_idx].enable = enable;
            rx_gain_fixed_p[table_idx].bt_or_ble = bt_ble_mode;
            rx_gain_fixed_p[table_idx].cs_id = cs_id;
            rx_gain_fixed_p[table_idx].gain_idx = gain_idx;
        }
    }
exit:
    BT_DRV_REG_OP_CLK_DIB();
}

void bt_drv_reg_op_set_ibrt_reject_sniff_req(bool en)
{
    BT_DRV_REG_OP_ENTER();
    BT_DRV_REG_OP_CLK_ENB();
    uint8_t *accept_remote_enter_sniff_ptr = NULL;
    uint32_t sniff_policy_addr = 0;

    if (dbg_bt_ibrt_setting_addr == 0)
    {
        BT_DRV_TRACE(1, "BT_REG_OP:set reject sniff req %d, please fix it", en);
        goto exit;
    }

    sniff_policy_addr = dbg_bt_ibrt_setting_addr + 0x19;

    if(BT_DRIVER_GET_U8_REG_VAL(sniff_policy_addr) == en)
    {
        BT_DRV_TRACE(1, "BT_REG_OP:set reject sniff req %d", en);

        accept_remote_enter_sniff_ptr = (uint8_t *)(sniff_policy_addr);

        *accept_remote_enter_sniff_ptr = en ? 0 : 1;
    }
exit:
    BT_DRV_REG_OP_CLK_DIB();
    BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_op_set_ibrt_second_sco_decision(uint8_t value)
{
    BT_DRV_REG_OP_ENTER();
    BT_DRV_REG_OP_CLK_ENB();
    uint8_t *ibrt_second_sco_decision_ptr = NULL;

    if (dbg_bt_ibrt_setting_addr == 0)
    {
        BT_DRV_TRACE(1, "BT_REG_OP:set ibrt second sco decision %d, please fix it", value);
        goto exit;
    }

    ibrt_second_sco_decision_ptr = (uint8_t *)(dbg_bt_ibrt_setting_addr + 21);

    *ibrt_second_sco_decision_ptr = value;

exit:
    BT_DRV_REG_OP_CLK_DIB();
    BT_DRV_REG_OP_EXIT();
}

uint8_t bt_drv_reg_op_get_sync_id_by_conhdl(uint16_t conhdl)
{
    uint8_t sco_link_id = MAX_NB_SYNC;
    uint32_t p_link_params_addr = 0;
    uint32_t link_id_addr = 0;

    BT_DRV_REG_OP_ENTER();
    BT_DRV_REG_OP_CLK_ENB();

    uint8_t idx = btdrv_conhdl_to_linkid(conhdl);
    if (!btdrv_is_link_index_valid(idx))
    {
        goto exit;
    }

    if(lc_sco_env_addr != 0)
    {
        // Find the SCO link under negotiation
        for(uint8_t i = 0; i < MAX_NB_SYNC ; i++)
        {
            p_link_params_addr = BT_DRIVER_GET_U32_REG_VAL(lc_sco_env_addr + i * 8);
            link_id_addr = p_link_params_addr + 2;
            // Check if sco link ID is free
            if(p_link_params_addr != 0)
            {
                if(BT_DRIVER_GET_U16_REG_VAL(link_id_addr) == idx)
                {
                    sco_link_id = i;
                    break;
                }
            }
        }
    }
exit:
    BT_DRV_REG_OP_CLK_DIB();
    BT_DRV_REG_OP_EXIT();
    return sco_link_id;
}

uint8_t bt_drv_reg_op_get_esco_nego_airmode(uint8_t sco_link_id)
{
    uint8_t airmode = 0xff;
    BT_DRV_REG_OP_ENTER();
    BT_DRV_REG_OP_CLK_ENB();

    if(lc_sco_env_addr != 0)
    {
        uint32_t p_nego_params_addr = BT_DRIVER_GET_U32_REG_VAL(lc_sco_env_addr + 4 + sco_link_id * 8);
        if (sco_link_id >= 2 || p_nego_params_addr == 0)
        {
            goto exit;
        }
        //air_mode off:5e
        airmode = BT_DRIVER_GET_U8_REG_VAL(p_nego_params_addr + 0x5e);

        BT_DRV_TRACE(1,"BT_REG_OP:Get nego esco airmode=%d",airmode);
    }

exit:
    BT_DRV_REG_OP_CLK_DIB();
    BT_DRV_REG_OP_EXIT();
    return airmode;
}

#if defined(PCM_FAST_MODE) && defined(PCM_PRIVATE_DATA_FLAG)
void bt_drv_reg_op_set_pcm_flag()
{
    BT_DRV_REG_OP_ENTER();
    BT_DRV_REG_OP_CLK_ENB();

    //config btpcm slot
    BTDIGITAL_REG_SET_FIELD(BT_BES_ENHPCM_CNTL_ADDR,0x1,0,1);
    BTDIGITAL_REG_SET_FIELD(BT_BES_ENHPCM_CNTL_ADDR,0x4,1,7);

    //config private data
    BTDIGITAL_REG_SET_FIELD(0xd0220c88,0xff,8,0x4b);
    BTDIGITAL_REG_SET_FIELD(0xd0220c88,0x1,15,1);
    BTDIGITAL_REG_SET_FIELD(0xd0220c88,0x1,17,1);
//    BTDIGITAL_REG_SET_FIELD(0xd0220648,0x1ff,9,8);
//    BTDIGITAL_REG_SET_FIELD(0xd0220648,0x1,31,1);

//    BTDIGITAL_REG_SET_FIELD(BT_BES_CNTL5_ADDR, 1, 11, 1);

    BT_DRV_REG_OP_CLK_DIB();
    BT_DRV_REG_OP_EXIT();
}
#endif

#ifndef __BT_RAMRUN__
void bt_drv_reg_op_ebq_test_setting(void)
{
    BT_DRV_REG_OP_CLK_ENB();
    struct dbg_set_ebq_test_cmd* set_ebq_test = (struct dbg_set_ebq_test_cmd*)hci_dbg_ebq_test_mode_addr;
    if(set_ebq_test)
    {
        BT_DRV_TRACE(0,"BT_REG_OP:ebq_test_setting");
        set_ebq_test->ssr = 0;//ebq case bb/prot/ssr/bv03 06
        set_ebq_test->aes_ccm_daycounter = 1;
        set_ebq_test->bb_prot_flh_bv02 = 0;
        set_ebq_test->bb_prot_arq_bv43 = 0;
        set_ebq_test->enc_mode_req = 0;
        set_ebq_test->lmp_lih_bv126 = 0;
        set_ebq_test->lsto_add = 5;
        set_ebq_test->bb_xcb_bv06 = 0;
        set_ebq_test->bb_xcb_bv20 = 0;
        set_ebq_test->bb_inq_pag_bv01 = 0;
        set_ebq_test->sam_status_change_evt = 0;
        set_ebq_test->sam_remap = 0;
        set_ebq_test->ll_con_sla_bi02c = 0;
        set_ebq_test->ll_con_sla_bi02c_offset = 0;
        set_ebq_test->ll_pcl_mas_sla_bv01 = 0;
        set_ebq_test->ll_pcl_sla_bv29 = 0;
        set_ebq_test->bb_prot_arq_bv0608 = 0;
        set_ebq_test->lmp_lih_bv48 = 0;
        set_ebq_test->hfp_hf_acs_bv17_i = 0;
        set_ebq_test->ll_not_support_phy = 0;
        set_ebq_test->ll_iso_hci_in_sdu_len_flag = 1;
        set_ebq_test->ll_iso_hci_in_buf_num = 0;
        set_ebq_test->ll_cis_mic_present = 0;
        set_ebq_test->ll_cig_param_check_disable = 1;
        set_ebq_test->ll_total_num_iso_data_pkts = 6;
        set_ebq_test->ll_isoal_ready_prepare = 1;
        set_ebq_test->framed_cis_param = 1;//ll/cis/mas/bv26c,bv28c
        set_ebq_test->cis_offset_min = SLOT_SIZE*2;//us
        set_ebq_test->ll_bi_alarm_init_distance = 12;//(in half slots)
        set_ebq_test->lld_sync_duration_min = HALF_SLOT_SIZE;//(in half us)

    }
    BT_DRV_REG_OP_CLK_DIB();
}

#else //__BT_RAMRUN__

void bt_drv_reg_op_ebq_test_setting(void)
{
    BT_DRV_REG_OP_CLK_ENB();
    struct dbg_set_ebq_test_cmd* set_ebq_test = (struct dbg_set_ebq_test_cmd*)hci_dbg_ebq_test_mode_addr;
    if(set_ebq_test)
    {
        BT_DRV_TRACE(0,"BT_REG_OP:ebq_test_setting");
        set_ebq_test->ssr = 0;//ebq case bb/prot/ssr/bv03 06
        set_ebq_test->aes_ccm_daycounter = 1;
        set_ebq_test->bb_prot_flh_bv02 = 0;
        set_ebq_test->bb_prot_arq_bv43 = 0;
        set_ebq_test->enc_mode_req = 0;
        set_ebq_test->lmp_lih_bv126 = 0;
        set_ebq_test->lsto_add = 5;
        set_ebq_test->bb_xcb_bv06 = 0;
        set_ebq_test->bb_xcb_bv20 = 0;
        set_ebq_test->bb_inq_pag_bv01 = 0;
        set_ebq_test->sam_status_change_evt = 0;
        set_ebq_test->sam_remap = 0;
        set_ebq_test->ll_con_sla_bi02c = 0;
        set_ebq_test->ll_con_sla_bi02c_offset = 0;
        set_ebq_test->ll_pcl_mas_sla_bv01 = 0;
        set_ebq_test->ll_pcl_sla_bv29 = 0;
        set_ebq_test->bb_prot_arq_bv0608 = 0;
        set_ebq_test->lmp_lih_bv48 = 0;
        set_ebq_test->hfp_hf_acs_bv17_i = 0;
        set_ebq_test->ll_not_support_phy = 0;
        set_ebq_test->ll_iso_hci_in_sdu_len_flag = 1;
        set_ebq_test->ll_iso_hci_in_buf_num = 0;
        set_ebq_test->ll_cis_mic_present = 0;
        set_ebq_test->ll_cig_param_check_disable = 1;
        set_ebq_test->ll_total_num_iso_data_pkts = 6;
        set_ebq_test->cis_offset_min = SLOT_SIZE*2;//us
        set_ebq_test->cis_sub_event_duration_config = 0;//24*HALF_SLOT_SIZE
        set_ebq_test->ll_bi_alarm_init_distance = 12;//(in half slots)
        set_ebq_test->lld_sync_duration_min = HALF_SLOT_SIZE;//(in half us)
        //iso rx test case
        set_ebq_test->lld_le_duration_min = 0;//HALF_SLOT_SIZE;//(in half us)
        //iso rx test case
        set_ebq_test->lld_cibi_sync_win_min_distance = 500;//(in half us)
        //cis last subevent duration min use half slot size
        set_ebq_test->iso_sub_event_duration_config = 1;
        //ebq bug only ll_bis_snc_bv7 and ll_bis_snc_bv11 enable
        set_ebq_test->ll_bis_snc_bv7_bv11 = 0;
    }
    BT_DRV_REG_OP_CLK_DIB();
}
#endif//__BT_RAMRUN__

uint8_t bt_drv_reg_op_get_controller_ble_tx_free_buffer(void)
{
    uint32_t *tx_buf_ptr=NULL;
    uint8_t tx_free_buf_count=0;
    BT_DRV_REG_OP_CLK_ENB();

    if(ble_util_buf_env_addr)
    {
        tx_buf_ptr = (uint32_t *)(ble_util_buf_env_addr+0x28); //ble_util_buf_env.acl_tx_free
    }
    else
    {
        BT_DRV_TRACE(1, "REG_OP: please fix %s", __func__);
        tx_free_buf_count =  0;
    }

    //check tx buff
    while(tx_buf_ptr && *tx_buf_ptr)
    {
        tx_free_buf_count++;
        tx_buf_ptr = (uint32_t *)(*tx_buf_ptr);
    }
    BT_DRV_REG_OP_CLK_DIB();

    return tx_free_buf_count;
}

void bt_drv_reg_op_ble_sync_swagc_en_set(uint8_t en)
{
    BT_DRV_REG_OP_CLK_ENB();

    uint8_t *le_sync_swagc_en = NULL;

    if (dbg_bt_hw_feat_setting_addr != 0)
    {
        le_sync_swagc_en = (uint8_t *)(dbg_bt_hw_feat_setting_addr + 0xb);

        *le_sync_swagc_en = en;
    }

    BT_DRV_REG_OP_CLK_DIB();
}

void bt_drv_reg_op_bt_sync_swagc_en_set(uint8_t en)
{
    BT_DRV_REG_OP_CLK_ENB();

    uint8_t *bt_sync_swagc_en = NULL;

    if (dbg_bt_hw_feat_setting_addr != 0)
    {
        bt_sync_swagc_en = (uint8_t *)(dbg_bt_hw_feat_setting_addr + 0xa);

        *bt_sync_swagc_en = en;
    }

    BT_DRV_REG_OP_CLK_DIB();
}

uint8_t bt_drv_reg_op_bt_sync_swagc_en_get(void)
{
    uint8_t *bt_sync_swagc_en = NULL;
    uint8_t ret = 0;
    BT_DRV_REG_OP_CLK_ENB();

    if (dbg_bt_hw_feat_setting_addr != 0)
    {
        bt_sync_swagc_en = (uint8_t *)(dbg_bt_hw_feat_setting_addr + 0xa);
    }

    if (bt_sync_swagc_en)
    {
        ret = *bt_sync_swagc_en;
    }

    BT_DRV_REG_OP_CLK_DIB();

    return ret;
}

void bt_drv_reg_op_swagc_mode_set(uint8_t mode)
{
#ifdef __NEW_SWAGC_MODE__
    BT_DRV_REG_OP_CLK_ENB();

    uint32_t lock = int_lock();
    if(mode == NEW_SYNC_SWAGC_MODE)
    {
        //open rf new sync agc mode
        bt_drv_rf_set_bt_sync_agc_enable(true);
        //open BT sync AGC process cbk
        bt_drv_reg_op_bt_sync_swagc_en_set(1);
    }
    else if(mode == OLD_SWAGC_MODE)
    {
        //close rf new sync agc mode
        bt_drv_rf_set_bt_sync_agc_enable(false);
        //close BT sync AGC process cbk
        bt_drv_reg_op_bt_sync_swagc_en_set(0);

        //[19:8]: rrcgain
        //[7]: rrc_engain
        BTDIGITAL_REG_SET_FIELD(0xd03502c0, 0x1,  7, 1);
        BTDIGITAL_REG_SET_FIELD(0xd03502c0, 0xfff,  8, 0x80);
    }
    int_unlock(lock);
    BT_DRV_REG_OP_CLK_DIB();
#endif
}

void bt_drv_reg_op_key_gen_after_reset(bool enable)
{
    BT_DRV_REG_OP_CLK_ENB();
    if(dbg_bt_common_setting_addr)
    {
        *(volatile uint8_t *)(dbg_bt_common_setting_addr+0x18)=enable;
    }
    BT_DRV_REG_OP_CLK_DIB();
}

void bt_drv_reg_op_set_fa_rx_gain_idx(uint8_t rx_gain_idx)
{
    BT_DRV_REG_OP_ENTER();
    BT_DRV_REG_OP_CLK_ENB();
    uint8_t * fa_gain_idx = NULL;

    if (dbg_bt_hw_feat_setting_addr != 0)
    {
        fa_gain_idx = (uint8_t *)(dbg_bt_hw_feat_setting_addr + 0xf);

        *fa_gain_idx = rx_gain_idx;
    }

    BT_DRV_REG_OP_CLK_DIB();
    BT_DRV_REG_OP_EXIT();
}

#ifdef PCM_PRIVATE_DATA_FLAG
void bt_drv_reg_op_sco_pri_data_init()
{
    BT_DRV_REG_OP_ENTER();
    BT_DRV_REG_OP_CLK_ENB();
    BTDIGITAL_REG_WR(BT_BES_PCM_DF_REG1_ADDR,BTDIGITAL_REG(BT_BES_PCM_DF_REG1_ADDR)|=0x000000ff);
    BTDIGITAL_REG_WR(BT_BES_PCM_DF_REG_ADDR,BTDIGITAL_REG(BT_BES_PCM_DF_REG_ADDR)|=0x00ffffff);
    BT_DRV_REG_OP_CLK_DIB();
    BT_DRV_REG_OP_EXIT();
}
#endif

void bt_drv_reg_op_init_rssi_setting(void)
{
    BT_DRV_REG_OP_CLK_ENB();

    if(hci_dbg_set_sw_rssi_addr)
    {
        *(uint8_t *)(hci_dbg_set_sw_rssi_addr+0) = 1; //sw_rssi_en
        *(uint32_t *)(hci_dbg_set_sw_rssi_addr+4) = 80;  //link_agc_thd_mobile
        *(uint32_t *)(hci_dbg_set_sw_rssi_addr+8) = 400;  //link_agc_thd_mobile_time
        *(uint32_t *)(hci_dbg_set_sw_rssi_addr+12) = 80;  //link_agc_thd_tws
        *(uint32_t *)(hci_dbg_set_sw_rssi_addr+16) = 400;  //link_agc_thd_tws_time
        *(uint8_t *)(hci_dbg_set_sw_rssi_addr+20) = 3;  //rssi_mobile_step
        *(uint8_t *)(hci_dbg_set_sw_rssi_addr+21) = 3;  //rssi_tws_step
        *(int8_t *)(hci_dbg_set_sw_rssi_addr+22) = -100;  //rssi_min_value_mobile
        *(int8_t *)(hci_dbg_set_sw_rssi_addr+23) = -100;  //rssi_min_value_tws
        *(uint8_t *)(hci_dbg_set_sw_rssi_addr+24) = 0;  //ble_sw_rssi_en
        *(uint32_t *)(hci_dbg_set_sw_rssi_addr+28) = 80;  //ble_link_agc_thd
        *(uint32_t *)(hci_dbg_set_sw_rssi_addr+32) = 100;  //ble_link_agc_thd_time
        *(uint8_t *)(hci_dbg_set_sw_rssi_addr+36) = 3;  //ble_rssi_step
        *(int8_t *)(hci_dbg_set_sw_rssi_addr+37) = -100;  //ble_rssidbm_min_value
        *(uint8_t *)(hci_dbg_set_sw_rssi_addr+38) = 1;  //bt_no_sync_en
        *(int8_t *)(hci_dbg_set_sw_rssi_addr+39) = -90;  //bt_link_no_sync_rssi
        *(uint16_t *)(hci_dbg_set_sw_rssi_addr+40) = 80;  //bt_link_no_snyc_thd
        *(uint16_t *)(hci_dbg_set_sw_rssi_addr+42) = 200;  //bt_link_no_sync_timeout
        *(uint8_t *)(hci_dbg_set_sw_rssi_addr+44) = 1;  //ble_no_sync_en
        *(int8_t *)(hci_dbg_set_sw_rssi_addr+45) = -90;  //ble_link_no_sync_rssi
        *(uint16_t *)(hci_dbg_set_sw_rssi_addr+46) = 20;  //ble_link_no_snyc_thd
        *(uint16_t *)(hci_dbg_set_sw_rssi_addr+48) = 800;  //ble_link_no_sync_timeout
    }
    BT_DRV_REG_OP_CLK_DIB();

}

uint32_t bt_drv_us_2_lpcycles(uint32_t us,uint32_t lp_clk)
{
    uint64_t lpcycles;
    lpcycles = ((uint64_t)us*lp_clk)/1000000ll;
    return((uint32_t)lpcycles);
}

static __INLINE uint32_t co_max(uint32_t a, uint32_t b)
{
    return a > b ? a : b;
}


void bt_drv_reg_op_init_sleep_para(void)
{

#ifdef BT_LOG_POWEROFF
    uint32_t twrm = 800;
#else
    uint32_t twrm = 1000;
#endif
#ifdef ABSOLUTE_OSC_TIME
    uint32_t twosc = 2000;
#endif
    uint32_t twext = 1500;
    uint32_t twrm_lpcycle;
    uint32_t twosc_lpcycle;
    uint32_t twext_lpcycle;

    uint8_t rwip_prog_delay = 3;
    uint8_t clk_corr = 2;
    uint16_t sleep_algo = 400;
    uint8_t cpu_idle_en = 1;
#ifdef BT_LOG_POWEROFF
    uint8_t wait_26m_cycle = 12;
    uint32_t poweroff_flag = 0x4;
#else
    uint8_t wait_26m_cycle = 12;
    uint32_t poweroff_flag = 0x0;
#endif
    uint32_t lp_clk;

    BT_DRV_REG_OP_CLK_ENB();

    if(lp_clk_addr)
    {
        lp_clk = *(uint32_t *)lp_clk_addr;
        BT_DRV_TRACE(1, "lp clk: %d", lp_clk);
        twrm_lpcycle = bt_drv_us_2_lpcycles(twrm,lp_clk);
#ifdef ABSOLUTE_OSC_TIME
        twosc_lpcycle = bt_drv_us_2_lpcycles(twosc,lp_clk);
#else
#ifdef BT_LOG_POWEROFF
        uint32_t power_loop_cnt;
        uint32_t power_wakeup_cnt;

        power_loop_cnt = hal_psc_get_power_loop_cycle_cnt();
        power_wakeup_cnt = power_loop_cnt / 2 + hal_cmu_get_osc_ready_cycle_cnt();
        twosc_lpcycle = (power_loop_cnt > power_wakeup_cnt) ? power_loop_cnt : power_wakeup_cnt;
#else
        twosc_lpcycle = hal_cmu_get_osc_ready_cycle_cnt() + hal_cmu_get_osc_switch_overhead();
#endif
        // CPU handling overhead
        twosc_lpcycle += bt_drv_us_2_lpcycles(1000, lp_clk);
        // Add some protection cycle cnt
        twosc_lpcycle += 4;
#endif
        twext_lpcycle = bt_drv_us_2_lpcycles(twext,lp_clk);
        BTDIGITAL_REG_SET_FIELD(0xd022003c,0x3ff,0,twrm_lpcycle);
        BTDIGITAL_REG_SET_FIELD(0xd022003c,0x7ff,10,twosc_lpcycle);
        BTDIGITAL_REG_SET_FIELD(0xd022003c,0x7ff,21,twext_lpcycle);
        twrm_lpcycle = co_max(twrm_lpcycle,twosc_lpcycle);
        twrm_lpcycle=  co_max(twrm_lpcycle,twext_lpcycle);
        if(rwip_env_addr)
        {
            *(uint32_t *)(rwip_env_addr+0xa4) = twrm_lpcycle+30;
        }
    }

    if(rwip_prog_delay_addr)
    {
        *(uint8_t *)rwip_prog_delay_addr = rwip_prog_delay;
    }

    if(rwip_env_addr)
    {
        *(uint8_t *)(rwip_env_addr+0xb8) = clk_corr;
        *(uint16_t *)(rwip_env_addr+0xa8) = sleep_algo;
        *(uint8_t *)(rwip_env_addr+0xb9) = cpu_idle_en;
        *(uint8_t *)(rwip_env_addr+0xba) = wait_26m_cycle;
        BTDIGITAL_REG_SET_FIELD(0xD0330024,0xff,8,wait_26m_cycle);
#if defined(BT_LOG_POWEROFF) && defined(__BT_DEBUG_TPORTS__)
        bt_drv_bt_tport_type_config();
#endif
        *(uint32_t *)(rwip_env_addr+0xbc) = poweroff_flag;
        //  BT_DRV_TRACE(1, "prevent sleep: %x", *(uint16_t *)(rwip_env_addr+0xaa));
        //    *(uint16_t *)(rwip_env_addr+0xaa) |= 0x8000;
        //  BT_DRV_TRACE(1, "prevent sleep: %x", *(uint16_t *)(rwip_env_addr+0xaa));
    }
    BT_DRV_REG_OP_CLK_DIB();

}


void bt_drv_reg_op_data_bakeup_init(void)
{
    BT_DRV_REG_OP_CLK_ENB();
    if(hal_get_chip_metal_id() <= 3)
    {
        if(data_backup_cnt_addr)
        {
            *(uint32_t *)data_backup_cnt_addr = 0xc000ac00;
            *(uint32_t *)data_backup_addr_ptr_addr = 0xc000ac04;//set 32 reg backup max
            *(uint32_t *)data_backup_val_ptr_addr = 0xc000ac84;
        }
    }
    BT_DRV_REG_OP_CLK_DIB();

}

void memcpy32(uint32_t * dst, const uint32_t * src, size_t length)
{
    for (uint32_t i = 0; i < length; i++)
    {
        dst[i] = src[i];
    }
}

void bt_drv_reg_op_data_backup_write(const uint32_t *ptr,uint32_t cnt)
{
    BT_DRV_REG_OP_CLK_ENB();
    if(data_backup_cnt_addr)
    {
        *(uint32_t *)(*(uint32_t *)data_backup_cnt_addr) = cnt;
        for(uint32_t i=0; i<cnt; i++)
        {
            memcpy32((uint32_t *)(*(uint32_t *)data_backup_addr_ptr_addr),ptr,cnt);
        }
    }
    BT_DRV_REG_OP_CLK_DIB();

}


#ifdef __SW_TRIG__
uint16_t bt_drv_reg_op_rxbit_1us_get(uint16_t conhdl)
{
    uint8_t conidx;
    conidx = btdrv_conhdl_to_linkid(conhdl);
    uint16_t rxbit_1us = 0;
    uint32_t acl_evt_ptr = 0x0;

    BT_DRV_REG_OP_CLK_ENB();
    if(ld_acl_env_addr)
    {
        acl_evt_ptr = *(uint32_t *)(ld_acl_env_addr+conidx*4);
    }

    if (acl_evt_ptr != 0)
    {
        rxbit_1us = *(uint16_t *)(acl_evt_ptr + 0xb6);//acl_par->rxbit_1us
        BT_DRV_TRACE(1,"[%s] rxbit_1us=%d\n",__func__,rxbit_1us);
    }
    else
    {
        BT_DRV_TRACE(1,"BT_REG_OP:ERROR LINK ID FOR RD rxbit_1us %x", conidx);
    }
    BT_DRV_REG_OP_CLK_DIB();

    return rxbit_1us;
}
#endif

void bt_drv_reg_op_set_btpcm_trig_flag(bool flag)
{
    BT_DRV_REG_OP_CLK_ENB();

    if(pcm_need_start_flag_addr != 0)
    {
        BTDIGITAL_REG(pcm_need_start_flag_addr) = flag;
    }
    BT_DRV_REG_OP_CLK_DIB();
}

void bt_drv_reg_op_multi_ibrt_sche_adjust(struct sch_multi_ibrt_adjust_timeslice_env_t* updated_sch_para)
{
    struct sch_multi_ibrt_adjust_timeslice_env_t* sch_multi_ibrt_adjust_env_ptr = 0x0;
    BT_DRV_REG_OP_CLK_ENB();

    if(sch_multi_ibrt_adjust_env_addr)
    {
        sch_multi_ibrt_adjust_env_ptr = (sch_multi_ibrt_adjust_timeslice_env_t *)sch_multi_ibrt_adjust_env_addr;
        if(sch_multi_ibrt_adjust_env_ptr->update)
        {
            TRACE(0,"WARNING: previous update sch not finished");
        }
        memcpy(sch_multi_ibrt_adjust_env_ptr->ibrt_link,updated_sch_para->ibrt_link,sizeof(struct sch_adjust_timeslice_per_link)*MULTI_IBRT_SUPPORT_NUM);
        sch_multi_ibrt_adjust_env_ptr->update = 1;
    }
    BT_DRV_REG_OP_CLK_DIB();

}

void bt_drv_reg_op_multi_ibrt_music_config(uint8_t* link_id, uint8_t* active, uint8_t num)
{
    sch_multi_ibrt_adjust_timeslice_env_t updated_sch_para;
    uint8_t i = 0;
    BT_DRV_REG_OP_CLK_ENB();

    while(num > 0)
    {
        TRACE(0,"multi_ibrt_music linkid %d,active %d",link_id[i],active[i]);
        updated_sch_para.ibrt_link[i].link_id = link_id[i];
        if(active[i] == 1)
        {
            updated_sch_para.ibrt_link[i].timeslice = 72;//active timeslice
        }
        else
        {
            updated_sch_para.ibrt_link[i].timeslice = 24;//inactive timeslice
        }
        num--;
        i++;
    }
    while(i < MULTI_IBRT_SUPPORT_NUM)
    {
        updated_sch_para.ibrt_link[i].link_id = 0xFF;
        i++;
    }
    bt_drv_reg_op_multi_ibrt_sche_adjust(&updated_sch_para);
    BT_DRV_REG_OP_CLK_DIB();
}
void bt_drv_reg_op_init_hwagc_corr_gain(void)
{
    BT_DRV_REG_OP_CLK_ENB();

    if(RF_HWAGC_RSSI_CORRECT_TBL_addr)
    {
        *(uint16_t *)RF_HWAGC_RSSI_CORRECT_TBL_addr = 0x1706;
    }
    BT_DRV_REG_OP_CLK_DIB();

}

///only used for testmode
void bt_drv_reg_op_init_swagc_3m_thd(void)
{
#if 0
    int8_t rxgain_3m_thd_tbl[15][2] =
    {
        {-81, -81},
        {-77, -77},
        {-70, -70},
        {-65, -65},
        {-61, -61},
        {-52,  -52},
        {-40,  -40},
        {-30,  127},
        {127,  127},
        {0x7f,0x7f},
        {0x7f,0x7f},
        {0x7f,0x7f},
        {0x7f,0x7f},
        {0x7f,0x7f},
        {0x7f,0x7f},
    };
#else
    int8_t rxgain_3m_thd_tbl[15][2] =
    {
        {-80, -80},
        {-72, -72},
        {-68, -68},
        {-63, -63},
        {-57, -57},
        {-45, -45},
        {-32, -32},
        {-30, 127},
        {127,  127},
        {0x7f,0x7f},
        {0x7f,0x7f},
        {0x7f,0x7f},
        {0x7f,0x7f},
        {0x7f,0x7f},
        {0x7f,0x7f},
    };
#endif
    BT_DRV_REG_OP_CLK_ENB();
    if(rf_rx_gain_ths_tbl_bt_3m_addr != 0)
    {
        memcpy((int8_t *)rf_rx_gain_ths_tbl_bt_3m_addr, rxgain_3m_thd_tbl, sizeof(rxgain_3m_thd_tbl));
    }
    BT_DRV_REG_OP_CLK_DIB();
}

///only used for testmode
bool bt_drv_reg_op_get_3m_flag(void)
{
    bool ret=false;

    uint32_t flag_addr = testmode_3m_flag_addr;
    BT_DRV_REG_OP_CLK_ENB();
    enum HAL_CHIP_METAL_ID_T metal_id = hal_get_chip_metal_id();

    if(flag_addr)
    {
        if (metal_id == HAL_CHIP_METAL_ID_0 || metal_id == HAL_CHIP_METAL_ID_1)
        {
            ret = *(uint32_t *)flag_addr;
        }
        else if (metal_id >= HAL_CHIP_METAL_ID_2)
        {
            ret = *(uint8_t *)flag_addr;
        }
    }
    BT_DRV_REG_OP_CLK_DIB();

    return ret;
}

void bt_drv_reg_op_low_txpwr_set(uint8_t enable, uint8_t factor, uint8_t bt_or_ble, uint8_t link_id)
{
    struct dbg_set_txpwr_mode_cmd low_txpwr;

    if(bt_or_ble)
    {
        ASSERT_ERR(link_id < BLE_LINK_ID_MAX);
    }
    else
    {
        ASSERT_ERR(link_id < BT_LINK_ID_MAX);
    }

    low_txpwr.enable = enable;
    low_txpwr.factor = factor;
    low_txpwr.bt_or_ble = bt_or_ble;
    low_txpwr.link_id  = link_id;

    btdrv_send_cmd(HCI_DBG_SET_TXPWR_MODE_CMD_OPCODE,sizeof(low_txpwr),(uint8_t *)&low_txpwr);
}

bool bt_drv_error_check_handler(void)
{
    bool ret = false;
    BT_DRV_REG_OP_CLK_ENB();
    if((BTDIGITAL_REG(BT_ERRORTYPESTAT_ADDR)&(~0x40)) !=0 ||
       (BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE) !=0 &&
        BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE) !=0x42))
    {
        BT_DRV_TRACE(1,"BT_DRV:digital assert,error code=0x%x", BTDIGITAL_REG(BT_ERRORTYPESTAT_ADDR));
        ret = true;
    }
    BT_DRV_REG_OP_CLK_DIB();
    return ret;
}

///only used for testmode
void bt_drv_iq_tab_set_testmode(void)
{
#ifdef RX_IQ_CAL
    BT_DRV_REG_OP_CLK_ENB();
    if (testmode_tab_en_addr && testmode_iqtab_addr)
    {
        uint32_t tab_en_addr = testmode_tab_en_addr;
        uint32_t iqtab_addr = testmode_iqtab_addr;
        uint32_t iq_rx_val = 0;
        for(uint8_t i=0; i<8; i++)
        {
            uint32_t val_1 = (iq_gain&0x800) ? (iq_gain|0xf000) : iq_gain;
            uint32_t val_2 =  (iq_phy&0x800) ? (iq_phy|0xf000)  : iq_phy;
            val_1 |=(val_2<<16);
            iq_rx_val = val_1;
            *(uint32_t *)(iqtab_addr+i*4) = val_1;//iq_gain|(iq_phy<<16);
        }
        *(uint32_t *)tab_en_addr = 1;
        BT_DRV_TRACE(2,"BT_DRV:testmode iq tab=0x%x 0x%x", BTDIGITAL_REG(TESTMODE_IQTAB_EN_T0_TESTMODE_ADDR),
                     BTDIGITAL_REG(TESTMODE_IQTAB_T0_TESTMODE_ADDR));

        bt_drv_iq_tab_set_value(iq_rx_val);
    }
    BT_DRV_REG_OP_CLK_DIB();
#endif
}

void bt_drv_iq_tab_set_normalmode(void)
{
#ifdef RX_IQ_CAL
    BT_DRV_REG_OP_CLK_ENB();
    if (normal_tab_en_addr && normal_iqtab_addr)
    {
        uint32_t tab_en_addr = normal_tab_en_addr;
        uint32_t iqtab_addr = normal_iqtab_addr;
        uint32_t iq_rx_val = 0;
        for(uint8_t i=0; i<8; i++)
        {
            uint32_t val_1 = (iq_gain&0x800) ? (iq_gain|0xf000) : iq_gain;
            uint32_t val_2 =  (iq_phy&0x800) ? (iq_phy|0xf000)  : iq_phy;
            val_1 |=(val_2<<16);
            iq_rx_val = val_1;
            *(uint32_t *)(iqtab_addr+i*4) = val_1;//iq_gain|(iq_phy<<16);
        }
        *(uint32_t *)tab_en_addr = 1;
        BT_DRV_TRACE(2,"BT_DRV:normal iq tab=0x%x 0x%x", BTDIGITAL_REG(NORMAL_IQTAB_EN_T0_ADDR),BTDIGITAL_REG(NORMAL_IQTAB_T0_ADDR));
        bt_drv_iq_tab_set_value(iq_rx_val);
    }
    BT_DRV_REG_OP_CLK_DIB();
#endif
}

void bt_drv_iq_tab_set_value(uint32_t val)
{
    BT_DRV_REG_OP_CLK_ENB();
    TRACE(2, "%s val=0x%x",__func__, val);
    for (uint8_t i = 0; i < 79; i++)
    {
        *(uint32_t *)(0xd0370000 + i * 4) = val;
    }
    *(volatile uint32_t *)(0xd0370000);
    BT_DRV_REG_OP_CLK_DIB();
}

///only used for testmode
void bt_drv_reg_op_i2v_checker_for_testmode(void)
{
    uint16_t temp = 0;
#ifdef BT_I2V_VAL_RD
    if(i2v_val_addr != 0)
    {
        BT_DRV_REG_OP_CLK_ENB();
        BT_DRV_TRACE(1,"i2v val=0x%x", BTDIGITAL_REG(i2v_val_addr));

        BTDIGITAL_REG_GET_FIELD(0xD0220E58, 0x3FF,10,temp); //[19:10]
        BT_DRV_TRACE(1, "i2v_val[19:10] 0x%x",temp);
        BTDIGITAL_REG_GET_FIELD(0xD0220E58, 0x3FF,0,temp); //[9:0]
        BT_DRV_TRACE(1, "Lna_val[9:0] 0x%x",temp);

        BT_DRV_REG_OP_CLK_DIB();
    }
#endif
}

void bt_drv_i2v_disable_sleep_for_bt_access(void)
{
    hal_intersys_wakeup_btcore();
}

void bt_drv_i2v_enable_sleep_for_bt_access(void)
{
    uint32_t flag_addr = rt_sleep_flag_clear_addr;
    ///warning no need BT_DRV_REG_OP_CLK_ENB protection
    ///this function is used by BT poweroff  function
    if(flag_addr !=0)
    {
        if(*(uint32_t *)flag_addr == 0)
        {
            *(uint32_t *)flag_addr = 1;
        }
    }
}

void bt_drv_reg_op_set_replace_mobile_addr(uint8_t enable, uint8_t* addr)
{
    BT_DRV_REG_OP_CLK_ENB();
    do
    {
        if(replace_mobile_valid_addr == 0 || replace_mobile_addr_addr == 0)
        {
            BT_DRV_TRACE(1,"BT_REG_OP:%s error !!!", __func__);
            break;
        }

        BT_DRV_TRACE(1,"BT_REG_OP:connection switch en %d",enable);
        BTDIGITAL_REG(replace_mobile_valid_addr) = 0;
        if(addr != NULL)
        {
            memcpy((void *)replace_mobile_addr_addr, addr, 6);
            BT_DRV_TRACE(6,"BT_REG_OP: replace mobile %02x %02x %02x %02x %02x %02x",addr[0],addr[1],addr[2],addr[3],addr[4],addr[5]);
        }
        BTDIGITAL_REG(replace_mobile_valid_addr) = enable;
    } while(0);

    BT_DRV_REG_OP_CLK_DIB();
}

void bt_drv_reg_op_trigger_controller_assert(void)
{
    ASSERT(0, "Trigger BTC crash on purpose!")
}

void bt_drv_reg_op_afh_assess_en(bool en)
{
#if (!defined(__BT_RAMRUN__) && defined(__AFH_ASSESS__) && !defined(__HW_AGC__))
    enum HAL_CHIP_METAL_ID_T metal_id = hal_get_chip_metal_id();
    static bool en_back = false;
    if(en == en_back)
    {
        return;
    }
    en_back = en;

    BT_DRV_TRACE(1,"BT_REG_OP:set afh assss=%d",en);


    if(metal_id >= HAL_CHIP_METAL_ID_2)
    {
        TRACE(0, "BT_DRV:afh assess enable\n");

        struct hci_dbg_set_afh_assess_params hci_afh_assess_init_config =
        {
            .enable = 0,
            .bd_addr = {.addr = {0x03,0x63,0x83,0x12,0x34,0x28}},
            .afh_monitor_negative_num = 0,
            .afh_monitor_positive_num = 0,

            .average_cnt = 7,
            .sch_prio_dft = 5,
            .auto_resch_att = 5,

            .rxgain = 2,
            .afh_good_chl_thr = -90,

            .lt_addr = 0,
            .edr = 0,

            .interval = 800,
            .sch_expect_assess_num = 38,
            .prog_spacing_hus = 340,
        };

        if(en)
        {
            hci_afh_assess_init_config.enable = 1;
        }
        btdrv_send_hci_cmd_bystack(HCI_DBG_SET_AFH_ASSESS_CMD_OPCODE, (uint8_t *)&hci_afh_assess_init_config, sizeof(struct hci_dbg_set_afh_assess_params));
    }
#endif
}

void bt_drv_reg_op_afh_assess_init(void)
{
    //afh assess only support metal_id >= HAL_CHIP_METAL_ID_2
    bt_drv_reg_op_afh_assess_en(false);

}

void bt_drv_reg_op_power_off_rx_gain_config()
{
    enum HAL_CHIP_METAL_ID_T metal_id = hal_get_chip_metal_id();

    if (metal_id == HAL_CHIP_METAL_ID_0 || metal_id == HAL_CHIP_METAL_ID_1)
    {
        BT_DRV_REG_OP_CLK_ENB();
        *(uint32_t *)POWER_OFF_FLAG_T0_ADDR = 1;
        *(uint32_t *)RX_GAIN_ADD_POWER_OFF_T0_ADDR = 3;
        BT_DRV_REG_OP_CLK_DIB();
    }
}

void bt_drv_reg_op_power_adjust_onoff(uint8_t en)
{
    if(power_adjust_en_addr != 0)
    {
        BT_DRV_REG_OP_CLK_ENB();
        enum HAL_CHIP_METAL_ID_T metal_id = hal_get_chip_metal_id();

        if (metal_id == HAL_CHIP_METAL_ID_0 || metal_id == HAL_CHIP_METAL_ID_1)
        {
            *(uint32_t *)power_adjust_en_addr = en;
        }
        else if (metal_id >= HAL_CHIP_METAL_ID_2)
        {
            *(uint8_t *)power_adjust_en_addr = en;
        }

        BT_DRV_REG_OP_CLK_DIB();
    }
}

uint32_t bt_drv_lp_clk_get()
{
    uint32_t lp_clk = 0xFFFFFFFF;
    BT_DRV_REG_OP_CLK_ENB();
    if(lp_clk_addr)
        lp_clk = BTDIGITAL_REG(lp_clk_addr);

    BT_DRV_REG_OP_CLK_DIB();
    return lp_clk;
}


//HW SPI TRIG

#define REG_SPI_TRIG_NUM_ADDR 0xd0340074
#define REG_SPI0_TRIG_POS_ADDR 0xd0340078
#define REG_SPI1_TRIG_POS_ADDR 0xd034007c

struct SPI_TRIG_NUM_T
{
    uint32_t spi0_txon_num:3;//spi0 number of tx rising edge
    uint32_t spi0_txoff_num:3;//spi0 number of tx falling edge
    uint32_t spi0_rxon_num:2;//spi0 number of rx rising edge
    uint32_t spi0_rxoff_num:2;//spi0 number of rx falling edge
    uint32_t spi0_fast_mode:1;
    uint32_t spi0_gap:4;
    uint32_t hwspi0_en:1;
    uint32_t spi1_txon_num:3;//spi1 number of tx rising edge
    uint32_t spi1_txoff_num:3;//spi1 number of tx falling edge
    uint32_t spi1_rxon_num:2;//spi1 number of rx rising edge
    uint32_t spi1_rxoff_num:2;//spi1 number of rx falling edge
    uint32_t spi1_fast_mode:1;
    uint32_t spi1_gap:4;
    uint32_t hwspi1_en:1;
};

struct SPI_TRIG_POS_T
{
    uint32_t spi_txon_pos:12;
    uint32_t spi_txoff_pos:4;
    uint32_t spi_rxon_pos:12;
    uint32_t spi_rxoff_pos:4;
};

struct spi_trig_data
{
    uint32_t reg;
    uint32_t value;
};

static const struct spi_trig_data spi0_trig_data_tbl[] =
{
    //{addr,data([23:0])}
    {0xd0340024,0x000000},//spi0_trig_txdata1
    {0xd0340028,0x000000},//spi0_trig_txdata2
    {0xd034002c,0x000000},//spi0_trig_txdata3
    {0xd0340030,0x000000},//spi0_trig_txdata4
    {0xd0340034,0x000000},//spi0_trig_trxdata5
    {0xd0340038,0x000000},//spi0_trig_trxdata6
    {0xd034003c,0x000000},//spi0_trig_rxdata1
    {0xd0340040,0x000000},//spi0_trig_rxdata2
    {0xd0340044,0x000000},//spi0_trig_rxdata3
    {0xd0340048,0x000000},//spi0_trig_rxdata4
};

static const struct spi_trig_data spi1_trig_data_tbl[] =
{
    //{addr,data([23:0])}
    {0xd034004c,0x000000},//spi1_trig_txdata1
    {0xd0340050,0x000000},//spi1_trig_txdata2
    {0xd0340054,0x000000},//spi1_trig_txdata3
    {0xd0340058,0x000000},//spi1_trig_txdata4
    {0xd034005c,0x000000},//spi1_trig_trxdata5
    {0xd0340060,0x000000},//spi1_trig_trxdata6
    {0xd0340064,0x000000},//spi1_trig_rxdata1
    {0xd0340068,0x000000},//spi1_trig_rxdata2
    {0xd034006c,0x000000},//spi1_trig_rxdata3
    {0xd0340070,0x000000},//spi1_trig_rxdata4
};

void btdrv_spi_trig_data_change(uint8_t spi_sel, uint8_t index, uint32_t value)
{
    BT_DRV_REG_OP_CLK_ENB();
    if(!spi_sel)
    {
        BTDIGITAL_REG(spi0_trig_data_tbl[index].reg) = value & 0xFFFFFF;
    }
    else
    {
        BTDIGITAL_REG(spi1_trig_data_tbl[index].reg) = value & 0xFFFFFF;
    }
    BT_DRV_REG_OP_CLK_DIB();
}

void btdrv_spi_trig_data_set(uint8_t spi_sel)
{
    BT_DRV_REG_OP_CLK_ENB();
    if(!spi_sel)
    {
        for(uint8_t i = 0; i < ARRAY_SIZE(spi0_trig_data_tbl); i++)
        {
            BTDIGITAL_REG(spi0_trig_data_tbl[i].reg) = spi0_trig_data_tbl[i].value;
        }
    }
    else
    {
        for(uint8_t i = 0; i < ARRAY_SIZE(spi1_trig_data_tbl); i++)
        {
            BTDIGITAL_REG(spi1_trig_data_tbl[i].reg) = spi1_trig_data_tbl[i].value;
        }
    }
    BT_DRV_REG_OP_CLK_DIB();
}

void btdrv_spi_trig_num_set(uint8_t spi_sel, struct SPI_TRIG_NUM_T *spi_trig_num)
{
    uint8_t tx_onoff_total_num;
    uint8_t rx_onoff_total_num;

    if(!spi_sel)
    {
        tx_onoff_total_num = spi_trig_num->spi0_txon_num + spi_trig_num->spi0_txoff_num;
        rx_onoff_total_num = spi_trig_num->spi0_rxon_num + spi_trig_num->spi0_rxoff_num;
    }
    else
    {
        tx_onoff_total_num = spi_trig_num->spi1_txon_num + spi_trig_num->spi1_txoff_num;
        rx_onoff_total_num = spi_trig_num->spi1_rxon_num + spi_trig_num->spi1_rxoff_num;
    }
    ASSERT((tx_onoff_total_num <= 6), "spi trig tx_onoff_total_num>6");
    ASSERT((rx_onoff_total_num <= 6), "spi trig rx_onoff_total_num>6");

    BT_DRV_REG_OP_CLK_ENB();
    BTDIGITAL_REG(REG_SPI_TRIG_NUM_ADDR) = *(uint32_t *)spi_trig_num;
    BT_DRV_REG_OP_CLK_DIB();
}

void btdrv_spi_trig_pos_set(uint8_t spi_sel, struct SPI_TRIG_POS_T *spi_trig_pos)
{
    BT_DRV_REG_OP_CLK_ENB();
    if(!spi_sel)
    {
        BTDIGITAL_REG(REG_SPI0_TRIG_POS_ADDR) = *(uint32_t *)spi_trig_pos;
    }
    else
    {
        BTDIGITAL_REG(REG_SPI1_TRIG_POS_ADDR) = *(uint32_t *)spi_trig_pos;
    }
    BT_DRV_REG_OP_CLK_DIB();
}

void btdrv_set_spi_trig_pos_enable(void)
{
    struct SPI_TRIG_POS_T spi0_trig_pos;
    struct SPI_TRIG_POS_T spi1_trig_pos;

    spi0_trig_pos.spi_txon_pos = SPI0_TXON_POS;
    spi0_trig_pos.spi_txoff_pos = 0;
    spi0_trig_pos.spi_rxon_pos = SPI0_RXON_POS;
    spi0_trig_pos.spi_rxoff_pos = 0;

    spi1_trig_pos.spi_txon_pos = 0;
    spi1_trig_pos.spi_txoff_pos = 0;
    spi1_trig_pos.spi_rxon_pos = 0;
    spi1_trig_pos.spi_rxoff_pos = 0;

    btdrv_spi_trig_pos_set(0,&spi0_trig_pos);
    btdrv_spi_trig_pos_set(1,&spi1_trig_pos);
}

void btdrv_clear_spi_trig_pos_enable(void)
{
    int sRet = 0;

    struct SPI_TRIG_POS_T spi0_trig_pos;
    struct SPI_TRIG_POS_T spi1_trig_pos;

    sRet = memset_s(&spi0_trig_pos, sizeof(struct SPI_TRIG_POS_T), 0, sizeof(struct SPI_TRIG_POS_T));
    if (sRet)
    {
        TRACE(1, "%s line:%d sRet:%d", __func__, __LINE__, sRet);
    }
    sRet = memset_s(&spi1_trig_pos, sizeof(struct SPI_TRIG_POS_T), 0, sizeof(struct SPI_TRIG_POS_T));
    if (sRet)
    {
        TRACE(1, "%s line:%d sRet:%d", __func__, __LINE__, sRet);
    }

    btdrv_spi_trig_pos_set(0,&spi0_trig_pos);
    btdrv_spi_trig_pos_set(1,&spi1_trig_pos);
}

void btdrv_set_spi_trig_num_enable(void)
{
    struct SPI_TRIG_NUM_T spi_trig_num;

    spi_trig_num.spi0_txon_num = SPI0_TXON_NUM;
    spi_trig_num.spi0_txoff_num = 0;
    spi_trig_num.spi0_rxon_num = SPI0_RXON_NUM;
    spi_trig_num.spi0_rxoff_num = 0;
    spi_trig_num.spi0_fast_mode = 0;
    spi_trig_num.spi0_gap = 4;
    spi_trig_num.hwspi0_en = SPI0_EN;

    spi_trig_num.spi1_txon_num = 0;
    spi_trig_num.spi1_txoff_num = 0;
    spi_trig_num.spi1_rxon_num = 0;
    spi_trig_num.spi1_rxoff_num = 0;
    spi_trig_num.spi1_fast_mode = 0;
    spi_trig_num.spi1_gap = 0;
    spi_trig_num.hwspi1_en = 0;

    btdrv_spi_trig_num_set(0,&spi_trig_num);
    btdrv_spi_trig_num_set(1,&spi_trig_num);
}

void btdrv_spi_trig_init(void)
{
    //spi number set
    btdrv_set_spi_trig_num_enable();
    //spi position set
    btdrv_set_spi_trig_pos_enable();
    //spi data set
    btdrv_spi_trig_data_set(0);
    btdrv_spi_trig_data_set(1);
}

uint8_t btdrv_get_spi_trig_enable(uint8_t spi_sel)
{
    uint8_t retVal = 0;
    BT_DRV_REG_OP_CLK_ENB();
    if(!spi_sel)
    {
        retVal = ((BTDIGITAL_REG(REG_SPI_TRIG_NUM_ADDR) & 0x8000) >> 15);
    }
    else
    {
        retVal = ((BTDIGITAL_REG(REG_SPI_TRIG_NUM_ADDR) & 0x80000000) >> 31);
    }
    BT_DRV_REG_OP_CLK_DIB();

    return retVal;
}

void btdrv_set_spi_trig_enable(uint8_t spi_sel)
{
    BT_DRV_REG_OP_CLK_ENB();
    if(!spi_sel)
    {
        BTDIGITAL_REG(REG_SPI_TRIG_NUM_ADDR) |= (1<<15);//spi0
    }
    else
    {
        BTDIGITAL_REG(REG_SPI_TRIG_NUM_ADDR) |= (1<<31);//spi1
    }
    BT_DRV_REG_OP_CLK_DIB();
}

void btdrv_clear_spi_trig_enable(uint8_t spi_sel)
{
    BT_DRV_REG_OP_CLK_ENB();
    if(!spi_sel)
    {
        BTDIGITAL_REG(REG_SPI_TRIG_NUM_ADDR) &= ~0x8000;
    }
    else
    {
        BTDIGITAL_REG(REG_SPI_TRIG_NUM_ADDR) &= ~0x80000000;
    }
    BT_DRV_REG_OP_CLK_DIB();
}

void btdrv_reg_op_vco_test_start(uint8_t chnl)
{
    BT_DRV_REG_OP_CLK_ENB();
    BTDIGITAL_REG(0xd0220c00) = (chnl & 0x7f) | 0xa0000;
    btdrv_delay(10);
    BT_DRV_REG_OP_CLK_DIB();
}

void btdrv_reg_op_vco_test_stop(void)
{
    BT_DRV_REG_OP_CLK_ENB();
    BTDIGITAL_REG(0xd0220c00) = 0;
    btdrv_delay(10);
    BT_DRV_REG_OP_CLK_DIB();
}

uint32_t btdrv_reg_op_syn_get_curr_ticks(void)
{
    uint32_t value;

    BT_DRV_REG_OP_CLK_ENB();
    value = BTDIGITAL_REG(BT_BES_CLK_REG1_ADDR) & 0x0fffffff;
    BT_DRV_REG_OP_CLK_DIB();
    return value;

}

uint32_t btdrv_reg_op_syn_get_cis_curr_time(void)
{
    uint32_t value;

    BT_DRV_REG_OP_CLK_ENB();
    *(volatile uint32_t *)IP_SLOTCLK_ADDR |= (1<<31);
    while (*(volatile uint32_t *)IP_SLOTCLK_ADDR & ((uint32_t)0x80000000));
    value = BTDIGITAL_REG(IP_ISOCNTSAMP_ADDR);
    BT_DRV_REG_OP_CLK_DIB();
    return value;
}

void btdrv_reg_op_sync_clr_trigger(uint8_t trig_route)
{
    BT_DRV_REG_OP_CLK_ENB();
    switch(trig_route)
    {
        case 0:
        {
            BTDIGITAL_REG(BT_BES_CNTL1_ADDR) |= (1<<4);
            break;
        }
        case 1:
        {
            BTDIGITAL_REG(BT_BES_CNTL1_ADDR) |= (1<<8);
            break;
        }
        case 2:
        {
            BTDIGITAL_REG(BT_BES_CNTL1_ADDR) |= (1<<9);
            break;
        }
        case 3:
        {
            BTDIGITAL_REG(BT_BES_CNTL1_ADDR) |= (1<<10);
            break;
        }
    }
    BT_DRV_TRACE(2,"[%s] 0xd0220c08=%x",__func__,*(volatile uint32_t *)(BT_BES_CNTL1_ADDR));
    BT_DRV_REG_OP_CLK_DIB();
}

#ifdef __SW_TRIG__
void btdrv_reg_op_sw_trig_tg_finecnt_set(uint16_t tg_bitcnt, uint8_t trig_route)
{
    BT_DRV_REG_OP_CLK_ENB();
    switch(trig_route)
    {
        case 0:
        {
            //0xc98 bit[9:0]
            BTDIGITAL_REG(BT_BES_TG_FINECNT_ADDR) = (BTDIGITAL_REG(BT_BES_TG_FINECNT_ADDR) & 0xFFFFFC00) | tg_bitcnt;
            BT_DRV_TRACE(1,"[%s] 0xd0220c98=%x, tg_bitcnt=%d\n",__func__,BTDIGITAL_REG(BT_BES_TG_FINECNT_ADDR), tg_bitcnt);
            break;
        }
        case 1:
        {
            //0xd0c bit[9:0]
            BTDIGITAL_REG(BT_BES_TG_FINECNT1_ADDR) = (BTDIGITAL_REG(BT_BES_TG_FINECNT1_ADDR) & 0xFFFFFC00) | tg_bitcnt;
            BT_DRV_TRACE(1,"[%s] 0xd0220d0c=%x, tg_bitcnt=%d\n",__func__,BTDIGITAL_REG(BT_BES_TG_FINECNT1_ADDR), tg_bitcnt);
            break;
        }
        case 2:
        {
            //0xd14 bit[9:0]
            BTDIGITAL_REG(BT_BES_TG_FINECNT2_ADDR) = (BTDIGITAL_REG(BT_BES_TG_FINECNT2_ADDR) & 0xFFFFFC00) | tg_bitcnt;
            BT_DRV_TRACE(1,"[%s] 0xd0220d14=%x, tg_bitcnt=%d\n",__func__,BTDIGITAL_REG(BT_BES_TG_FINECNT2_ADDR), tg_bitcnt);
            break;
        }
        case 3:
        {
            //0xd1c bit[9:0]
            BTDIGITAL_REG(BT_BES_TG_FINECNT3_ADDR) = (BTDIGITAL_REG(BT_BES_TG_FINECNT3_ADDR) & 0xFFFFFC00) | tg_bitcnt;
            BT_DRV_TRACE(1,"[%s] 0xd0220d1c=%x, tg_bitcnt=%d\n",__func__,BTDIGITAL_REG(BT_BES_TG_FINECNT3_ADDR), tg_bitcnt);
            break;
        }
    }
    BT_DRV_REG_OP_CLK_DIB();
}

uint16_t btdrv_reg_op_sw_trig_tg_finecnt_get(uint8_t trig_route)
{
    uint16_t finecnt = 0;
    BT_DRV_REG_OP_CLK_ENB();
    switch(trig_route)
    {
        case 0:
        {
            finecnt = BTDIGITAL_REG(BT_BES_TG_FINECNT_ADDR) & 0x000003FF;
            break;
        }
        case 1:
        {
            finecnt = BTDIGITAL_REG(BT_BES_TG_FINECNT1_ADDR) & 0x000003FF;
            break;
        }
        case 2:
        {
            finecnt = BTDIGITAL_REG(BT_BES_TG_FINECNT2_ADDR) & 0x000003FF;
            break;
        }
        case 3:
        {
            finecnt = BTDIGITAL_REG(BT_BES_TG_FINECNT3_ADDR) & 0x000003FF;
            break;
        }
    }
    BT_DRV_REG_OP_CLK_DIB();
    BT_DRV_TRACE(1,"[%s] finecnt=%d\n",__func__,finecnt);
    return finecnt;
}

void btdrv_reg_op_sw_trig_tg_clkncnt_set(uint32_t num, uint8_t trig_route)
{
    BT_DRV_REG_OP_CLK_ENB();
    switch(trig_route)
    {
        case 0:
        {
            //0xc94 bit[27:0]
            BTDIGITAL_REG(BT_BES_TG_CLKNCNT_ADDR) = (BTDIGITAL_REG(BT_BES_TG_CLKNCNT_ADDR) & 0xf0000000) | (num & 0x0fffffff);
            BT_DRV_TRACE(1,"[%s] 0xd0220c94=0x%x, num=%d\n",__func__,BTDIGITAL_REG(BT_BES_TG_CLKNCNT_ADDR), num);
            break;
        }
        case 1:
        {
            //0xd08 bit[27:0]
            BTDIGITAL_REG(BT_BES_TG_CLKNCNT1_ADDR) = (BTDIGITAL_REG(BT_BES_TG_CLKNCNT1_ADDR) & 0xf0000000) | (num & 0x0fffffff);
            BT_DRV_TRACE(1,"[%s] 0xd0220d08=0x%x, num=%d\n",__func__,BTDIGITAL_REG(BT_BES_TG_CLKNCNT1_ADDR), num);
            break;
        }
        case 2:
        {
            //0xd10 bit[27:0]
            BTDIGITAL_REG(BT_BES_TG_CLKNCNT2_ADDR) = (BTDIGITAL_REG(BT_BES_TG_CLKNCNT2_ADDR) & 0xf0000000) | (num & 0x0fffffff);
            BT_DRV_TRACE(1,"[%s] 0xd0220d10=0x%x, num=%d\n",__func__,BTDIGITAL_REG(BT_BES_TG_CLKNCNT2_ADDR), num);
            break;
        }
        case 3:
        {
            //0xd18 bit[27:0]
            BTDIGITAL_REG(BT_BES_TG_CLKNCNT3_ADDR) = (BTDIGITAL_REG(BT_BES_TG_CLKNCNT3_ADDR) & 0xf0000000) | (num & 0x0fffffff);
            BT_DRV_TRACE(1,"[%s] 0xd0220d18=0x%x, num=%d\n",__func__,BTDIGITAL_REG(BT_BES_TG_CLKNCNT3_ADDR), num);
            break;
        }
    }
    BT_DRV_REG_OP_CLK_DIB();
}

void btdrv_sw_trig_en_set(uint8_t trig_route)
{
    BT_DRV_REG_OP_CLK_ENB();
    switch(trig_route)
    {
        case 0:
        {
            BTDIGITAL_REG(BT_BES_CNTLX_ADDR) |= 0x1;
            BT_DRV_TRACE(1,"[%s] 0xd0220c7c=0x%x,0xd0220c94=0x%x,0xd0220c98=0x%x\n",__func__,
                         BTDIGITAL_REG(BT_BES_CNTLX_ADDR),BTDIGITAL_REG(BT_BES_TG_CLKNCNT_ADDR),BTDIGITAL_REG(BT_BES_TG_FINECNT_ADDR));
            break;
        }
        case 1:
        {
            BTDIGITAL_REG(BT_BES_TG_FINECNT1_ADDR) |= (1<<31);
            BT_DRV_TRACE(1,"[%s] 0xd0220d08=0x%x,0xd0220d0c=0x%x\n",__func__,
                         BTDIGITAL_REG(BT_BES_TG_CLKNCNT1_ADDR),BTDIGITAL_REG(BT_BES_TG_FINECNT1_ADDR));
            break;
        }
        case 2:
        {
            BTDIGITAL_REG(BT_BES_TG_FINECNT2_ADDR) |= (1<<31);
            BT_DRV_TRACE(1,"[%s] 0xd0220d10=0x%x,0xd0220d14=0x%x\n",__func__,
                         BTDIGITAL_REG(BT_BES_TG_CLKNCNT2_ADDR),BTDIGITAL_REG(BT_BES_TG_FINECNT2_ADDR));
            break;
        }
        case 3:
        {
            BTDIGITAL_REG(BT_BES_TG_FINECNT3_ADDR) |= (1<<31);
            BT_DRV_TRACE(1,"[%s] 0xd0220d18=0x%x,0xd0220d1c=0x%x\n",__func__,
                         BTDIGITAL_REG(BT_BES_TG_CLKNCNT3_ADDR),BTDIGITAL_REG(BT_BES_TG_FINECNT3_ADDR));
            break;
        }
    }
    BT_DRV_REG_OP_CLK_DIB();
}

void btdrv_sw_trig_disable_set(uint8_t trig_route)
{
    BT_DRV_REG_OP_CLK_ENB();
    switch(trig_route)
    {
        case 0:
        {
            BTDIGITAL_REG(BT_BES_CNTLX_ADDR) &= ~0x1;
            BT_DRV_TRACE(1,"[%s] 0xd0220c7c=0x%x,0xd0220c94=0x%x,0xd0220c98=0x%x\n",__func__,
                         BTDIGITAL_REG(BT_BES_CNTLX_ADDR),BTDIGITAL_REG(BT_BES_TG_CLKNCNT_ADDR),BTDIGITAL_REG(BT_BES_TG_FINECNT_ADDR));
            break;
        }
        case 1:
        {
            BTDIGITAL_REG(BT_BES_TG_FINECNT1_ADDR) &= ~(1<<31);
            BT_DRV_TRACE(1,"[%s] 0xd0220d08=0x%x,0xd0220d0c=0x%x\n",__func__,
                         BTDIGITAL_REG(BT_BES_TG_CLKNCNT1_ADDR),BTDIGITAL_REG(BT_BES_TG_FINECNT1_ADDR));
            break;
        }
        case 2:
        {
            BTDIGITAL_REG(BT_BES_TG_FINECNT2_ADDR) &= ~(1<<31);
            BT_DRV_TRACE(1,"[%s] 0xd0220d10=0x%x,0xd0220d14=0x%x\n",__func__,
                         BTDIGITAL_REG(BT_BES_TG_CLKNCNT2_ADDR),BTDIGITAL_REG(BT_BES_TG_FINECNT2_ADDR));
            break;
        }
        case 3:
        {
            BTDIGITAL_REG(BT_BES_TG_FINECNT3_ADDR) &= ~(1<<31);
            BT_DRV_TRACE(1,"[%s] 0xd0220d18=0x%x,0xd0220d1c=0x%x\n",__func__,
                         BTDIGITAL_REG(BT_BES_TG_CLKNCNT3_ADDR),BTDIGITAL_REG(BT_BES_TG_FINECNT3_ADDR));
            break;
        }
    }
    BT_DRV_REG_OP_CLK_DIB();
}

#endif

void btdrv_reg_op_syn_set_tg_ticks_master_role(void)
{
    BT_DRV_REG_OP_CLK_ENB();
    BT_DRV_TRACE(2,"master 0xd0220c84:%x,0xd0220c7c:%x\n",*(volatile uint32_t *)(BT_TWSBTCLKREG_ADDR),*(volatile uint32_t *)(BT_BES_CNTLX_ADDR));
    BT_DRV_REG_OP_CLK_DIB();
}

void btdrv_reg_op_syn_set_tg_ticks_slave_role(uint32_t num)
{
    BT_DRV_REG_OP_CLK_ENB();
    BTDIGITAL_REG(BT_TWSBTCLKREG_ADDR) = (BTDIGITAL_REG(BT_TWSBTCLKREG_ADDR) & 0xf0000000) | (num & 0x0fffffff);
    BTDIGITAL_REG(BT_BES_CNTLX_ADDR) = (BTDIGITAL_REG(BT_BES_CNTLX_ADDR) & 0xffffe0ff) | (0x12<<8);//bit[12:8] trip_delay=18
    BTDIGITAL_REG(BT_BES_CNTLX_ADDR) |= (1<<15);//rxbit sel en
    BTDIGITAL_REG(BT_BES_CNTLX_ADDR) = (BTDIGITAL_REG(BT_BES_CNTLX_ADDR) & 0xfffffffe) | (0x1);
    BTDIGITAL_REG(BT_BES_CNTL1_ADDR) |= (1<<7);
    BT_DRV_TRACE(1,"slave 0xd0220c84:0x%x,0xd0220c7c:0x%x,0xd0220c08:0x%x\n",BTDIGITAL_REG(BT_TWSBTCLKREG_ADDR),BTDIGITAL_REG(BT_BES_CNTLX_ADDR),BTDIGITAL_REG(BT_BES_CNTL1_ADDR));
    BT_DRV_REG_OP_CLK_DIB();
}

void btdrv_reg_op_syn_trigger_codec_en(uint32_t v)
{
    BT_DRV_REG_OP_CLK_ENB();
    BT_DRV_REG_OP_CLK_DIB();
}

uint32_t btdrv_reg_op_get_syn_trigger_codec_en(void)
{
    uint32_t value;
    BT_DRV_REG_OP_CLK_ENB();
    value = BTDIGITAL_REG(BT_BES_CNTLX_ADDR);
    BT_DRV_REG_OP_CLK_DIB();
    return value;
}

uint32_t btdrv_reg_op_get_trigger_ticks(void)
{
    uint32_t value;
    BT_DRV_REG_OP_CLK_ENB();
    value = BTDIGITAL_REG(BT_BES_CNTLX_ADDR);
    BT_DRV_REG_OP_CLK_DIB();
    return value;
}

void btdrv_reg_op_syn_set_tg_ticks_linkid(uint8_t link_id)
{
    BT_DRV_REG_OP_CLK_ENB();
    BTDIGITAL_REG_SET_FIELD(BT_BES_CNTLX_ADDR,0xF,16,(link_id+1));
    BT_DRV_REG_OP_CLK_DIB();
}

void btdrv_reg_op_enable_playback_triggler(uint8_t triggle_mode)
{
    BT_DRV_REG_OP_CLK_ENB();
    if(triggle_mode == ACL_TRIGGLE_MODE)
    {
        BTDIGITAL_REG(BT_BES_CNTLX_ADDR) &= ~(1<<4|1<<3);
        BTDIGITAL_REG(BT_BES_CNTLX_ADDR) |= (1<<3);
    }
    else if(triggle_mode == SCO_TRIGGLE_MODE)
    {
        BTDIGITAL_REG(BT_BES_CNTLX_ADDR) &= ~(1<<4|1<<3);
        BTDIGITAL_REG(BT_BES_CNTLX_ADDR) |= (1<<4);
    }
    BT_DRV_TRACE(2,"[%s] 0xd0220c7c=%x",__func__,*(volatile uint32_t *)(BT_BES_CNTLX_ADDR));
    BT_DRV_REG_OP_CLK_DIB();
}

void btdrv_reg_op_play_trig_mode_set(uint8_t mode)
{
    BT_DRV_REG_OP_CLK_ENB();
    ///0xc7c bit[14:13] 00:hw trig, 01:sw trig, 1x:self trig
    BTDIGITAL_REG(BT_BES_CNTLX_ADDR) = (BTDIGITAL_REG(BT_BES_CNTLX_ADDR) & 0xFFFF9FFF) | (mode << 13);
    BT_DRV_TRACE(1,"btdrv_play_trig_mode_set:%d",mode);
    BT_DRV_REG_OP_CLK_DIB();
}

void btdrv_reg_op_disable_playback_triggler(void)
{
    BT_DRV_REG_OP_CLK_ENB();
    BTDIGITAL_REG(BT_BES_CNTLX_ADDR) &= ~(1<<4|1<<3);
    BT_DRV_TRACE(2,"[%s] 0xd0220c7c=%x\n",__func__,*(volatile uint32_t *)(BT_BES_CNTLX_ADDR));
    BT_DRV_REG_OP_CLK_DIB();
}

void btdrv_reg_op_set_pcm_data_ignore_crc(void)
{
    BT_DRV_REG_OP_CLK_ENB();
    BTDIGITAL_REG(BT_E_SCOMUTECNTL_0_ADDR) &= ~0x800000;
    BT_DRV_REG_OP_CLK_DIB();
}

void btdrv_reg_op_linear_format_16bit_set(void)
{
    BT_DRV_REG_OP_CLK_ENB();
    *(volatile uint32_t *)(BT_AUDIOCNTL0_ADDR) |= 0x00300000;
    BT_DRV_REG_OP_CLK_DIB();
}

void btdrv_reg_op_pcm_enable(void)
{
    BT_DRV_REG_OP_CLK_ENB();
    *(volatile uint32_t *)(BT_PCMGENCNTL_ADDR) |= 0x01; //pcm enable
    BT_DRV_REG_OP_CLK_DIB();
}

void btdrv_reg_op_pcm_disable(void)
{
    BT_DRV_REG_OP_CLK_ENB();
    *(volatile uint32_t *)(BT_PCMGENCNTL_ADDR) &= 0xfffffffe; //pcm disable
    BT_DRV_REG_OP_CLK_DIB();
}

#ifdef PCM_FAST_MODE
void btdrv_reg_op_open_pcm_fast_mode_enable(void)
{
    BT_DRV_REG_OP_CLK_ENB();
    BT_DRV_TRACE(0,"pcm fast mode %x,%x\n",BTDIGITAL_REG(BT_BES_PCMCNTL_ADDR),BTDIGITAL_REG(BT_PCMPHYSCNTL1_ADDR));
    BTDIGITAL_REG_SET_FIELD(BT_BES_PCMCNTL_ADDR, 1, 15, 1);
    BTDIGITAL_REG_SET_FIELD(BT_BES_PCMCNTL_ADDR, 0x7f, 8, 0x3b);
    BTDIGITAL_REG_SET_FIELD(BT_PCMPHYSCNTL1_ADDR, 0x1FF, 0, 8);
    BTDIGITAL_REG_SET_FIELD(BT_PCMPHYSCNTL1_ADDR, 1, 31, 0);
    BT_DRV_TRACE(0,"pcm fast mode config done %x,%x\n",BTDIGITAL_REG(BT_BES_PCMCNTL_ADDR),BTDIGITAL_REG(BT_PCMPHYSCNTL1_ADDR));
    BT_DRV_REG_OP_CLK_DIB();
}
void btdrv_reg_op_open_pcm_fast_mode_disable(void)
{
    //BT_DRV_REG_OP_CLK_ENB();
    //BTDIGITAL_REG_SET_FIELD(BT_BES_PCMCNTL_ADDR, 1, 15, 0);
    //BTDIGITAL_REG_SET_FIELD(BT_PCMPHYSCNTL1_ADDR, 0x1F, 0, 0);
    //BT_DRV_REG_OP_CLK_DIB();
}
#endif

#if defined(CVSD_BYPASS)
void btdrv_reg_op_cvsd_bypass_enable(void)
{
    BT_DRV_REG_OP_CLK_ENB();
    BTDIGITAL_REG(BT_E_SCOMUTECNTL_0_ADDR) |= 0x5555;
    BTDIGITAL_REG(BT_E_SCOMUTECNTL_1_ADDR) |= 0x5555;
    BTDIGITAL_REG_SET_FIELD(BT_E_SCOMUTECNTL_0_ADDR, 1, 23, 0);
    BTDIGITAL_REG_SET_FIELD(BT_E_SCOMUTECNTL_1_ADDR, 1, 23, 0);
    // BTDIGITAL_REG(0xD02201E8) |= 0x04000000; //test sequecy
    BTDIGITAL_REG(BT_AUDIOCNTL0_ADDR) &= ~(1<<7); //soft cvsd
    BTDIGITAL_REG(BT_AUDIOCNTL1_ADDR) &= ~(1<<7); //soft cvsd
    //BTDIGITAL_REG(0xD02201b8) |= (1<<31); //revert clk
    BT_DRV_REG_OP_CLK_DIB();
}

#endif

bool btdrv_reg_op_get_local_name(uint8_t* name)
{
    uint32_t btname_ptr = 0;
    bool ret = false;
    BT_DRV_REG_OP_CLK_ENB();
    if(lm_env_addr)
    {
        btname_ptr = *(uint32_t *)(lm_env_addr+0x5c);
        if(btname_ptr)
        {
            strncpy((char*)name, (char*)btname_ptr, BD_NAME_SIZE);
            BT_DRV_TRACE(0,"bt name %s\n",name);
            ret = true;
        }
        else
        {
            BT_DRV_TRACE(0,"bt name not set\n");
        }
    }
    BT_DRV_REG_OP_CLK_DIB();
    return ret;
}

int8_t btdrv_reg_op_txpwr_idx_to_rssidbm(uint8_t txpwr_idx)
{
    const int8_t RF_RPL_TX_PW_CONV_TBL[6] =
    {
        [0] = -23,
        [1] = -20,
        [2] = -17,
        [3] = -14,
        [4] = -11,
        [5] = -8
    };

    if(txpwr_idx > 5)
    {
        txpwr_idx = 5;
    }

    return RF_RPL_TX_PW_CONV_TBL[txpwr_idx];
}

void bt_drv_reg_op_ble_rx_gain_thr_tbl_set(void)
{
    BT_DRV_REG_OP_CLK_ENB();

    int sRet = 0;
    sRet = memcpy_s((uint8_t *)rf_rx_gain_ths_tbl_le_addr,sizeof(btdrv_rxgain_ths_tbl_le),btdrv_rxgain_ths_tbl_le,sizeof(btdrv_rxgain_ths_tbl_le));
    if (sRet)
    {
        TRACE(1, "%s line:%d sRet:%d", __func__, __LINE__, sRet);
    }
    BT_DRV_REG_OP_CLK_DIB();
}

void btdrv_reg_op_bts_to_bt_time(uint32_t bts, uint32_t* p_hs, uint16_t* p_hus)
{
    /// Integer part of the time (in half-slot)
    uint32_t last_hs = *(volatile uint32_t *)(rwip_env_addr + 0x0C);
    static uint32_t record_last_hs = 0;
    /// Fractional part of the time (in half-us) (range: 0-624)
    uint16_t last_hus = *(volatile uint32_t *)(rwip_env_addr + 0x10);
    static uint16_t record_last_hus = 0;
    /// Bluetooth timestamp value (in us) 32 bits counter
    uint32_t last_bts = *(volatile uint32_t *)(rwip_env_addr + 0x14);
    static uint32_t record_last_bts = 0;
    /// the times that record bt time
    uint8_t record_time = 0;
    static bool bt_time_recorded = false;
    uint8_t i=0;

    if (false == bt_time_recorded)
    {
        record_last_hs = last_hs;
        record_last_hus = last_hus;
        record_last_bts = last_bts;
        for (i=0; i<100; i++)
        {
            last_hs = *(volatile uint32_t *)(rwip_env_addr + 0x0C);
            last_hus = *(volatile uint32_t *)(rwip_env_addr + 0x10);
            last_bts = *(volatile uint32_t *)(rwip_env_addr + 0x14);

            if ((record_last_hs == last_hs) &&
                (record_last_hus == last_hus) &&
                (record_last_bts == last_bts))
            {
                record_time++;
                if (record_time >= 10)
                {
                    bt_time_recorded = true;
                    break;
                }
            }
            else
            {
                record_last_hs = last_hs;
                record_last_hus = last_hus;
                record_last_bts = last_bts;
                last_hs = *(volatile uint32_t *)(rwip_env_addr + 0x0C);
                last_hus = *(volatile uint32_t *)(rwip_env_addr + 0x10);
                last_bts = *(volatile uint32_t *)(rwip_env_addr + 0x14);
                record_time = 0;
            }
        }
        ASSERT(i<100, "%s i %d record %d", __func__, i, record_time);
    }

    // compute time in half microseconds between last sampled time and BT time provided in parameters
    int32_t clk_diff_hus = CLK_DIFF(record_last_hs, last_hs) * HALF_SLOT_SIZE;
    clk_diff_hus        += (int32_t)last_hus - (int32_t)record_last_hus;
    if (last_bts == (record_last_bts + (clk_diff_hus >> 1)))
    {
        record_last_hs = last_hs;
        record_last_hus = last_hus;
        record_last_bts = last_bts;
    }

    clk_diff_hus = 0;
    int32_t clk_diff_us = (int32_t) (bts - record_last_bts);

    //*p_hus = HALF_SLOT_SIZE - last_hus - 1;
    *p_hus = record_last_hus;
    *p_hs  = record_last_hs;

    // check if clock difference is positive or negative
    if(clk_diff_us > 0)
    {
        clk_diff_hus = (clk_diff_us << 1);
        *p_hus += CO_MOD(clk_diff_hus, HALF_SLOT_SIZE);
        *p_hs  += (clk_diff_hus / HALF_SLOT_SIZE);

        if(*p_hus >= HALF_SLOT_SIZE)
        {
            *p_hus -= HALF_SLOT_SIZE;
            *p_hs  += 1;
        }
        *p_hus = HALF_SLOT_SIZE - *p_hus - 1;
    }
    else
    {
        uint32_t diff_hus;
        clk_diff_hus = (record_last_bts - bts) << 1;
        diff_hus = CO_MOD(clk_diff_hus, HALF_SLOT_SIZE);

        if(*p_hus < diff_hus)
        {
            *p_hus += HALF_SLOT_SIZE;
            *p_hs  -= 1;
        }

        *p_hus -= diff_hus;
        *p_hus = HALF_SLOT_SIZE - *p_hus - 1;
        *p_hs  -= (clk_diff_hus / HALF_SLOT_SIZE);
    }

    // wrap clock
    *p_hs = (*p_hs & RWIP_MAX_CLOCK_TIME);
}

uint32_t btdrv_reg_op_bt_time_to_bts(uint32_t hs, uint16_t hus)
{
    /// Integer part of the time (in half-slot)
    uint32_t last_hs = *(volatile uint32_t *)(rwip_env_addr + 0x0C);
    static uint32_t record_last_hs = 0;
    /// Fractional part of the time (in half-us) (range: 0-624)
    uint16_t last_hus = *(volatile uint32_t *)(rwip_env_addr + 0x10);
    static uint16_t record_last_hus = 0;
    /// Bluetooth timestamp value (in us) 32 bits counter
    uint32_t last_bts = *(volatile uint32_t *)(rwip_env_addr + 0x14);
    static uint32_t record_last_bts = 0;
    /// the times that record bt time
    uint8_t record_time = 0;
    static bool bt_time_recorded = false;
    uint8_t i=0;

    if (false == bt_time_recorded)
    {
        record_last_hs = last_hs;
        record_last_hus = last_hus;
        record_last_bts = last_bts;
        for (i=0; i<100; i++)
        {
            last_hs = *(volatile uint32_t *)(rwip_env_addr + 0x0C);
            last_hus = *(volatile uint32_t *)(rwip_env_addr + 0x10);
            last_bts = *(volatile uint32_t *)(rwip_env_addr + 0x14);

            if ((record_last_hs == last_hs) &&
                (record_last_hus == last_hus) &&
                (record_last_bts == last_bts))
            {
                record_time++;
                if (record_time >= 10)
                {
                    bt_time_recorded = true;
                    break;
                }
            }
            else
            {
                record_last_hs = last_hs;
                record_last_hus = last_hus;
                record_last_bts = last_bts;
                last_hs = *(volatile uint32_t *)(rwip_env_addr + 0x0C);
                last_hus = *(volatile uint32_t *)(rwip_env_addr + 0x10);
                last_bts = *(volatile uint32_t *)(rwip_env_addr + 0x14);
                record_time = 0;
            }
        }
        ASSERT(i<100, "%s i %d record %d", __func__, i, record_time);
    }

    // compute time in half microseconds between last sampled time and BT time provided in parameters
    int32_t clk_diff_hus = CLK_DIFF(record_last_hs, last_hs) * HALF_SLOT_SIZE;
    clk_diff_hus        += (int32_t)last_hus - (int32_t)record_last_hus;
    if (last_bts == (record_last_bts + (clk_diff_hus >> 1)))
    {
        record_last_hs = last_hs;
        record_last_hus = last_hus;
        record_last_bts = last_bts;
    }

    // compute time in half microseconds between last record sampled time and BT time provided in parameters
    clk_diff_hus = CLK_DIFF(record_last_hs, hs) * HALF_SLOT_SIZE;
    clk_diff_hus        += (int32_t)(HALF_SLOT_SIZE - hus + 1) - (int32_t)record_last_hus;

    // compute BTS time in microseconds
    return (record_last_bts + (clk_diff_hus >> 1));
}


void bt_drv_reg_op_set_nosig_sch_flag(uint8_t enable)
{
    BT_DRV_REG_OP_CLK_ENB();
    if(nosig_sch_flag_addr)
    {
        *(uint32_t *)nosig_sch_flag_addr = enable;
        TRACE(0,"1501 nosig_sch_flag=%d",*(uint32_t *)nosig_sch_flag_addr);
    }
    BT_DRV_REG_OP_CLK_DIB();
}

void bt_drv_reg_op_le_pwr_ctrl_feats_disable(void)
{
    if(llm_local_le_feats_addr)
    {
        BT_DRV_REG_OP_ENTER();
        BT_DRV_REG_OP_CLK_ENB();
        //close BLE_FEATURES_BYTE4 for ss
        *(uint8_t *) (llm_local_le_feats_addr + 4) = 0;
        BT_DRV_REG_OP_CLK_DIB();
        BT_DRV_REG_OP_EXIT();
    }
}

void bt_drv_reg_op_le_ext_adv_txpwr_independent(uint8_t en)
{
    if(le_ext_adv_tx_pwr_independent_addr)
    {
        if(en)
        {
            bt_drv_reg_op_le_pwr_ctrl_feats_disable();
        }

        BT_DRV_REG_OP_ENTER();
        BT_DRV_REG_OP_CLK_ENB();
        *(uint8_t *) le_ext_adv_tx_pwr_independent_addr = en;
        BT_DRV_REG_OP_CLK_DIB();
        BT_DRV_REG_OP_EXIT();
    }
}

void bt_drv_reg_op_le_rf_reg_txpwr_set(uint8_t rf_reg, uint8_t val)
{
    BT_DRV_REG_OP_ENTER();
    BT_DRV_REG_OP_CLK_ENB();
    bt_drv_ble_rf_reg_txpwr_set(rf_reg, val);
    BT_DRV_REG_OP_CLK_DIB();
    BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_op_ble_sync_agc_mode_set(uint8_t en)
{

    BT_DRV_REG_OP_ENTER();
    BT_DRV_REG_OP_CLK_ENB();
    uint32_t lock = int_lock();
    if(en)
    {
        //open rf new sync agc mode
        bt_drv_rf_set_ble_sync_agc_enable(true);
        //open ble sync AGC process cbk
        bt_drv_reg_op_ble_sync_swagc_en_set(1);
    }
    else
    {
        //close rf new sync agc mode
        bt_drv_rf_set_ble_sync_agc_enable(false);
        //close ble sync AGC process cbk
        bt_drv_reg_op_ble_sync_swagc_en_set(0);

    }
    int_unlock(lock);
    BT_DRV_REG_OP_CLK_DIB();
    BT_DRV_REG_OP_EXIT();
}


int32_t bt_drv_reg_op_get_clkoffset(uint16_t linkid)
{
    uint32_t clkoff = 0;
    uint32_t acl_evt_ptr = 0x0;
    int32_t offset;
    uint32_t local_offset;

    BT_DRV_REG_OP_CLK_ENB();
    if(ld_acl_env_addr)
    {
        acl_evt_ptr = *(uint32_t *)(ld_acl_env_addr+linkid*4);
    }

    if (acl_evt_ptr != 0)
    {
        clkoff = *(uint32_t *)(acl_evt_ptr + 0xa8);
        //BT_DRV_TRACE(1,"[%s] clkoff=%d\n",__func__,clkoff);
    }
    else
    {
        BT_DRV_TRACE(1,"BT_REG_OP:ERROR LINK ID FOR RD clkoff %x", linkid);
    }

    BT_DRV_REG_OP_CLK_DIB();
    local_offset = clkoff & 0x0fffffff;
    offset = local_offset;
    offset = (offset << 4)>>4;
   return offset;
}

void btdrv_regop_get_ble_event_info(uint16_t ble_conhdl, uint16_t *evt_count, uint32_t *interval, uint32_t *anchor_ts, uint16_t *bit_off)
{
    BT_DRV_REG_OP_CLK_ENB();
    if(lld_con_env_addr)
    {
        uint32_t lld_con_evt_addr = BTDIGITAL_REG(lld_con_env_addr + ble_conhdl*4);
        BT_DRV_TRACE(1, "[%s] conhdl 0x%02x addr 0x%02x",__func__, ble_conhdl, lld_con_evt_addr);
        if(lld_con_evt_addr)
        {
            *anchor_ts = BTDIGITAL_REG(lld_con_evt_addr + 0x68);    // Timestamp (in half-slots) of the last connection anchor point where sync was detected
            *bit_off = BTDIGITAL_BT_EM(lld_con_evt_addr + 0x86);      // Value of bit offset when the last sync was detected
            *interval = BTDIGITAL_REG(lld_con_evt_addr + 0x7C);   // Interval in half-slots
            *evt_count = BTDIGITAL_BT_EM(lld_con_evt_addr + 0x90);  // Connection event counter
            BT_DRV_TRACE(1, "%d %d %d %d", *anchor_ts, *bit_off, *interval, *evt_count);
        }
    }
    BT_DRV_REG_OP_CLK_DIB();
}


void btdrv_regop_dump_btpcm_reg(void)
{
    BT_DRV_REG_OP_CLK_ENB();
    BT_DRV_TRACE(2,"btpcm reg = %x,%x\n",BTDIGITAL_REG(0xd0220c88),BTDIGITAL_REG(0xd0220688));
    BT_DRV_REG_OP_CLK_DIB();

}

int8_t bt_drv_reg_op_get_tx_pwr_dbm(uint16_t conhdl)
{
    int8_t tx_pwr_dbm = -127;

    if(!tx_power_val_bkup_addr)
    {
       return tx_pwr_dbm;
    }
    BT_DRV_REG_OP_ENTER();
    BT_DRV_REG_OP_CLK_ENB();
    uint8_t link_id = btdrv_conhdl_to_linkid(conhdl);
    uint32_t* tx_pwr_idx_base_addr =  (uint32_t*)tx_power_val_bkup_addr ;
    uint32_t* tx_pwr_idx_addr = tx_pwr_idx_base_addr + link_id;
    uint32_t txpwr_idx = *tx_pwr_idx_addr;
    uint8_t twpwr_val = bt_drv_tx_pwr_val_get(txpwr_idx);

    bool is_high_efficency =bt_drv_rf_is_high_efficency_tx();

    if(is_high_efficency)
    {
        switch(twpwr_val)
        {
            case TX_PWR_0dbm_VAL:
                tx_pwr_dbm = 0;
                break;
            case TX_PWR_1dbm_VAL:
                tx_pwr_dbm = 1;
                break;
            case TX_PWR_2dbm_VAL:
                tx_pwr_dbm = 2;
                break;
            case TX_PWR_3dbm_VAL:
                tx_pwr_dbm = 3;
                break;
            case TX_PWR_4dbm_VAL:
                tx_pwr_dbm = 4;
                break;
            case TX_PWR_5dbm_VAL:
                tx_pwr_dbm = 5;
                break;
            case TX_PWR_6dbm_VAL:
                tx_pwr_dbm = 6;
                break;
            case TX_PWR_7dbm_VAL:
                tx_pwr_dbm = 7;
                break;
            case TX_PWR_8dbm_VAL:
                tx_pwr_dbm = 8;
                break;
            case TX_PWR_9dbm_VAL:
                tx_pwr_dbm = 9;
                break;
            case TX_PWR_10dbm_VAL:
                tx_pwr_dbm = 10;
                break;
            case TX_PWR_11dbm_VAL:
                tx_pwr_dbm = 11;
                break;
            case TX_PWR_12dbm_VAL:
                tx_pwr_dbm = 12;
                break;
            case TX_PWR_13dbm_VAL:
                tx_pwr_dbm = 13;
                break;
            case TX_PWR_14dbm_VAL:
                tx_pwr_dbm = 14;
                break;
            case TX_PWR_15dbm_VAL:
                tx_pwr_dbm = 15;
                break;
            case TX_PWR_16dbm_VAL:
                tx_pwr_dbm = 16;
                break;
            case TX_PWR_N1dbm_VAL:
                tx_pwr_dbm = -1;
                break;
            case TX_PWR_N2dbm_VAL:
                tx_pwr_dbm = -2;
                break;
            case TX_PWR_N3dbm_VAL:
                tx_pwr_dbm = -3;
                break;
            default:
                ASSERT_ERR(0);
                break;
        }
    }
    else
    {
        switch(twpwr_val)
        {
            case TX_PWR_0dbm_VAL:
                tx_pwr_dbm = 0;
                break;
            case TX_PWR_1dbm_VAL:
                tx_pwr_dbm = 1;
                break;
            case TX_PWR_2dbm_VAL:
                tx_pwr_dbm = 2;
                break;
            case TX_PWR_3dbm_VAL:
                tx_pwr_dbm = 3;
                break;
            case TX_PWR_4dbm_VAL:
                tx_pwr_dbm = 4;
                break;
            case TX_PWR_5dbm_VAL:
                tx_pwr_dbm = 5;
                break;
            case TX_PWR_6dbm_VAL:
                tx_pwr_dbm = 6;
                break;
            case TX_PWR_7dbm_VAL:
                tx_pwr_dbm = 7;
                break;
            case TX_PWR_8dbm_VAL:
                tx_pwr_dbm = 8;
                break;
            case TX_PWR_9dbm_VAL:
                tx_pwr_dbm = 8;
                break;
            case TX_PWR_10dbm_VAL:
                tx_pwr_dbm = 8;
                break;
            case TX_PWR_11dbm_VAL:
                tx_pwr_dbm = 8;
                break;
            case TX_PWR_12dbm_VAL:
                tx_pwr_dbm = 8;
                break;
            case TX_PWR_13dbm_VAL:
                tx_pwr_dbm = 8;
                break;
            case TX_PWR_14dbm_VAL:
                tx_pwr_dbm = 8;
                break;
            case TX_PWR_15dbm_VAL:
                tx_pwr_dbm = 8;
                break;
            case TX_PWR_16dbm_VAL:
                tx_pwr_dbm = 8;
                break;
            case TX_PWR_N1dbm_VAL:
                tx_pwr_dbm = -1;
                break;
            case TX_PWR_N2dbm_VAL:
                tx_pwr_dbm = -2;
                break;
            case TX_PWR_N3dbm_VAL:
                tx_pwr_dbm = -3;
                break;
            default:
                ASSERT_ERR(0);
                break;
        }
    }

    BT_DRV_REG_OP_CLK_DIB();
    BT_DRV_REG_OP_EXIT();

    return tx_pwr_dbm;
}


void btdrv_regop_force_preferred_rate_send(uint16_t conhdl)
{
    BT_DRV_REG_OP_CLK_ENB();
    if(preferr_rate_rate_addr)
    {
        *(uint32_t *)preferr_rate_rate_addr = ((conhdl-0x80)<<8) | 0x6f;
    }
    BT_DRV_REG_OP_CLK_DIB();

}


void btdrv_regop_host_set_txpwr(uint16_t acl_connhdl, uint8_t txpwr_idx)
{
    static uint8_t txpwr_buf[4] = {0, 0, 0, 0,};

    BT_DRV_REG_OP_ENTER();
    BT_DRV_REG_OP_CLK_ENB();
    if(txpwr_val_buf_addr)
    {
        uint8_t* p_txpwr_buf = (uint8_t*)txpwr_val_buf_addr;
        uint8_t link_id = btdrv_conhdl_to_linkid(acl_connhdl);

        txpwr_buf[link_id] = txpwr_idx;
        memcpy(p_txpwr_buf, txpwr_buf, 4);
    }
    BT_DRV_REG_OP_CLK_DIB();
    BT_DRV_REG_OP_EXIT();
    btdrv_regop_set_txpwr_flg(1);
}


void btdrv_regop_set_txpwr_flg(uint32_t flg)
{
    BT_DRV_REG_OP_ENTER();
    BT_DRV_REG_OP_CLK_ENB();
    if(txpwr_host_set_flg_addr)
    {
        *(volatile uint32_t*)txpwr_host_set_flg_addr = flg;
    }
    BT_DRV_REG_OP_CLK_DIB();
    BT_DRV_REG_OP_EXIT();
}

uint8_t btdrv_reg_op_isohci_in_nb_buffer(uint8_t link_id)
{
    uint32_t p_in_env_addr;
    uint8_t nb_buffer = 0;

    if(isoohci_env_addr)
    {
        BT_DRV_REG_OP_ENTER();
        BT_DRV_REG_OP_CLK_ENB();

        p_in_env_addr = *(uint32_t *)(isoohci_env_addr + 4 * link_id);

        if(p_in_env_addr)
        {
            nb_buffer = *(uint8_t *)(p_in_env_addr + 48);
        }

        BT_DRV_REG_OP_CLK_DIB();
        BT_DRV_REG_OP_EXIT();
    }

    return nb_buffer;
}

void bt_drv_reg_op_low_txpwr(uint8_t enable, uint8_t factor, uint8_t bt_or_ble, uint8_t link_id)
{
   enum HAL_CHIP_METAL_ID_T metal_id = hal_get_chip_metal_id();

    BT_DRV_TRACE(3,"BT_REG_OP:low txpwr: %u %u ,link_id:%u", enable, bt_or_ble,link_id);

    if(metal_id >= HAL_CHIP_METAL_ID_2)
    {
        struct hci_dbg_set_txpwr_mode_cmd hci_param;

        hci_param.enable = enable;
        hci_param.factor = factor;
        hci_param.bt_or_ble = bt_or_ble;
        hci_param.link_id = link_id;

        btdrv_send_hci_cmd_bystack(HCI_DBG_SET_TXPWR_MODE_CMD_OPCODE, (uint8_t *)&hci_param, sizeof(struct hci_dbg_set_txpwr_mode_cmd));
    }
}


void bt_drv_reg_op_get_pkt_ts_rssi(uint16_t connhdl, CLKN_SER_NUM_T* buf)
{
    BT_DRV_REG_OP_ENTER();
    BT_DRV_REG_OP_CLK_ENB();
    uint8_t link_id = btdrv_conhdl_to_linkid(connhdl);
    if (clkn_sn_buff_addr && btdrv_is_link_index_valid(link_id))
    {
        memcpy((void*)buf, (void*)(clkn_sn_buff_addr + link_id * sizeof(CLKN_SER_NUM_T) * 5), 5*sizeof(CLKN_SER_NUM_T));
    }
    BT_DRV_REG_OP_CLK_DIB();
    BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_op_rssi_adjust_param(const struct hci_dbg_set_sw_rssi_cmd *parm)
{
    BT_DRV_REG_OP_ENTER();
    BT_DRV_REG_OP_CLK_ENB();

    if(hci_dbg_set_sw_rssi_addr)
    {
         memcpy((uint8_t *)hci_dbg_set_sw_rssi_addr, parm, sizeof(struct hci_dbg_set_sw_rssi_cmd));
    }

    BT_DRV_REG_OP_CLK_DIB();
    BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_op_rssi_adjust_mode_setting_for_test_mode(void)
{
    //extern const struct hci_dbg_set_sw_rssi_cmd  sw_rssi_work_test_mode;
    //bt_drv_reg_op_rssi_adjust_param(&sw_rssi_work_test_mode);
    BT_DRV_REG_OP_CLK_ENB();
    BTDIGITAL_REG(hci_dbg_set_sw_rssi_addr) |= 0x1;
    BT_DRV_REG_OP_CLK_DIB();
}



void bt_drv_reg_op_sync_found_check_hecerror(bool enable)
{
    if(sync_found_check_hecerror_addr)
    {
        BT_DRV_REG_OP_ENTER();
        BT_DRV_REG_OP_CLK_ENB();
        *(uint8_t *)sync_found_check_hecerror_addr = enable;
        BT_DRV_REG_OP_CLK_DIB();
        BT_DRV_REG_OP_EXIT();
    }
}

bool bt_drv_reg_op_check_link_exist(uint16_t con_hdl)
{
    uint32_t acl_evt_ptr = 0x0;
    uint8_t link_id;
    bool ret = false;

    link_id = btdrv_conhdl_to_linkid(con_hdl);

    if(link_id != HCI_LINK_INDEX_INVALID)
    {
        BT_DRV_REG_OP_ENTER();
        BT_DRV_REG_OP_CLK_ENB();
        if(ld_acl_env_addr)
        {
            acl_evt_ptr = *(volatile uint32_t *)(ld_acl_env_addr+link_id*4);
        }
        if(acl_evt_ptr != 0)
        {
            ret = true;
        }
        BT_DRV_REG_OP_CLK_DIB();
        BT_DRV_REG_OP_EXIT();
    }
    return ret;
}

void bt_drv_reg_op_setting_wesco(uint8_t wesco)
{
    if(wesco < 2 || wesco > 6)
    {
        BT_DRV_TRACE(1,"BT_REG_OP:wesco error:%u\n",wesco);
        return;
    }

    BT_DRV_TRACE(1,"BT_REG_OP:wesco:%u\n",wesco);

    BT_DRV_REG_OP_ENTER();
    BT_DRV_REG_OP_CLK_ENB();
    if(dbg_bt_common_setting_addr)
    {
        *(volatile uint8_t *)(dbg_bt_common_setting_addr+0x7) = wesco;
    }
    BT_DRV_REG_OP_CLK_DIB();
    BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_op_unify_testmode(uint8_t enable)
{
    if(unify_testmode_addr)
    {
        BT_DRV_REG_OP_ENTER();
        BT_DRV_REG_OP_CLK_ENB();
        *(uint8_t *)unify_testmode_addr = enable;
        BT_DRV_REG_OP_CLK_DIB();
        BT_DRV_REG_OP_EXIT();
    }
    TRACE(0,"1501 unify testmode");
}

void bt_drv_reg_op_afh_reporting_interval(uint16_t interval)
{
    if(afh_reporting_interval_addr)
    {
        BT_DRV_TRACE(1,"BT_REG_OP:afh reporting interval:%u\n",interval);

        BT_DRV_REG_OP_ENTER();
        BT_DRV_REG_OP_CLK_ENB();
        *(uint16_t *)afh_reporting_interval_addr = interval;
        BT_DRV_REG_OP_CLK_DIB();
        BT_DRV_REG_OP_EXIT();
    }
}
#ifdef __A2DP_ESCO_DYNAMIC_AGC_TABLE__
int8_t a2dp_agc_thd_tbl[] =
{
    -88,-88,
    -78,-78,
    -74,-74,
    -70,-70,
    -67,-67,
    -58,-58,
    -45,-45,
    -44,0x7f,
};

int8_t esco_agc_thd_tbl[] =
{
    -83,-83,
    -73,-73,
    -69,-69,
    -65,-65,
    -62,-62,
    -53,-53,
    -40,-40,
    -39,0x7f,
};

void bt_drv_reg_op_swagc_bt_thd(int8_t *thd_tbl)
{
    int sRet = 0;
    BT_DRV_REG_OP_ENTER();
    BT_DRV_REG_OP_CLK_ENB();
    if(rf_rx_gain_ths_tbl_bt_addr != 0)
    {
        sRet = memcpy_s((int8_t *)rf_rx_gain_ths_tbl_bt_addr, 16, thd_tbl, 16);
        if(sRet != 0)
        {
            TRACE(1,"func-s line:%d sRet:%d %s ", __LINE__, sRet, __func__);
        }
    }
    BT_DRV_REG_OP_CLK_DIB();
    BT_DRV_REG_OP_EXIT();
}
void bt_drv_reg_op_agc_table_mode(uint32_t mode)
{
    static uint8_t agc_table_mode_bak = AGC_INIT_MODE;
    uint8_t agc_table_mode = AGC_A2DP_MODE;

    switch(mode)
    {
        case BT_HFP_WORK_MODE:
            agc_table_mode = AGC_ESCO_MODE;

            break;
        case BT_A2DP_WORK_MODE:
        case BT_IDLE_MODE:
            agc_table_mode = AGC_A2DP_MODE;
            break;
        default:
            BT_DRV_TRACE(1,"BT_DRV:agc table mode set error mode=%d",mode);
            break;
    }

    if(agc_table_mode != agc_table_mode_bak)
    {
        agc_table_mode_bak = agc_table_mode;
        BT_DRV_TRACE(1,"BT_DRV:use agc table mode=%d[2:A2DP,1:ESCO]",agc_table_mode);
        if(agc_table_mode == AGC_A2DP_MODE)
        {
            bt_drv_reg_op_swagc_bt_thd(a2dp_agc_thd_tbl);
        }
        else
        {
            bt_drv_reg_op_swagc_bt_thd(esco_agc_thd_tbl);
        }
    }
}
#endif

void bt_drv_reg_op_fa_agc_init(void)
{
#ifndef __FIX_FA_RX_GAIN___
    BT_DRV_REG_OP_ENTER();
    BT_DRV_REG_OP_CLK_ENB();

    if(fa_agc_cfg_addr)
    {
        struct bes_fa_agc_config cfg;

        cfg.en = true;//enable fa new agc

        cfg.fa_gain_max = 3;//max fa gain limit

        cfg.no_sync_adujust_type  = 0;//type 1 decrease 1| type 0 change to max

        cfg.no_sync_ts_thr = 200;//half slot

        *(struct bes_fa_agc_config*)fa_agc_cfg_addr = cfg;
    }

    BT_DRV_REG_OP_CLK_DIB();
    BT_DRV_REG_OP_EXIT();
#endif
}

uint8_t bt_drv_reg_op_fa_agc_get(void)
{
    uint32_t fa_gain = 0xff;
    BT_DRV_REG_OP_ENTER();
    BT_DRV_REG_OP_CLK_ENB();

    uint32_t localVal = BT_DRIVER_GET_U32_REG_VAL(BT_BES_CNTL3_ADDR);
    fa_gain = ((localVal & ((uint32_t)0x01E00000)) >> 21);
    BT_DRV_REG_OP_CLK_DIB();
    BT_DRV_REG_OP_EXIT();
    return fa_gain;
}

void bt_drv_reg_op_ifs_winsize(uint8_t winsize)
{

        BT_DRV_REG_OP_ENTER();
        BT_DRV_REG_OP_CLK_ENB();

        BTDIGITAL_REG_SET_FIELD(0xd0220800, 0xf, 0, winsize);

        BT_DRV_REG_OP_CLK_DIB();
        BT_DRV_REG_OP_EXIT();
}
