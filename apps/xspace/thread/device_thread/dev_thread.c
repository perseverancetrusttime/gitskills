#if defined(__DEV_THREAD__)

#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_trace.h"
#include "app_utils.h"
#include "hal_timer.h"
#include "dev_thread.h"

static osThreadId dev_thread_tid;
static osMailQId dev_mailbox = NULL;
static int dev_thread_default_priority;
static dev_message_block_s debug_dev_thread_mod;
static DEV_MOD_HANDLER_T dev_mod_handler[DEV_MODUAL_NUM];

uint32_t dev_thread_mailbox_count(void);

static void dev_thread(void const *argument);
osThreadDef(dev_thread, osPriorityAboveNormal, 1, (1024 * 8), "dev_thread");
osMailQDef (dev_mailbox, DEV_THREAD_MAILBOX_MAX, dev_message_block_s);

int dev_thread_handle_set(enum dev_dodual_id_e mod_id, DEV_MOD_HANDLER_T handler)
{
    if (mod_id >= DEV_MODUAL_NUM) {
        return -1;
    }

    dev_mod_handler[mod_id] = handler;
    return 0;
}

bool dev_thread_module_is_registered(enum dev_dodual_id_e mod_id)
{
    return dev_mod_handler[mod_id];
}

void dev_thread_priority_set(int priority)
{
    osThreadSetPriority(dev_thread_tid, priority);
}

int dev_thread_priority_get(void)
{
    return (int)osThreadGetPriority(dev_thread_tid);
}

int dev_thread_get_default_priority(void)
{
    return dev_thread_default_priority;
}

void dev_thread_priority_reset(void)
{
    osThreadSetPriority(dev_thread_tid, dev_thread_default_priority);
}

static int dev_thread_mailbox_init(void)
{
    dev_mailbox = osMailCreate(osMailQ(dev_mailbox), NULL);
    if (dev_mailbox == NULL) {
        DEV_THREAD_TRACE(0, "Failed to Create dev_mailbox\n");
        return -1;
    }

    return 0;
}

int dev_thread_mailbox_put(dev_message_block_s *msg_src)
{
    dev_message_block_s *msg_p = NULL;
    uint32_t dev_mb_count = dev_thread_mailbox_count();

    if(dev_mb_count > DEV_THREAD_MAILBOX_MAX / 2)
    {
        app_sysfreq_req(APP_SYSFREQ_USER_DEV_THREAD, APP_SYSFREQ_104M);
        DEV_THREAD_TRACE(1, "dev_thread freq improve to 104M,%d.", dev_mb_count);
    }

    if ((dev_mb_count + 1) > DEV_THREAD_MAILBOX_MAX) {
        DEV_THREAD_TRACE(1, "osMailAlloc error dump,%d.", dev_mb_count);
        DEV_THREAD_TRACE(7, "dead in event:0x%x src:%08x id:%x ptr:%08x para:%08x/%08x/%08x",
                           debug_dev_thread_mod.mod_id,
                           debug_dev_thread_mod.src_thread,
                           debug_dev_thread_mod.msg_body.message_id,
                           (uint32_t)debug_dev_thread_mod.msg_body.message_ptr,
                           debug_dev_thread_mod.msg_body.message_Param0,
                           debug_dev_thread_mod.msg_body.message_Param1,
                           debug_dev_thread_mod.msg_body.message_Param2);

        for(uint8_t i = 0; i < DEV_THREAD_MAILBOX_MAX; i++){
            osEvent evt = osMailGet(dev_mailbox, 0);
            if (evt.status == osEventMail) {
                TRACE_IMM(8, "i:%d mod:%d src:%08x id:%x ptr:%08x para:%08x/%08x/%08x",
                           i,
                           ((dev_message_block_s *)(evt.value.p))->mod_id,
                           ((dev_message_block_s *)(evt.value.p))->src_thread,
                           ((dev_message_block_s *)(evt.value.p))->msg_body.message_id,
                           (uint32_t)((dev_message_block_s *)(evt.value.p))->msg_body.message_ptr,
                           ((dev_message_block_s *)(evt.value.p))->msg_body.message_Param0,
                           ((dev_message_block_s *)(evt.value.p))->msg_body.message_Param1,
                           ((dev_message_block_s *)(evt.value.p))->msg_body.message_Param2);
            }else{
                TRACE_IMM(2,"i:%d %d", i, evt.status);
                break;
            }
        }

        DEV_THREAD_ASSERT(0, "dev_thread mailbox full!!!");
        return -1;
    }

    msg_p = (dev_message_block_s *)osMailAlloc(dev_mailbox, 0);
    if (!msg_p) {
        DEV_THREAD_TRACE(2, " module:%d,id=%d \n", msg_src->mod_id, msg_src->msg_body.message_id);
        return -2;
    }

    DEV_THREAD_ASSERT(msg_p, "dev_thread osMailAlloc error");
    msg_p->src_thread = (uint32_t)osThreadGetId();
    msg_p->dest_thread = (uint32_t)NULL;
    msg_p->mod_id = msg_src->mod_id;
    msg_p->msg_body.message_id = msg_src->msg_body.message_id;
    msg_p->msg_body.message_ptr = msg_src->msg_body.message_ptr;
    msg_p->msg_body.message_Param0 = msg_src->msg_body.message_Param0;
    msg_p->msg_body.message_Param1 = msg_src->msg_body.message_Param1;
    msg_p->msg_body.message_Param2 = msg_src->msg_body.message_Param2;

    osStatus status = osMailPut(dev_mailbox, msg_p);
    if (osOK != status) {
        DEV_THREAD_TRACE(2, "status:0x%x,dev_thread_mailbox_cnt:%d.\n", status, dev_mb_count);
    }

    return (int)status;
}

