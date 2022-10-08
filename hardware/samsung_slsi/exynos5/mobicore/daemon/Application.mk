# =============================================================================
#
# Main build file defining the project modules and their global variables.
#
# =============================================================================

# Don't remove this - mandatory
APP_PROJECT_PATH := $(abspath $(call my-dir))

# The only STL implementation currently working with exceptions
APP_STL := stlport_static

# Don't optimize for better debugging
APP_OPTIM := debug

# Application wide Cflags
GLOBAL_INCLUDES := $(COMP_PATH_MobiCore)/inc \
	$(COMP_PATH_MobiCoreDriverMod)/Public \
	$(COMP_PATH_MobiCore)/inc/TlCm

# Show all warnings
APP_CFLAGS += -Wall

LOG_WRAPPER := $(COMP_PATH_Logwrapper)
