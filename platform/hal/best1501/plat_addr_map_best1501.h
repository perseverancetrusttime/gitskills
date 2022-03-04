/***************************************************************************
 *
 * Copyright 2015-2020 BES.
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
#ifndef __PLAT_ADDR_MAP_BEST1501_H__
#define __PLAT_ADDR_MAP_BEST1501_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef LARGE_SENS_RAM
#ifdef SMALL_RET_RAM
#error "LARGE_SENS_RAM conflicts with SMALL_RET_RAM"
#endif
#ifdef CORE_SLEEP_POWER_DOWN
#error "MCU retention ram needs shrinking when both LARGE_SENS_RAM and CORE_SLEEP_POWER_DOWN are defined"
#endif
#endif

#ifndef SMALL_RET_RAM
#define SENS_RAMRET
#endif

#define ROM_BASE                                0x24000000
#define ROMX_BASE                               0x00020000

#ifndef ROM_SIZE
#define ROM_SIZE                                0x00010000
#endif
#define ROM_EXT_SIZE                            0x00008000

#define PATCH_ENTRY_NUM                         8
#define PATCH_CTRL_BASE                         0x0004F000
#define PATCH_DATA_BASE                         0x0004F020

#define RAM0_BASE                               0x20000000
#define RAMX0_BASE                              0x00200000
#define RAM1_BASE                               0x20020000
#define RAMX1_BASE                              0x00220000
#define RAM2_BASE                               0x20040000
#define RAMX2_BASE                              0x00240000
#define RAM3_BASE                               0x20080000
#define RAMX3_BASE                              0x00280000
#define RAM4_BASE                               0x200C0000
#define RAMX4_BASE                              0x002C0000
#define RAM5_BASE                               0x20100000
#define RAMX5_BASE                              0x00300000
#define RAM6_BASE                               0x20140000
#define RAMX6_BASE                              0x00340000
#define RAM7_BASE                               0x20160000
#define RAMX7_BASE                              0x00360000
#define RAM8_BASE                               0x20180000
#define RAMX8_BASE                              0x00380000

#define RAM8_SIZE                               0x00020000

#define RAM_TOTAL_SIZE                          (RAM8_BASE + RAM8_SIZE - RAM0_BASE)

#define RAMSENS_BASE                            0x201A0000
#define RAMXSENS_BASE                           0x003A0000

#define SENS_MIN_RAM_SIZE                       0x00010000
#define CODEC_VAD_MAX_BUF_SIZE                  0x00018000

#ifdef CODEC_VAD_CFG_BUF_SIZE
#if (CODEC_VAD_CFG_BUF_SIZE & (0x4000 - 1)) || (CODEC_VAD_CFG_BUF_SIZE > CODEC_VAD_MAX_BUF_SIZE)
#error "Bad sensor CODEC_VAD_CFG_BUF_SIZE"
#endif
#define RAMSENS_SIZE                            (SENS_MIN_RAM_SIZE + (CODEC_VAD_MAX_BUF_SIZE - CODEC_VAD_CFG_BUF_SIZE))
#else
#define RAMSENS_SIZE                            (SENS_MIN_RAM_SIZE + CODEC_VAD_MAX_BUF_SIZE)
#endif

#define SENS_NORM_MAILBOX_BASE                  (RAMSENS_BASE + RAMSENS_SIZE - SENS_NORM_MAILBOX_SIZE)

#ifdef SENS_RAMRET
#define SENS_MAILBOX_BASE                       SENS_RET_MAILBOX_BASE
#define SENS_RET_MAILBOX_SIZE                   SENS_MAILBOX_SIZE
#define SENS_NORM_MAILBOX_SIZE                  0
#else
#define SENS_MAILBOX_BASE                       SENS_NORM_MAILBOX_BASE
#define SENS_RET_MAILBOX_SIZE                   0
#define SENS_NORM_MAILBOX_SIZE                  SENS_MAILBOX_SIZE
#endif
#define SENS_MAILBOX_SIZE                       0x10

#ifdef SENS_RAM_BASE
#if (SENS_RAM_BASE < RAM4_BASE) || (SENS_RAM_BASE >= SENS_NORM_MAILBOX_BASE)
#error "Bad sensor SENS_RAM_BASE"
#endif
#else
#define SENS_RAM_BASE                           RAMSENS_BASE
#endif
#define SENS_RAMX_BASE                          (SENS_RAM_BASE - RAM0_BASE + RAMX0_BASE)

#ifdef SENS_RAM_SIZE
#if (SENS_RAM_BASE + SENS_RAM_SIZE > SENS_NORM_MAILBOX_BASE)
#error "Bad sensor SENS_RAM_SIZE"
#endif
#else
#define SENS_RAM_SIZE                           (SENS_NORM_MAILBOX_BASE - SENS_RAM_BASE)
#endif

#ifdef SENS_RAMRET
#ifdef LARGE_SENS_RAM
#define SENS_RAMRET_BASE                        RAM5_BASE
#endif

#ifdef SENS_RAMRET_BASE
#if (SENS_RAMRET_BASE < RAM4_BASE) || (SENS_RAMRET_BASE > RAMSENS_BASE) || (SENS_RAMRET_BASE > SENS_RAM_BASE)
#error "Bad sensor SENS_RAMRET_BASE"
#endif
#else
#define SENS_RAMRET_BASE                        RAM8_BASE
#endif
#define SENS_RAMXRET_BASE                       (SENS_RAMRET_BASE - RAM0_BASE + RAMX0_BASE)

#ifdef SENS_RAMRET_SIZE
#if ((SENS_RAMRET_BASE + SENS_RAMRET_SIZE + SENS_RET_MAILBOX_SIZE) > (RAM8_BASE + RAM8_SIZE)) || \
        ((SENS_RAMRET_BASE + SENS_RAMRET_SIZE + SENS_RET_MAILBOX_SIZE) > SENS_RAM_BASE)
#error "Bad sensor SENS_RAMRET_SIZE"
#endif
#else
#if (SENS_RAM_BASE <= (RAM8_BASE + RAM8_SIZE))
#define SENS_RAMRET_SIZE                        (SENS_RAM_BASE - SENS_RET_MAILBOX_SIZE - SENS_RAMRET_BASE)
#else
#define SENS_RAMRET_SIZE                        (RAM8_BASE + RAM8_SIZE - SENS_RET_MAILBOX_SIZE - SENS_RAMRET_BASE)
#endif
#endif

#define SENS_RET_MAILBOX_BASE                   (SENS_RAMRET_BASE + SENS_RAMRET_SIZE)

#else /* !SENS_RAMRET */
#define SENS_RAMRET_BASE                        SENS_RAM_BASE
#define SENS_RAMRET_SIZE                        0
#endif /* !SENS_RAMRET */

