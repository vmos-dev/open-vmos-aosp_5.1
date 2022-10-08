SHELL=/bin/bash

TARGET       ?= android
PRODUCT      ?= beagleboard
ANDROID_ROOT ?= /Android/trunk/0xdroid/beagle-eclair
KERNEL_ROOT  ?= /Android/trunk/0xdroid/kernel
MLSDK_ROOT   ?= $(CURDIR)

ifeq ($(VERBOSE),1)
	DUMP=1>/dev/stdout
else
	DUMP=1>/dev/null
endif

include common.mk

################################################################################
## targets

INV_ROOT = ../..
LIB_FOLDERS  = $(INV_ROOT)/core/mllite/build/$(TARGET)
ifeq ($(BUILD_MPL),1)
	LIB_FOLDERS += $(INV_ROOT)/core/mpl/build/$(TARGET)
endif
APP_FOLDERS  = $(INV_ROOT)/simple_apps/mpu_iio/build/$(TARGET)
APP_FOLDERS += $(INV_ROOT)/simple_apps/self_test/build/$(TARGET)
APP_FOLDERS += $(INV_ROOT)/simple_apps/gesture_test/build/$(TARGET)

INSTALL_DIR = $(CURDIR)

################################################################################
## macros

define echo_in_colors
	echo -ne "\e[1;34m"$(1)"\e[0m"
endef

################################################################################
## rules

.PHONY : all mllite mpl clean

all:
	for DIR in $(LIB_FOLDERS); do (				\
		cd $$DIR && $(MAKE) -f shared.mk $@ ); 	\
	done
	for DIR in $(APP_FOLDERS); do (				\
		cd $$DIR && $(MAKE) -f shared.mk $@ ); 	\
	done

clean: 
	for DIR in $(LIB_FOLDERS); do (				\
		cd $$DIR && $(MAKE) -f shared.mk $@ ); 	\
	done
	for DIR in $(APP_FOLDERS); do (				\
		cd $$DIR && $(MAKE) -f shared.mk $@ ); 	\
	done

cleanall:
	for DIR in $(LIB_FOLDERS); do (				\
		cd $$DIR && $(MAKE) -f shared.mk $@ ); 	\
	done
	for DIR in $(APP_FOLDERS); do (				\
		cd $$DIR && $(MAKE) -f shared.mk $@ ); 	\
	done

install:
	for DIR in $(LIB_FOLDERS); do (				\
		cd $$DIR && $(MAKE) -f shared.mk $@ ); 	\
	done
	for DIR in $(APP_FOLDERS); do (				\
		cd $$DIR && $(MAKE) -f shared.mk $@ ); 	\
	done

