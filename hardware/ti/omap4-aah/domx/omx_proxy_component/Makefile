#
#  Copyright 2001-2008 Texas Instruments - http://www.ti.com/
# 
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
#  ----------------------------------------------------------------------------
#  Revision History
#
#
#  SEP xx,2002    REF=ORG			Swamy MC
#      Original version.
#  ----------------------------------------------------------------------------

#PROJROOT = $(ROOTDIR)/src

include $(PROJROOT)/make/start.mk

# Do not change above "include" line(s)

# Arguments to tools, will move to make system once finalized.

CFLAGS         = 
CDEFS          = DEBUG

EXEC_ARGS      = 
ST_LIB_ARGS    = 
SH_LIB_ARGS    = 

# Define this macro if target runs in kernel mode
#__KERNEL__ = 1

# Target name and extension
# static library        (ST_LIB): filename.a
# shared library soname (SH_LIB): filename.so.maj_ver.min_ver
# executable            (EXEC)  : filename.out

TARGETNAME  =


# TARGETTYPE must be EXEC, ST_LIB or SH_LIB in upper case.
#TARGETTYPE  = ST_LIB
TARGETTYPE  =

# For shared object library, soname is filename.so.maj_ver
SH_SONAME = 

# Folders in which gmake will run before building current target

SUBMODULES  = \
omx_sample \
omx_video_dec \
omx_h264_enc \
omx_mpeg4_enc \
 
#video/omx_proxy_component/test 

# Filename must not begin with '.', '/' or '\'

SOURCES     =

# Search path for include files

INCLUDES    =

# Libraries needed for linking.

ST_LIBS        =
SH_LIBS        =

# Search path for library (and linker command) files.
# Current folder and target folder are included by default.

LIBINCLUDES =


# Do not change below "include" line(s)

include $(PROJROOT)/make/build.mk

