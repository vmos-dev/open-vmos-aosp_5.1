# Copyright 2012 The Android Open Source Project

# Temporarily disable building monitor
# The current mechanism for building monitor using PDE build does
# not work with Java7 on Macs. So building monitor is temporarily
# disabled until we figure out how to build monitor using Tycho
# and incorporate it into this build script.
# See b/8992787
ifeq (0,1)

# Expose the Monitor RCP only for the SDK builds.
ifneq (,$(is_sdk_build)$(filter sdk sdk_x86 sdk_mips,$(TARGET_PRODUCT)))

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := monitor
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_TAGS := optional
LOCAL_IS_HOST_MODULE := true
include $(BUILD_SYSTEM)/base_rules.mk

RCP_LOG_FILE := out/host/eclipse/rcp/build/monitor.log
RCP_MONITOR_DIR := $(TOPDIR)out/host/eclipse/rcp/build/I.RcpBuild

define mk-rcp-monitor-atree-file
    srczip=$(RCP_MONITOR_DIR)/RcpBuild-$(1).$(2).zip && \
    dstdir=$(HOST_OUT)/eclipse/monitor-$(1).$(2) && \
    rm -rf $(V) $$dstdir && \
    mkdir -p $$dstdir && \
    unzip -q $$srczip -d $$dstdir
endef

MONITOR_DEP_LIBRARIES := $(shell $(TOPDIR)sdk/eclipse/scripts/create_all_symlinks.sh -d)
MONITOR_DEPS := $(foreach m,$(MONITOR_DEP_LIBRARIES),$(HOST_OUT_JAVA_LIBRARIES)/$(m).jar)

# The RCP monitor. It is referenced by build/target/products/sdk.mk
$(LOCAL_BUILT_MODULE) : $(TOPDIR)sdk/monitor/monitor \
			$(TOPDIR)sdk/monitor/build.xml \
			$(TOPDIR)sdk/monitor/build.properties \
			$(MONITOR_DEPS)
	@mkdir -p $(dir $@)
	$(hide) $(TOPDIR)sdk/eclipse/scripts/create_all_symlinks.sh -c
	$(hide)cd $(TOPDIR)sdk/monitor && \
		rm -f ../../$(RCP_LOG_FILE) && mkdir -p ../../$(dir $(RCP_LOG_FILE)) && \
		( java -jar ../../external/eclipse-basebuilder/basebuilder-3.6.2/org.eclipse.releng.basebuilder/plugins/org.eclipse.equinox.launcher_1.1.0.v20100507.jar \
			org.eclipse.equinox.launcher.Main \
			-application org.eclipse.ant.core.antRunner \
			-configuration ../../out/host/eclipse/rcp/build/configuration \
			-DbuildFor=$(HOST_OS) 2>&1 && \
		  mv -f ../../$(RCP_LOG_FILE) ../../$(RCP_LOG_FILE).1 ) \
		| tee ../../$(RCP_LOG_FILE) \
		| sed -E '/\[java\]/!d; /SUCCESSFUL/d ; s/^ +\[java\] //; /^ *$$/d; /:$$/d; /\[javac\] [^C]/d; s/^/monitor: /'; \
		if [[ -f ../../$(RCP_LOG_FILE) ]]; then \
		  echo "Monitor failed. Full log:" ; \
		  cat ../../$(RCP_LOG_FILE) ; \
		  exit 1 ; \
		fi
	$(hide)if [[ $(HOST_OS) == "linux" ]]; then \
		$(call mk-rcp-monitor-atree-file,linux.gtk,x86)    ; \
		$(call mk-rcp-monitor-atree-file,linux.gtk,x86_64) ; \
	elif [[ $(HOST_OS) == "darwin" ]]; then \
		$(call mk-rcp-monitor-atree-file,macosx.cocoa,x86_64) ; \
	elif [[ $(HOST_OS) == "windows" ]]; then \
		$(call mk-rcp-monitor-atree-file,win32.win32,x86)    ; \
		$(call mk-rcp-monitor-atree-file,win32.win32,x86_64) ; \
	fi
	$(hide)$(ACP) -fp $(V) $(TOPDIR)sdk/monitor/monitor $@

endif
endif
