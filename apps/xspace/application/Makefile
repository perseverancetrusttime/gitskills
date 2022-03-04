
ifeq ($(XSPACE_FLASH_ACCESS),1)
obj-y += flash_access/
endif

ifneq ($(findstring __FORCE_OTA_BIN_UPDATE__,$(KBUILD_CPPFLAGS)), )
obj-y += ota_bin_update/
endif

ifneq ($(findstring __XSPACE_XBUS_OTA__,$(KBUILD_CPPFLAGS)), )
obj-y += xbus_ota/
endif

obj-y += ota_bin_check/

ifneq ($(findstring __XSPACE_TWS_SWITCH__,$(KBUILD_CPPFLAGS)), )
obj-y += xspace_tws_switch/
endif
