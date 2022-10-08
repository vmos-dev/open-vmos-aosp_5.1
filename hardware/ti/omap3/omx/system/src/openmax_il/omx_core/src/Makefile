# Debug Flags:
#     0 - NO DEBUG MESSAGES
#     1 - DEBUG MESSAGES are enabled

OMX_DEBUG ?= 0      # master switch: turn debug on or off

SRC=\
	OMX_Core.c

HSRC=$(wildcard ../inc/*)

OBJ=$(SRC:.c=.o)

include $(OMXROOT)/Master.mk

CFLAGS+=-I../inc -Wall -ansi -fpic -pipe -DSTATIC_TABLE
ifeq ($(OMX_DEBUG), 1)
    CFLAGS += -O0
else
    CFLAGS += -O2
endif

COMPONENT_LIB=libOMX_Core.so

COMPONENT_TARGET=$(OMXLIBDIR)/$(COMPONENT_LIB)

all install:: $(COMPONENT_TARGET)

omxcore $(COMPONENT_TARGET): $(COMPONENT_LIB) $(OMXLIBDIR) 
	@echo "Installing $(COMPONENT_LIB)"
	cp -f $(COMPONENT_LIB) $(COMPONENT_TARGET)

$(COMPONENT_LIB): $(OBJ)
	$(CROSS)gcc $(CFLAGS) -shared -o $(COMPONENT_LIB) $(OBJ) -ldl

$(SRC): $(HSRC)

clean:: 
	rm -f $(COMPONENT_LIB)
	rm -f $(OBJ)
	
distclean:: clean

clobber:: clean
	rm -f $(OMXLIBDIR)/$(COMPONENT_LIB)
