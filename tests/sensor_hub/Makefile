cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.S))

ifeq ($(SENSOR_TEST),1)
obj-y += ../../tests/sensor_test/lis3dsh_test.c	 ../../platform/drivers/sensor/st/lis3dsh_reg.c
obj-y += ../../tests/sensor_test/lsm6dsl_test.c	 ../../platform/drivers/sensor/st/lsm6dsl_reg.c
obj-y += ../../tests/sensor_test/lsm6dsox_test.c	../../platform/drivers/sensor/st/lsm6dsox_reg.c
obj-y += ../../tests/sensor_test/bmp280_test.c	  ../../platform/drivers/sensor/bosch/bmp280.c
obj-y += ../../tests/sensor_test/bmx055_test.c	  ../../platform/drivers/sensor/bosch/bma2x2.c
obj-y += ../../tests/sensor_test/sensor_test.c
ifneq ($(VOICE_DETECTOR_EN),1)
obj-y += ../../platform/drivers/ana/
endif
ccflags-y += -DSENSOR_TEST
ccflags-y += -Iplatform/drivers/sensor/st/
ccflags-y += -Iplatform/drivers/sensor/bosch/
ccflags-y += -Itests/sensor_test/
endif #SENSOR_TEST

ifeq ($(MINIMA_TEST),1)
obj-y += ../../tests/minima_test/minima_test.c
ccflags-y += -DMINIMA_TEST
ccflags-y += -Itests/minima_test/
ccflags-y += -Iplatform/drivers/minima/Dmip/
ccflags-y += -Iplatform/drivers/minima/minima_avs/
ccflags-y += -Iplatform/drivers/minima/minima_dvfs/
ccflags-y += -Iplatform/drivers/minima/initClockTop/
ccflags-y += -Iplatform/drivers/minima/minima_driver_adaptation/
endif #MINIMA_TEST

