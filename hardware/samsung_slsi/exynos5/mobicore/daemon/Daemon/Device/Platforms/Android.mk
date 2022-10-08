# =============================================================================
#
# Makefile pointing to the platform specific makefile.
#
# =============================================================================

PLATFORMS_PATH := $(LOCAL_PATH)/Daemon/Device/Platforms

# Always include the Generic code
include $(PLATFORMS_PATH)/Generic/Android.mk

ifneq ($(filter-out Generic,$(PLATFORM)),)
  $(info PLATFORM: $(PLATFORM))
  include $(PLATFORMS_PATH)/$(PLATFORM)/Android.mk
endif
