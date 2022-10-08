AM_CFLAGS = -Wundef \
        -Wstrict-prototypes \
        -Wno-trigraphs \
        -g -O0 \
        -fno-inline \
        -fno-short-enums \
        -fpic \
        -DQC_PROP

AM_CPPFLAGS = -I. \
               $(ACDBLOADER_CFLAGS)

c_sources = alsa_mixer.c \
            alsa_pcm.c \
            alsa_ucm.c

h_sources = alsa_ucm.h \
            msm8960_use_cases.h \
            alsa_audio.h

library_includedir = $(pkgincludedir)
library_include_HEADERS = $(h_sources)

lib_LTLIBRARIES = libalsa_intf.la
libalsa_intf_la_CC = @CC@
libalsa_intf_la_SOURCES = $(c_sources) $(h_sources)
libalsa_intfdir = $(prefix)/snd_soc_msm
dist_libalsa_intf_DATA    = snd_soc_msm/snd_soc_msm \
                            snd_soc_msm/snd_soc_msm_2x \
                            snd_soc_msm/snd_soc_msm_2x_Fusion3 \
                            snd_soc_msm/snd_soc_msm_Sitar
libalsa_intf_la_CFLAGS = $(AM_CFLAGS) -DUSE_GLIB @GLIB_CFLAGS@ -DCONFIG_DIR=\"/etc/snd_soc_msm/\"
libalsa_intf_la_CPPFLAGS = $(AM_CPPFLAGS) -DUSE_GLIB @GLIB_CFLAGS@
libalsa_intf_la_LDFLAGS = $(ACDBLOADER_LIBS) -lm -lpthread @GLIB_LIBS@ -shared -version-info 1:0:0

requiredlibs = libalsa_intf.la

bin_PROGRAMS = aplay amix arec

aplay_SOURCES = aplay.c
aplay_LDADD = -lpthread $(requiredlibs)

amix_SOURCES = amix.c
amix_LDADD = -lpthread $(requiredlibs)

arec_SOURCES = arec.c
arec_LDADD = -lpthread $(requiredlibs)
