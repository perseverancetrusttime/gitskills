obj-y := userapi/

ifeq ($(THIRDPARTY_LIB),)
THIRDPARTY_LIB := demo
$(info demo)
else ifeq ($(THIRDPARTY_LIB),voicespot)
KBUILD_CPPFLAGS += -D_VOICESPOT_
endif

ifneq ($(THIRDPARTY_LIB),kws/bes)
ifneq ($(VOICE_DETECTOR_SENS_EN),1)
obj-y += $(foreach lib,$(THIRDPARTY_LIB),$(lib)_lib/)
endif
endif
