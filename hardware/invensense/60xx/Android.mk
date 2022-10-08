# Can't have both manta and non-manta libsensors.
ifneq ($(filter manta, $(TARGET_DEVICE)),)
# libsensors_iio expects IIO drivers for an MPU6050+AK8963 which are only available on manta.
include $(call all-named-subdir-makefiles,libsensors_iio)
else
# libsensors expects an non-IIO MPU3050.
include $(call all-named-subdir-makefiles,mlsdk libsensors)
endif
