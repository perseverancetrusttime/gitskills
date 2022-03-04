obj-y := audio_dump/ \
         audioflinger/ \
         audio_process/ \
         hw_dsp/ \
         voice_dev/ \
         bt_app/ \
         overlay/ \
         resources/ \
         ../utils/crc32/ \
         ../utils/crc16/ \
         ../utils/heap/ \
         osif/ \
         norflash_api/

ifeq ($(BT_SOURCE),1)
obj-y += app_bt_source/
endif

ifeq ($(SPEECH_BONE_SENSOR),1)
obj-y += bone_sensor/
endif

ifneq ($(filter 1,$(INTERCONNECTION) $(INTERACTION) $(AI_VOICE)),)
obj-y += interconnection/
endif

obj-y += bt_profiles_enhanced/
obj-y += bt_if_enhanced/

ifeq ($(MBED),1)
obj-y += fs/
endif

ifneq ($(filter 1, $(MBED) $(AI_VOICE) $(TOTA_v2)),)
obj-y +=../utils/kfifo/
endif

ifeq ($(VOICE_DATAPATH_ENABLED),1)
obj-y += voicepath/
obj-y += voicepath/$(VOICE_DATAPATH_TYPE)/
endif

ifneq ($(VOICE_DATAPATH_ENABLED)_$(AI_VOICE),0_0)
obj-y += app_ai/
endif

ifeq ($(BLE),1)
ifeq ($(BLE_V2),1)
obj-y += ble_app_v2/ \
         bridge/ \
         ble_stack_v2/ \
         ../utils/retention_ram/ \
         ble_profiles_v2/

ifeq ($(BLE_AUDIO_ENABLED),1)
obj-y += ble_audio_stm/
endif

ifneq ($(IBRT_CORE_V2),1)
obj-y += ../utils/hsm/
endif

else
obj-y += ble_app/ \
         bridge/ \
         ble_stack/ \
         ../utils/retention_ram/ \
         ble_profiles/

endif
endif

ifeq ($(AUTO_TEST),1)
obj-y += auto_test/
endif

obj-y += nv_section/

ifneq ($(FPGA),1)
obj-y += nvrecord/
endif

ifeq ($(VOICE_RECOGNITION),1)
obj-y += voice_recognition/
endif

ifeq ($(APP_ANC_TEST),1)
obj-y += anc_spp_tool/
endif

ifeq ($(TEST_OVER_THE_AIR),1)
obj-y += tota/
endif

ifeq ($(TEST_OVER_THE_AIR_v2),1)
obj-y += tota_v2/
endif

ifneq ($(filter 1, $(BES_OTA) $(AI_OTA)$(GMA_VOICE)),)
ifeq ($(FPGA_IBRT_OTA),1)
obj-y += fpga_ibrt_ota/
else
obj-y += ota/
endif
endif

ifeq ($(TILE_DATAPATH_ENABLED),1)
obj-y += ../thirdparty/tile/
endif

ifeq ($(AI_VOICE),1)
obj-y += ai_voice/
endif

ifeq ($(IBRT),1)
ifeq ($(IBRT_UI_V1),1)
obj-y += ibrt_core/
obj-y += app_ibrt/
obj-y += ibrt_ui/
endif
ifeq ($(IBRT_CORE_V2),1)
obj-y += ../utils/hsm/
obj-y += app_ibrt_v2/
obj-y += ibrt_core_v2/
obj-y += custom_api/
endif
ifeq ($(IBRT_UI_V2),1)
obj-y += ibrt_ui_v2/
endif
endif

ifeq ($(TWS_SYSTEM_ENABLED),1)
obj-y += ibrt_middleware/
endif

ifeq ($(RSA_SHA),1)
obj-y +=../utils/sha256/
endif

ifeq ($(AES256_FOR_BIN),1)
obj-y +=../utils/aes256/
endif

obj-y += communication/

ifeq ($(RPC_SUPPORT),1)
obj-y += rpc/
endif

ifeq ($(THROUGH_PUT),1)
obj-y += through_put/
endif

ifeq ($(APP_MIC_CAPTURE_LOOPBACK) , 1)
obj-y += sealingcheck/
endif

ifeq ($(EPLAYER_ON),1)
obj-y += eplayer/eplayer/
endif

ifeq ($(ETRANSPORT_ON),1)
obj-y += etransport/
endif

ifeq ($(ECOMM_ON),1)
obj-y += ecomm/
endif

ifeq ($(BT_SERVICE_ON),1)
obj-y += bt_service/
ifeq ($(BT_SERVICE_TEST),1)
obj-y += bt_service/tests/
endif
endif