#ifdef CHIP_SUBSYS_SENS

#ifdef RAM_BASE
#if (RAM_BASE < SENS_RAM_BASE) || (RAM_BASE >= (SENS_RAM_BASE + SENS_RAM_SIZE))
#error "Bad sensor RAM_BASE"
#endif
#else
#define RAM_BASE                                SENS_RAM_BASE
#endif
#define RAMX_BASE                               (RAM_BASE - RAM0_BASE + RAMX0_BASE)

#ifdef RAM_SIZE
#if ((RAM_BASE + RAM_SIZE) > (SENS_RAM_BASE + SENS_RAM_SIZE))
#error "Bad sensor RAM_SIZE"
#endif
#else
#define RAM_SIZE                                SENS_RAM_SIZE
#endif

#ifdef SENS_RAMRET

#ifdef RAMRET_BASE
#if (RAMRET_BASE < SENS_RAMRET_BASE) || (RAMRET_BASE >= (SENS_RAMRET_BASE + SENS_RAMRET_SIZE))
#error "Bad sensor RAMRET_BASE"
#endif
#else
#define RAMRET_BASE                             SENS_RAMRET_BASE
#endif
#define RAMXRET_BASE                            (RAMRET_BASE - RAM0_BASE + RAMX0_BASE)

#ifdef RAMRET_SIZE
#if ((RAMRET_BASE + RAMRET_SIZE) > (SENS_RAMRET_BASE + SENS_RAMRET_SIZE))
#error "Bad sensor RAMRET_SIZE"
#endif
#else
#define RAMRET_SIZE                             SENS_RAMRET_SIZE
#endif
#endif /* SENS_RAMRET */

