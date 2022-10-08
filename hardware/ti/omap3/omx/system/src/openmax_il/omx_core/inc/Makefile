
HSRC=$(wildcard *.h)
HTARGET=$(HSRC:%=$(OMXINCLUDEDIR)/%)

include $(OMXROOT)/Master.mk

all install:: $(OMXINCLUDEDIR) $(HTARGET) 

$(OMXINCLUDEDIR)/%: % 
	cp -f $< $@

clobber::
	rm -f $(OMXINCLUDEDIR)/OMX_Audio.h
	rm -f $(OMXINCLUDEDIR)/OMX_Component.h
	rm -f $(OMXINCLUDEDIR)/OMX_Core.h
	rm -f $(OMXINCLUDEDIR)/OMX_Debug.h
	rm -f $(OMXINCLUDEDIR)/OMX_DebugMem.h
	rm -f $(OMXINCLUDEDIR)/OMX_Image.h
	rm -f $(OMXINCLUDEDIR)/OMX_Index.h
	rm -f $(OMXINCLUDEDIR)/OMX_IVCommon.h
	rm -f $(OMXINCLUDEDIR)/OMX_Other.h
	rm -f $(OMXINCLUDEDIR)/OMX_Types.h
	rm -f $(OMXINCLUDEDIR)/OMX_Video.h
	rm -f $(OMXINCLUDEDIR)/OMX_ContentPipe.h
	rm -f $(OMXINCLUDEDIR)/OMX_ComponentRegistry.h