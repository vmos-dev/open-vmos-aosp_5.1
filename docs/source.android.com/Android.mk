LOCAL_PATH := $(call my-dir)

#===================== docs for the s.a.c site =======================
include $(CLEAR_VARS)
LOCAL_MODULE_CLASS:=JAVA_LIBRARIES
LOCAL_DROIDDOC_HTML_DIR:=src
# Droiddoc needs java source to run. Just pointing to a dummy location
# and deleting output later in delete-ref target
LOCAL_ADDITIONAL_JAVA_DIR := frameworks/base/core/java/android/annotation
# FIXME FIXME FIXME LOCAL_ADDITIONAL_DEPENDENCIES := tradefed-docs
LOCAL_DROIDDOC_CUSTOM_TEMPLATE_DIR := build/tools/droiddoc/templates-sac
LOCAL_MODULE := online-sac
LOCAL_DROIDDOC_OPTIONS:= \
        -toroot / \
        -hdf android.whichdoc online \
        -hdf sac true

include $(BUILD_DROIDDOC)

# Sets up the Doxygen HAL reference docs and puts them in the right place
# Need doxygen in your path (1.8.3 was used when this target was created)
setup-hal-ref:
	$(hide) doxygen docs/source.android.com/Doxyfile

# Put HAL refs in PDK instead and strip nav to s.a.c.
pdk-hal-ref:
	$(hide) doxygen vendor/pdk/data/google/Doxyfile

# Run "make tradefed-docs" in "tradefed" branch before running this target
# This sets up the tradefed javadocs for viewing in s.a.c.
# Target assumes that you have a "tradefed" directory
# that contains a sync'ed copy of the "tradefed" branch at the same level as the 
# live docs branch.

setup-tradefed-ref:
	$(hide) rm -rf $(OUT_DOCS)/online-sac/reference
	$(hide) cp -R out/target/common/docs/tradefed/reference $(OUT_DOCS)/online-sac
	$(hide) cp out/target/common/docs/tradefed/navtree_data.js $(OUT_DOCS)/online-sac/navtree_data.js

