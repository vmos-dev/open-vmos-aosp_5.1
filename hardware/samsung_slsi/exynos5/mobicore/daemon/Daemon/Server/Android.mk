# =============================================================================
#
# MC driver server files
#
# =============================================================================

# This is not a separate module.
# Only for inclusion by other modules.

SERVER_PATH := Daemon/Server

# Add new folders with header files here
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(SERVER_PATH)/public

# Add new source files here
LOCAL_SRC_FILES += $(SERVER_PATH)/Server.cpp \
		$(SERVER_PATH)/NetlinkServer.cpp
