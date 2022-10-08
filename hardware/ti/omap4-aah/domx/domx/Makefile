#  
#  Copyright (C) Texas Instruments - http://www.ti.com/
#  
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#  
#        http://www.apache.org/licenses/LICENSE-2.0
#  
#   Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#  
#  ----------------------------------------------------------------------------
#  Revision History
#
#
#      REF=ORG
#      Original version.
#  ----------------------------------------------------------------------------

#PROJROOT = ..

include $(PROJROOT)/make/start.mk

# Do not change above "include" line(s)

# Arguments to tools, will move to make system once finalized.

CFLAGS         = 
CDEFS          = 
ifeq ($(BUILD),udeb)
CDEFS          += DEBUG 
endif
CDEFS          +=

EXEC_ARGS      = 
ST_LIB_ARGS    = 
SH_LIB_ARGS    = 

# Define this macro if target runs in kernel mode
#__KERNEL__ = 1

# Target name and extension
# static library        (ST_LIB): filename.a
# shared library soname (SH_LIB): filename.so.maj_ver.min_ver
# executable            (EXEC)  : filename.out

TARGETNAME  = libdomx


# TARGETTYPE must be EXEC, ST_LIB or SH_LIB in upper case.

TARGETTYPE  = SH_LIB

# install directory relative to the HOSTTARGET directory
HOSTRELEASE = lib

# install directory relative to the root filesystem
ROOTFSRELEASE = lib

# Folders in which gmake will run before building current target

SUBMODULES  = \

# Filename must not begin with '.', '/' or '\'

SOURCES     = \
omx_rpc/src/omx_rpc.c \
omx_rpc/src/omx_rpc_skel.c \
omx_rpc/src/omx_rpc_stub.c \
omx_proxy_common/src/omx_proxy_common.c \
profiling/profile.c
# The below files are currently empty, so removed them from building
# omx_rpc/src/omx_rpc_config.c \
# omx_rpc/src/omx_rpc_platform.c \

# Search path for include files

INCLUDES    = \
    $(PROJROOT)/omx_core/inc \
    $(PROJROOT)/mm_osal/inc \
    $(PROJROOT)/domx \
    $(PROJROOT)/domx/omx_rpc \
    $(PROJROOT)/domx/omx_rpc/inc \
    ../../memmgrheaders \


# Libraries needed for linking.

ST_LIBS        = 
#mm_osal
SH_LIBS        = omx_core mm_osal 
#rcm ipc
#pthread rt utils procmgr ipc rcm notify
#SH_LIBS        += sysmgr sysmemmgr


# Search path for library (and linker command) files.
# Current folder and target folder are included by default.

LIBINCLUDES = $(PROJROOT)/mm_osal \
              $(PROJROOT)/omx_core


# Do not change below "include" line(s)

include $(PROJROOT)/make/build.mk

