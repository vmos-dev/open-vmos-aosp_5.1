# BoardConfig.mk
#
# Product-specific compile-time definitions.
#

include device/generic/armv7-a-neon/BoardConfig.mk

ifndef PDK_FUSION_PLATFORM_ZIP
ifeq ($(HOST_OS),linux)
  WITH_DEXPREOPT := true
endif
endif # PDK_FUSION_PLATFORM_ZIP
