ifneq ($(findstring __CHARGER_SUPPORT__,$(KBUILD_CPPFLAGS)), )

ifneq ($(findstring __CHARGER_CW6305__,$(KBUILD_CPPFLAGS)), )
obj-y += cw6305/
endif

ifneq ($(findstring __CHARGER_SY5500__,$(KBUILD_CPPFLAGS)), )
obj-y += sy5500/
endif

ifneq ($(findstring __CHARGER_SIMULATION__,$(KBUILD_CPPFLAGS)), )
obj-y += charger_simu/
endif

endif

