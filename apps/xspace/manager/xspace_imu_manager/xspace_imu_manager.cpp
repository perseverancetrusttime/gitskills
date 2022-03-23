#if defined(__XSPACE_IMU_MANAGER__)
#include "stdio.h"
#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_pmu.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "hal_imu.h"
#include "dev_thread.h"
#include "xspace_imu_manager.h"
#include "xspace_interface.h"



static int32_t xspace_imu_manager_init_if(void);
static void xspace_imu_manager_start_if(void);
static void xspace_imu_manager_stop_if(void);



static int32_t xspace_imu_manager_init_if(void)
{
    int32_t ret = -1;
    IMU_TRACE(0," - IN ");
    ret = hal_imu_init();
    return ret;
}

static void xspace_imu_manager_start_if(void)
{
    //Note(Mike)
    IMU_TRACE(0, " Inear detect Manager Start !!!");
}

static void xspace_imu_manager_stop_if(void)
{
    //TODO(Mike):

    IMU_TRACE(0, "Wear manage Stop !!!");
}

void xspace_imu_manager_start(void)
{
    dev_thread_ctrl_event_process(DEV_THREAD_CTRL_EVENT_FUNC_CALL, (void *)xspace_imu_manager_start_if, 0, 0, 0);
}

void xspace_imu_manager_stop(void)
{
    dev_thread_ctrl_event_process(DEV_THREAD_CTRL_EVENT_FUNC_CALL, (void *)xspace_imu_manager_stop_if, 0, 0, 0);
}

int32_t xspace_imu_manager_init(void)
{
    IMU_TRACE(0, "xspace_imu_manager_init");
    dev_thread_ctrl_event_process(DEV_THREAD_CTRL_EVENT_FUNC_CALL, (void *)xspace_imu_manager_init_if, 0, 0, 0);

    return 0;
}

#endif  // (__XSPACE_IMU_MANAGER__)

