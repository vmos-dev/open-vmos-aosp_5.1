
HSRC=$(wildcard *.h)
HTARGET=$(HSRC:%=$(OMXINCLUDEDIR)/%)

include $(OMXROOT)/Master.mk

all install:: $(OMXINCLUDEDIR) $(HTARGET) 

$(OMXINCLUDEDIR)/%: % 
	cp -f $< $@

clobber::
	rm -f $(OMXINCLUDEDIR)/LCML_CodecInterface.h
	rm -f $(OMXINCLUDEDIR)/LCML_DspCodec.h
	rm -f $(OMXINCLUDEDIR)/LCML_Types.h
	rm -f $(OMXINCLUDEDIR)/usn.h
