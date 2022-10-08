# =============================================================================
#
# Module: mcDriverDaemon
#
# =============================================================================

# Add new source files here
LOCAL_SRC_FILES += Daemon/MobiCoreDriverDaemon.cpp

# Includes required for the Daemon
LOCAL_C_INCLUDES += $(LOCAL_PATH)/Daemon/public \

# Internal components
include $(LOCAL_PATH)/Daemon/Device/Android.mk
include $(LOCAL_PATH)/Daemon/Server/Android.mk
