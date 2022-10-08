# =============================================================================
#
# MobiCore Android build components
#
# =============================================================================

LOCAL_PATH := $(call my-dir)

# Client Library
# =============================================================================
include $(CLEAR_VARS)
LOCAL_MODULE := libMcClient
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES += $(GLOBAL_INCLUDES)
LOCAL_SHARED_LIBRARIES += $(GLOBAL_LIBRARIES)

LOCAL_CFLAGS := -fvisibility=hidden -fvisibility-inlines-hidden
LOCAL_CFLAGS += -DLOG_TAG=\"McClient\"

# Add new source files here
LOCAL_SRC_FILES += \
	ClientLib/Device.cpp \
	ClientLib/ClientLib.cpp \
	ClientLib/Session.cpp \
	Common/CMutex.cpp \
	Common/Connection.cpp

LOCAL_EXPORT_C_INCLUDE_DIRS +=\
	$(COMP_PATH_MobiCore)/inc \
	$(LOCAL_PATH)/ClientLib/public

LOCAL_C_INCLUDES += $(LOCAL_PATH)/Common

include $(LOCAL_PATH)/Kernel/Android.mk
# Import logwrapper
include $(LOG_WRAPPER)/Android.mk

include $(BUILD_SHARED_LIBRARY)

# Daemon Application
# =============================================================================
include $(CLEAR_VARS)

LOCAL_MODULE := mcDriverDaemon
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS += -include buildTag.h
LOCAL_CFLAGS += -DLOG_TAG=\"McDaemon\"
LOCAL_C_INCLUDES += $(GLOBAL_INCLUDES)
LOCAL_SHARED_LIBRARIES += $(GLOBAL_LIBRARIES)

include $(LOCAL_PATH)/Daemon/Android.mk

# Common Source files required for building the daemon
LOCAL_SRC_FILES += Common/CMutex.cpp \
	Common/Connection.cpp \
	Common/NetlinkConnection.cpp \
	Common/CSemaphore.cpp \
	Common/CThread.cpp

# Includes required for the Daemon
LOCAL_C_INCLUDES += $(LOCAL_PATH)/ClientLib/public \
	$(LOCAL_PATH)/Common

# Common components
include $(LOCAL_PATH)/Kernel/Android.mk
include $(LOCAL_PATH)/Registry/Android.mk
# Logwrapper
include $(LOG_WRAPPER)/Android.mk

include $(BUILD_EXECUTABLE)
