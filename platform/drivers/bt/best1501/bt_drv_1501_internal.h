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
#ifndef __BT_DRV_1501_INTERNAL_H__
#define __BT_DRV_1501_INTERNAL_H__

//#define __HIGH_TX_POWER__       //BR 14dbm EDR 11dbm by luobin
//#define BT_IF_750K
//#define BT_IF_1M
#define BT_IF_1P05M
#define BT_1M_N98
#define __PWR_FLATNESS__

/***************************************************************************
 *Patch entry registers
 ****************************************************************************/
///enable m33 patch func
#define BTDRV_PATCH_EN_REG                  0xa01ffe00

//instruction patch compare src address
#define BTDRV_PATCH_INS_COMP_ADDR_START     0xa01ffe00

#define BTDRV_PATCH_INS_REMAP_ADDR_START    0xa01fff00

#define BT_PATCH_WR(addr, value)         (*(volatile uint32_t *)(addr)) = (value)
/***************************************************************************
 *RF conig macro
 ****************************************************************************/

//#define __SNR_WEIGHTING_MODE__

//#define BT_RF_OLD_CORR_MODE
/***************************************************************************
 *BT clock gate disable
 ****************************************************************************/
#define __CLK_GATE_DISABLE__

/***************************************************************************
 *BT read controller rssi
 ****************************************************************************/
#define  BT_RSSI_MONITOR  1

/***************************************************************************
 *PCM config macro
 ****************************************************************************/
//#define APB_PCM
//#define SW_INSERT_MSBC_TX_OFFSET

/***************************************************************************
 *BT afh follow function
 ****************************************************************************/
#define  BT_AFH_FOLLOW  0


#define SWAGC_INIT_MODE   0
#define NEW_SYNC_SWAGC_MODE   1
#define OLD_SWAGC_MODE   2

#define AGC_INIT_MODE   0
#define AGC_ESCO_MODE   1
#define AGC_A2DP_MODE   2

#define __A2DP_ESCO_DYNAMIC_AGC_TABLE__

//1501 t2 ble link number 11
#define BLE_ACTIVITY_MAX   11

 /**************************************
enable i2v read
****************************/
#define BT_I2V_VAL_RD

//set bt and ble addr
#define HCI_DBG_SET_BD_ADDR_CMD_OPCODE                0xFC32

//set clk drift and jitter
#define HCI_DBG_SET_LPCLK_DRIFT_JITTER_CMD_OPCODE     0xFC44

//set sleep enable and external wakeup enable
#define HCI_DBG_SET_SLEEP_EXWAKEUP_EN_CMD_OPCODE      0xFC47

//set edr threshold
#define HCI_DBG_SET_EDR_THRESHOLD_CMD_OPCODE          0xFC4C

//set edr alorithm
#define HCI_DBG_SET_EDR_ALGORITHM_CMD_OPCODE          0xFC4E

//set ble rl size
#define HCI_DBG_SET_RL_SIZE_CMD_OPCODE                0xFC5D

//set exernal wake up time oscillater wakeup time and radio wakeup time
#define HCI_DBG_SET_WAKEUP_TIME_CMD_OPCODE            0xFC71

//set pcm setting
#define HCI_DBG_SET_PCM_SETTING_CMD_OPCODE            0xFC74

//set tx pwr ctrl rssi thd
#define HCI_DBG_SET_RSSI_THRHLD_CMD_OPCODE            0xFC76

//set sleep setting
#define HCI_DBG_SET_SLEEP_SETTING_CMD_OPCODE          0xFC77

//set local feature
#define HCI_DBG_SET_LOCAL_FEATURE_CMD_OPCODE          0xFC81

//set local extend feature
#define HCI_DBG_SET_LOCAL_EX_FEATURE_CMD_OPCODE       0xFC82

//set bt rf timing
#define HCI_DBG_SET_BT_RF_TIMING_CMD_OPCODE           0xFC83

//set ble rf timing
#define HCI_DBG_SET_BLE_RF_TIMING_CMD_OPCODE          0xFC84

//bt setting interface
#define HCI_DBG_SET_BT_SETTING_CMD_OPCODE             0xFC86

//set customer param
#define HCI_DBG_SET_CUSTOM_PARAM_CMD_OPCODE           0xFC88

