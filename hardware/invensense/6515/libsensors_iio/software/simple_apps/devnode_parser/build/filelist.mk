#### filelist.mk for inv_devnode_parser ####

# headers
HEADERS += $(APP_DIR)/iio_utils.h

# sources
SOURCES := $(APP_DIR)/read_device_node.c

INV_SOURCES += $(SOURCES)

VPATH += $(APP_DIR) $(COMMON_DIR) $(HAL_DIR)/linux
