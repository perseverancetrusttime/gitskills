obj-y += battery/ bus/ charger/ 

ifneq ($(findstring __NTC_SUPPORT__,$(KBUILD_CPPFLAGS)), )
obj-y += ntc/
endif

ifneq ($(findstring __TOUCH_SUPPORT__,$(KBUILD_CPPFLAGS)), )
obj-y += touch/
endif

ifneq ($(findstring __PRESSURE_SUPPORT__,$(KBUILD_CPPFLAGS)), )
obj-y += pressure/
endif

ifneq ($(findstring __XBUS_UART_SUPPORT__,$(KBUILD_CPPFLAGS)), )
obj-y += xbus/
endif

ifneq ($(findstring __IMU_SUPPORT__,$(KBUILD_CPPFLAGS)), )
obj-y += imu/
endif