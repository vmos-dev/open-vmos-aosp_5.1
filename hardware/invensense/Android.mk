# Can't have both 65xx and 60xx sensors.
ifneq ($(filter hammerhead, $(TARGET_DEVICE)),)
# hammerhead expects 65xx sensors.
include $(call all-named-subdir-makefiles,65xx)
else
ifneq ($(filter guppy dory, $(TARGET_DEVICE)),)
# dory and guppy expect 6515 sensors.
include $(call all-named-subdir-makefiles,6515)
else
ifneq ($(filter manta grouper tuna mako, $(TARGET_DEVICE)),)
# manta, grouper, tuna, and mako expect 60xx sensors.
include $(call all-named-subdir-makefiles,60xx)
endif
endif
endif
