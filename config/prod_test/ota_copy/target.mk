export SINGLE_WIRE_DOWNLOAD ?= 1

include config/programmer_inflash/target.mk

core-y += tests/programmer_ext/ota_copy/