//switch sco path
#define HCI_DBG_SET_SCO_SWITCH_CMD_OPCODE             0xFC89

//set sco path
#define HCI_DBG_SET_SYNC_CONFIG_CMD_OPCODE            0xFC8F

//lmp message record
#define HCI_DBG_LMP_MESSAGE_RECORD_CMD_OPCODE         0xFC9C

//bt setting interface
#define HCI_DBG_SET_BT_SETTING_EXT1_CMD_OPCODE        0xFCAE

//set tx pwr ctrl rssi thd
#define HCI_DBG_SET_RSSI_TX_POWER_DFT_THR_CMD_OPCODE  0xFCB3

//set tx pwr mode
#define HCI_DBG_SET_TXPWR_MODE_CMD_OPCODE             0xFCC0

//bt setting interface
#define HCI_DBG_SET_BT_SCHE_SETTING_CMD_OPCODE        0xFCC3

//bt setting interface
#define HCI_DBG_SET_BT_IBRT_SETTING_CMD_OPCODE        0xFCC4

//bt setting interface
#define HCI_DBG_SET_BT_HW_FEAT_SETTING_CMD_OPCODE     0xFCC5

//bt setting interface
#define HCI_DBG_BT_COMMON_SETTING_T1_CMD_OPCODE       0xFCCA

//set afh assess mode
#define HCI_DBG_SET_AFH_ASSESS_CMD_OPCODE             0xFCCB

#define BT_CONTROLLER_CRASH_DUMP_ADDR_BASE  (0xc0004050)
#define BT_ERRORTYPESTAT_ADDR   (0xd0220460)
#define DEFAULT_XTAL_FCAP                       0x1300
#define MAX_NB_ACTIVE_ACL                   (4)

