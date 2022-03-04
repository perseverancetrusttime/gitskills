cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.cpp $(cur_dir)*.S))
obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.S=.o)
obj-y := $(obj-y:.cpp=.o)

ifeq ($(SP_TRIP_MIC),1)
obj-y += sndp/
endif

ifeq ($(WENWEN_TRIP_MIC),1)
obj-y += wenwen/
endif

ifeq ($(ELEVOC_TRIP_MIC),1)
obj-y += elevoc/
endif

obj-y += enc_adapter/ 

