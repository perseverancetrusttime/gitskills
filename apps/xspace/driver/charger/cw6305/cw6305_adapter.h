#ifndef __CW6305_ADAPTER_H__
#define __CW6305_ADAPTER_H__

#if defined(__CHARGER_CW6305__)
#include <stdint.h>
#include "hal_charger.h"

#ifdef __cplusplus
extern "C" {
#endif

#define POLLING_STATUS_REPORT
#define CW6305_DEV_ID_VALUE  (0X14)

#define BIT0 (1<<0)
#define BIT1 (1<<1)
#define BIT3 (1<<3)
#define BIT4 (1<<4)
#define BIT5 (1<<5)
#define BIT6 (1<<6)

#ifdef __cplusplus
}
#endif

extern const hal_charger_ctrl_s cw6305_ctrl;

#endif /* __CHARGER_CW6305__ */
#endif /* __CW6305_ADAPTER_H__ */