ifeq ($(AI_VOICE),1)
obj-y += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)sensor_hub_ai/*.c))
ccflags-y += -DAI_VOICE -Itests/sensor_hub/sensor_hub_ai
endif	##ifeq ($(AI_VOICE),1)

ifeq ($(NOAPP),1)
obj-y := $(filter-out sensor_hub_core_app.c,$(obj-y))
ccflags-y += -DNOAPP
else
obj-y += \
	../../services/audioflinger/ \
	../../platform/drivers/codec/ \
	../../platform/drivers/ana/ \
	../../apps/common/

CFLAGS_sensor_hub_core_app.o += \
	-Iapps/common \
	-Iapps/sensorhub \
	-Iservices/audioflinger
endif

obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.cpp=.o)
obj-y := $(obj-y:.S=.o)

ccflags-y += \
	-Iplatform/drivers/ana \
	-Iplatform/drivers/sensor_hub \
	-Iutils/hwtimer_list \
	-Iservices/audioflinger \
	-Iapps/sensorhub \

ifneq ($(SENS_TRC_BUF_SIZE),)
CFLAGS_sens_trc_to_mcu.o += -DSENS_TRC_BUF_SIZE=$(SENS_TRC_BUF_SIZE)
endif

ifneq ($(SENS_TRC_ENTRY_CNT),)
CFLAGS_sens_trc_to_mcu.o += -DSENS_TRC_ENTRY_CNT=$(SENS_TRC_ENTRY_CNT)
endif

ifneq ($(SENS_TRC_INTVL_MS),)
CFLAGS_sens_trc_to_mcu.o += -DSENS_TRC_INTVL_MS=$(SENS_TRC_INTVL_MS)
endif

ifeq ($(SENS_TRC_TO_MCU),1)
CFLAGS_sensor_hub_main.o += -DSENS_TRC_TO_MCU
endif

ifeq ($(MCU2SENS_MSG_TEST),1)
ccflags-y += -DMCU2SENS_MSG_TEST
endif

ifeq ($(SLEEP_TEST),1)
CFLAGS_sensor_hub_main.o += -DSLEEP_TEST
ifeq ($(NO_TIMER),1)
CFLAGS_sensor_hub_main.o += -DNO_TIMER
endif
endif

ifeq ($(BT_CONN_SLEEP_TEST),1)
obj-y += ../../tests/hwtest/bt_conn_sleep.o
ccflags-y += -DBT_CONN_SLEEP_TEST -Itests/hwtest
endif

ifeq ($(FULL_WORKLOAD_MODE_ENABLED),1)
ccflags-y += -DFULL_WORKLOAD_MODE_ENABLED
endif

ifeq ($(VAD_CODEC_TEST),1)
obj-y += ../../services/audioflinger/
obj-y += ../../platform/drivers/codec/
obj-y += ../../platform/drivers/ana/
export AF_DEVICE_INT_CODEC ?= 1
export AF_DEVICE_I2S ?= 1
ccflags-y += -DVAD_CODEC_TEST
CFLAGS_vad_codec_test.o += -Iservices/audioflinger
endif

ifeq ($(I2S_TEST),1)
obj-y += ../../tests/hwtest/i2s_test.o
ccflags-y += -DI2S_TEST -Itests/hwtest
endif

ifneq ($(DEBUG_PORT),)
CFLAGS_sensor_hub_main.o += -DDEBUG_PORT=$(DEBUG_PORT)
endif

ifeq ($(VOICE_DETECTOR_EN),1)
export AF_DEVICE_INT_CODEC ?= 1
export AF_DEVICE_I2S ?= 1
ccflags-y += -DVOICE_DETECTOR_EN \
	-Iapps/voice_detector \
	-Iutils/cqueue

ifeq ($(VD_TEST),1)
ccflags-y += -DVD_TEST \
	-Iservices/audio_dump/include \
	-Iservices/multimedia/speech/inc \

obj-y += \
	../../apps/common/ \
	../../services/audioflinger/ \
	../../platform/drivers/codec/ \
	../../platform/drivers/ana/ \
	../../apps/voice_detector/voice_detector.o \
	../../services/audio_dump/ \

else

ccflags-y += -Iservices/audio_dump/include

obj-y += \
	../../apps/voice_detector/ \
	../../services/audio_dump/
endif # VD_TEST

ifeq ($(VAD_ADPT_TEST_ENABLE),1)
ccflags-y += -DVAD_ADPT_TEST_ENABLE
endif

ifneq ($(VAD_ADPT_CLOCK_FREQ),)
ccflags-y +=-DVAD_ADPT_CLOCK_FREQ=$(VAD_ADPT_CLOCK_FREQ)
endif

ifneq ($(THIRDPARTY_LIB),)
ifeq ($(THIRDPARTY_BIXBY),1)
ccflags-y += -DTHIRDPARTY_LIB_SS
ccflags-y += -Ithirdparty/senshub_lib/bixby/inc
endif
ifeq ($(THIRDPARTY_GSOUND),1)
ccflags-y += -DTHIRDPARTY_LIB_GG
ccflags-y += -Ithirdparty/senshub_lib/gsound/inc
endif
ifeq ($(THIRDPARTY_ALEXA),1)
ccflags-y += -DTHIRDPARTY_LIB_ALEXA
ccflags-y += -Ithirdparty/senshub_lib/alexa/inc
endif

endif # THIRDPARTY_LIB

endif # VOICE_DETECTOR_EN

ifeq ($(SENSOR_HUB_MINIMA),1)
ccflags-y += -DSENSOR_HUB_MINIMA            \
	-Iplatform/drivers/minima/Dmip          \
	-Iplatform/drivers/minima/minima_avs    \
	-Iplatform/drivers/minima/minima_dvfs   \
	-Iplatform/drivers/minima/initClockTop  \
	-Iplatform/drivers/minima/minima_driver_adaptation
endif

ifeq ($(FAST_TIMER_COMPENSATE),1)
ccflags-y += -DFAST_TIMER_COMPENSATE
endif
