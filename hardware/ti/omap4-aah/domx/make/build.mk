#
#  Copyright 2001-2009 Texas Instruments - http://www.ti.com/
# 
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.

#
#  dspbridge/mpu_api/make/build.mk
#
#  DSP-BIOS Bridge build rules.

# ALL PATHS IN MAKEFILE MUST BE RELATIVE TO ITS DIRECTORY

CDEFS       += $(CMDDEFS) # Add command line definitions
CDEFS       += $(PROCFAMILY) # Processor Family e.g. 3430
CDEFS	    += $(CMDDEFS_START) # Definitions from start.mk
#   ----------------------------------------------------------
#   REMOVE LEADING AND TRAILING SPACES FROM MAKEFILE MACROS
#   ----------------------------------------------------------

TARGETNAME  := $(strip $(TARGETNAME))
TARGETTYPE  := $(strip $(TARGETTYPE))
SUBMODULES  := $(strip $(SUBMODULES))
SOURCES     := $(strip $(SOURCES))
INCLUDES    := $(strip $(INCLUDES))
LIBINCLUDES := $(strip $(LIBINCLUDES))

SH_SONAME   := $(strip $(SH_SONAME))
ST_LIBS     := $(strip $(ST_LIBS))
SH_LIBS     := $(strip $(SH_LIBS))

CFLAGS      := $(strip $(CFLAGS))
CDEFS       := $(strip $(CDEFS))
EXEC_ARGS   := $(strip $(EXEC_ARGS))
ST_LIB_ARGS := $(strip $(ST_LIB_ARGS))
SH_LIB_ARGS := $(strip $(SH_LIB_ARGS))

#   ----------------------------------------------------------
#   COMPILER OPTIONS
#   ----------------------------------------------------------

# Preprocessor : dependency file generation
ifndef NODEPENDS
ifndef nodepends
CFLAGS += -MD
endif
endif

#   Overall
CFLAGS += -pipe
#   Preprocessor
CFLAGS +=
#   Debugging
ifeq ($(BUILD),deb)
CFLAGS += -g
else
CFLAGS += -fomit-frame-pointer
endif
#   Warnings
CFLAGS += -Wall  -Wno-trigraphs -Werror-implicit-function-declaration #-Wno-format
#   Optimizations
#CFLAGS += -O2 -fno-strict-aliasing
#CFLAGS += -Os -fno-strict-aliasing
CFLAGS += -fno-strict-aliasing
#   Machine dependent

ifeq ($(PROCFAMILY),OMAP_3430)
CFLAGS += -mapcs -mno-sched-prolog -mabi=aapcs-linux -mno-thumb-interwork -march=armv7-a -msoft-float -Uarm -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR($(basename $(TARGETNAME)))" -D"KBUILD_MODNAME=KBUILD_STR($(basename $(TARGETNAME)))"  -DMODULE -D__LINUX_ARM_ARCH__=7 
endif

ifeq ($(PROCFAMILY),OMAP_4430)
CFLAGS += -mapcs -mno-sched-prolog -mabi=aapcs-linux -mno-thumb-interwork -march=armv7-a -msoft-float -Uarm -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR($(basename $(TARGETNAME)))" -D"KBUILD_MODNAME=KBUILD_STR($(basename $(TARGETNAME)))"  -DMODULE -D__LINUX_ARM_ARCH__=7
endif

#   Code generation
CFLAGS += -fno-common
#   Macros
CFLAGS += -DLINUX $(addprefix -D, $(CDEFS))

ifdef __KERNEL__
CFLAGS      += -D__KERNEL__  -fno-builtin
endif

#   ----------------------------------------------------------
#   OBJECTS
#   ----------------------------------------------------------

BUILDDIR    = .obj/

# setup the target - check the given type - make sure we have the
# correct suffix on it
# TARGETNAME should not have a suffix on it - give an error if it does
#ifneq ($(suffix $(TARGETNAME)),)
#   $(error TARGETNAME can not have a suffix)
#endif
ifeq ($(TARGETTYPE),SH_LIB)
   TARGET      := $(basename $(TARGETNAME)).so
