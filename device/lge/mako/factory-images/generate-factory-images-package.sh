#!/bin/sh

# Copyright 2012 The Android Open Source Project
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

# start jb-mr1-dev
# 477685 = JOO88
# 521994 = JOP32B
# 526897 = JOP39B
# 527221 = JOP40
# 527662 = JOP40C
# end jb-mr1-dev
# start jb-mr1.1-dev
# 573038 = JDQ39
# end jb-mr1.1-dev
# start jb-mr2-dev
# 683083 = JWR51
# 689345 = JWR58
# 690834 = JWR59
# 704243 = JWR66G
# 709441 = JWR66M
# 711294 = JWR66N
# 737497 = JWR66V
# end jb-mr2-dev

source ../../../common/clear-factory-images-variables.sh
BUILD=937116
DEVICE=mako
PRODUCT=occam
VERSION=kot49h
SRCPREFIX=signed-
BOOTLOADER=makoz30d
RADIO=m9615a-cefwmazm-2.0.1700.98
source ../../../common/generate-factory-images-common.sh
