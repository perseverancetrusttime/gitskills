#ifndef __XSPACE_INEAR_DETECT_MANAGER_H__
#define __XSPACE_INEAR_DETECT_MANAGER_H__

#if defined(__XSPACE_INEAR_DETECT_MANAGER__)

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "stdbool.h"
#include "string.h"

#if defined(__XSPACE_INEAR_DEBUG__)
#define INEAR_TRACE(num, str, ...)  TRACE(num + 2, "[INEAR]%s," str, __func__, ##__VA_ARGS__)
#else
#define INEAR_TRACE(num, str, ...)
#endif

#define INEAR_DETECT_MANAGER_EAR_IN_DEBOUNCE_MS             (10)
#define INEAR_DETECT_MANAGER_EAR_DETECH_DEBOUNCE_MS         (150)

typedef enum
{
    INEAR_DETECT_MG_UNKNOWN = 0,
    INEAR_DETECT_MG_DETACH_EAR,
    INEAR_DETECT_MG_IN_EAR
} inear_detect_manager_status_e;

typedef struct
{
    bool detect_status; // 0: Don't Detect  1：Dectection work-on
    inear_detect_manager_status_e inear_status;
} inear_detect_manager_env_s;

typedef void(*inear_detect_manager_status_change_func)(inear_detect_manager_status_e wear_status);
typedef bool(*inear_detect_manager_ui_need_execute_func)(inear_detect_manager_status_e wear_status);


void xspace_inear_detect_manager_stop(void);
void xspace_inear_detect_manager_start(void);
int32_t xspace_inear_detect_manager_init(void);
bool xspace_inear_detect_manager_is_inear(void);
bool xspace_inear_detect_manager_is_workon(void);
inear_detect_manager_status_e xspace_inear_detect_manager_inear_status(void);
void xspace_inear_detect_manager_register_ui_cb(inear_detect_manager_status_change_func cb);
void xspace_inear_detect_manager_register_ui_need_execute_cb(inear_detect_manager_ui_need_execute_func cb);
void xspace_inear_off_set_self_cali(void);


#ifdef __cplusplus
}
#endif

#endif  // (__XSPACE_INEAR_DETECT_MANAGER__)

#endif  // (__XSPACE_INEAR_DETECT_MANAGER_H__)

