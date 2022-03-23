#ifndef __TDK42607P_ADAPTER_H__
#define __TDK42607P_ADAPTER_H__

#if defined(__IMU_TDK42607P__)
#include <stdint.h>

//TODO(Mike):Declaration Register Map Here!
#define TDK32607P_DEV_ADDR 0xFF

#ifdef __cplusplus
extern "C" {
#endif

#define __TDK42607P_DEBUG__
#if defined(__TDK42607P_DEBUG__)
#define TDK42607P_TRACE(num, str, ...) TRACE(num + 1, "[TDK42607P]%s," str, __func__, ##__VA_ARGS__)
#else
#define TDK42607P_TRACE(num, str, ...)
#endif

typedef enum {
    TDK42607P_MSG_ID_INT,
    TDK42607_MSG_ID_MAX,
} tdk42607p_msg_id_e;

#ifdef __cplusplus
}
#endif

#endif /* __CHARGER_TDK42607P__ */
#endif /* __IMU_TDK42607P__ */
