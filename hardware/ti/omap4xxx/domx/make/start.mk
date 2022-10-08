#
#  Copyright 2001-2009 Texas Instruments - http://www.ti.com/
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
#  dspbridge/mpu_api/make/start.mk
#
#  DSP-BIOS Bridge build rules.
#

# make sure we have a rootdir
ifndef PROJROOT
$(error Error: variable PROJROOT not defined)
endif

# make sure we have a bridgeroot
#ifndef BRIDGEROOT
#$(error Error: variable BRIDGEROOT not defined)
#endif


CMDDEFS =
CMDDEFS_START =


CROSS=arm-none-linux-gnueabi-
PROCFAMILY=OMAP_4430


ifndef PROCFAMILY
$(error Error: PROCFAMILY can not be determined from Kernel .config)
endif

ifndef TARGETDIR
TARGETDIR=$(PROJROOT)/target
endif



#default (first) target should be "all"
#make sure the target directories are created
#all: $(HOSTDIR) $(ROOTFSDIR) $(TARGETDIR)
#all: $(TARGETDIR)

CONFIG_SHELL := /bin/bash

SHELL := $(CONFIG_SHELL)

# Current version of gmake (3.79.1) cannot run windows shell's internal commands
# We need to invoke command interpreter explicitly to do so.
# for winnt it is cmd /c <command>
SHELLCMD:=

ifneq ($(SHELL),$(CONFIG_SHELL))
CHECKSHELL:=SHELLERR
else
CHECKSHELL:=
endif

# Error string to generate fatal error and abort gmake
ERR = $(error Makefile generated fatal error while building target "$@")

CP  :=   cp

MAKEFLAGS = r

QUIET := &> /dev/null

# Should never be :=
RM    = rm $(1) 
MV    = mv $(1) $(2)
RMDIR = rm -r $(1)
MKDIR = mkdir -p $(1)
INSTALL = install

# Current Makefile directory
MAKEDIR := $(CURDIR)

# Implicit rule search not needed for *.d, *.c, *.h
%.d:
%.c:
%.h:

#   Tools
CC	:= $(CROSS)gcc
AR	:= $(CROSS)ar
LD	:= $(CROSS)ld
STRIP	:= $(CROSS)strip