#ifdef __cplusplus
extern "C" {
#endif
#define TRC_KE_MSG_TYPE              0x00000001
#define TRC_KE_TMR_TYPE              0x00000002
#define TRC_KE_EVT_TYPE              0x00000004
#define TRC_MEM_TYPE                 0x00000008
#define TRC_SLEEP_TYPE               0x00000010
#define TRC_SW_ASS_TYPE              0x00000020
#define TRC_PROG_TYPE                0x00000040
#define TRC_CS_BLE_TYPE              0x00000080
#define TRC_CS_BT_TYPE               0x00000100
#define TRC_RX_DESC_TYPE             0x00000200
#define TRC_LLCP_TYPE                0x00000400
#define TRC_LMP_TYPE                 0x00000800
#define TRC_L2CAP_TYPE               0x00001000
#define TRC_ARB_TYPE                 0x00002000
#define TRC_LLC_STATE_TRANS_TYPE     0x00004000
#define TRC_KE_STATE_TRANS_TYPE      0x00008000
#define TRC_HCI_TYPE                 0x00010000
#define TRC_ADV_TYPE                 0x00020000
#define TRC_ACL_TYPE                 0x00040000
#define TRC_CUSTOM_TYPE              0x00080000
#define TRC_IBRT_TYPE                0x00100000
#define TRC_ALL_TYPE                 0xffffffff
#define TRC_ALL_EXCEP_CS_BT_TYPE     0xfffffeff
#define TRC_DEFAULT_TYPE             (TRC_SW_ASS_TYPE|TRC_LMP_TYPE|TRC_LLC_STATE_TRANS_TYPE|TRC_KE_STATE_TRANS_TYPE|TRC_IBRT_TYPE)

#define FAST_LOCK_ENABLE 1
#define FAST_LOCK_DISABLE 0

#define NEW_SYNC_SWAGC_MODE   1
#define OLD_SWAGC_MODE   2

#define BT_LOW_TXPWR_MODE    0
#define BLE_LOW_TXPWR_MODE   1
#define BLE_ADV_LOW_TXPWR_MODE   2

#define FACTOR_ATTENUATION_0DBM  0
#define FACTOR_ATTENUATION_6DBM  1
#define FACTOR_ATTENUATION_12DBM  2
#define FACTOR_ATTENUATION_18DBM  3


#if (__FASTACK_ECC_CONFIG_BLOCK__ == 1)    // 1 BLOCK
    #define ECC_MODU_MODE ECC_DPSK
    #define ECC_BLK_MODE ECC_1BLOCK
#elif (__FASTACK_ECC_CONFIG_BLOCK__ == 2)    // 2BLOCK
    #define ECC_MODU_MODE ECC_8PSK
    #define ECC_BLK_MODE ECC_2BLOCK
#elif (__FASTACK_ECC_CONFIG_BLOCK__ == 3)    // 3BLOCK
    #define ECC_MODU_MODE ECC_8PSK
    #define ECC_BLK_MODE ECC_3BLOCK
#endif


#define  INVALID_SYNC_WORD  (0)

// TO DO: need confirm fa reg addr
#define BT_REG_CLKCAP_REG_ADDR   0xD0220E00
#define BT_BES_CLK_REG1_ADDR   0xD0220E04
#define BT_BES_CLK_REG2_ADDR   0xD0220E08
#define BT_REG_CLKNCNT_CAP4_REG_ADDR   0xD0220E34
#define BT_BES_FINECNT_CAP4_REG_ADDR   0xD0220E38
#define BT_REG_CLKNCNT_CAP5_REG_ADDR   0xD0220E3C
#define BT_BES_FINECNT_CAP5_REG_ADDR   0xD0220E40
#define BT_REG_CLKNCNT_CAP6_REG_ADDR   0xD0220E44
#define BT_BES_FINECNT_CAP6_REG_ADDR   0xD0220E48
#define BT_REG_CLKNCNT_CAP7_REG_ADDR   0xD0220E4C
#define BT_BES_FINECNT_CAP7_REG_ADDR   0xD0220E50
#define BT_BES_CNTL1_ADDR   0xD0220C08
#define BT_BES_CNTL2_ADDR   0xD0220C0C
#define BT_BES_CNTL3_ADDR   0xD0220C10
#define BT_BES_CNTL5_ADDR   0xD0220C18
#define BT_BES_CNTLX_ADDR   0xD0220C7C
#define BT_CAP_SEL_ADDR   0xd0220C50
#define BT_TRIGREG_ADDR   0xD0220C80
#define BT_TWSBTCLKREG_ADDR   0xD0220C84
#define BT_BES_PCMCNTL_ADDR   0xD0220C88
#define BT_BES_FACNTL0_ADDR   0xD0220C8C
#define BT_BES_FACNTL1_ADDR   0xD0220C90
#define BT_BES_TG_CLKNCNT_ADDR  0xD0220C94
#define BT_BES_TG_FINECNT_ADDR  0xD0220C98
#define BT_BES_TG_CLKNCNT1_ADDR  0xD0220D08
#define BT_BES_TG_FINECNT1_ADDR  0xD0220D0C
#define BT_BES_TG_CLKNCNT2_ADDR  0xD0220D10
#define BT_BES_TG_FINECNT2_ADDR  0xD0220D14
#define BT_BES_TG_CLKNCNT3_ADDR  0xD0220D18
#define BT_BES_TG_FINECNT3_ADDR  0xD0220D1C
#define BT_BES_PCM_DF_REG1_ADDR   0xD0220CA0
#define BT_BES_PCM_DF_REG_ADDR   0xD0220C9C
#define BT_BES_FA_SWREG0_ADDR   0xD0220CC4
#define BT_BES_FA_SWREG1_ADDR   0xD0220CC8
#define BT_E_SCOMUTECNTL_0_ADDR   0xD0220614
#define BT_E_SCOMUTECNTL_1_ADDR   0xD0220634
#define BT_E_SCOCURRENTTXPTR_ADDR   0xD0220618
#define BT_E_SCOCURRENTRXPTR_ADDR   0xD022061C
#define BT_AUDIOCNTL0_ADDR   0xD0220670
#define BT_AUDIOCNTL1_ADDR   0xD0220674
#define BT_PCMGENCNTL_ADDR   0xD0220680
#define BT_PCMPHYSCNTL1_ADDR   0xD0220688
#define BT_RADIOPWRUPDN_ADDR   0xD022048C
#define BT_RADIOTXRXTIM_ADDR   0xD0220490
#define BT_BES_ENHPCM_CNTL_ADDR   0xD0220CBC
#define BT_CURRENTRXDESCPTR_ADDR   0xD0220428

#define IP_SLOTCLK_ADDR         0xD0220100
#define IP_ISOCNTSAMP_ADDR      0xD02209C4

#define BLE_RADIOCNTL3_ADDR   0xD022087C
#define BLE_RADIOPWRUPDN0_ADDR   0xD0220880
#define BLE_RADIOPWRUPDN1_ADDR   0xD0220884
#define BLE_RADIOPWRUPDN2_ADDR   0xD0220888
#define BLE_RADIOPWRUPDN3_ADDR   0xD022088c

/// Combined duration of Preamble and Access Address depending on the PHY used (in us)
#define BLE_PREAMBLE_ACCESS_ADDR_DUR_1MBPS      (5*8)
#define BLE_PREAMBLE_ACCESS_ADDR_DUR_2MBPS      (6*4)
#define BLE_PREAMBLE_ACCESS_ADDR_DUR_125KBPS    (80+256)
#define BLE_PREAMBLE_ACCESS_ADDR_DUR_500KBPS    (80+256)

#define BLE_RADIOTXRXTIM0_ADDR   0xD0220890
#define BLE_RADIOTXRXTIM1_ADDR   0xD0220894
#define BLE_RADIOTXRXTIM2_ADDR   0xD0220898
#define BLE_RADIOTXRXTIM3_ADDR   0xD022089C
//#define BLE_RXGAIN_THS_TBL_ADDR   0xc00043dc
#define CMU_CLKREG_ADDR   0xD0330038

#define RWIP_MAX_CLOCK_TIME              ((1L<<28) - 1)
#define CLK_SUB(clock_a, clock_b)     ((uint32_t)(((clock_a) - (clock_b)) & RWIP_MAX_CLOCK_TIME))
#define CLK_LOWER_EQ(clock_a, clock_b)      (CLK_SUB(clock_b, clock_a) < (RWIP_MAX_CLOCK_TIME >> 1))
#define CLK_DIFF(clock_a, clock_b)     ( (CLK_SUB((clock_b), (clock_a)) > ((RWIP_MAX_CLOCK_TIME+1) >> 1)) ? \
                          ((int32_t)((-CLK_SUB((clock_a), (clock_b))))) : ((int32_t)((CLK_SUB((clock_b), (clock_a))))) )

