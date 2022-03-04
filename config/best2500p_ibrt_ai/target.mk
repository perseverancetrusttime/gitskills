#### enable anc ####
AI_ANC_ENABLE ?= 1

#### alexa ai voice ####
AMA_VOICE := 1

#### xiaodu ai voice ####
DMA_VOICE := 0

#### ali ai voice ####
GMA_VOICE := 0

#### BES ai voice ####
SMART_VOICE := 0
SV_ENCODE_USE_SBC ?=0

#### xiaowei ai voice ####
TENCENT_VOICE := 0
CUSTOMIZE_VOICE := 0

#### Google related feature ####
# the overall google service switch
# currently, google service includes BISTO and GFPS
export GOOGLE_SERVICE_ENABLE := 1

# BISTO is a GVA service on Bluetooth audio device
# BISTO is an isolated service relative to GFPS
export BISTO_ENABLE ?= 1

# macro switch for reduced_guesture
export REDUCED_GUESTURE_ENABLE ?= 0

# GSOUND_HOTWORD is a hotword library running on Bluetooth audio device
# GSOUND_HOTWORD is a subset of BISTO
export GSOUND_HOTWORD_ENABLE ?= 1

# this is a subset choice for gsound hotword
export GSOUND_HOTWORD_EXTERNAL ?= 1

# GFPS is google fastpair service
# GFPS is an isolated service relative to BISTO
export GFPS_ENABLE ?= 1
#### Google related feature ####

SLAVE_ADV_BLE_ENABLED := 0

CTKD_ENABLE ?= 0

export GATT_OVER_BR_EDR ?= 1
ifeq ($(GATT_OVER_BR_EDR),1)
KBUILD_CPPFLAGS += -D__GATT_OVER_BR_EDR__
endif

export MUTLI_POINT_AI ?= 0
ifeq ($(MUTLI_POINT_AI),1)
KBUILD_CPPFLAGS += -D__MUTLI_POINT_AI__
endif

BLE := 1
FAST_XRAM_SECTION_SIZE ?= 0x8000

#### VAD config are both for bisto and alexa ####
# 1 to enable the VAD feature, 0 to disable the VAD feature
export VOICE_DETECTOR_SENS_EN ?= 0
ifeq ($(VOICE_DETECTOR_SENS_EN),1)
FAST_XRAM_SECTION_SIZE := 0x14000
endif

#### use the hot word lib of amazon ####
export ALEXA_WWE := 1
#### a subset choice for the hot word lib of amazon -- lite mode####
export ALEXA_WWE_LITE := 1
ifeq ($(ALEXA_WWE),1)
KBUILD_CPPFLAGS += -D__ALEXA_WWE
export USE_THIRDPARTY := 1
TRACE_BUF_SIZE := 40*1024

ifeq ($(ALEXA_WWE_LITE),1)
KBUILD_CPPFLAGS += -D__ALEXA_WWE_LITE
export THIRDPARTY_LIB := kws/alexa_lite
else
export THIRDPARTY_LIB := kws/alexa
export MCU_HIGH_PERFORMANCE_MODE := 1
endif
endif

#### use the hot word lib of BES ####
export KWS_ALEXA := 0
ifeq ($(KWS_ALEXA),1)
export MCU_HIGH_PERFORMANCE_MODE :=1
export USE_THIRDPARTY := 1
export THIRDPARTY_LIB := kws/bes

FAST_XRAM_SECTION_SIZE := 0x14000
AQE_KWS_NAME := ALEXA
endif

KBUILD_CPPFLAGS += -DFAST_XRAM_SECTION_SIZE=$(FAST_XRAM_SECTION_SIZE)

#RAMCP_SIZE ?= 0x18C00
#RAMCPX_SIZE ?= 0x15000
#KBUILD_CPPFLAGS += -DRAMCP_SIZE=$(RAMCP_SIZE)
#KBUILD_CPPFLAGS += -DRAMCPX_SIZE=$(RAMCPX_SIZE)

ifeq ($(ALEXA_WWE)_$(KWS_ALEXA),0_0)
KBUILD_CPPFLAGS += -DRTOS_IN_RAM
endif

export FLASH_SIZE := 0x800000


ifeq ($(AI_ANC_ENABLE),1)
KBUILD_CPPFLAGS += -DAI_ANC_ENABLE
include config/best2500p_ibrt_anc/target.mk
else
include config/best2500p_ibrt/target.mk
endif

