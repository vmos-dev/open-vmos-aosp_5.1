#!/bin/bash

set +x

source ./build/envsetup.sh > /dev/null 2>&1
lunch aosp_arm64-user > /dev/null 2>&1


bash build_zip.sh