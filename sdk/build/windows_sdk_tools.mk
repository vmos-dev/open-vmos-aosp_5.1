# Makefile to build the Windows SDK Tools under linux.
#
# This makefile is included by development/build/tools/windows_sdk.mk
# to device which tools we want to build from the sdk.git project.

# This contains the SDK Tools modules to build during a
# *platform* builds. Right now we are not building any SDK Tools
# during platform builds anymore.
# They are now built in an unbundled branch.

WIN_SDK_TARGETS := 


# Define the list of tool-dependent modules requisites needed
# for the Windows SDK. These will be built using HOST_OS=linux.
# Temporarily disabled: See b/8992787
#WIN_SDK_BUILD_PREREQ := \
#	monitor