else
ifeq ($(TARGETTYPE),MODULE)
   TARGET      := $(basename $(TARGETNAME)).o
   TARGETKO    := $(addsuffix .ko,$(basename $(TARGET)))
   TARGETMOD   := $(addsuffix .mod,$(basename $(TARGET)))
else
ifeq ($(TARGETTYPE),ST_LIB)
   TARGET      := $(basename $(TARGETNAME)).a
else
ifeq ($(TARGETTYPE),EXEC)
   TARGET      := $(basename $(TARGETNAME)).out
else
ifneq ($(TARGETTYPE),)
TARGET         := $(error Invalid TARGETTYPE)
endif
endif
endif
endif
endif

#LIBINCLUDES += $(TARGETDIR) $(TGTROOT)/lib $(TGTROOT)/usr/lib
LIBINCLUDES += $(TARGETDIR)/lib $(TARGETDIR)/usr/lib 
SRCDIRS :=  $(sort $(dir $(SOURCES)))
OBJDIRS :=  $(addprefix $(BUILDDIR),$(SRCDIRS)) $(BUILDDIR)

BASEOBJ := $(addsuffix .o,$(basename $(SOURCES)))
OBJECTS := $(addprefix $(BUILDDIR), $(BASEOBJ))

ST_LIBNAMES := $(addsuffix .a, $(addprefix lib, $(ST_LIBS)))
DL_LIBNAMES := $(addsuffix .so, $(addprefix lib, $(SH_LIBS)))

vpath %.a $(LIBINCLUDES) $(TGTROOT)/lib $(TGTROOT)/usr/lib
vpath %.so $(LIBINCLUDES) $(TGTROOT)/lib $(TGTROOT)/usr/lib

#   ----------------------------------------------------------
#   BUILD ARGUMENTS
#   ----------------------------------------------------------

MAPFILE := -Wl,-Map,$(TARGET).map
INCPATH := $(addprefix -I, . $(INCLUDES)) $(LINUXINCLUDE)
LIBPATH := $(addprefix -L, $(LIBINCLUDES))
LIBFILE := $(addprefix -l, $(ST_LIBS) $(SH_LIBS)) $(LIB_OBJS)

ifeq ($(TARGETTYPE),SH_LIB)
CFLAGS += -fpic
TARGETARGS := $(SH_LIB_ARGS) -nostartfiles -nodefaultlibs -nostdlib -shared -Wl
ifneq ($(SH_SONAME),)
TARGETARGS += -Wl,-soname,$(SH_SONAME)
endif
endif

ifeq ($(TARGETTYPE),MODULE)
TARGETARGS := $(SH_LIB_ARGS) -nostartfiles -nodefaultlibs -nostdlib -Wl,-r
ifneq ($(SH_SONAME),)
TARGETARGS += -Wl,-soname,$(SH_SONAME)
endif
endif

ifeq ($(TARGETTYPE),ST_LIB)
TARGETARGS := $(ST_LIB_ARGS) -nostartfiles -nodefaultlibs -nostdlib -Wl,-r
endif

ifeq ($(TARGETTYPE),EXEC)
TARGETARGS := $(EXEC_ARGS)
endif

.PHONY  :   all $(SUBMODULES) clean cleantrg SHELLERR Debug

#   ==========================================================
#   all
#   ==========================================================
all :  $(CHECKSHELL) $(SUBMODULES)

#   ==========================================================
#   Make submodules
#   ==========================================================
$(SUBMODULES):
ifndef NORECURSE
ifndef norecurse
	$(MAKE) -C $@ $(filter-out $(SUBMODULES),$(MAKECMDGOALS))
endif
endif

ifneq ($(TARGETTYPE),)

# if this is driver module level , build KO file too
ifneq ($(TOPLEVEL),)
all :  $(OBJDIRS) $(TARGETKO)
else
all :  $(OBJDIRS) $(TARGET)
endif

#   ==========================================================
#   Create directories
#   ==========================================================
$(OBJDIRS) $(TARGETDIR) :
	@$(call MKDIR, $@)

#   ==========================================================
#   Product 2.6.x kernel module based on target
#   ==========================================================