#define __FIX_FA_RX_GAIN___//

#define __FA_10US_WIN_SIZE__

#ifdef __FA_10US_WIN_SIZE__
#define FA_RX_WIN_SIZE      (0xA)
#define FA_RXPWRUP_TIMING   (0x39)
#else
#define FA_RX_WIN_SIZE      (0x1E)
#define FA_RXPWRUP_TIMING   (0x2d)
#endif

#define FA_FIX_TX_GIAN_IDX  (0x4)
#define FA_FIX_RX_GIAN_IDX  (0x1)
#define FA_NEW_CORR_VALUE   (0x40)
//lager than 2 will causes error synchronization
#define FA_OLD_CORR_VALUE   (0x1)
#define FA_MULTI_TX_COUNT   (0x3)
#define INVALID_FA_FREQ_TABLE       (0)

#define CMD_FILTER_LEN              (10)
#define EVT_FILTER_LEN              (10)

#ifdef BT_FAST_LOCK_ENABLE
#define SPI0_EN        (1)
#define SPI0_TXON_NUM  (3)
#define SPI0_RXON_NUM  (3)
#define SPI0_TXON_POS  (6)
#define SPI0_RXON_POS  (36)
#else
#define SPI0_EN        (0)
#define SPI0_TXON_NUM  (0)
#define SPI0_RXON_NUM  (0)
#define SPI0_TXON_POS  (0)
#define SPI0_RXON_POS  (0)
#endif

#ifdef TX_PWR_LOWER_2DBM
#define DIGITAL_POWER_ADJUST_EN (0)
#define DIGITAL_DEC_EDR_POWER_VAL (0x1F)
#else
#define DIGITAL_POWER_ADJUST_EN (0)
#define DIGITAL_DEC_EDR_POWER_VAL (0x1F)
#endif

enum{
    ECC_8PSK = 0,
    ECC_DPSK = 1,
    ECC_GFSK = 2,
    ECC_MODU_MAX,
};

enum{
    ECC_1BLOCK = 0,
    ECC_2BLOCK = 1,
    ECC_3BLOCK = 2,
    ECC_BLOCK_MAX,
};

