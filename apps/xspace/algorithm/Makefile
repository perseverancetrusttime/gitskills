cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.cpp $(cur_dir)*.S))
obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.S=.o)
obj-y := $(obj-y:.cpp=.o)

ccflags-y += -Iservices/bt_if_enhanced/inc

ifeq ($(XSPACE_ENC_SUPPORT),1)
obj-y += enc/

ifeq ($(SP_TRIP_MIC),1)
ccflags-y += -Iapps/xspace/algorithm/enc/sndp/if
endif

ifeq ($(SCO_CP_ACCEL),1)
ccflags-y += -Iapps/audioplayers
endif

ifeq ($(WENWEN_TRIP_MIC),1)
ccflags-y += -Iapps/xspace/algorithm/enc/wenwen/if
endif

endif

obj-y += sha256/ cache/ crc16/ g7721_enc_dec/