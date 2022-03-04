#if defined(__XSPACE_UI__)

#include "stdio.h"
#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_trace.h"
#include "xspace_dev_info.h"

const uint8_t xui_dev_sw_version[4] = {SW_VERSION1, SW_VERSION2, SW_VERSION3, SW_VERSION4};
uint8_t xui_dev_hw_version[2] = {HW_VERSION1, HW_VERSION2};

#if defined(__XSPACE_BT_NAME__)
const char xui_bt_init_name[] = "xspace\0";
#elif defined(__XSPACE_AUTO_TEST__)
const char xui_bt_init_name[] = "xspace_AUTO_TEST\0";
#else
const char xui_bt_init_name[] = "MASTER_BRANCH\0";
#endif

#endif   // __XSPACE_UI__