#else /* !CHIP_SUBSYS_SENS */

#ifdef CHIP_HAS_CP
#ifndef RAMCPX_SIZE
#define RAMCPX_SIZE                             (RAMX1_BASE - RAMX0_BASE)
#endif
#define RAMCPX_BASE                             RAMX0_BASE

#ifndef RAMCP_SIZE
#define RAMCP_SIZE                              (RAM2_BASE - RAM1_BASE)
#endif
#define RAMCP_BASE                              (RAMCPX_BASE + RAMCPX_SIZE - RAMX0_BASE + RAM0_BASE)

#define RAMCP_TOP                               (RAMCP_BASE + RAMCP_SIZE)
#else
#define RAMCP_TOP                               RAM0_BASE
#endif

#ifdef CORE_SLEEP_POWER_DOWN

#ifdef __BT_RAMRUN__
/* RAM0-RAM3: 768K volatile memory
 * RAM4-RAM5: 512K BT controller memory
 * RAM6-RAM8: 384K retention memory
 */
#error "New LDS configuration is needed"
#endif

#ifdef MEM_POOL_BASE
#if (MEM_POOL_BASE < RAMCP_TOP) || (MEM_POOL_BASE >= SENS_RAMRET_BASE)
#error "Bad MEM_POOL_BASE with CORE_SLEEP_POWER_DOWN"
#endif
#else
#define MEM_POOL_BASE                           RAMCP_TOP
#endif

#ifdef MEM_POOL_SIZE
#if (MEM_POOL_BASE + MEM_POOL_SIZE > SENS_RAMRET_BASE)
#error "Bad MEM_POOL_SIZE with CORE_SLEEP_POWER_DOWN"
#endif
#else
#define MEM_POOL_SIZE                           (RAM4_BASE - MEM_POOL_BASE)
#endif

#ifdef SMALL_RET_RAM
#define RAM_BASE                                RAM6_BASE
#endif

#ifdef RAM_BASE
#if (RAM_BASE < MEM_POOL_BASE) || (RAM_BASE >= SENS_RAMRET_BASE)
#error "Bad RAM_BASE with CORE_SLEEP_POWER_DOWN"
#endif
#else
#define RAM_BASE                                (MEM_POOL_BASE + MEM_POOL_SIZE)
#endif
#define RAMX_BASE                               (RAM_BASE - RAM0_BASE + RAMX0_BASE)

#else /* !CORE_SLEEP_POWER_DOWN */

#ifdef RAM_BASE
#if (RAM_BASE < RAMCP_TOP) || (RAM_BASE >= SENS_RAMRET_BASE)
#error "Bad RAM_BASE"
#endif
#else
#define RAM_BASE                                RAMCP_TOP
#endif
#define RAMX_BASE                               (RAM_BASE - RAM0_BASE + RAMX0_BASE)

#ifdef __BT_RAMRUN__
#if (RAM_BASE >= RAM4_BASE)
#error "Bad RAM_BASE with BT ramrun"
#endif
#define RAM_SIZE                                (RAM4_BASE - RAM_BASE)
#endif

#endif /* !CORE_SLEEP_POWER_DOWN */

#ifdef RAM_SIZE
#if (RAM_BASE + RAM_SIZE) > SENS_RAMRET_BASE
#error "Bad RAM_SIZE"
#endif
#else
#define RAM_SIZE                                (SENS_RAMRET_BASE - RAM_BASE)
#endif

