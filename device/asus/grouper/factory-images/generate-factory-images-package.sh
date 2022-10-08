#!/bin/sh

# Copyright 2011 The Android Open Source Project
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

# start jb-dev
# 334698 = JRN19
# 342231 = JRN26D
# 367578 = JRN60B
# 386704 = JRN80
# 391496 = JRN83D
# 392829 = JRN84D
# 397360 = JRO02C
# 398337 = JRO03C
# 402395 = JRO03D
# 447484 = JZO54
# 477516 = JZO54I
# 481723 = JZO54J
# 485486 = JZO54K
# end jb-dev
# start jb-mr1-dev
# 526897 = JOP39B
# 527221 = JOP40
# 527662 = JOP40C
# 533553 = JOP40D
# end jb-mr1-dev
# start jb-mr1.1-dev
# 551245 = JDP82
# 573038 = JDQ39
# end jb-mr1.1-dev
# start jb-mr2-dev
# 683083 = JWR51
# 689345 = JWR58
# 690834 = JWR59
# 704243 = JWR66G
# 711294 = JWR66N
# 737497 = JWR66V
# end jb-mr2-dev

source ../../../common/clear-factory-images-variables.sh
BUILD=937116
DEVICE=grouper
PRODUCT=nakasi
VERSION=kot49h
SRCPREFIX=signed-
BOOTLOADERFILE=bootloader.img
BOOTLOADER=4.23
SLEEPDURATION=10
UNLOCKBOOTLOADER=true
ERASEALL=true
source ../../../common/generate-factory-images-common.sh
