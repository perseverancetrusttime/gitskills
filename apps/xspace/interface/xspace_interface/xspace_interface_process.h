#ifndef __XSPACE_INTERFACE_PROCESS_H__
#define __XSPACE_INTERFACE_PROCESS_H__

#ifdef __cplusplus
extern "C" {
#endif

#define XIF_DIFF_TICK_MAX (60000)   //60s

typedef enum {
    XIF_EVENT_NONE = 0,
    XIF_EVENT_FUNC_CALL,
} xif_event_e;

typedef struct {
    uint8_t en;
    uint8_t timer_id;
    uint32_t start_tick;
    uint32_t end_tick;
    uint32_t ptr;
    uint32_t id;
    uint32_t param0;
    uint32_t param1;
} xif_delay_timer_s;

typedef void (*xif_process_func_cb)(uint32_t id, uint32_t param0, uint32_t param1);

void xspace_interface_process_init(void);
int xspace_interface_process_delay_timer_stop(uint32_t ptr);
int xspace_interface_process_delay_timer_start(uint32_t time, uint32_t ptr, uint32_t id, uint32_t param0, uint32_t param1);
void xspace_interface_process_event_process(xif_event_e msg_id, uint32_t ptr, uint32_t id, uint32_t param0, uint32_t param1);

#ifdef __cplusplus
}
#endif

#endif   //__XSPACE_INTERFACE_PROCESS_H__
