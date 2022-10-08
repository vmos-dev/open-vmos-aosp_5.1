# =============================================================================
#
# MC driver device files
#
# =============================================================================

# This is not a separate module.
# Only for inclusion by other modules.
# All paths are relative to APP_PROJECT_PATH

DEVICE_PATH := Daemon/Device
include $(LOCAL_PATH)/$(DEVICE_PATH)/Platforms/Android.mk

# Add new folders with header files here
# Include paths are absolute paths
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(DEVICE_PATH) \
	$(LOCAL_PATH)/$(DEVICE_PATH)/public

# Add new source files here
LOCAL_SRC_FILES += $(DEVICE_PATH)/DeviceIrqHandler.cpp \
	$(DEVICE_PATH)/DeviceScheduler.cpp \
	$(DEVICE_PATH)/MobiCoreDevice.cpp \
	$(DEVICE_PATH)/NotificationQueue.cpp \
	$(DEVICE_PATH)/TrustletSession.cpp \