#if defined(ARM_CMSE) || defined(ARM_CMNS)
/*MPC: SRAM block size: 0x8000, FLASH block size 0x40000*/
#define RAM_S_SIZE                              0x18000
#define RAM_NSC_SIZE                            0x8000
#define FLASH_S_SIZE                            0x40000

#undef RAM_BASE
#undef RAMX_BASE
#undef RAM_SIZE
#undef RAMCP_BASE
#undef RAMCPX_BASE
#undef RAMCP_SIZE
#undef RAMCPX_SIZE

#if defined(ARM_CMNS)
#define RAMCPX_BASE                             (RAMX0_BASE + RAM_S_SIZE + RAM_NSC_SIZE)
#define RAMCPX_SIZE                             0x20000
#define RAMCP_BASE                              (RAM0_BASE + RAM_S_SIZE + RAM_NSC_SIZE + RAMCPX_SIZE)
#define RAMCP_SIZE                              0x20000
#define RAM_BASE                                (RAMCP_BASE + RAMCP_SIZE)
#define RAMX_BASE                               (RAMCPX_BASE + RAMCPX_SIZE + RAMCP_SIZE)
#define RAM_SIZE                                (SENS_RAMRET_BASE - RAM_BASE)
#else
#define RAM_BASE                                RAM0_BASE
#define RAMX_BASE                               RAMX0_BASE
#define RAM_SIZE                                RAM_S_SIZE
#endif
#endif /* defined(ARM_CMSE) || defined(ARM_CMNS) */
#endif /* !CHIP_SUBSYS_SENS */

#define REAL_FLASH_BASE                         0x2C000000
#define REAL_FLASH_NC_BASE                      0x28000000
#define REAL_FLASHX_BASE                        0x0C000000
#define REAL_FLASHX_NC_BASE                     0x08000000

#define REAL_FLASH1_BASE                        0x34000000
#define REAL_FLASH1_NC_BASE                     0x30000000
#define REAL_FLASH1X_BASE                       0x14000000
#define REAL_FLASH1X_NC_BASE                    0x10000000

#define ICACHE_CTRL_BASE                        0x07FFA000

#define CMU_BASE                                0x40000000
#define MCU_WDT_BASE                            0x40001000
#define MCU_TIMER0_BASE                         0x40002000
#define MCU_TIMER1_BASE                         0x40003000
#define MCU_TIMER2_BASE                         0x40004000
#define I2C0_BASE                               0x40005000
#define I2C1_BASE                               0x40006000
#define SPI_BASE                                0x40007000
#define TRNG_BASE                               0x40008000
#define ISPI_BASE                               0x40009000
#define UART0_BASE                              0x4000B000
#define UART1_BASE                              0x4000C000
#define UART2_BASE                              0x4000D000
#define BTPCM_BASE                              0x4000E000
#define I2S0_BASE                               0x4000F000
#define SPDIF0_BASE                             0x40010000
#define I2S1_BASE                               0x40011000
#define SEC_ENG_BASE                            0x40020000
#define SEC_CTRL_BASE                           0x40030000
#define MPC_FLASH0_BASE                         0x40032000
#define DISPLAY_BASE                            0x40040000
#define PAGE_SPY_BASE                           0x40050000

#define AON_CMU_BASE                            0x40080000
#define AON_GPIO_BASE                           0x40081000
#define AON_WDT_BASE                            0x40082000
#define AON_PWM_BASE                            0x40083000
#define AON_TIMER0_BASE                         0x40084000
#define AON_PSC_BASE                            0x40085000
#define AON_IOMUX_BASE                          0x40086000
#define I2C_SLAVE_BASE                          0x40087000
#define AON_SEC_CTRL_BASE                       0x40088000
#define AON_GPIO1_BASE                          0x40089000
#define AON_GPIO2_BASE                          0x4008A000
#define AON_TIMER1_BASE                         0x4008B000

