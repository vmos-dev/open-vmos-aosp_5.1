#Common headers
common_includes := hardware/qcom/display/msm8974/libgralloc
common_includes += hardware/qcom/display/msm8974/liboverlay
common_includes += hardware/qcom/display/msm8974/libcopybit
common_includes += hardware/qcom/display/msm8974/libqdutils
common_includes += hardware/qcom/display/msm8974/libhwcomposer
common_includes += hardware/qcom/display/msm8974/libexternal
common_includes += hardware/qcom/display/msm8974/libqservice

common_header_export_path := qcom/display

#Common libraries external to display HAL
common_libs := liblog libutils libcutils libhardware

#Common C flags
common_flags := -DDEBUG_CALC_FPS -Wno-missing-field-initializers
common_flags += -Werror -Wno-unused-parameter

ifeq ($(ARCH_ARM_HAVE_NEON),true)
    common_flags += -D__ARM_HAVE_NEON
endif

ifneq ($(filter msm8974 msm8x74 msm8610 apq8084,$(TARGET_BOARD_PLATFORM)),)
    common_flags += -DVENUS_COLOR_FORMAT
    common_flags += -DMDSS_TARGET
endif

ifeq ($(TARGET_FORCE_HWC_FOR_VIRTUAL_DISPLAYS), true)
    common_flags += -DFORCE_HWC_FOR_VIRTUAL_DISPLAYS
endif

common_deps  :=
kernel_includes :=

# Executed only on QCOM BSPs
ifeq ($(TARGET_USES_QCOM_BSP),true)
# On jb_mr2- dont enable QCOM Display features
ifneq ($(call is-platform-sdk-version-at-least,18),true)
# This flag is used to compile out any features that depend on framework changes
    common_flags += -DQCOM_BSP
    common_flags += -DANDROID_JELLYBEAN_MR1=1
endif
endif
ifeq ($(call is-vendor-board-platform,QCOM),true)
# This check is to pick the kernel headers from the right location.
# If the macro above is defined, we make the assumption that we have the kernel
# available in the build tree.
# If the macro is not present, the headers are picked from hardware/qcom/msmXXXX
# failing which, they are picked from bionic.
    common_deps += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr
    kernel_includes += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
endif
