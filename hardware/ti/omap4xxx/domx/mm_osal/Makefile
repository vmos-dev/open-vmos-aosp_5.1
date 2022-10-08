
include $(PROJROOT)/make/start.mk



# Target name and extension
# static library        (ST_LIB): filename.a
# shared library soname (SH_LIB): filename.so.maj_ver.min_ver
# executable            (EXEC)  : filename.out

TARGETNAME  = libmm_osal


# TARGETTYPE must be EXEC, ST_LIB or SH_LIB in upper case.

TARGETTYPE  = SH_LIB

# install directory relative to the HOSTTARGET directory
HOSTRELEASE = lib

# install directory relative to the root filesystem
ROOTFSRELEASE = lib

# Filename must not begin with '.', '/' or '\'

SOURCES     = \
    src/timm_osal.c \
	src/timm_osal_events.c \
	src/timm_osal_memory.c \
	src/timm_osal_mutex.c \
	src/timm_osal_pipes.c \
	src/timm_osal_semaphores.c \
	src/timm_osal_task.c \
	src/timm_osal_trace.c



# Search path for include files

INCLUDES    = \
    inc/ \



ST_LIBS        = 
SH_LIBS        = 
#pthread
#pthread rt utils procmgr ipc rcm notify
#SH_LIBS        += sysmgr sysmemmgr


# Search path for library (and linker command) files.
# Current folder and target folder are included by default.

LIBINCLUDES =


# Do not change below "include" line(s)

include $(PROJROOT)/make/build.mk

