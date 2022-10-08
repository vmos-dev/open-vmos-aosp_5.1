AC_PREREQ(2.57)
AC_INIT([libwsbm], 1.1.0, [thomas@tungstengraphics.com], libwsbm)
AC_CONFIG_SRCDIR([Makefile.am])
AM_INIT_AUTOMAKE([dist-bzip2])

AM_CONFIG_HEADER([config.h])

AC_DISABLE_STATIC
AC_PROG_LIBTOOL
AC_PROG_CC
PKG_PROG_PKG_CONFIG

PKG_CHECK_MODULES(libdrm, libdrm)
AC_SUBST(libdrm_CFLAGS)
AC_SUBST(libdrm_LIBS)
AC_HEADER_STDC
AC_SYS_LARGEFILE

AC_CHECK_HEADER(pthread.h, [
	AC_SEARCH_LIBS(pthread_cond_init, pthread,
		[AC_DEFINE(HAVE_PTHREADS, 1, "os has pthreads")],,,)
	      ],,,)

pkgconfigdir=${libdir}/pkgconfig
AC_SUBST(pkgconfigdir)

AC_OUTPUT([
	Makefile
	src/Makefile
	libwsbm.pc])
