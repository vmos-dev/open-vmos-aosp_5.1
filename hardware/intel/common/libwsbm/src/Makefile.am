
lib_LTLIBRARIES = libwsbm.la

libwsbm_la_CFLAGS = @libdrm_CFLAGS@ -Wall 
libwsbm_la_LDFLAGS = -version-number 1:1:0 -no-undefined @libdrm_LIBS@
libwsbm_la_SOURCES = \
	wsbm_fencemgr.c \
	wsbm_fencemgr.h \
	wsbm_manager.c \
	wsbm_manager.h	\
	wsbm_mm.c \
	wsbm_mm.h \
	wsbm_pool.h \
	wsbm_util.h \
	wsbm_mallocpool.c \
	wsbm_driver.h \
	wsbm_driver.c \
	wsbm_ttmpool.c \
	wsbm_slabpool.c \
	wsbm_userpool.c \
	wsbm_priv.h


libwsbmincludedir = ${includedir}/wsbm
libwsbminclude_HEADERS = wsbm_manager.h wsbm_pool.h wsbm_driver.h \
	wsbm_fencemgr.h wsbm_util.h wsbm_atomic.h

