hardware_modules := gralloc hwcomposer audio nfc nfc-nci local_time \
	power usbaudio audio_remote_submix camera consumerir sensors vibrator \
	tv_input fingerprint
include $(call all-named-subdir-makefiles,$(hardware_modules))