enum{
    FA_SYNCWORD_32BIT = 0,
    FA_SYNCWORD_64BIT = 1,
    FA_SYNCWORD_MAX,
};

enum{
    ECC_8_BYTE_MODE = 0,
    ECC_16_BYTE_MODE = 1,
    ECC_12_BYTE_MODE = 2,
    ECC_MAX_BYTE_MODE,
};

enum{
    ECC_DISABLE_PKT_TYPE_2DH3   = 0,
    ECC_DISABLE_PKT_TYPE_DM3    = 1,
    ECC_DISABLE_PKT_TYPE_3DH3   = 2,
    ECC_DISABLE_PKT_TYPE_DH3    = 3,
    ECC_DISABLE_PKT_TYPE_2DH5   = 4,
    ECC_DISABLE_PKT_TYPE_DM5    = 5,
    ECC_DISABLE_PKT_TYPE_3DH5   = 6,
    ECC_DISABLE_PKT_TYPE_DH5    = 7,
    ECC_DISABLE_PKT_TYPE_3DH1   = 8,
    ECC_DISABLE_PKT_TYPE_2DH1   = 9,
    ECC_DISABLE_PKT_TYPE_DH1    = 10,
    ECC_DISABLE_PKT_TYPE_DM1    = 11,
    ECC_DISABLE_PKT_TYPE_2EV3   = 12,
    ECC_DISABLE_PKT_TYPE_3EV3   = 13,
    ECC_DISABLE_PKT_TYPE_EV3    = 14,
    ECC_DISABLE_PKT_TYPE_2EV5   = 15,
    ECC_DISABLE_PKT_TYPE_EV4    = 16,
    ECC_DISABLE_PKT_TYPE_3EV5   = 17,
    ECC_DISABLE_PKT_TYPE_EV5    = 18,
    ECC_DISABLE_PKT_TYPE_Others = 19,
    ECC_DISABLE_PKT_TYPE_MAX,
};

enum
{
    METRIC_TYPE_TX_DM1_COUNTS = 0,
    METRIC_TYPE_TX_DH1_COUNTS,//1
    METRIC_TYPE_TX_DM3_COUNTS,//2
    METRIC_TYPE_TX_DH3_COUNTS,//3
    METRIC_TYPE_TX_DM5_COUNTS,//4
    METRIC_TYPE_TX_DH5_COUNTS,//5
    METRIC_TYPE_TX_2_DH1_COUNTS,//6
    METRIC_TYPE_TX_3_DH1_COUNTS,//7
    METRIC_TYPE_TX_2_DH3_COUNTS,//8
    METRIC_TYPE_TX_3_DH3_COUNTS,//9
    METRIC_TYPE_TX_2_DH5_COUNTS,//10
    METRIC_TYPE_TX_3_DH5_COUNTS,//11
    METRIC_TYPE_RX_DM1_COUNTS,//12
    METRIC_TYPE_RX_DH1_COUNTS,//13
    METRIC_TYPE_RX_DM3_COUNTS,//14
    METRIC_TYPE_RX_DH3_COUNTS,//15
    METRIC_TYPE_RX_DM5_COUNTS,//16
    METRIC_TYPE_RX_DH5_COUNTS,//17
    METRIC_TYPE_RX_2_DH1_COUNTS,//18
    METRIC_TYPE_RX_3_DH1_COUNTS,//19
    METRIC_TYPE_RX_2_DH3_COUNTS,//20
    METRIC_TYPE_RX_3_DH3_COUNTS,//21
    METRIC_TYPE_RX_2_DH5_COUNTS,//22
    METRIC_TYPE_RX_3_DH5_COUNTS,//23
    METRIC_TYPE_RX_HEC_ERROR_COUNTS,//24
    METRIC_TYPE_RX_CRC_ERROR_COUNTS,//25
    METRIC_TYPE_RX_FEC_ERROR_COUNTS,//26
    METRIC_TYPE_RX_GUARD_ERROR_COUNTS,//27
    METRIC_TYPE_RX_WRONGPKTFLAG_ERROR_COUNTS,//28
    METRIC_TYPE_RADIO_COUNTS,//29
    METRIC_TYPE_SLEEP_DURATION_COUNTS,//30
    METRIC_TYPE_RADIO_TX_SUCCESS_COUNTS,//31
    METRIC_TYPE_RADIO_TX_COUNTS,//32
    METRIC_TYPE_SOFTBIT_SUCCESS_COUNTS,//33
    METRIC_TYPE_SOFTBIT_COUNTS,//34
    METRIC_TYPE_RXSEQERR_COUNTS,//35
    METRIC_TYPE_RECV_FA_COUNTS,//36
    METRIC_TYPE_RX_2_EV3_COUNTS,//37
    METRIC_TYPE_MAX,
};

