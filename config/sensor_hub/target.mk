CHIP		?= best1501

CHIP_SUBSYS ?= sensor_hub

DEBUG		?= 1

DEBUG_PORT	?= 2

FAULT_DUMP	?= 1

NO_ISPI ?= 1
export NO_ISPI

NOSTD		?= 0

ifneq ($(NOSTD),1)
RTOS 		?= 1
#KERNEL 		:=  FREERTOS
endif

ifneq ($(RTOS),1)
NOAPP		?= 1
export NOAPP
endif

AUDIO_RESAMPLE ?= 1

export OSC_26M_X4_AUD2BB ?= 1

export ULTRA_LOW_POWER ?= 1

ifeq ($(SENSOR_HUB_VAD_PWR_TEST),1)
export SENSOR_HUB_MINIMA	?= 1
export VOICE_DETECTOR_EN	?= 1
export VAD_USE_SAR_ADC		?= 1
export VAD_ADPT_CLOCK_FREQ	?= 2
export VAD_ADPT_TEST_ENABLE	?= 1
export FULL_WORKLOAD_MODE_ENABLED ?= 1
export THIRDPARTY_GSOUND	?=0
endif
ifeq ($(MINIMA_TEST),1)
export SENSOR_HUB_MINIMA ?= 1
endif

export FULL_WORKLOAD_MODE_ENABLED ?= 0

init-y		:=
core-y		:= tests/sensor_hub/ platform/cmsis/ platform/hal/ utils/hwtimer_list/
ifneq ($(NOAPP),1)
core-y		+= apps/sensorhub/ utils/heap/ utils/cqueue/
endif

export THIRDPARTY_ALEXA	?=0
export THIRDPARTY_GSOUND	?=0
export THIRDPARTY_BES		?=0
export THIRDPARTY_BIXBY	?=0

ifneq ($(filter 1, $(THIRDPARTY_ALEXA) $(THIRDPARTY_GSOUND) $(THIRDPARTY_BES) $(THIRDPARTY_BIXBY)),)
export THIRDPARTY_LIB := 1
export AI_VOICE ?= 1
endif

ifneq ($(THIRDPARTY_LIB),)
ifeq ($(THIRDPARTY_BIXBY),1)
NOSTD	:= 0
SOFT_FLOAT_ABI := 1
core-y		+= thirdparty/senshub_lib/bixby/
endif 	### ifeq ($(THIRDPARTY_BIXBY),1)

ifeq ($(THIRDPARTY_GSOUND),1)
export DSP_LIB ?= 1
NOSTD	:= 0
core-y		+= thirdparty/senshub_lib/gsound/
endif 	### ifeq ($(THIRDPARTY_GSOUND),1)

ifeq ($(THIRDPARTY_ALEXA),1)
export DSP_LIB ?= 1
NOSTD	:= 0
#export AF_STACK_SIZE ?= 5120	##5*1024
core-y		+= thirdparty/senshub_lib/alexa/
endif	### ifeq ($(THIRDPARTY_ALEXA),1)

endif	### ifneq ($(THIRDPARTY_LIB),)


ifeq ($(SENSOR_HUB_MINIMA),1)
LARGE_SENS_RAM ?= 1
core-y		+= platform/drivers/minima/
endif

LDS_FILE	:= sensor_hub.lds

export SENS_TRC_TO_MCU ?= 0

ifeq ($(SENS_TRC_TO_MCU),1)
export TRACE_BUF_SIZE ?= 0
export TRACE_GLOBAL_TAG ?= 1
export DUMP_LOG_ENABLE ?= 1
KBUILD_CPPFLAGS += -DSENS_TRC_TO_MCU
endif

ifeq ($(I2S_TEST),1)
export BEST1501_SENS_I2S_DMA_ENABLE ?= 1
endif

KBUILD_CPPFLAGS += -Iplatform/cmsis/inc -Iplatform/hal

KBUILD_CPPFLAGS += -DconfigTOTAL_HEAP_SIZE=0x4000
export VOICE_DETECTOR_EN ?=1
ifeq ($(VOICE_DETECTOR_EN),1)
LARGE_SENS_RAM ?= 1
VAD_USE_SAR_ADC ?= 0
CODEC_VAD_CFG_BUF_SIZE ?= 0x18000
KBUILD_CPPFLAGS += -DVOICE_DETECTOR_EN \
	-Iservices/audioflinger -Iapps/common
endif

export OS_THREAD_TIMING_STATISTICS_ENABLE ?= 0
ifeq ($(OS_THREAD_TIMING_STATISTICS_ENABLE),1)
KBUILD_CPPFLAGS += -DOS_THREAD_TIMING_STATISTICS_ENABLE
KBUILD_CPPFLAGS += -DOS_THREAD_TIMING_STATISTICS_PEROID_MS=6000
endif

KBUILD_CFLAGS +=

LIB_LDFLAGS +=

CFLAGS_IMAGE +=

LDFLAGS_IMAGE +=

ifeq ($(SENSOR_HUB_MINIMA),1)
CFLAGS_IMAGE += -u _printf_float -u _scanf_float
endif