# Link module .o with vermagic .o
$(TARGETKO): $(TARGETMOD).o $(TARGET) 
	$(LD) -EL -r -o $@ $^ 

# Compile vermagic
$(TARGETMOD).o: $(TARGETMOD).c 
	$(CC) -c $(TARGETARGS) $(CFLAGS) $(INCPATH) -o $@ $<

# Generate Module vermagic
$(TARGETMOD).c: $(TARGET) 
	$(MODPOST) $(TARGET)
# removed - need to be done as a pre-step to building
#	$(MAKE) -C $(KRNLSRC) modules	

#   ==========================================================
#   Build target
#   ==========================================================
$(TARGET):$(OBJECTS) $(ST_LIBNAMES) $(DL_LIBNAMES)
#   @$(SHELLCMD) echo Building $@
#	$(CC) $(TARGETARGS) $(CFLAGS) $(LIBPATH) $(MAPFILE) -o $@ $(BASEOBJ) $(LIBFILE)
#	$(CC) $(TARGETARGS) $(CFLAGS) $(LIBPATH) $(MAPFILE) -o $@ $(OBJECTS) $(LIBFILE)
ifeq ($(TARGETTYPE),ST_LIB)
	$(AR) r $@ $(OBJECTS)
else
	$(CC) $(TARGETARGS) $(CFLAGS) $(LIBPATH) $(MAPFILE) -o $@ $(OBJECTS) $(LIBFILE)
endif

#   ==========================================================
#   Compile .c file
#   ==========================================================
$(BUILDDIR)%.o:%.c
#   echo Compiling $(patsubst $(BUILDDIR)%.o,%.c, $@)
	$(CC) -c $(CFLAGS) $(INCPATH) -o $@ $(patsubst $(BUILDDIR)%.o,%.c, $@)

#   ==========================================================
#   Compile .S file
#   ==========================================================
$(BUILDDIR)%.o:%.S
#   echo Compiling $(patsubst $(BUILDDIR)%.o,%.S, $@)
	$(CC) -c $(CFLAGS) $(INCPATH) -o $@ $(patsubst $(BUILDDIR)%.o,%.S, $@)

endif   # ifneq ($(TARGETTYPE),)

#   ----------------------------------------------------------
#   install - install the files
#   ----------------------------------------------------------
install:: $(TARGETDIR) $(SUBMODULES) $(TARGET)
ifdef HOSTRELEASE
ifdef SH_SONAME
	$(INSTALL) -D $(TARGET) $(TARGETDIR)/$(HOSTRELEASE)/$(SH_SONAME)
	$(RM) -f $(TARGETDIR)/$(HOSTRELEASE)/$(TARGET)
	ln -s $(SH_SONAME) $(TARGETDIR)/$(HOSTRELEASE)/$(TARGET)
else
ifneq ($(TOPLEVEL),)
	$(INSTALL) -D $(TARGETKO) $(TARGETDIR)/$(HOSTRELEASE)/$(TARGETKO)
else
	$(INSTALL) -D $(TARGET) $(TARGETDIR)/$(HOSTRELEASE)/$(TARGET)
endif
endif
endif
ifdef 0 # removed - components shouldn't put things in the production fs
ifdef ROOTFSRELEASE
	$(call MKDIR, $(ROOTFSDIR)/$(ROOTFSRELEASE))
ifdef SH_SONAME
	$(STRIP) --strip-unneeded -xgo $(ROOTFSDIR)/$(ROOTFSRELEASE)/$(SH_SONAME) $(TARGET)
	$(RM) -f $(ROOTFSDIR)/$(ROOTFSRELEASE)/$(TARGET)
	ln -s $(SH_SONAME) $(ROOTFSDIR)/$(ROOTFSRELEASE)/$(TARGET)
else
ifneq ($(TOPLEVEL),)
	$(STRIP) --strip-unneeded -xgo $(ROOTFSDIR)/$(ROOTFSRELEASE)/$(TARGETKO) $(TARGETKO)
else
	$(STRIP) --strip-unneeded -xgo $(ROOTFSDIR)/$(ROOTFSRELEASE)/$(TARGET) $(TARGET)
endif
endif
endif
endif

