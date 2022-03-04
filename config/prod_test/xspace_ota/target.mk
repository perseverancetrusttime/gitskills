export SINGLE_WIRE_DOWNLOAD ?= 1
export XSPACE_PROJ  ?= 1

include config/programmer_inflash/target.mk


KBUILD_CPPFLAGS += -D__PROJ_XSPACE__
KBUILD_CPPFLAGS += -D__APP_BIN_INTEGRALITY_CHECK__

ifneq ($(findstring __APP_BIN_INTEGRALITY_CHECK__,$(KBUILD_CPPFLAGS)), )
core-y += apps/xspace/algorithm/sha256/ apps/xspace/application/ota_bin_check/
endif

#Compressed image to reduce size of ota area
export COMPRESSED_IMAGE ?= 1
ifeq ($(COMPRESSED_IMAGE),1)
core-y += apps/xspace/algorithm/xz/ utils/heap/
KBUILD_CPPFLAGS += -D__COMPRESSED_IMAGE__
endif


core-y += tests/programmer_ext/ota_copy/
