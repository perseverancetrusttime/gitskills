/**
 * @copyright (c) 2003 - 2020, Goodix Co., Ltd. All rights reserved.
 *
 * @file    gh621x_example_config.h
 *
 * @brief   example code for gh621x
 * 
 * @version  @ref GH621X_EXAMPLE_VER_STRING
 *
 */


#ifndef _GH3011_EXAMPLE_CONFIG_H_
#define _GH3011_EXAMPLE_CONFIG_H_


/* common */

/// version string
#define GH621X_EXAMPLE_VER_STRING                           "example code v0.1.3.0\r\n"

/// platform delay config
#define __PLATFORM_DELAY_FUNC_CONFIG__                      (1)

/// platform reset pin config
#define __PLATFORM_RESET_PIN_FUNC_CONFIG__                  (0)

/// abnormal reset retry max count
#define __ABNORMAL_RESET_RETRY_CNT_MAX__                    (100)

/* dbg log lvl */

/// log debug lvl: <0=> off , <1=> normal info ,  <2=> with inernal info
#define __EXAMPLE_DEBUG_LOG_LVL__                           (2)

/// log support len
#define __EXAMPLE_LOG_DEBUG_SUP_LEN__                       (128)

/* protocol */

/// communicate pkg size max suport. e.g. as ble, should set val < (mtu - 3)
#define __UPOROTOCOL_COMMM_PKG_SIZE_MAX__                   (128)

/// if need mutli pkg, = (__UPOROTOCOL_REPORT_RAWDATA_PKG_MULTI_PKG_NUM__ + 1), e.g. set 4 that mean 5
#define __UPOROTOCOL_REPORT_RAWDATA_PKG_MULTI_PKG_NUM__     (0)

/// patch update cmd buffer
#define __UPOROTOCOL_PATCH_UPDATE_BUFFER_SUP_LEN__          (0)

/// cali param rw cmd buffer
#define __UPOROTOCOL_CALI_PARAM_RW_BUFFER_SUP_LEN__         (256)

/// save load reg list buffer
#define __UPOROTOCOL_SAVE_LOAD_REG_LIST_BUFFER_SUP_LEN__    (0)

/// enable report event resend mechanism
#define __UPOROTOCOL_REPORT_EVENT_RESEND_ENABLE__           (1)

/* patch file */

/// default patch enable
#define __EXAMPLE_PATCH_LOAD_DEFAULT_ENABLE__               (1)

/// default patch include file name
#define __EXAMPLE_PATCH_INCLUDE_FILE_NAME__                 "gh621x_fw_1_1_1.0.0(111).h"

/// default patch include array variable
#define __EXAMPLE_PATCH_INCLUDE_ARRAY_VAR__                 (uchGh621xFwPatchAddr)


/* config dependence macro */


#endif /* _GH3011_EXAMPLE_CONFIG_H_ */

/********END OF FILE********* Copyright (c) 2003 - 2020, Goodix Co., Ltd. ********/
