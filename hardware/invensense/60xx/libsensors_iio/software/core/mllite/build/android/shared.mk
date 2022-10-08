MLLITE_LIB_NAME = mllite
LIBRARY = $(LIB_PREFIX)$(MLLITE_LIB_NAME).$(SHARED_LIB_EXT)

MK_NAME = $(notdir $(CURDIR)/$(firstword $(MAKEFILE_LIST)))

CROSS ?= $(ANDROID_ROOT)/prebuilt/linux-x86/toolchain/arm-eabi-4.4.0/bin/arm-eabi-
COMP  ?= $(CROSS)gcc
LINK  ?= $(CROSS)gcc 

OBJFOLDER = $(CURDIR)/obj

INV_ROOT = ../../../../..
MLLITE_DIR  = $(INV_ROOT)/software/core/mllite

include $(INV_ROOT)/software/build/android/common.mk

CFLAGS += $(CMDLINE_CFLAGS)
CFLAGS += $(ANDROID_COMPILE)
CFLAGS += -Wall
CFLAGS += -fpic
CFLAGS += -nostdlib
CFLAGS += -DNDEBUG
CFLAGS += -D_REENTRANT
CFLAGS += -DLINUX
CFLAGS += -DANDROID
CFLAGS += -mthumb-interwork
CFLAGS += -fno-exceptions
CFLAGS += -ffunction-sections
CFLAGS += -funwind-tables
CFLAGS += -fstack-protector
CFLAGS += -fno-short-enums
CFLAGS += -fmessage-length=0
CFLAGS += -I$(MLLITE_DIR)
CFLAGS += -I$(INV_ROOT)/simple_apps/common
CFLAGS += $(INV_INCLUDES)
CFLAGS += $(INV_DEFINES)

LLINK  = -lc 
LLINK += -lm 
LLINK += -lutils 
LLINK += -lcutils 
LLINK += -lgcc 
LLINK += -ldl

LFLAGS += $(CMDLINE_LFLAGS)
LFLAGS += -shared 
LFLAGS += -Wl,-soname,$(LIBRARY)
LFLAGS += -Wl,-shared,-Bsymbolic
LFLAGS += $(ANDROID_LINK)
LFLAGS += -Wl,-rpath,$(ANDROID_ROOT)/out/target/product/$(PRODUCT)/obj/lib:$(ANDROID_ROOT)/out/target/product/$(PRODUCT)/system/lib

####################################################################################################
## sources

#INV_SOURCES provided by Makefile.filelist
include ../filelist.mk

INV_OBJS := $(addsuffix .o,$(INV_SOURCES))
INV_OBJS_DST = $(addprefix $(OBJFOLDER)/,$(addsuffix .o, $(notdir $(INV_SOURCES))))

####################################################################################################
## rules

.PHONY: all mllite clean cleanall makefiles

all: mllite

mllite: $(LIBRARY) $(MK_NAME)

$(LIBRARY) : $(OBJFOLDER) $(INV_OBJS_DST) $(MK_NAME)
	@$(call echo_in_colors, "\n<linking $(LIBRARY) with objects $(INV_OBJS_DST)\n")
	$(LINK) $(LFLAGS) -o $(LIBRARY) $(INV_OBJS_DST) $(LLINK) $(INV_LIBS) $(LLINK)

$(OBJFOLDER) :
	@$(call echo_in_colors, "\n<creating object's folder 'obj/'>\n")
	mkdir obj

$(INV_OBJS_DST) : $(OBJFOLDER)/%.c.o : %.c  $(MK_NAME)
	@$(call echo_in_colors, "\n<compile $< to $(OBJFOLDER)/$(notdir $@)>\n")
	$(COMP) $(ANDROID_INCLUDES) $(KERNEL_INCLUDES) $(CFLAGS) -o $@ -c $<

clean : 
	rm -fR $(OBJFOLDER)

cleanall : 
	rm -fR $(LIBRARY) $(OBJFOLDER)

