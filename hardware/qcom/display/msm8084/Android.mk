display-hals := libgralloc libcopybit liblight libvirtual
display-hals += libhwcomposer liboverlay libqdutils libexternal libqservice
display-hals += libmemtrack
ifeq ($(call is-vendor-board-platform,QCOM),true)
    include $(call all-named-subdir-makefiles,$(display-hals))
else
ifneq ($(filter msm% apq%,$(TARGET_BOARD_PLATFORM)),)
    include $(call all-named-subdir-makefiles,$(display-hals))
endif
endif