#define REAL_FLASH1_CTRL_BASE                   0x40100000
#define SDMMC_BASE                              0x40110000
#define AUDMA_BASE                              0x40120000
#define GPDMA_BASE                              0x40130000
#define REAL_FLASH_CTRL_BASE                    0x40140000
#define BTDUMP_BASE                             0x40150000
#define LCDC_BASE                               0x40170000
#define USB_BASE                                0x40180000
#define SEDMA_BASE                              0x401D0000

#define CODEC_BASE                              0x40300000

#define BT_SUBSYS_BASE                          0xA0000000
#define BT_RAM_BASE                             0xC0000000
#define BT_RAM_SIZE                             0x00008000
#define BT_EXCH_MEM_BASE                        0xD0210000
#define BT_EXCH_MEM_SIZE                        0x00008000
#define BT_UART_BASE                            0xD0300000
#define BT_CMU_BASE                             0xD0330000

#define SENS_CMU_BASE                           0x50000000
#define SENS_WDT_BASE                           0x50001000
#define SENS_TIMER0_BASE                        0x50002000
#define SENS_TIMER1_BASE                        0x50003000
#define SENS_TIMER2_BASE                        0x50004000
#define SENS_I2C0_BASE                          0x50005000
#define SENS_I2C1_BASE                          0x50006000
#define SENS_SPI_BASE                           0x50007000
#define SENS_SPILCD_BASE                        0x50008000
#define SENS_ISPI_BASE                          0x50009000
#define SENS_UART0_BASE                         0x5000A000
#define SENS_UART1_BASE                         0x5000B000
#define SENS_BTPCM_BASE                         0x5000C000
#define SENS_I2S0_BASE                          0x5000D000
#define SENS_I2C2_BASE                          0x5000E000
#define SENS_I2C3_BASE                          0x5000F000
#define SENS_CAP_SENSOR_BASE                    0x50010000
#define SENS_PAGE_SPY_BASE                      0x50011000

#define SENS_SENSOR_ENG_BASE                    0x50100000
#define SENS_CODEC_SENSOR_BASE                  0x50110000
#define SENS_SDMA_BASE                          0x50120000
#define SENS_TEP_BASE                           0x50130000
#define SENS_AVS_BASE                           0x50140000

#define SENS_VAD_BASE                           0x50200000

#ifdef CHIP_SUBSYS_SENS

#undef SEC_ENG_BASE
#undef BTDUMP_BASE

#undef I2C0_BASE
#define I2C0_BASE                               SENS_I2C0_BASE
#undef I2C1_BASE
#define I2C1_BASE                               SENS_I2C1_BASE
#undef I2C2_BASE
#define I2C2_BASE                               SENS_I2C2_BASE
#undef I2C3_BASE
#define I2C3_BASE                               SENS_I2C3_BASE
#undef SPI_BASE
#define SPI_BASE                                SENS_SPI_BASE
#undef SPILCD_BASE
#define SPILCD_BASE                             SENS_SPILCD_BASE
#undef ISPI_BASE
#define ISPI_BASE                               SENS_ISPI_BASE
#undef UART0_BASE
#define UART0_BASE                              SENS_UART0_BASE
#undef UART1_BASE
#define UART1_BASE                              SENS_UART1_BASE
#undef BTPCM_BASE
#define BTPCM_BASE                              SENS_BTPCM_BASE
#undef I2S0_BASE
#define I2S0_BASE                               SENS_I2S0_BASE
#undef CAP_SENSOR_BASE
#define CAP_SENSOR_BASE                         SENS_CAP_SENSOR_BASE
#undef PAGE_SPY_BASE
#define PAGE_SPY_BASE                           SENS_PAGE_SPY_BASE
#undef SENSOR_ENG_BASE
#define SENSOR_ENG_BASE                         SENS_SENSOR_ENG_BASE
#undef AUDMA_BASE
#define AUDMA_BASE                              SENS_SDMA_BASE
#undef TEP_BASE
#define TEP_BASE                                SENS_TEP_BASE
#undef AVS_BASE
#define AVS_BASE                                SENS_AVS_BASE

