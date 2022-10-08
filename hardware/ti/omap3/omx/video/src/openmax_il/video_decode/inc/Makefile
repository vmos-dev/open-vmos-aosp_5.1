HSRC=OMX_VidDec_CustomCmd.h OMX_VideoDecoder.h

HTARGET=$(HSRC:%=$(OMXINCLUDEDIR)/%)

include $(OMXROOT)/Master.mk

all install:: $(OMXINCLUDEDIR) $(HTARGET) 

$(OMXINCLUDEDIR)/%: % 
	cp -f $< $@

clobber::
	rm -f $(OMXINCLUDEDIR)/OMX_VidDec_CustomCmd.h
	rm -f $(OMXINCLUDEDIR)/OMX_VideoDecoder.h
