# Debug Flags:
#     0 - NO DEBUG MESSAGES
#     1 - DEBUG MESSAGES are enabled

OMX_DEBUG ?= 0      # master switch: turn debug on or off

# Flag to use Audio Manager/DASF
# 1 - use Audio Manager/DASF, this is Default mode for Linux
# 0 - don't use Audio Manager/DASF

SRC=\
	OMX_AmrEnc_ComponentThread.c \
	OMX_AmrEnc_Utils.c \
	OMX_AmrEncoder.c 

HSRC=$(wildcard ../inc/*)

OBJ=$(SRC:.c=.o)

include $(OMXROOT)/Master.mk

CFLAGS+=-I../inc -I$(OMXINCLUDEDIR) -I$(SYSTEMINCLUDEDIR) -I$(BRIDGEINCLUDEDIR) -I$(SYSTEMINCLUDEDIR) -Wall -fpic -pipe -DOMAP_2430 -DOMX_DEBUG 
ifeq ($(OMX_DEBUG), 1)
    CFLAGS += -O0
else
    CFLAGS += -O2
endif

ifeq ($(OMX_DEBUG), 1)
    CFLAGS += -DOMX_DEBUG=1
endif

ifneq ($(OMX_RENDERING_DISABLE), 1)
    CFLAGS += -DDSP_RENDERING_ON
endif

ifneq ($(OMX_RESOURCEMANAGER_DISABLE), 1)
    CFLAGS += -DRESOURCE_MANAGER_ENABLED
endif

OMXCORE_LIB=$(OMXLIBDIR)/libOMX_Core.so
COMPONENT_LIB=libOMX.TI.AMR.encode.so

COMPONENT_TARGET=$(OMXLIBDIR)/$(COMPONENT_LIB)

all install:: $(COMPONENT_TARGET)

omxcore $(COMPONENT_TARGET): $(COMPONENT_LIB) $(OMXLIBDIR) $(OMXCORE_LIB)
	@echo "Installing $(COMPONENT_LIB)"
	cp -f $(COMPONENT_LIB) $(COMPONENT_TARGET)

$(COMPONENT_LIB): $(OBJ)
	$(CROSS)gcc $(CFLAGS) -shared -o $(COMPONENT_LIB) $(OBJ)  -L$(OMXLIBDIR) -lOMX_ResourceManagerProxy

$(SRC): $(HSRC)

clean:: 
	rm -f $(COMPONENT_LIB)
	rm -f $(OBJ)

distclean:: clean

clobber:: clean
	rm -f $(OMXLIBDIR)/$(COMPONENT_LIB)
