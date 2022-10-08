# Debug Flags:
#     0 - NO DEBUG MESSAGES
#     1 - DEBUG MESSAGES are enabled

OMX_DEBUG ?= 0      # master switch: turn debug on or off

SRC=\
	AacEncTest.c 

ifeq ($(OMX_GTTOOLS),1)
    SRC +=            $(GTTOOLSSRCDIR)/OMX_Common_Utils.c
endif
	
HSRC=$(wildcard ../inc/*)

OBJ=$(SRC:.c=.o)

include $(OMXROOT)/Master.mk

CFLAGS+=-I../inc -I$(OMXINCLUDEDIR) -I$(SYSTEMINCLUDEDIR)  -I$(BRIDGEINCLUDEDIR) -I$(GTTOOLSINCDIR) -Wall -fpic -pipe -DOMAP_2430 -DETEEDN_PATH
ifeq ($(OMX_DEBUG), 1)
    CFLAGS += -O0
else
    CFLAGS += -O2
endif

ifeq ($(OMX_DEBUG), 1)
    CFLAGS += -DOMX_DEBUG=1
endif

ifeq ($(OMX_GTTOOLS),1)
    CFLAGS += -DOMX_GETTIME
endif 

ifneq ($(OMX_RENDERING_DISABLE), 1)
    CFLAGS += -DDSP_RENDERING_ON
endif



OMXCORE_LIB=$(OMXLIBDIR)/libOMX_Core.so
OMXRMPROXY_LIB=$(OMXLIBDIR)/libOMX_ResourceManagerProxy.so
COMPONENT_TEST=AacEncTest_common

COMPONENT_TARGET=$(OMXTESTDIR)/$(COMPONENT_TEST)

all install:: $(COMPONENT_TARGET)

omxcore $(COMPONENT_TARGET): $(OMXTESTDIR) $(COMPONENT_TEST) $(OMXLIBDIR) $(OMXCORE_LIB)  
	@echo "Installing $(COMPONENT_TEST)"
	cp -f $(COMPONENT_TEST) $(COMPONENT_TARGET)

$(COMPONENT_TEST): $(OBJ)
	$(CROSS)gcc $(CFLAGS) -o $(COMPONENT_TEST) $(OBJ) -L$(OMXLIBDIR) -ldl -lOMX_Core -lpthread

$(SRC): $(HSRC)

clean:: 
	rm -f $(COMPONENT_TEST)
	rm -f $(OBJ)

distclean:: clean

clobber:: clean
	rm -f $(OMXTESTDIR)/$(COMPONENT_TEST)
