export PROJ_NAME    ?= master_ibrt
export XSPACE_PROJ  ?= 1

ifeq ($(XSPACE_PROJ),1)
    KBUILD_CPPFLAGS += -D__PROJ_XSPACE__
    #Version Control
    export HARDWARE_VERSION ?= 0.0
    export SOFTWARE_VERSION ?= 0.0.0.1
    KBUILD_CPPFLAGS += -D__HARDWARE_VERSION_V1_1_20201214__
    ###################################################################

    # Flash config & ota address config
    export XSPACE_FLASH_ACCESS              ?= 1
    OTA_CODE_OFFSET := 0x18000
    APP_IMAGE_FLASH_SIZE := 0x280000
    NEW_IMAGE_FLASH_OFFSET := 0x298000
    NEW_IMAGE_FLASH_SIZE := 0x150000
    NEW_IMAGE_FLASH_END_ADDR := 0x2C3E8000


    KBUILD_CPPFLAGS += -DFLASH_OTA_SIZE=$(OTA_CODE_OFFSET)
    #KBUILD_CPPFLAGS += -DFLASH_REGION_BASE=$(FLASH_REGION_BASE)
    KBUILD_CPPFLAGS += -DAPP_IMAGE_FLASH_SIZE=$(APP_IMAGE_FLASH_SIZE)
    KBUILD_CPPFLAGS += -DNEW_IMAGE_FLASH_START_ADDR=$(NEW_IMAGE_FLASH_OFFSET)
    KBUILD_CPPFLAGS += -DNEW_IMAGE_FLASH_SIZE=$(NEW_IMAGE_FLASH_SIZE)
    KBUILD_CPPFLAGS += -DNEW_IMAGE_FLASH_END_ADDR=$(NEW_IMAGE_FLASH_END_ADDR)

    KBUILD_CPPFLAGS += -DXSPACE_RESERVED_SECTION_SIZE=0*1024
    KBUILD_CPPFLAGS += -DXSPACE_PARAMETER_SECTION_SIZE=4*1024
    KBUILD_CPPFLAGS += -DXSPACE_OTA_BACKUP_SECTION_SIZE=$(NEW_IMAGE_FLASH_SIZE)
    KBUILD_CPPFLAGS += -DXSPACE_OTA_BACKUP_SECTION_START_ADDR=$(NEW_IMAGE_FLASH_OFFSET)
    KBUILD_CPPFLAGS += -DXSPACE_OTA_BACKUP_SECTION_END_ADDR=$(NEW_IMAGE_FLASH_END_ADDR)


    ## xspace flash param & backup section selected
    ifeq ($(XSPACE_FLASH_ACCESS),1)
        KBUILD_CPPFLAGS += -D__XSPACE_FLASH_ACCESS__
        KBUILD_CPPFLAGS += -D__XSPACE_RAM_ACCESS__
    endif

    ## Ota & Xbus OTA & OTA integrity checks
    #Note：force update ota bin
    export FORCE_OTA_BIN_UPDATE             ?= 0
    export XSPACE_XBUS_OTA                  ?= 1
    export COMPRESSED_IMAGE                 ?= 1

    ifeq ($(FORCE_OTA_BIN_UPDATE),1)
        KBUILD_CPPFLAGS += -D__FORCE_OTA_BIN_UPDATE__
        BOOT_UPDATE_SECTION_SIZE ?= 0x18000
        CPPFLAGS_${LDS_FILE} += -DBOOT_UPDATE_SECTION_SIZE=$(BOOT_UPDATE_SECTION_SIZE)
        KBUILD_CPPFLAGS += -DBOOT_UPDATE_SECTION_SIZE=$(BOOT_UPDATE_SECTION_SIZE)
    endif

    #Note：OTA integrity checks
    KBUILD_CPPFLAGS += -D__APP_BIN_INTEGRALITY_CHECK__

    #Note：Compressed image to reduce size of ota area
    ifeq ($(COMPRESSED_IMAGE),1)
        core-y += apps/xspace/algorithm/xz/
        KBUILD_CPPFLAGS += -D__COMPRESSED_IMAGE__
    endif

    #Note：XBUS ota
    ifeq ($(XSPACE_XBUS_OTA), 1)
        KBUILD_CPPFLAGS += -D__XSPACE_XBUS_OTA__
    endif
    ###########################################################################

    # UI
    KBUILD_CPPFLAGS += -D__XSPACE_UI__

    #Note:default volume select
    export AUDIO_OUTPUT_VOLUME_DEFAULT = 21
    export AUDIO_OUTPUT_VOLUME_DEFAULT_HFP = 15
    export AUDIO_ABS_VOLUME_DISTINGUISH = 0
    export CODEC_DAC_MULTI_VOLUME_TABLE = 1

    #KBUILD_CPPFLAGS += -D__REBOOT_FORCE_PAIRING__
    KBUILD_CPPFLAGS += -D__FORCE_UPDATE_A2DP_VOL__
    KBUILD_CPPFLAGS += -DDEBUG_SLEEP_USER
    KBUILD_CPPFLAGS += -DAPP_TRACE_RX_ENABLE

    ifeq ($(SEARCH_UI_COMPATIBLE_UI_V2),0)
    #KBUILD_CPPFLAGS += -D__XSPACE_BT_NAME__
    endif

    KBUILD_CPPFLAGS += -D__XSPACE_TWS_SWITCH__
    ifneq ($(findstring __XSPACE_TWS_SWITCH__,$(KBUILD_CPPFLAGS)), )
    #KBUILD_CPPFLAGS += -D__XSPACE_RSSI_TWS_SWITCH__
    #KBUILD_CPPFLAGS += -D__XSPACE_SNR_WNR_TWS_SWITCH__
    KBUILD_CPPFLAGS += -D__XSPACE_LOW_BATTERY_TWS_SWITCH__
    endif
    ###########################################################################


    # ALL Manager Macro
    export XSPACE_BATTERY_MANAGER           ?= 1
    export XSPACE_COVER_SWITCH_MANAGER      ?= 1
    export XSPACE_GESTURE_MANAGER           ?= 1
    export XSPACE_INEAR_DETECTION_MANAGER   ?= 1
    export XSPACE_INOUT_BOX_MANAGER         ?= 1
    export XSPACE_XBUS_MANAGER              ?= 1
    export XSPACE_IMU_MANAGER               ?= 1
    export AUDIO_DUMP_FIVE_DEBUG             = 0
    export DEV_THREAD ?= 1
    export XSPACE_AUTO_TEST ?= 0

    ## Battery Manager Control Macro
    ifeq ($(XSPACE_BATTERY_MANAGER),1)
        KBUILD_CPPFLAGS += -D__XSPACE_BATTERY_MANAGER__
        KBUILD_CPPFLAGS += -D__BAT_ADC_INFO_GET__
        #KBUILD_CPPFLAGS += -D__COULOMBMETER_SUPPORT__
        KBUILD_CPPFLAGS += -D__ADC_BAT_CAP_COULOMBMETER__
        KBUILD_CPPFLAGS += -D__CHARGER_SUPPORT__
        KBUILD_CPPFLAGS += -D__NTC_SUPPORT__
    endif
    ## Cover Switch
    ifeq ($(XSPACE_COVER_SWITCH_MANAGER),1)
        KBUILD_CPPFLAGS += -D__XSPACE_COVER_SWITCH_MANAGER__
    endif
    ## Gesture Manager Macro Control
    ifeq ($(XSPACE_GESTURE_MANAGER),1)
        KBUILD_CPPFLAGS += -D__XSPACE_GESTURE_MANAGER__
        #KBUILD_CPPFLAGS += -D__GESTURE_MANAGER_USE_PRESSURE__
        KBUILD_CPPFLAGS += -D__GESTURE_MANAGER_USE_TOUCH__
        #KBUILD_CPPFLAGS += -D__GESTURE_MANAGER_USE_ACC__

        ifneq ($(findstring __GESTURE_MANAGER_USE_PRESSURE__,$(KBUILD_CPPFLAGS)), )
            KBUILD_CPPFLAGS += -D__PRESSURE_SUPPORT__
        endif

        ifneq ($(findstring __GESTURE_MANAGER_USE_TOUCH__,$(KBUILD_CPPFLAGS)), )
            KBUILD_CPPFLAGS += -D__TOUCH_SUPPORT__
        endif

        ifneq ($(findstring __GESTURE_MANAGER_USE_ACC__,$(KBUILD_CPPFLAGS)), )
            KBUILD_CPPFLAGS += -D__ACCELERATOR_SUPPORT__
        endif
    endif
    ## Inear Detection Manager Micro Control
    ifeq ($(XSPACE_INEAR_DETECTION_MANAGER),1)
        KBUILD_CPPFLAGS += -D__XSPACE_INEAR_DETECT_MANAGER__
        KBUILD_CPPFLAGS += -D__SMART_INEAR_MUSIC_REPLAY__
        #KBUILD_CPPFLAGS += -D__SMART_INEAR_MIC_SWITCH__
        #KBUILD_CPPFLAGS += -D__INEAR_DETECTION_USE_IR__
        KBUILD_CPPFLAGS += -D__INEAR_DETECTION_USE_TOUCH__
        #KBUILD_CPPFLAGS += -D__WEAR_OFF_MUSIC_MUTE__

        ifneq ($(findstring __INEAR_DETECTION_USE_TOUCH__,$(KBUILD_CPPFLAGS)), )
            KBUILD_CPPFLAGS += -D__TOUCH_SUPPORT__
        endif

        ifneq ($(findstring __WEAR_MANAGE_USE_IR__,$(KBUILD_CPPFLAGS)), )
            KBUILD_CPPFLAGS += -D__INFRARED_SUPPORT__
        endif
    endif
    ## INOUT BOX Manager Micro Control
    ifeq ($(XSPACE_INOUT_BOX_MANAGER),1)
        KBUILD_CPPFLAGS += -D__XSPACE_INOUT_BOX_MANAGER__
    endif
    ## XBUS Manager Micro Control
    ifeq ($(XSPACE_XBUS_MANAGER),1)
        KBUILD_CPPFLAGS += -D__XSPACE_XBUS_MANAGER__
        KBUILD_CPPFLAGS += -D__XBUS_UART_SUPPORT__
        #KBUILD_CPPFLAGS += -D__XBUS_PMU_SUPPORT__

        ifneq ($(findstring __XBUS_UART_SUPPORT__, $(KBUILD_CPPFLAGS)), )

        endif
    endif
    ## Five mics auido dump & dev_thred & auto test
    ifeq ($(AUDIO_DUMP_FIVE_DEBUG),1)
        KBUILD_CPPFLAGS += -D__AUDIO_DUMP_FIVE_DEBUG__
    endif

    ifeq ($(DEV_THREAD),1)
        KBUILD_CPPFLAGS += -D__DEV_THREAD__
    endif

    ifeq ($(XSPACE_AUTO_TEST),1)
        KBUILD_CPPFLAGS += -D__XSPACE_AUTO_TEST__
        export XSPACE_PRODUCT_TEST ?= 0
        #AUTO_TEST := 1
    else
        export XSPACE_PRODUCT_TEST ?= 1
    endif

    ifeq ($(XSPACE_PRODUCT_TEST),1)
        KBUILD_CPPFLAGS += -D__XSPACE_PRODUCT_TEST__
    endif
    ## IMU- ACCELEROMETER and GYROSOPE feature Manager
    ifeq ($(XSPACE_IMU_MANAGER),1)
        KBUILD_CPPFLAGS += -D__XSPACE_IMU_MANAGER__
        KBUILD_CPPFLAGS += -D__IMU_SUPPORT__
    endif

    ###########################################################################

    # Components & peripherals selected
    ifneq ($(findstring __PRESSURE_SUPPORT__,$(KBUILD_CPPFLAGS)), )
            #KBUILD_CPPFLAGS += -D__PRESSURE_AW8680X__
            #KBUILD_CPPFLAGS += -D__PRESSURE_CSA37F71__
            KBUILD_CPPFLAGS += -D__PRESSURE_NEXTINPUT__
            #KBUILD_CPPFLAGS += -D__USE_HW_I2C__
    endif

    ifneq ($(findstring __TOUCH_SUPPORT__,$(KBUILD_CPPFLAGS)), )
        ifneq ($(findstring __XSPACE_AUTO_TEST__,$(KBUILD_CPPFLAGS)), )
            KBUILD_CPPFLAGS += -D__TOUCH_SIMULATION__
        else
            KBUILD_CPPFLAGS += -D__TOUCH_GH621X__
            #KBUILD_CPPFLAGS += -DPO_QUICKLY_OUT_BOX_WITHOUT_WEAR_FIX
            #KBUILD_CPPFLAGS += -D__TOUCH_GH61X__
            #KBUILD_CPPFLAGS += -D__USE_HW_I2C__
            #KBUILD_CPPFLAGS += -D__USE_SW_I2C__
        endif
    endif

    ifneq ($(findstring __CHARGER_SUPPORT__,$(KBUILD_CPPFLAGS)), )
        ifneq ($(findstring __XSPACE_AUTO_TEST__,$(KBUILD_CPPFLAGS)), )
            KBUILD_CPPFLAGS += -D__CHARGER_SIMULATION__
        else
            #KBUILD_CPPFLAGS += -D__CHARGER_CW6305__
            KBUILD_CPPFLAGS += -D__CHARGER_SY5500__
            KBUILD_CPPFLAGS += -D__USE_SW_I2C__
        endif
    endif
  
    ifneq ($(findstring __IMU_SUPPORT__,$(KBUILD_CPPFLAGS)), )
        KBUILD_CPPFLAGS += -D__IMU_TDK42607P__
        KBUILD_CPPFLAGS += -D__USE_HW_I2C__
        #KBUILD_CPPFLAGS += -D__USE_SW_I2C__
    endif

    ifneq ($(findstring __USE_HW_I2C__,$(KBUILD_CPPFLAGS)), )
        KBUILD_CPPFLAGS += -DI2C1_IOMUX_INDEX=22
    endif

    ifneq ($(findstring __INFRARED_SUPPORT__,$(KBUILD_CPPFLAGS)), )
        #Note(TODO):
    endif

    ifneq ($(findstring __ACCELERATOR_SUPPORT__,$(KBUILD_CPPFLAGS)), )
        #Note(TODO):
    endif

    ifneq ($(findstring __COULOMBMETER_SUPPORT__,$(KBUILD_CPPFLAGS)), )
        #Note(TODO):
    endif
    ###########################################################################

    #Log control 
    export XSPACE_TRACE ?= 1
    #Zhangbin add trace switch(defalut off) start
    KBUILD_CPPFLAGS += -D__XSPACE_TRACE_DISABLE__
    #Zhangbin add trace switch(defalut off) end
    #call audio debug info
    #KBUILD_CPPFLAGS += -D__XSPACE_PLC_DEBUG_PRINT_DATA__
    ifeq ($(XSPACE_TRACE),1)
        # Driver layout trace
        #KBUILD_CPPFLAGS += -D__XBUS_UART_DEBUG__
        #KBUILD_CPPFLAGS += -D__DRV_ADC_DEBUG__

        # Hal layout trace
        #KBUILD_CPPFLAGS += -D__HAL_PMU_DEBUG__

        # UI layout trace
        #KBUILD_CPPFLAGS += -D__XSPACE_UI_DEBUG__

        # Manager layout trace
        #KBUILD_CPPFLAGS += -D__XSPACE_INEAR_DEBUG__
        #KBUILD_CPPFLAGS += -D__XSPACE_GM_DEBUG__
        #KBUILD_CPPFLAGS += -D__XSPACE_CSM_DEBUG__
        #KBUILD_CPPFLAGS += -D__XSPACE_IOBOX_DEBUG__
        #KBUILD_CPPFLAGS += -D__XSPACE_XBM_DEBUG__
        #KBUILD_CPPFLAGS += -D__XSPACE_BM_DEBUG__
        KBUILD_CPPFLAGS += -D__XSPACE_IMU_DEBUG__

        # Interface layout trace
        KBUILD_CPPFLAGS += -D__XSPACE_INTETFACE_TRACE__
        KBUILD_CPPFLAGS += -D__XSPACE_INTETFACE_PROCESS_TRACE__

        KBUILD_CPPFLAGS += -D__XSPACE_PT_DEBUG__

        # Thread trace
        KBUILD_CPPFLAGS += -D__DEV_THREAD_DEBUG__
    endif
    ###########################################################################


    # Customize Feature defined Here!!!!!!!!!!!
    export XSPACE_CUSTOMIZE_ANC             ?= 0
    export XSPACE_ENC_SUPPORT               ?= 0
    export XSPACE_LOW_LATENCY               ?= 0
    export XSPACE_APP_INTERACTION           ?= 0

    ## ANC
    ifeq ($(XSPACE_CUSTOMIZE_ANC),1)
        #Note(Mike.Cheng):add CXX FLAG FOR CUSTOMIZE ANC FEATURE SELECT
        ifneq ($(ANC_APP),1)        
            $(error ANC_APP wasn't opened in "config/${T}/target.mk",plz open it)
        endif
        KBUILD_CPPFLAGS += -D__XSPACE_CUSTOMIZE_ANC__
        export XSPACE_CUSTOMIZE_ASSIST_ANC    ?= 0
        ifeq ($(XSPACE_CUSTOMIZE_ASSIST_ANC),1)
            ifneq ($(ANC_ASSIST_ENABLED),1)  
                $(error ANC_ASSIST_ENABLED wasn't opened in "config/${T}/target.mk",plz open it)
            else
            KBUILD_CPPFLAGS += -D__XSPACE_CUSTOMIZE_ASSIST_ANC__
            endif
        endif
    endif

    ## ENC
    ifeq ($(XSPACE_ENC_SUPPORT),1)
        KBUILD_CPPFLAGS += -D__XSPACE_ENC_SUPPORT__
        # Note:THRIDPARTY ENC algorithm
        export AUDIO_RESAMPLE = 1
        export SP_TRIP_MIC = 0
        export WENWEN_TRIP_MIC = 0
        export ELEVOC_TRIP_MIC = 1
        ifeq ($(ANC_APP),1)
            KBUILD_CPPFLAGS += -DANALOG_ADC_E_GAIN_DB=6
        endif
        ifeq ($(SP_TRIP_MIC),1)
            KBUILD_CPPFLAGS += -DCODEC_MIC_CH0_SADC_VOL=1
            KBUILD_CPPFLAGS += -DCODEC_MIC_CH1_SADC_VOL=12
            KBUILD_CPPFLAGS += -DCODEC_MIC_CH4_SADC_VOL=12
            KBUILD_CPPFLAGS += -D__SP_REF_CHA__
            KBUILD_CPPFLAGS += -D__SP_TRIP_MIC__
            export NN_LIB =1
        endif
        ifeq ($(WENWEN_TRIP_MIC),1)
            KBUILD_CPPFLAGS += -DCODEC_MIC_CH0_SADC_VOL=1
            KBUILD_CPPFLAGS += -DCODEC_MIC_CH1_SADC_VOL=1
            KBUILD_CPPFLAGS += -DCODEC_MIC_CH4_SADC_VOL=1
            KBUILD_CPPFLAGS += -D__WENWEN_TRIP_MIC__
            KBUILD_CPPFLAGS += -D__WENWEN__CODEC_REF__
            #KBUILD_CPPFLAGS += -D__WENWEN_WAV_GEN__
        endif
        ifeq ($(ELEVOC_TRIP_MIC),1)
            KBUILD_CPPFLAGS += -DCODEC_MIC_CH0_SADC_VOL=1
            KBUILD_CPPFLAGS += -DCODEC_MIC_CH1_SADC_VOL=1
            KBUILD_CPPFLAGS += -DCODEC_MIC_CH4_SADC_VOL=1
            KBUILD_CPPFLAGS += -D__ELEVOC_TRIP_MIC__
            KBUILD_CPPFLAGS += -D__ELEVOC_CODEC_REF__
            KBUILD_CPPFLAGS += -DFLASH_UNIQUE_ID
            export FLASH_SECURITY_REGISTER = 1
        endif
    # mimi add for THRIDPARTY ENC algorithm
    endif

    ## Low Latency
    ifeq ($(XSPACE_LOW_LATENCY),1)
        KBUILD_CPPFLAGS +=-D__LOW_LATENCY_SUPPORT__
    endif

    ## APP_INTERACTION (BLE & SPP)
    ifeq ($(XSPACE_APP_INTERACTION),1)
    #TODO

    
    endif

    # Google Fast Pair
    ifeq ($(GFPS_ENABLE),1)
        # Google fast pair
        KBUILD_CPPFLAGS += -DIS_USE_CUSTOM_FP_INFO
    endif

    #read bt chip uid info
    export FLASH_UNIQUE_ID ?= 1

 endif
##end