enum
{
    TX_PWR_IDX_0 = 0,
    TX_PWR_IDX_1 = 1,
    TX_PWR_IDX_2 = 2,
    TX_PWR_IDX_3 = 3,
    TX_PWR_IDX_4 = 4,
    TX_PWR_IDX_MAX = 5,
    TX_PWR_IDX_PAGE = 6,
};

#define TX_PWR_16dbm_VAL (0x34)
#define TX_PWR_15dbm_VAL (0x32)
#define TX_PWR_14dbm_VAL (0x31)
#define TX_PWR_13dbm_VAL (0x30)
#define TX_PWR_12dbm_VAL (0x21)
#define TX_PWR_11dbm_VAL (0x20)
#define TX_PWR_10dbm_VAL (0x16)
#define TX_PWR_9dbm_VAL (0x15)
#define TX_PWR_8dbm_VAL (0x14)
#define TX_PWR_7dbm_VAL (0x13)
#define TX_PWR_6dbm_VAL (0x12)
#define TX_PWR_5dbm_VAL (0x11)
#define TX_PWR_4dbm_VAL (0x10)
#define TX_PWR_3dbm_VAL (0x7)
#define TX_PWR_2dbm_VAL (0x5)
#define TX_PWR_1dbm_VAL (0x4)
#define TX_PWR_0dbm_VAL (0x3)
#define TX_PWR_N1dbm_VAL (0x2)
#define TX_PWR_N2dbm_VAL (0x1)
#define TX_PWR_N3dbm_VAL (0x0)

typedef struct
{
    bool fa_tx_gain_en;
    uint8_t fa_tx_gain_idx;
    bool fa_rx_gain_en;
    uint8_t fa_rx_gain_idx;
    bool fa_2m_mode;
    uint8_t fa_rx_winsz;
    uint8_t syncword_len;
    bool fa_vendor_syncword_en;
    uint32_t syncword_high;
    uint32_t syncword_low;
    bool is_new_corr;
    uint8_t new_mode_corr_thr;
    uint8_t old_mode_corr_thr;
    bool enhance_fa_mode_en;
    bool fa_tx_preamble_en;
    bool fa_freq_table_en;
    uint32_t fa_freq_table0;
    uint32_t fa_freq_table1;
    uint32_t fa_freq_table2;
    bool fa_multi_mode0_en;
    bool fa_multi_mode1_en;
    uint8_t fa_multi_tx_count;
}btdrv_fa_basic_config_t;

typedef struct
{
    bool ecc_mode_enable;
    uint8_t ecc_modu_mode_acl;
    uint8_t ecc_modu_mode_sco;
    uint8_t ecc_blk_mode;
    uint8_t ecc_len_mode_sel;
}btdrv_ecc_basic_config_t;

typedef struct
{
    uint32_t dat_arr[4];
}ecc_trx_dat_arr_t;


typedef struct
{
    ecc_trx_dat_arr_t trx_dat;
}ecc_trx_dat_t;


struct hci_acl_data_pkt
{
    uint16_t handle  :10;
    uint16_t ecc_flg : 1;
    uint16_t softbit_flg : 1;

    //0b00 First non-automatically-flushable packet of a higher layer message
    //(start of a non-automatically-flushable L2CAP PDU) from Host to Controller.
    //0b01 Continuing fragment of a higher layer message
    //0b10 First automatically flushable packet of a higher layer message (start of an automatically-flushable L2CAP PDU)
    //0b11 A complete L2CAP PDU. Automatically flushable.

    uint16_t pb_flag :2;
    uint16_t bc_flag :2;
    uint16_t data_load_len;
    uint8_t sdu_data[0];
}__attribute__((packed));

#define MULTI_IBRT_SUPPORT_NUM     3
struct sch_adjust_timeslice_per_link
{
    uint8_t link_id;
    uint32_t timeslice;
};

