LOCAL_PATH:= $(call my-dir)

ifeq ($(TARGET_BOARD_PLATFORM),msm8960)
ifneq ($(USE_CAMERA_STUB),true)
ifeq ($(USE_DEVICE_SPECIFIC_CAMERA),true)

    # When zero we link against libmmcamera; when 1, we dlopen libmmcamera.
    DLOPEN_LIBMMCAMERA:=1
    ifneq ($(BUILD_TINY_ANDROID),true)
      V4L2_BASED_LIBCAM := true

      LOCAL_PATH1:= $(call my-dir)

      include $(CLEAR_VARS)

      LOCAL_CFLAGS:= -DDLOPEN_LIBMMCAMERA=$(DLOPEN_LIBMMCAMERA)

      #define BUILD_UNIFIED_CODE
      ifeq ($(TARGET_BOARD_PLATFORM),msm7627a)
        BUILD_UNIFIED_CODE := true
      else
        BUILD_UNIFIED_CODE := false
      endif

      ifeq ($(TARGET_BOARD_PLATFORM),msm7627a)
        LOCAL_CFLAGS+= -DVFE_7X27A
      endif

      ifeq ($(strip $(TARGET_USES_ION)),true)
        LOCAL_CFLAGS += -DUSE_ION
      endif

      LOCAL_CFLAGS += -DCAMERA_ION_HEAP_ID=ION_IOMMU_HEAP_ID
      LOCAL_CFLAGS += -DCAMERA_ZSL_ION_HEAP_ID=ION_IOMMU_HEAP_ID
      ifeq ($(TARGET_BOARD_PLATFORM),msm8960)
        LOCAL_CFLAGS += -DCAMERA_GRALLOC_HEAP_ID=GRALLOC_USAGE_PRIVATE_IOMMU_HEAP
        LOCAL_CFLAGS += -DCAMERA_GRALLOC_FALLBACK_HEAP_ID=GRALLOC_USAGE_PRIVATE_SYSTEM_HEAP
        LOCAL_CFLAGS += -DCAMERA_ION_FALLBACK_HEAP_ID=ION_IOMMU_HEAP_ID
        LOCAL_CFLAGS += -DCAMERA_ZSL_ION_FALLBACK_HEAP_ID=ION_IOMMU_HEAP_ID
        LOCAL_CFLAGS += -DCAMERA_GRALLOC_CACHING_ID=0
      else ifeq ($(TARGET_BOARD_PLATFORM),msm8660)
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

      ifeq ($(V4L2_BASED_LIBCAM),true)
        ifeq ($(TARGET_BOARD_PLATFORM),msm7627a)
          LOCAL_HAL_FILES := QCameraHAL.cpp QCameraHWI_Parm.cpp\
            QCameraHWI.cpp QCameraHWI_Preview.cpp \
            QCameraHWI_Record_7x27A.cpp QCameraHWI_Still.cpp \
            QCameraHWI_Mem.cpp QCameraHWI_Display.cpp \
            QCameraStream.cpp QualcommCamera2.cpp
        else
          LOCAL_HAL_FILES := QCameraHAL.cpp QCameraHWI_Parm.cpp\
            QCameraHWI.cpp QCameraHWI_Preview.cpp \
            QCameraHWI_Record.cpp QCameraHWI_Still.cpp \
            QCameraHWI_Mem.cpp QCameraHWI_Display.cpp \
            QCameraStream.cpp QualcommCamera2.cpp QCameraParameters.cpp
        endif

      else
        LOCAL_HAL_FILES := QualcommCamera.cpp QualcommCameraHardware.cpp
      endif

      LOCAL_CFLAGS+= -DHW_ENCODE

      # if debug service layer and up , use stub camera!
      LOCAL_C_INCLUDES += \
        frameworks/base/services/camera/libcameraservice #

      LOCAL_SRC_FILES := $(MM_CAM_FILES) $(LOCAL_HAL_FILES)

      ifeq ($(TARGET_BOARD_PLATFORM),msm7627a)
        LOCAL_CFLAGS+= -DNUM_PREVIEW_BUFFERS=6 -D_ANDROID_
      else
        LOCAL_CFLAGS+= -DNUM_PREVIEW_BUFFERS=4 -D_ANDROID_
      endif

      # To Choose neon/C routines for YV12 conversion
      LOCAL_CFLAGS+= -DUSE_NEON_CONVERSION
      # Uncomment below line to enable smooth zoom
      #LOCAL_CFLAGS+= -DCAMERA_SMOOTH_ZOOM

      ifeq ($(V4L2_BASED_LIBCAM),true)
        LOCAL_C_INCLUDES+= hardware/qcom/media/mm-core/inc
        LOCAL_C_INCLUDES+= $(LOCAL_PATH)/mm-camera-interface
      endif

      LOCAL_C_INCLUDES+= hardware/qcom/display/msm8960/libgralloc
      LOCAL_C_INCLUDES+= hardware/qcom/display/msm8960/libgenlock
      LOCAL_C_INCLUDES+= frameworks/native/include/media/hardware
      LOCAL_C_INCLUDES+= hardware/qcom/media/libstagefrighthw


      ifeq ($(V4L2_BASED_LIBCAM),true)
        LOCAL_SHARED_LIBRARIES:= libutils libui libcamera_client libcamera_metadata liblog libcutils
        LOCAL_SHARED_LIBRARIES += libmmcamera_interface2
      else
         LOCAL_SHARED_LIBRARIES:= libutils libui libcamera_client liblog libcamera_metadata libcutils libmmjpeg
      endif

      LOCAL_SHARED_LIBRARIES+= libgenlock libbinder
      ifneq ($(DLOPEN_LIBMMCAMERA),1)
        LOCAL_SHARED_LIBRARIES+= liboemcamera
      else
        LOCAL_SHARED_LIBRARIES+= libdl
      endif

      LOCAL_CFLAGS += -include bionic/libc/include/sys/socket.h

      LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
      LOCAL_MODULE:= camera.$(TARGET_DEVICE)
      LOCAL_MODULE_TAGS := optional
      include $(BUILD_SHARED_LIBRARY)

    endif # BUILD_TINY_ANDROID

ifeq ($(V4L2_BASED_LIBCAM),true)
include $(LOCAL_PATH)/mm-camera-interface/Android.mk
endif

#Enable only to compile new interafece and HAL files.
ifeq ($(V4L2_BASED_LIBCAM),true)
#include $(LOCAL_PATH1)/QCamera/Android.mk
endif

endif # USE_CAMERA_STUB
endif
endif
