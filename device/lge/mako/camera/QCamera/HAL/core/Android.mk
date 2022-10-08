
#ifeq ($(call is-board-platform,msm8960),true)
OLD_LOCAL_PATH := $(LOCAL_PATH)
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(strip $(TARGET_USES_ION)),true)
LOCAL_CFLAGS += -DUSE_ION
endif

DLOPEN_LIBMMCAMERA:=0

LOCAL_CFLAGS:= -DDLOPEN_LIBMMCAMERA=$(DLOPEN_LIBMMCAMERA)

LOCAL_CFLAGS += -DCAMERA_ION_HEAP_ID=ION_CP_MM_HEAP_ID # 8660=SMI, Rest=EBI
LOCAL_CFLAGS += -DCAMERA_ZSL_ION_HEAP_ID=ION_CP_MM_HEAP_ID

LOCAL_CFLAGS+= -DHW_ENCODE
LOCAL_CFLAGS+= -DUSE_NEON_CONVERSION

ifeq ($(call is-board-platform,msm8960),true)
        LOCAL_CFLAGS += -DCAMERA_GRALLOC_HEAP_ID=GRALLOC_USAGE_PRIVATE_IOMMU_HEAP
        LOCAL_CFLAGS += -DCAMERA_GRALLOC_FALLBACK_HEAP_ID=GRALLOC_USAGE_PRIVATE_IOMMU_HEAP
        LOCAL_CFLAGS += -DCAMERA_ION_FALLBACK_HEAP_ID=ION_IOMMU_HEAP_ID
        LOCAL_CFLAGS += -DCAMERA_ZSL_ION_FALLBACK_HEAP_ID=ION_IOMMU_HEAP_ID
        LOCAL_CFLAGS += -DCAMERA_GRALLOC_CACHING_ID=0
else ifeq ($(call is-chipset-prefix-in-board-platform,msm8660),true)
        LOCAL_CFLAGS += -DCAMERA_GRALLOC_HEAP_ID=GRALLOC_USAGE_PRIVATE_CAMERA_HEAP
        LOCAL_CFLAGS += -DCAMERA_GRALLOC_FALLBACK_HEAP_ID=GRALLOC_USAGE_PRIVATE_CAMERA_HEAP # Don't Care
        LOCAL_CFLAGS += -DCAMERA_ION_FALLBACK_HEAP_ID=ION_CAMERA_HEAP_ID # EBI
        LOCAL_CFLAGS += -DCAMERA_ZSL_ION_FALLBACK_HEAP_ID=ION_CAMERA_HEAP_ID
        LOCAL_CFLAGS += -DCAMERA_GRALLOC_CACHING_ID=0
else
        LOCAL_CFLAGS += -DCAMERA_GRALLOC_HEAP_ID=GRALLOC_USAGE_PRIVATE_ADSP_HEAP
        LOCAL_CFLAGS += -DCAMERA_GRALLOC_FALLBACK_HEAP_ID=GRALLOC_USAGE_PRIVATE_ADSP_HEAP # Don't Care
        LOCAL_CFLAGS += -DCAMERA_GRALLOC_CACHING_ID=GRALLOC_USAGE_PRIVATE_UNCACHED #uncached
endif

LOCAL_HAL_FILES := \
        src/QCameraHAL.cpp \
        src/QCameraHWI_Parm.cpp \
        src/QCameraHWI.cpp \
        src/QCameraHWI_Preview.cpp \
        src/QCameraHWI_Record.cpp \
        src/QCameraHWI_Still.cpp \
        src/QCameraHWI_Mem.cpp \
        src/QCameraStream.cpp

LOCAL_HAL_WRAPPER_FILES := ../wrapper/QualcommCamera.cpp

LOCAL_C_INCLUDES := \
        $(LOCAL_PATH)/../wrapper \
        $(LOCAL_PATH)/inc \
        $(TARGET_OUT_INTERMEDIATES)/include/mm-camera-interface_badger \

# may need remove this includes 
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/mm-camera 
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/mm-still \
                    $(TARGET_OUT_HEADERS)/mm-still/jpeg 
#end

LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include/media
LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

LOCAL_C_INCLUDES += hardware/qcom/display/msm8960/libgralloc \
        hardware/qcom/display/msm8960/libgenlock \
        hardware/qcom/media/libstagefrighthw

# if debug service layer and up , use stub camera!
LOCAL_C_INCLUDES += \
        frameworks/base/services/camera/libcameraservice

LOCAL_SRC_FILES := \
        $(LOCAL_HAL_WRAPPER_FILES) \
        $(LOCAL_HAL_FILES)

LOCAL_SHARED_LIBRARIES := libutils libui libcamera_client liblog libcutils
LOCAL_SHARED_LIBRARIES += libmmcamera_interface_badger
LOCAL_SHARED_LIBRARIES+= libgenlock libbinder

LOCAL_CFLAGS += -include bionic/libc/include/sys/socket.h

LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MODULE:= camera_badger.$(TARGET_BOARD_PLATFORM)
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)

LOCAL_PATH := $(OLD_LOCAL_PATH)