struct sch_multi_ibrt_adjust_timeslice_env_t
{
    uint8_t update;
    struct sch_adjust_timeslice_per_link ibrt_link[MULTI_IBRT_SUPPORT_NUM];
};

///BD Address structure
struct bd_addr
{
    ///6-byte array address value
    uint8_t  addr[6];
};
struct dbg_afh_assess_params
{
    struct bd_addr bd_addr;

    uint8_t afh_monitor_negative_num;
    uint8_t afh_monitor_positive_num;

    uint8_t average_cnt;
    uint8_t sch_prio_dft;
    uint8_t auto_resch_att;

    uint8_t rxgain;
    int8_t  afh_good_chl_thr;

    uint8_t lt_addr;
    bool edr;

    uint16_t interval;
    uint16_t sch_expect_assess_num;
    uint16_t prog_spacing_hus;
};

struct hci_dbg_set_afh_assess_params
{
    uint8_t enable;
    /// BD_ADDR
    struct bd_addr bd_addr;

    uint8_t afh_monitor_negative_num;
    uint8_t afh_monitor_positive_num;

    uint8_t average_cnt;
    uint8_t sch_prio_dft;
    uint8_t auto_resch_att;

    uint8_t rxgain;
    int8_t  afh_good_chl_thr;

    /// LT_ADDR
    uint8_t lt_addr;
    /// Enhanced data rate
    bool edr;

    /// Interval (in slots)
    uint16_t interval;
    uint16_t sch_expect_assess_num;
    uint16_t prog_spacing_hus;
}__attribute__((packed));

struct hci_dbg_set_txpwr_mode_cmd
{
    uint8_t     enable;
    //1: -6dbm, 2:-12dbm
    uint8_t     factor;
    //0:bt  1:ble 2:ble adv
    uint8_t     bt_or_ble;

    uint8_t     link_id;
}__attribute__((packed));

struct hci_dbg_set_sw_rssi_cmd
{
    bool sw_rssi_en;
    uint32_t link_agc_thd_mobile;
    uint32_t link_agc_thd_mobile_time;//(in BT half-slots)
    uint32_t link_agc_thd_tws;
    uint32_t link_agc_thd_tws_time;//(in BT half-slots)
    uint8_t rssi_mobile_step;
    uint8_t rssi_tws_step;
    int8_t rssi_min_value_mobile;
    int8_t rssi_min_value_tws;

    bool        ble_sw_rssi_en;
    uint32_t    ble_link_agc_thd;
    uint32_t    ble_link_agc_thd_time;//(in BT half-slots)
    uint8_t     ble_rssi_step;
    int8_t      ble_rssidbm_min_value;

    uint8_t  bt_no_sync_en;
    int8_t  bt_link_no_sync_rssi;
    uint16_t bt_link_no_snyc_thd;
    uint16_t bt_link_no_sync_timeout;//(in BT half-slots)

    uint8_t  ble_no_sync_en;
    int8_t   ble_link_no_sync_rssi;
    uint16_t ble_link_no_snyc_thd;
    uint16_t ble_link_no_sync_timeout;//(in BT half-slots)
};

struct bes_fa_agc_config
{
    bool en;
    uint8_t no_sync_adujust_type;
    uint8_t fa_gain_max;
    uint16_t no_sync_ts_thr;
};

typedef struct
{
    union
    {
        uint16_t reg_val;
        struct
        {
            uint16_t bt_pwr  :6;
            uint16_t ble_pwr :6;
            uint16_t reserve :4;
        }reg_bit;
    }u;
}TX_PWR_REG_T;

extern const int8_t btdrv_rxgain_ths_tbl_le[0xf * 2];
///only used for testmode
extern uint16_t RF_I2V_CAP_2M, RF_I2V_CAP_3M;
#ifdef RX_IQ_CAL
extern int iq_gain,iq_phy;
#endif

