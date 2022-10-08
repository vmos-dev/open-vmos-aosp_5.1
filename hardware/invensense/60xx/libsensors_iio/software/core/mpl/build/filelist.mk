#### filelist.mk for mpl ####

# headers only
HEADERS := $(MPL_DIR)/mpu.h

# headers for sources
HEADERS := $(MPL_DIR)/fast_no_motion.h
HEADERS += $(MPL_DIR)/fusion_9axis.h
HEADERS += $(MPL_DIR)/motion_no_motion.h
HEADERS += $(MPL_DIR)/no_gyro_fusion.h
HEADERS += $(MPL_DIR)/quaternion_supervisor.h
HEADERS += $(MPL_DIR)/gyro_tc.h
HEADERS += $(MPL_DIR)/authenticate.h
HEADERS += $(MPL_DIR)/accel_auto_cal.h
HEADERS += $(MPL_DIR)/accel_auto_cal_protected.h
HEADERS += $(MPL_DIR)/compass_vec_cal.h
HEADERS += $(MPL_DIR)/compass_vec_cal_protected.h
HEADERS += $(MPL_DIR)/mag_disturb.h
HEADERS += $(MPL_DIR)/mag_disturb_protected.h
HEADERS += $(MPL_DIR)/compass_bias_w_gyro.h
HEADERS += $(MPL_DIR)/heading_from_gyro.h
HEADERS += $(MPL_DIR)/compass_fit.h
HEADERS += $(MPL_DIR)/quat_accuracy_monitor.h
#HEADERS += $(MPL_DIR)/auto_calibration.h

# sources
SOURCES := $(MPL_DIR)/fast_no_motion.c
SOURCES += $(MPL_DIR)/fusion_9axis.c
SOURCES += $(MPL_DIR)/motion_no_motion.c
SOURCES += $(MPL_DIR)/no_gyro_fusion.c
SOURCES += $(MPL_DIR)/quaternion_supervisor.c
SOURCES += $(MPL_DIR)/gyro_tc.c
SOURCES += $(MPL_DIR)/authenticate.c
SOURCES += $(MPL_DIR)/accel_auto_cal.c
SOURCES += $(MPL_DIR)/compass_vec_cal.c
SOURCES += $(MPL_DIR)/mag_disturb.c
SOURCES += $(MPL_DIR)/compass_bias_w_gyro.c
SOURCES += $(MPL_DIR)/heading_from_gyro.c
SOURCES += $(MPL_DIR)/compass_fit.c
SOURCES += $(MPL_DIR)/quat_accuracy_monitor.c
#SOURCES += $(MPL_DIR)/auto_calibration.c

INV_SOURCES += $(SOURCES)

VPATH = $(MPL_DIR)

