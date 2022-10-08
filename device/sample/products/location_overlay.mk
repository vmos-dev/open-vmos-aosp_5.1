#
# Copyright (C) 2008 The Android Open Source Project
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
#

# This is an example of how to set an overlay that configures the
# default backup transport. In this example, the configuration points
# to the Google implementation, to show the exact syntax in a real-world
# example.

PRODUCT_PACKAGE_OVERLAYS := device/sample/overlays/location

# Uncomment this line to include the Google network and fused location providers
#PRODUCT_PACKAGES := NetworkLocation
