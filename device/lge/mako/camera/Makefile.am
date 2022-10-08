ACLOCAL_AMFLAGS = -I m4

#libcamera.so
lib_LTLIBRARIES = libcamera.la

libcamera_la_CFLAGS := $(DEBUG_CFLAGS)
libcamera_la_CFLAGS += -DAMSS_VERSION=$(AMSS_VERSION)
libcamera_la_CFLAGS += -DMSM_CAMERA_GCC
libcamera_la_CFLAGS += -DLINUX_ENABLED
libcamera_la_CFLAGS += -Dstrlcpy=g_strlcpy
libcamera_la_CFLAGS += -fPIC

LOCAL_CFLAGS:= -DDLOPEN_LIBMMCAMERA=1

libcamera_la_CFLAGS += -DHW_ENCODE

if MSM7X27A
libcamera_la_CFLAGS+= -DNUM_PREVIEW_BUFFERS=6
libcamera_la_CFLAGS+= -DVFE_7X27A
else
libcamera_la_CFLAGS+= -DNUM_PREVIEW_BUFFERS=4
endif

# To Choose neon/C routines for YV12 conversion
libcamera_la_CFLAGS+= -DUSE_NEON_CONVERSION
# Uncomment below line to enable smooth zoom
#libcamera_la_CFLAGS+= -DCAMERA_SMOOTH_ZOOM

libcamera_la_SOURCES := mm_camera.c
libcamera_la_SOURCES += mm_camera_channel.c
libcamera_la_SOURCES += mm_camera_helper.c
libcamera_la_SOURCES += mm_camera_interface2.c
libcamera_la_SOURCES += mm_camera_notify.c
libcamera_la_SOURCES += mm_camera_poll_thread.c
libcamera_la_SOURCES += mm_camera_sock.c
libcamera_la_SOURCES += mm_camera_stream.c

if BUILD_JPEG
libcamera_la_SOURCES += mm_jpeg_encoder.c
libcamera_la_SOURCES += mm_omx_jpeg_encoder.c
endif

if BUILD_UNIFIED_CODE
if MSM8960
libcamera_la_SOURCES += QCameraHAL.cpp
libcamera_la_SOURCES += QCameraHWI_Parm.cpp
libcamera_la_SOURCES += QCameraHWI.cpp
libcamera_la_SOURCES += QCameraHWI_Preview.cpp
libcamera_la_SOURCES += QCameraHWI_Record.cpp
libcamera_la_SOURCES += QCameraHWI_Still.cpp
libcamera_la_SOURCES += QCameraHWI_Mem.cpp
libcamera_la_SOURCES += QCameraHWI_Display.cpp
libcamera_la_SOURCES += QCameraStream.cpp
libcamera_la_SOURCES += QualcommCamera2.cpp
elif MSM7X27A
libcamera_la_SOURCES += QCameraHAL.cpp
libcamera_la_SOURCES += QCameraHWI_Parm.cpp
libcamera_la_SOURCES += QCameraHWI.cpp
libcamera_la_SOURCES += QCameraHWI_Preview_7x27A.cpp
libcamera_la_SOURCES += QCameraHWI_Record_7x27A.cpp
libcamera_la_SOURCES += QCameraHWI_Still.cpp
libcamera_la_SOURCES += QCameraHWI_Mem.cpp
libcamera_la_SOURCES += QCameraHWI_Display.cpp
libcamera_la_SOURCES += QCameraStream.cpp
libcamera_la_SOURCES += QualcommCamera2.cpp
endif
endif

libcamera_la_LDFLAGS := $(DEBUG_LDFLAGS)
libcamera_la_LDFLAGS += -shared
libcamera_la_LIBADD = -ldl

dirs :=
SUBDIRS = $(dirs)

