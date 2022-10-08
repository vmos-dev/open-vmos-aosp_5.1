
HSRC=TIDspOmx.h
HTARGET=$(HSRC:%=$(OMXINCLUDEDIR)/%)

include $(OMXROOT)/Master.mk

all install:: $(OMXINCLUDEDIR) $(SYSTEMINCLUDEDIR) $(HTARGET) 

$(OMXINCLUDEDIR)/%: % 
	cp -f $< $@

clobber::
	rm -f $(OMXINCLUDEDIR)/TIDspOmx.h
