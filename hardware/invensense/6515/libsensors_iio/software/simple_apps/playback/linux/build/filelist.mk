#### filelist.mk for mpu_iio ####

# headers
HEADERS += $(APP_DIR)/iio_utils.h
HEADERS += $(APP_DIR)/and_constructor.h
HEADERS += $(APP_DIR)/datalogger_outputs.h
HEADERS += $(COMMON_DIR)/console_helper.h
HEADERS += $(COMMON_DIR)/mlerrorcode.h
HEADERS += $(COMMON_DIR)/testsupport.h

# sources
SOURCES := $(APP_DIR)/main.c
SOURCES += $(APP_DIR)/and_constructor.c
SOURCES += $(APP_DIR)/datalogger_outputs.c
SOURCES += $(COMMON_DIR)/console_helper.c
SOURCES += $(COMMON_DIR)/mlerrorcode.c

INV_SOURCES += $(SOURCES)

VPATH += $(APP_DIR) $(COMMON_DIR)
