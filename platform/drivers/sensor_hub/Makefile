
cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.S))
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))

obj-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

ifeq ($(ISPI_SENS_EXTPMU),1)
ccflags-y += -DISPI_SENS_EXTPMU
endif
ifeq ($(ISPI_SENS_ANA),1)
ccflags-y += -DISPI_SENS_ANA
endif
ifeq ($(ISPI_SENS_RF),1)
ccflags-y += -DISPI_SENS_RF
endif
ifeq ($(ISPI_SENS_ITNPMU),1)
ccflags-y += -DISPI_SENS_ITNPMU
endif
ifeq ($(ISPI_SENS_DPA),1)
ccflags-y += -DISPI_SENS_DPA
endif

ifeq ($(SENS_TRC_TO_MCU),1)
ccflags-y += -DSENS_TRC_TO_MCU
endif

SENSOR_HUB_TGT ?= sensor_hub
$(obj)/sensor_hub_bin.o : out/$(SENSOR_HUB_TGT)/$(SENSOR_HUB_TGT).bin
AFLAGS_sensor_hub_bin.o += -DSENSOR_HUB_BIN_NAME=$(SENSOR_HUB_TGT).bin -Iout/$(SENSOR_HUB_TGT)

