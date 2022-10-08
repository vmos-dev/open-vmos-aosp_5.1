# Copyright (c) 2011 Intel Corporation. All Rights Reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sub license, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice (including the
# next paragraph) shall be included in all copies or substantial portions
# of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
# IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
# ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
# Makefile to build MSVDX firmware

CFLAGS = -DFRAME_SWITCHING_VARIANT=1 -DSLICE_SWITCHING_VARIANT=1
firmware_DATA = topazhp_fw.bin
firmwaredir = /lib/firmware
topazhp_fw_bin_DEPENDENCIES = topazhp_bin

noinst_PROGRAMS = topazhp_bin

topazhp_bin_SOURCES = topazhp_bin.c H263Firmware_bin.c H263FirmwareCBR_bin.c H263FirmwareVBR_bin.c H263MasterFirmware_bin.c H263MasterFirmwareCBR_bin.c H263MasterFirmwareERC_bin.c \
		      H263MasterFirmwareLLRC_bin.c H263MasterFirmwareVBR_bin.c H263SlaveFirmware_bin.c H263SlaveFirmwareCBR_bin.c H263SlaveFirmwareVBR_bin.c H264Firmware_bin.c \
		      H264FirmwareCBR_bin.c H264FirmwareVBR_bin.c H264MasterFirmware_bin.c H264MasterFirmwareCBR_bin.c H264MasterFirmwareERC_bin.c H264MasterFirmwareLLRC_bin.c \
		      H264MasterFirmwareVBR_bin.c H264MasterFirmwareVCM_bin.c H264MVCMasterFirmware_bin.c H264MVCMasterFirmwareCBR_bin.c H264MVCMasterFirmwareERC_bin.c H264MasterFirmwareALL_bin.c\
		      H264MVCMasterFirmwareLLRC_bin.c H264MVCMasterFirmwareVBR_bin.c H264SlaveFirmware_bin.c H264SlaveFirmwareCBR_bin.c H264SlaveFirmwareVBR_bin.c JPEGFirmware_bin.c \
		      JPEGMasterFirmware_bin.c JPEGSlaveFirmware_bin.c MPG2MasterFirmware_bin.c MPG2MasterFirmwareCBR_bin.c MPG2MasterFirmwareERC_bin.c MPG2MasterFirmwareLLRC_bin.c \
		      MPG2MasterFirmwareVBR_bin.c MPG4Firmware_bin.c MPG4FirmwareCBR_bin.c MPG4FirmwareVBR_bin.c MPG4MasterFirmware_bin.c MPG4MasterFirmwareCBR_bin.c MPG4MasterFirmwareERC_bin.c \
		      MPG4MasterFirmwareLLRC_bin.c MPG4MasterFirmwareVBR_bin.c MPG4SlaveFirmware_bin.c MPG4SlaveFirmwareCBR_bin.c MPG4SlaveFirmwareVBR_bin.c thread0_bin.c

topazhp_fw.bin: topazhp_bin
	./topazhp_bin

clean-generic:
	rm -f ./topazhp_fw.bin ./topazhp_bin
