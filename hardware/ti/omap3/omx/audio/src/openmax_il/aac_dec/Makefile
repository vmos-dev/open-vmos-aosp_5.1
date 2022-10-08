

all::

# pull in the master "inherited" definitions
include $(OMXROOT)/Master.mk

DIRS= \
	src \
	inc \

EXTRA=\
	docs \
	patterns \
	test


$(BASETARGETS)::
	@$(call traverse_dirs,$(DIRS),$@)

