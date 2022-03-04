#ifndef __SY5500_ADAPTER_H__
#define __SY5500_ADAPTER_H__

#if defined(__CHARGER_SY5500__)
#include <stdint.h>
#include "hal_charger.h"

#define SY5500_PRINTF_REG_VALUE_DEBUG
#define POLLING_STATUS_REPORT

#define SY5500_DEV_ADDR   (0X16) // 7-bit address
#define SY5500_ENABLE		1
#define SY5500_DISABLE		0
//第 1  类  状态寄存器
#define REG_STATE0     			0x10	//R
#define REG_STATE1     			0x11	//R
#define REG_STATE2     			0x20	//R
//第 2  类  版本配置寄存器
#define REG_IO_CASECHK_SET		0x40	//R/W
#define REG_RST_DELAY_SET		0x41	//R/W
#define REG_PWK_SET				0x42	//R/W
#define REG_VINPULLDOWN_SET		0x43	//R/W
#define REG_I2C_CMD				0x44	//W
//第 3  类  带保护的版本配置寄存器
#define REG_VTK_IBATOC_TRX_SET	0x50	//R/WP
#define REG_NTC_VIBATF_OD_SET	0x60	//R/WP
#define REG_ITK_VRECG_ICC_SET	0x61	//R/WP
#define REG_CHGTO_ODVER_PTVER	0x62	//R/WP
//第 4  类  密码位寄存器
/*
当写入 0x39 时类型 3 的寄存器可写，写入其他值时
类型 3 的寄存器只读。任何时候读取寄存器<0x73>
都将返回数据 0。
*/
#define REG_WPEN_72				0x72
#define REG_WPEN_73				0x73

//REG_STATE0

#define ST_CHIP_STANDBY_MODE		0
#define ST_CHIP_SHIP_MODE			1
#define ST_CHIP_TRX_MODE			2
#define ST_CHIP_NOT_CHARGE_MODE		3
#define ST_CHIP_CHARGE_TRICKLE_MODE	4
#define ST_CHIP_CHARGE_CC_MODE		5
#define ST_CHIP_CHARGE_CV_MODE		6
#define ST_CHIP_CHARGEEND_MODE		7

//REG_I2C_CMD
#define ENABLE_CG		0x01
#define DISABLE_CG		0x02
#define TURNON_SHIPMODE	0x03
#define EXIT_TRX_MODE	0x05

#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
}
#endif


#endif /* __CHARGER_SY5500__ */
#endif /* __SY5500_ADAPTER_H__ */

