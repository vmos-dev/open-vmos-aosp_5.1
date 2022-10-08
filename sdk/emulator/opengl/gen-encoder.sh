#!/bin/sh
# Generate the Android-side encoder source files. 
# Requirements:
# (a) The ANDROID_BUILD_TOP and ANDROID_HOST_OUT environment variables must be
#     defined appropriately. The Android "lunch" bash function will do this.
# (b) The emugen binary must already be built. Any normal build that includes
#     the emulator will do this.

if [ -z "$ANDROID_BUILD_TOP" ]; then
    echo error: ANDROID_BUILD_TOP not set
    exit 1
fi
cd "$ANDROID_BUILD_TOP" >/dev/null
SRCDIR="sdk/emulator/opengl/host/libs"
DSTDIR="device/generic/goldfish/opengl/system"
EMUGEN="external/qemu/objs/emugen"
if [ ! -d "$SRCDIR" -o ! -d "$DSTDIR" ]; then
    echo error: can\'t find source and/or destination directory
    exit 1
fi
if [ ! -x "$EMUGEN" ]; then
    echo error: emugen not available, did you forget to build?
    exit 1
fi

function gen() {
    local src="$SRCDIR/$1_dec"
    local dst="$DSTDIR/$1_enc"
    local name="$2"
    echo "${EMUGEN}" -E "$dst" -i "$src" "$name" "$src/$name".{attrib,in,types}
}

$(gen GLESv1 gl)
$(gen GLESv2 gl2)
$(gen renderControl renderControl)
