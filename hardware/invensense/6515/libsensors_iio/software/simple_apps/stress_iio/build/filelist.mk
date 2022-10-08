#### filelist.mk for stress_iio ####

# headers
#HEADERS += $(HAL_DIR)/include/inv_sysfs_utils.h
HEADERS += $(APP_DIR)/iio_utils.h

# sources
SOURCES := $(APP_DIR)/stress_iio.c

INV_SOURCES += $(SOURCES)

VPATH += $(APP_DIR) $(COMMON_DIR) $(HAL_DIR)/linux
