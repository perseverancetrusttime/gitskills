#ifndef __DEV_THREAD_H__
#define __DEV_THREAD_H__

#if defined(__DEV_THREAD__)

#include "stdint.h"
#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_trace.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__DEV_THREAD_DEBUG__)
#define DEV_THREAD_TRACE(num, str, ...)     TRACE(num+1, "[DEV_TH] %s," str, __func__, ##__VA_ARGS__)
#define DEV_THREAD_ASSERT                   ASSERT
#else
#define DEV_THREAD_TRACE(num, str, ...)
#define DEV_THREAD_ASSERT(num, str, ...)
#endif

#define DEV_THREAD_MAILBOX_MAX (60)

enum dev_dodual_id_e
{
    DEV_MODUAL_NONE = 0,

    DEV_MODUAL_IR,
    DEV_MODUAL_ACC,
    DEV_MODUAL_TOUCH,
    DEV_MODUAL_CTRL,
    DEV_MODUAL_CHARGER,
    DEV_MODULE_NTC,
    DEV_MODULE_BATTERY,
    DEV_MODUAL_PRESSURE,
    DEV_MODUAL_IMU,

    DEV_MODUAL_NUM
};

typedef struct
{
    uint32_t message_id;
    void *message_ptr;
    uint32_t message_Param0;
    uint32_t message_Param1;
    uint32_t message_Param2;
} dev_message_body_s;

typedef struct
{
    uint32_t src_thread;
    uint32_t dest_thread;
    uint32_t mod_id;
    dev_message_body_s msg_body;
} dev_message_block_s;

typedef enum
{
    DEV_THREAD_CTRL_EVENT_NONE = 0,
    DEV_THREAD_CTRL_EVENT_FUNC_CALL,
} dev_thread_ctrl_event_e;

typedef int (*DEV_MOD_HANDLER_T)(dev_message_body_s *);
typedef void (*dev_thread_ctrl_func_cb)(uint32_t id, uint32_t data, uint32_t len);

int dev_thread_init(void);
void dev_thread_exit(void);
void dev_thread_ctrl_init(void);
int dev_thread_priority_get(void);
void dev_thread_priority_reset(void);
void dev_thread_priority_set(int priority);
int dev_thread_get_default_priority(void);
int dev_thread_mailbox_get(dev_message_block_s **msg_p);
int dev_thread_mailbox_free(dev_message_block_s *msg_p);
int dev_thread_mailbox_put(dev_message_block_s *msg_src);
bool dev_thread_module_is_registered(enum dev_dodual_id_e mod_id);
int dev_thread_handle_set(enum dev_dodual_id_e mod_id, DEV_MOD_HANDLER_T handler);
void dev_thread_ctrl_event_process(dev_thread_ctrl_event_e msg_id, void *ptr, uint32_t param0, uint32_t param1, uint32_t param2);

#ifdef __cplusplus
}
#endif

#endif  // __DEV_THREAD__


#endif  // __DEV_THREAD_H__

