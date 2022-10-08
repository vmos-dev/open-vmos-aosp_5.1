#### filelist.mk for mllite ####

# headers only
HEADERS := $(MLLITE_DIR)/invensense.h

# headers for sources
HEADERS += $(MLLITE_DIR)/data_builder.h
HEADERS += $(MLLITE_DIR)/hal_outputs.h
HEADERS += $(MLLITE_DIR)/message_layer.h
HEADERS += $(MLLITE_DIR)/ml_math_func.h
HEADERS += $(MLLITE_DIR)/mpl.h
HEADERS += $(MLLITE_DIR)/results_holder.h
HEADERS += $(MLLITE_DIR)/start_manager.h
HEADERS += $(MLLITE_DIR)/storage_manager.h

# headers (linux specific)
HEADERS += $(MLLITE_DIR)/linux/mlos.h
HEADERS += $(MLLITE_DIR)/linux/ml_stored_data.h
HEADERS += $(MLLITE_DIR)/linux/ml_load_dmp.h
HEADERS += $(MLLITE_DIR)/linux/ml_sysfs_helper.h

# sources
SOURCES := $(MLLITE_DIR)/data_builder.c
SOURCES += $(MLLITE_DIR)/hal_outputs.c
SOURCES += $(MLLITE_DIR)/message_layer.c
SOURCES += $(MLLITE_DIR)/ml_math_func.c
SOURCES += $(MLLITE_DIR)/mpl.c
SOURCES += $(MLLITE_DIR)/results_holder.c
SOURCES += $(MLLITE_DIR)/start_manager.c
SOURCES += $(MLLITE_DIR)/storage_manager.c

# sources (linux specific)
SOURCES += $(MLLITE_DIR)/linux/mlos_linux.c
SOURCES += $(MLLITE_DIR)/linux/ml_stored_data.c
SOURCES += $(MLLITE_DIR)/linux/ml_load_dmp.c
SOURCES += $(MLLITE_DIR)/linux/ml_sysfs_helper.c


INV_SOURCES += $(SOURCES)

VPATH += $(MLLITE_DIR) $(MLLITE_DIR)/linux

