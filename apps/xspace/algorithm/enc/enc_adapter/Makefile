cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.cpp $(cur_dir)*.S))
obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.S=.o)
obj-y := $(obj-y:.cpp=.o)

ifeq ($(SP_TRIP_MIC),1)
ccflags-y += -Iapps/xspace/algorithm/enc/sndp/sndp_adapter
endif

ifeq ($(WENWEN_TRIP_MIC),1)
ccflags-y += -Iapps/xspace/algorithm/enc/wenwen/wenwen_adapter
endif

ifeq ($(ELEVOC_TRIP_MIC),1)
ccflags-y += -Iapps/xspace/algorithm/enc/elevoc/elevoc_adapter
endif

ifeq ($(SCO_CP_ACCEL),1)
ccflags-y += -Iapps/audioplayers
endif

ccflags-y += -Iservices/bt_if_enhanced/inc 
ccflags-y += -Iservices/multimedia/speech/inc
ccflags-y += -Iinclude/cmsis_dsp
		     



ifeq ($(WENWEN_TRIP_MIC),1)
obj-y += lib_enc_adapter.a
endif
ifeq ($(SP_TRIP_MIC),1)
obj-y += lib_enc_adapter_sp.a
endif
ifeq ($(ELEVOC_TRIP_MIC),1)
obj-y += lib_enc_adapter_elevoc.a
endif