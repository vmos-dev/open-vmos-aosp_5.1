# =============================================================================
#
# Generic TrustZone device includes
#
# =============================================================================

# This is not a separate module.
# Only for inclusion by other modules.

GENERIC_PATH := Daemon/Device/Platforms/Generic

# Add new source files here
LOCAL_SRC_FILES += $(GENERIC_PATH)/TrustZoneDevice.cpp

# Header files for components including this module
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(GENERIC_PATH)
