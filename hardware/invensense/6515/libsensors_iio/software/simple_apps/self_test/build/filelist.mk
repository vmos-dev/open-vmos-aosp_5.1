#### filelist.mk for console_test ####

# headers
HEADERS += $(HAL_DIR)/include/inv_sysfs_utils.h

# sources
SOURCES := $(APP_DIR)/inv_self_test.c

INV_SOURCES += $(SOURCES)

VPATH += $(APP_DIR) $(COMMON_DIR) $(HAL_DIR)/linux