void bt_drv_reg_op_ll_monitor(uint16_t connhdl, uint8_t metric_type, uint32_t* ret_val);
void btdrv_ecc_basic_config(btdrv_ecc_basic_config_t* p_ecc_basic_config);
void btdrv_ecc_disable_spec_pkt_type(uint32_t ptk_type); // 1 -> disable FA, ptk type enum in bt_drv_1501_internal.h
void btdrv_ecc_config_usr_tx_dat_set(ecc_trx_dat_t* p_ecc_trx_dat);
void btdrv_ecc_config_usr_rx_dat_get(ecc_trx_dat_t* p_ecc_trx_dat);
void bt_drv_reg_op_set_rf_rx_hwgain_tbl(int8_t (*p_tbl)[3]);
void bt_drv_reg_op_set_rf_hwagc_rssi_correct_tbl(int8_t* p_tbl);
void btdrv_52m_sys_enable();
void btdrv_26m_sys_enable();
void btdrv_fa_corr_mode_setting(bool is_new_corr);
void btdrv_rf_rx_gain_adjust_req(uint32_t user, bool lowgain);
void bt_drv_reg_op_for_test_mode_disable(void);
void bt_drv_reg_op_config_controller_log(uint32_t log_mask, uint16_t* p_cmd_filter, uint8_t cmd_nb, uint8_t* p_evt_filter, uint8_t evt_nb);
void bt_drv_reg_op_set_controller_trace_curr_sw(uint32_t log_mask);
void btdrv_txpower_calib(void);

void btdrv_fa_config_tx_gain(bool ecc_tx_gain_en, uint8_t ecc_tx_gain_idx);
void btdrv_fa_config_rx_gain(bool ecc_rx_gain_en, uint8_t ecc_rx_gain_idx);
void bt_drv_reg_op_set_fa_rx_gain_idx(uint8_t rx_gain_idx);
void bt_drv_reg_op_config_cotroller_cmd_filter(uint16_t* p_cmd_filter);
void bt_drv_reg_op_config_cotroller_evt_filter(uint8_t* p_evt_filter);
void bt_drv_rf_set_bt_sync_agc_enable(bool enable);
void bt_drv_rf_set_ble_sync_agc_enable(bool enable);
void btdrv_set_spi_trig_pos_enable(void);
void btdrv_clear_spi_trig_pos_enable(void);
void bt_drv_reg_op_afh_assess_init(void);
void bt_drv_reg_op_power_off_rx_gain_config(void);
void bt_drv_reg_op_ble_rx_gain_thr_tbl_set(void);
void bt_drv_reg_op_init_swagc_3m_thd(void);
void bt_drv_iq_tab_set_testmode(void);
void bt_drv_reg_op_le_ext_adv_txpwr_independent(uint8_t en);
void bt_drv_ble_rf_reg_txpwr_set(uint8_t rf_reg, uint8_t val);
void bt_drv_reg_op_le_rf_reg_txpwr_set(uint8_t rf_reg, uint8_t val);
void bt_drv_reg_op_ble_sync_agc_mode_set(uint8_t en);
void btdrv_regop_set_txpwr_flg(uint32_t flg);
uint8_t bt_drv_tx_pwr_val_get(uint32_t idx);
int8_t bt_drv_reg_op_get_tx_pwr_dbm(uint16_t conhdl);
void btdrv_regop_host_set_txpwr(uint16_t acl_connhdl, uint8_t txpwr_idx);
void btdrv_regop_set_txpwr_flg(uint32_t flg);
uint8_t btdrv_reg_op_isohci_in_nb_buffer(uint8_t link_id);
void bt_drv_rf_high_efficency_tx_pwr_ctrl_for_testmode(void);
void bt_drv_reg_op_low_txpwr(uint8_t enable, uint8_t factor, uint8_t bt_or_ble, uint8_t link_id);
void btdrv_bridge_send_data(const uint8_t *buff,uint8_t len);
uint16_t bt_drv_rf_get_e1_val(void);
bool bt_drv_rf_is_high_efficency_tx(void);
void bt_drv_reg_op_sync_found_check_hecerror(bool enable);
bool bt_drv_reg_op_check_link_exist(uint16_t con_hdl);
void bt_drv_reg_op_setting_wesco(uint8_t wesco);
void bt_drv_reg_op_unify_testmode(uint8_t enable);
void bt_drv_reg_op_afh_reporting_interval(uint16_t interval);
void bt_drv_reg_op_agc_table_mode(uint32_t mode);
void bt_drv_reg_op_fa_agc_init(void);
#ifdef __cplusplus
}
#endif

#endif
