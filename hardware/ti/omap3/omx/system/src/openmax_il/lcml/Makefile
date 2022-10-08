

all::

# pull in the master "inherited" definitions
include $(OMXROOT)/Master.mk

DIRS= \
	src \
	inc \
    docs 
      	
EXTRA=\
	patterns 


$(BASETARGETS)::
	@$(call traverse_dirs,$(DIRS),$@)

