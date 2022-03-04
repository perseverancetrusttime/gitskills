cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.cpp $(cur_dir)*.S))
obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.S=.o)
obj-y := $(obj-y:.cpp=.o)

ifeq ($(PROJ_NAME),xspace_ibrt)
ifeq ($(SP_TRIP_MIC),1)
obj-y += lib_sp_trip_mic.a
endif
endif
subdir-ccflags-y += \
	-Iservices/bt_app \
	-Iservices/bt_app/a2dp_codecs/include \
	-Iservices/resources \
	-Iservices/multimedia/audio/codec/sbc/inc \
	-Iservices/multimedia/audio/codec/sbc/src/inc \
	-Iservices/bt_if/inc \
	-Iservices/app_ai/inc \
	-Iservices/multimedia/speech/inc
			  
