SRC=\
	perf_unittest.c

HSRC=$(wildcard ../inc/*)



OBJ=$(SRC:.c=.o)

include $(OMXROOT)/Master.mk

CFLAGS+=-I../inc -I$(OMXINCLUDEDIR) -Wall -fpic -pipe -D__PERF_UNIT_TEST__
# optimization flags
CFLAGS += -O2

COMPONENT_LIB=$(OMXLIBDIR)/lib$(OMXPERFLIB).so
COMPONENT_TEST=PERF_test

COMPONENT_TARGET=$(OMXTESTDIR)/$(COMPONENT_TEST)

all install:: $(COMPONENT_TARGET)

$(COMPONENT_TARGET): $(OMXTESTDIR) $(COMPONENT_TEST) $(OMXLIBDIR) $(COMPONENT_LIB) 
	@echo "Installing $(COMPONENT_TEST)"
	cp -f $(COMPONENT_TEST) $(COMPONENT_TARGET)
	cp -f _perf.ini $(OMXTESTDIR)
	cp -f PERF.awk $(OMXTESTDIR)
	cp -f perf $(TARGETDIR)/bin
	cp -f armtime $(TARGETDIR)/bin
	chmod +x $(TARGETDIR)/bin/armtime
	chmod +x $(TARGETDIR)/bin/perf

$(COMPONENT_TEST): $(OBJ)
	$(CROSS)gcc $(CFLAGS) -o $(COMPONENT_TEST) $(OBJ) -L$(OMXLIBDIR) -l$(OMXPERFLIB) -lm

$(SRC): $(HSRC)

clean:: 
	rm -f $(COMPONENT_TEST)
	rm -f $(OBJ)

distclean:: clean

clobber:: clean
	rm -f $(OMXTESTDIR)/$(COMPONENT_TEST)
	rm -f $(OMXTESTDIR)/_perf.ini 
	rm -f $(OMXTESTDIR)/PERF.awk
	rm -f $(TARGETDIR)/bin/perf
	rm -f $(TARGETDIR)/bin/armtime