int dev_thread_mailbox_free(dev_message_block_s *msg_p)
{
    osStatus status = osMailFree(dev_mailbox, msg_p);
    POSSIBLY_UNUSED uint32_t dev_mb_count = dev_thread_mailbox_count();
    
    if (osOK != status) {
        DEV_THREAD_TRACE(2, "status:0x%x,dev_thread_mailbox_cnt=%d.\n", status, dev_mb_count);
    }

    return (int)status;
}

int dev_thread_mailbox_get(dev_message_block_s **msg_p)
{
    osEvent evt = osMailGet(dev_mailbox, osWaitForever);
    if (evt.status == osEventMail) {
        *msg_p = (dev_message_block_s *)evt.value.p;
        return 0;
    }

    DEV_THREAD_TRACE(1, "evt.status:0x%x.\n", evt.status);

    return -1;
}

uint32_t dev_thread_mailbox_count(void)
{
    return osMailGetCount(dev_mailbox);
}


void dev_thread_exit(void)
{
    app_sysfreq_req(APP_SYSFREQ_USER_DEV_THREAD, APP_SYSFREQ_32K);
}

static void dev_thread(void const *argument)
{
	uint32_t dev_mb_count = 0;

    DEV_THREAD_TRACE(0, " enter funtion");

    app_sysfreq_req(APP_SYSFREQ_USER_DEV_THREAD, APP_SYSFREQ_52M);

    dev_thread_ctrl_init();

    while (1) {
        dev_message_block_s *msg_p = NULL;
        dev_mb_count = dev_thread_mailbox_count();


        if (dev_mb_count == 0)
        {
        	//DEV_THREAD_TRACE(0, "empty msg, enter wait...");
            app_sysfreq_req(APP_SYSFREQ_USER_DEV_THREAD, APP_SYSFREQ_32K);
        }
        dev_thread_mailbox_get(&msg_p);
        if(msg_p != NULL)
        {
			dev_mb_count = dev_thread_mailbox_count();
            if(dev_mb_count > DEV_THREAD_MAILBOX_MAX / 2)
            {
                if (hal_sysfreq_get() == HAL_CMU_FREQ_104M)
                {
	                DEV_THREAD_TRACE(1, "dev_thread freq improve to 147M,%d.", dev_mb_count);
	                app_sysfreq_req(APP_SYSFREQ_USER_DEV_THREAD, APP_SYSFREQ_208M);
                }
                else if (hal_sysfreq_get() < HAL_CMU_FREQ_104M)
  				{
  					DEV_THREAD_TRACE(1, "dev_thread freq improve to 104M,%d.", dev_mb_count);
  					app_sysfreq_req(APP_SYSFREQ_USER_DEV_THREAD, APP_SYSFREQ_104M);
  				}
            }
            else
            {
            	//DEV_THREAD_TRACE(1, "dev_thread %d msg wait.", dev_mb_count);
            	if (hal_sysfreq_get() < HAL_CMU_FREQ_52M)
            	{
                	app_sysfreq_req(APP_SYSFREQ_USER_DEV_THREAD, APP_SYSFREQ_52M);
                }
            }

            if (msg_p->mod_id < DEV_THREAD_MAILBOX_MAX) {
                debug_dev_thread_mod = *msg_p;
                DEV_THREAD_ASSERT(dev_mod_handler[msg_p->mod_id] != NULL, "msg_p->mod_id = %d.", msg_p->mod_id);

                if (dev_mod_handler[msg_p->mod_id] != NULL) {
                    dev_mod_handler[msg_p->mod_id](&(msg_p->msg_body));
                }

                debug_dev_thread_mod.mod_id = 0xffffffff;
            }
            dev_thread_mailbox_free(msg_p);
        }
    }
}

int dev_thread_init(void)
{
    if (dev_thread_mailbox_init()) {
        return -1;
    }

    dev_thread_tid = osThreadCreate(osThread(dev_thread), NULL);
    dev_thread_default_priority = dev_thread_priority_get();

    if (dev_thread_tid == NULL) {
        DEV_THREAD_TRACE(0, " Failed to Create dev_thread!!!\n");
        return -1;
    }

    return 0;
}

static int dev_thread_handle_process(dev_message_body_s *msg_body)
{
    switch (msg_body->message_id)
    {
        case DEV_THREAD_CTRL_EVENT_FUNC_CALL:
            if (msg_body->message_ptr) {
                ((dev_thread_ctrl_func_cb)(msg_body->message_ptr))(msg_body->message_Param0, msg_body->message_Param1, msg_body->message_Param2);
            }
            break;

        default:
            break;
    }

    return 0;
}

void dev_thread_ctrl_event_process(dev_thread_ctrl_event_e msg_id, void *ptr, uint32_t param0, uint32_t param1, uint32_t param2)
{
    dev_message_block_s msg;

    if(dev_thread_module_is_registered(DEV_MODUAL_CTRL) == false) {
        return;
    }

    msg.mod_id = DEV_MODUAL_CTRL;
    msg.msg_body.message_id = msg_id;
    msg.msg_body.message_ptr = ptr;
    msg.msg_body.message_Param0 = param0;
    msg.msg_body.message_Param1 = param1;
    msg.msg_body.message_Param2 = param2;
    dev_thread_mailbox_put(&msg);
}

void dev_thread_ctrl_init(void)
{
    dev_thread_handle_set(DEV_MODUAL_CTRL, dev_thread_handle_process);
}
#endif  // __DEV_THREAD__

