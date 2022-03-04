obj-y += pmu/

ifneq ($(findstring __XSPACE_BATTERY_MANAGER__,$(KBUILD_CPPFLAGS)), )

obj-y += battery/
obj-y += charger/

ifneq ($(findstring __NTC_SUPPORT__,$(KBUILD_CPPFLAGS)), )
obj-y += ntc/
endif
endif

ifneq ($(findstring __TOUCH_SUPPORT__,$(KBUILD_CPPFLAGS)), )
obj-y += touch/
endif

ifneq ($(findstring __XSPACE_XBUS_MANAGER__,$(KBUILD_CPPFLAGS)), )
obj-y += xbus/
endif

ifneq ($(findstring __PRESSURE_SUPPORT__,$(KBUILD_CPPFLAGS)), )
obj-y += pressure/
endif