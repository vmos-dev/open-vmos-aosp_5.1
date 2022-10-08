#!/bin/sh

# Copyright 2013 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# start jb-mr2-dev
# 704765 = JSR72
# 740015 = JSS11F
# 741000 = JSS11I
# 741250 = JSS15
# 746990 = JSS15H
# 748502 = JSS15I
# 748593 = JSS15J
# 750418 = JSS15K
# end jb-mr2-dev

source ../../../common/clear-factory-images-variables.sh
BUILD=937116
DEVICE=deb
PRODUCT=razorg
VERSION=kot49h
SRCPREFIX=signed-
BOOTLOADER=flo-04.02
RADIO=deb-g00_2.37.0_1024
source ../../../common/generate-factory-images-common.sh