#   ----------------------------------------------------------
#   clean - Remove build directory and target files
#   Linux : Removes object and dependency files in build folder
#   DOS   : Removes object dirs in build folder
#   ----------------------------------------------------------
clean : $(SUBMODULES)
ifneq ($(TARGETTYPE),)
ifneq ($(OBJECTS),)
	- $(call RM,-f $(OBJECTS))
	- $(call RM,-f $(OBJECTS:.o=.d))
	- $(call RMDIR,-f $(BUILDDIR))
endif
	- $(call RM,-f $(TARGET))
	- $(call RM,-f $(TARGET).map)
	- $(call RM,-f $(TARGETKO))
	- $(call RM,-f $(TARGETMOD).c)
	- $(call RM,-f $(TARGETMOD).o)
	- $(call RM,-f $(TARGETMOD).d)
ifneq ($(TOPLEVEL),)
	- @$(call RM,-f $(TARGETKO))
	- @$(call RM,-f $(TARGETMOD).c)
	- @$(call RM,-f $(TARGETMOD).o)
	- @$(call RM,-f $(TARGETMOD).d)
endif
endif

cleantrg : $(SUBMODULES)
ifneq ($(TARGETTYPE),)
	- @$(call RM, $(TARGET))
	- @$(call RM, $(TARGET).map)
ifneq ($(TOPLEVEL),)
	- @$(call RM, $(TARGETKO))
	- @$(call RM, $(TARGETMOD).c)
	- @$(call RM, $(TARGETMOD).o)
	- @$(call RM, $(TARGETMOD).d)
endif
endif

#   ----------------------------------------------------------
#   Include dependency files generated by preprocessor.
#
#   Dependency files are placed in main object directory because
#   dependent files' paths for same source file varies with the
#   directory from where gmake is run
#   ----------------------------------------------------------
ifndef NODEPENDS
ifndef nodepends
ifneq ($(OBJECTS),)
-include $(OBJECTS:.o=.d)
endif
endif
endif

#   ----------------------------------------------------------
#   Generate fatal error if make variable SHELL is incorrect
#   ----------------------------------------------------------
SHELLERR::
	@$(SHELLCMD) echo Fatal error: SHELL set to $(SHELL) instead of $(MYSHELL)
	@$(SHELLCMD) echo set $(MYSHELL) to correct path and CASE SENSITIVE FILE NAME and EXTENSTION
	@$(SHELLCMD) echo of your command shell
	$(ERR)


#   ----------------------------------------------------------
#   For debugging script
#   ----------------------------------------------------------
Debug::$(SUBMODULES)
	@$(SHELLCMD) echo SHELL: $(SHELL)
	@$(SHELLCMD) echo
	@$(SHELLCMD) echo CDEFS: $(CDEFS)
	@$(SHELLCMD) echo
	@$(SHELLCMD) echo CONFIG_SHELL: $(CONFIG_SHELL)
	@$(SHELLCMD) echo
	@$(SHELLCMD) echo CURDIR: $(CURDIR)
	@$(SHELLCMD) echo
	@$(SHELLCMD) echo SRCDIRS: $(SRCDIRS)
	@$(SHELLCMD) echo
	@$(SHELLCMD) echo OBJDIRS: $(OBJDIRS)
	@$(SHELLCMD) echo
	@$(SHELLCMD) echo OBJECTS: $(OBJECTS)
	@$(SHELLCMD) echo
	@$(SHELLCMD) echo BUILDDIR: $(BUILDDIR)
	@$(SHELLCMD) echo
	@$(SHELLCMD) echo TARGETDIR TARGETNAME: $(TARGET)
	@$(SHELLCMD) echo
	@$(SHELLCMD) echo MAKEDIR: $(MAKEDIR)
	@$(SHELLCMD) echo
	@$(SHELLCMD) echo INCLUDES: $(INCLUDES)
	@$(SHELLCMD) echo
	@$(SHELLCMD) echo DL_LIBNAMES: $(DL_LIBNAMES)
	@$(SHELLCMD) echo
	@$(SHELLCMD) echo LIBFILE: $(LIBFILE)
	@$(SHELLCMD) echo

