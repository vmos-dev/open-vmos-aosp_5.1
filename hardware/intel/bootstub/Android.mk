LOCAL_PATH := $(call my-dir)

ifeq ($(TARGET_BOARD_PLATFORM),moorefield)

CMDLINE_SIZE ?= 0x400
BOOTSTUB_SIZE ?= 8192

BOOTSTUB_SRC_FILES := bootstub.c sfi.c ssp-uart.c imr_toc.c spi-uart.c
BOOTSTUB_SRC_FILES_x86 := head.S e820_bios.S

include $(CLEAR_VARS)

# bootstub.bin
LOCAL_SRC_FILES := $(BOOTSTUB_SRC_FILES)
LOCAL_SRC_FILES_x86 := $(BOOTSTUB_SRC_FILES_x86)
ANDROID_TOOLCHAIN_FLAGS := -m32 -ffreestanding
LOCAL_CFLAGS := $(ANDROID_TOOLCHAIN_FLAGS) -Wall -O1 -DCMDLINE_SIZE=${CMDLINE_SIZE}
LOCAL_MODULE := bootstub.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(PRODUCT_OUT)
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_FORCE_STATIC_EXECUTABLE := true


include $(BUILD_SYSTEM)/binary.mk

$(LOCAL_INTERMEDIATE_TARGETS): PRIVATE_TARGET_GLOBAL_CFLAGS := $(LOCAL_CFLAGS)
$(LOCAL_BUILT_MODULE) : PRIVATE_ELF_FILE := $(intermediates)/$(PRIVATE_MODULE).elf
$(LOCAL_BUILT_MODULE) : PRIVATE_LINK_SCRIPT := $(LOCAL_PATH)/bootstub.lds
$(LOCAL_BUILT_MODULE) : BOOTSTUB_OBJS := $(patsubst %.c, %.o , $(LOCAL_SRC_FILES))
$(LOCAL_BUILT_MODULE) : BOOTSTUB_OBJS += $(patsubst %.S, %.o , $(LOCAL_SRC_FILES_x86))
$(LOCAL_BUILT_MODULE) : BOOTSTUB_OBJS := $(addprefix $(intermediates)/, $(BOOTSTUB_OBJS))

$(LOCAL_BUILT_MODULE): $(all_objects)
	@$(mkdir -p $(dir $@)
	@echo "Generating bootstub.bin: $@"
	$(hide) $(TARGET_LD) \
		-m elf_i386 \
		-T $(PRIVATE_LINK_SCRIPT) \
		$(BOOTSTUB_OBJS) \
		-o $(PRIVATE_ELF_FILE)
	$(hide) $(TARGET_OBJCOPY) -O binary -R .note -R .comment -S $(PRIVATE_ELF_FILE) $@

# Then assemble the final bootstub file
bootstub_bin := $(PRODUCT_OUT)/bootstub.bin
bootstub_full := $(PRODUCT_OUT)/bootstub

CHECK_BOOTSTUB_SIZE : $(bootstub_bin)
	$(hide) ACTUAL_SIZE=`$(call get-file-size,$(bootstub_bin))`; \
	if [ "$$ACTUAL_SIZE" -gt "$(BOOTSTUB_SIZE)" ]; then \
		echo "$(bootstub_bin): $$ACTUAL_SIZE exceeds size limit of $(BOOTSTUB_SIZE) bytes, aborting."; \
		exit 1; \
	fi

$(bootstub_full) : CHECK_BOOTSTUB_SIZE
	@echo "Generating bootstub: $@"
	$(hide) cat $(bootstub_bin) /dev/zero | dd bs=$(BOOTSTUB_SIZE) count=1 > $@

# build specific bootstub for GPT/AOSP image support
include $(CLEAR_VARS)

# 2ndbootloader.bin
LOCAL_SRC_FILES := $(BOOTSTUB_SRC_FILES)
LOCAL_SRC_FILES_x86 := $(BOOTSTUB_SRC_FILES_x86)
ANDROID_TOOLCHAIN_FLAGS := -m32 -ffreestanding
LOCAL_CFLAGS := $(ANDROID_TOOLCHAIN_FLAGS) -Wall -O1 -DCMDLINE_SIZE=${CMDLINE_SIZE} -DBUILD_RAMDUMP
LOCAL_MODULE := 2ndbootloader.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(PRODUCT_OUT)
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_FORCE_STATIC_EXECUTABLE := true

include $(BUILD_SYSTEM)/binary.mk

$(LOCAL_INTERMEDIATE_TARGETS): PRIVATE_TARGET_GLOBAL_CFLAGS := $(LOCAL_CFLAGS)
$(LOCAL_BUILT_MODULE) : PRIVATE_ELF_FILE := $(intermediates)/$(PRIVATE_MODULE).elf
$(LOCAL_BUILT_MODULE) : PRIVATE_LINK_SCRIPT := $(LOCAL_PATH)/2ndbootloader.lds
$(LOCAL_BUILT_MODULE) : BOOTSTUB_OBJS := $(patsubst %.c, %.o , $(LOCAL_SRC_FILES))
$(LOCAL_BUILT_MODULE) : BOOTSTUB_OBJS += $(patsubst %.S, %.o , $(LOCAL_SRC_FILES_x86))
$(LOCAL_BUILT_MODULE) : BOOTSTUB_OBJS := $(addprefix $(intermediates)/, $(BOOTSTUB_OBJS))

$(LOCAL_BUILT_MODULE): $(all_objects)
	@$(mkdir -p $(dir $@)
	@echo "Generating bootstub $@"
	$(hide) $(TARGET_LD) \
		-m elf_i386 \
		-T $(PRIVATE_LINK_SCRIPT) \
		$(BOOTSTUB_OBJS) \
		-o $(PRIVATE_ELF_FILE)
	$(hide) $(TARGET_OBJCOPY) -O binary -R .note -R .comment -S $(PRIVATE_ELF_FILE) $@

# Then assemble the final 2ndbootloader file
bootstub_aosp_bin := $(PRODUCT_OUT)/2ndbootloader.bin
bootstub_aosp_full := $(PRODUCT_OUT)/2ndbootloader

CHECK_BOOTSTUB_AOSP_SIZE : $(bootstub_aosp_bin)
	$(hide) ACTUAL_SIZE=`$(call get-file-size,$(bootstub_aosp_bin))`; \
	if [ "$$ACTUAL_SIZE" -gt "$(BOOTSTUB_SIZE)" ]; then \
		echo "$(bootstub_aosp_bin): $$ACTUAL_SIZE exceeds size limit of $(BOOTSTUB_SIZE) bytes, aborting."; \
		exit 1; \
	fi

$(bootstub_aosp_full) : CHECK_BOOTSTUB_AOSP_SIZE
	@echo "Generating bootstub $@"
	$(hide) cat $(bootstub_aosp_bin) /dev/zero | dd bs=$(BOOTSTUB_SIZE) count=1 > $@

endif #($(TARGET_BOARD_PLATFORM),moorefield)
