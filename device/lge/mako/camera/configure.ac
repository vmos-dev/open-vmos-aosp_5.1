AC_PREREQ(2.61)

AC_INIT([camera-hal],1.0.0)

AM_INIT_AUTOMAKE([-Werror -Wall gnu foreign])

AM_MAINTAINER_MODE

AC_CONFIG_HEADER([configure.h])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AM_PROG_AS
AC_PROG_CC
AC_PROG_CXX
AM_PROG_CC_C_O
AC_PROG_LIBTOOL
AC_PROG_AWK
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

AC_CHECK_TOOL(OBJCOPY, objcopy, false)

COMPILE_CAMERA=yes
AMSS_VERSION=
VFE_VERS=
MSM_VERSION=
DEBUG_CPPFLAGS=
DEBUG_CFLAGS=
DEBUG_LDFLAGS=

AC_ARG_ENABLE([target],
	[AS_HELP_STRING([--enable-target=TARGET],[target to build for])],
	[],
	[enable_target=none]
)

if test "x$enable_target" = "xmsm7627a"; then
	MSM_VERSION=7x27A
	BUILD_UNIFIED_CODE=false
	BUILD_JPEG=false
elif test "x$enable_target" = "xmsm8960"; then
	MSM_VERSION=8960
	BUILD_UNIFIED_CODE=false
	BUILD_JPEG=true
else
	MSM_VERSION=
	BUILD_UNIFIED_CODE=false
	BUILD_JPEG=false
fi

AC_ARG_ENABLE([debug],
	[AS_HELP_STRING([--enable-debug],[Build with debug flags and options])],
	[DEBUG=$enableval],
	[DEBUG=no]
)
if test "x$DEBUG" = "xyes"; then
	DEBUG_CPPFLAGS="${DEBUG_CPPFLAGS} -DLOG_DEBUG -DLOG_TAG=\"CameraService\""
	DEBUG_CFLAGS="${DEBUG_CFLAGS} -DLOG_DEBUG -DLOG_TAG=\"CameraService\" -g -O0"
fi

AC_SUBST([MSM_VERSION])
AM_CONDITIONAL([MSM7X27A], [test "x$MSM_VERSION" = "x7x27A"])
AM_CONDITIONAL([MSM8960], [test "x$MSM_VERSION" = "x8960"])
AM_CONDITIONAL([BUILD_UNIFIED_CODE], [test "x$BUILD_UNIFIED_CODE" = "xtrue"])
AM_CONDITIONAL([BUILD_JPEG], [test "x$BUILD_JPEG" = "xtrue"])

AC_OUTPUT([ \
	Makefile
])

