# Debug Flags:
#     0 - NO DEBUG MESSAGES
#     1 - DEBUG MESSAGES are enabled

#OMX_DEBUG ?= 0      # master switch: turn debug on or off

SRC=\
	OMX_VideoEnc_Thread.c \
	OMX_VideoEnc_Utils.c \
	OMX_VideoEncoder.c \
	OMX_VideoEnc_Debug.c 
EXTRA=\

HSRC=$(wildcard ../inc/*)

OBJ=$(SRC:.c=.o)

include $(OMXROOT)/Master.mk

CFLAGS+=-I../inc -I$(OMXINCLUDEDIR) -I$(SYSTEMINCLUDEDIR) -I$(BRIDGEINCLUDEDIR) -Wall -ansi -fpic -pipe #-DOMX_DEBUG


ifeq ($(VIDENC_DEBUG), 1)
    CFLAGS += -DOMX_DEBUG
endif

ifeq ($(VIDENC_LVALUE), 1)
    CFLAGS += -DVIDENC_LVALUE
endif

ifneq ($(OMX_RESOURCEMANAGER_DISABLE), 1)
    CFLAGS += -DRESOURCE_MANAGER_ENABLED
endif

OMXCORE_LIB=$(OMXLIBDIR)/libOMX_Core.so 
COMPONENT_LIB=libOMX.TI.Video.encoder.so

#COMPONENT_LIB=libOMX_VideoEncoder.so

COMPONENT_TARGET=$(OMXLIBDIR)/$(COMPONENT_LIB)

all install:: $(COMPONENT_TARGET)

omxcore $(COMPONENT_TARGET): $(COMPONENT_LIB) $(OMXLIBDIR) $(OMXCORE_LIB)
	@echo "Installing $(COMPONENT_LIB)"
	cp -f $(COMPONENT_LIB) $(COMPONENT_TARGET)

$(COMPONENT_LIB): $(OBJ)
	$(CROSS)gcc $(CFLAGS) -shared -o $(COMPONENT_LIB) $(OBJ) -L$(OMXLIBDIR) -ldl -lpthread -lOMX_ResourceManagerProxy 

$(SRC): $(HSRC)

clean:: 
	rm -f $(COMPONENT_LIB)
	rm -f $(OBJ)

distclean:: clean

clobber:: clean
	rm -f $(OMXLIBDIR)/$(COMPONENT_LIB)