#ifdef CORE_SLEEP_POWER_DOWN
#define TIMER0_BASE                             AON_TIMER1_BASE
#else
#define TIMER0_BASE                             SENS_TIMER0_BASE
#endif
#define TIMER1_BASE                             SENS_TIMER1_BASE

#else /* !CHIP_SUBSYS_SENS */

#ifdef CORE_SLEEP_POWER_DOWN
#define TIMER0_BASE                             AON_TIMER0_BASE
#define TIMER2_BASE                             MCU_TIMER0_BASE
#else
#define TIMER0_BASE                             MCU_TIMER0_BASE
#define TIMER2_BASE                             AON_TIMER0_BASE
#endif
#define TIMER1_BASE                             MCU_TIMER1_BASE

#endif /* !CHIP_SUBSYS_SENS */

#define IOMUX_BASE                              AON_IOMUX_BASE
#define GPIO_BASE                               AON_GPIO_BASE
#define GPIO1_BASE                              AON_GPIO1_BASE
#define GPIO2_BASE                              AON_GPIO2_BASE
#define PWM_BASE                                AON_PWM_BASE
#define WDT_BASE                                AON_WDT_BASE

#ifdef ALT_BOOT_FLASH
#define FLASH_BASE                              REAL_FLASH1_BASE
#define FLASH_NC_BASE                           REAL_FLASH1_NC_BASE
#define FLASHX_BASE                             REAL_FLASH1X_BASE
#define FLASHX_NC_BASE                          REAL_FLASH1X_NC_BASE

#define FLASH1_BASE                             REAL_FLASH_BASE
#define FLASH1_NC_BASE                          REAL_FLASH_NC_BASE
#define FLASH1X_BASE                            REAL_FLASHX_BASE
#define FLASH1X_NC_BASE                         REAL_FLASHX_NC_BASE

#define FLASH_CTRL_BASE                         REAL_FLASH1_CTRL_BASE
#define FLASH1_CTRL_BASE                        REAL_FLASH_CTRL_BASE
#else
#define FLASH_BASE                              REAL_FLASH_BASE
#define FLASH_NC_BASE                           REAL_FLASH_NC_BASE
#define FLASHX_BASE                             REAL_FLASHX_BASE
#define FLASHX_NC_BASE                          REAL_FLASHX_NC_BASE

#define FLASH1_BASE                             REAL_FLASH1_BASE
#define FLASH1_NC_BASE                          REAL_FLASH1_NC_BASE
#define FLASH1X_BASE                            REAL_FLASH1X_BASE
#define FLASH1X_NC_BASE                         REAL_FLASH1X_NC_BASE

#define FLASH_CTRL_BASE                         REAL_FLASH_CTRL_BASE
#define FLASH1_CTRL_BASE                        REAL_FLASH1_CTRL_BASE
#endif

/* For linker scripts */

#define VECTOR_SECTION_SIZE                     320
#define REBOOT_PARAM_SECTION_SIZE               64
#define ROM_BUILD_INFO_SECTION_SIZE             40
#define ROM_EXPORT_FN_SECTION_SIZE              128
#define BT_INTESYS_MEM_OFFSET                   0x00004000

/* For module features */
#define SEC_ENG_HAS_HASH

#define DCDC_CLOCK_CONTROL

#define GPADC_DYNAMIC_DATA_BITS

/* For boot struct version */
#ifndef SECURE_BOOT_VER
#define SECURE_BOOT_VER                         3
#endif

/* For ROM export functions */
#define NO_EXPORT_QSORT
#define NO_EXPORT_BSEARCH
#define NO_EXPORT_VSSCANF

#ifdef __cplusplus
}
#endif

#endif
