# =============================================================================
#
# Generic  TrustZone device includes
#
# =============================================================================

# This is not a separate module.
# All paths are relative to APP_PROJECT_PATH!
KERNEL_PATH := Kernel/Platforms/Generic

# Add new source files here
LOCAL_SRC_FILES += $(KERNEL_PATH)/CMcKMod.cpp

# Header files for components including this module
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(KERNEL_PATH)
