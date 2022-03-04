obj-y := voice_manager/ voice_bt/ voice_ble/

ifeq ($(VOC_ENCODE_SCALABLE), 1)
obj-y +=../../../../thirdparty/binaural_lib/
endif

ifeq ($(BLE_V2),1)
obj-y += voice_gatt_v2/
else
obj-y += voice_gatt/
endif

ccflags-y += \
    -Iservices/app_ibrt/inc \
    -Iservices/ibrt_ui/inc
