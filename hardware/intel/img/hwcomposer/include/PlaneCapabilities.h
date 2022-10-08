/*
// Copyright (c) 2014 Intel Corporation 
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
#ifndef PLANE_CAPABILITIES_H
#define PLANE_CAPABILITIES_H

#include <DataBuffer.h>

namespace android {
namespace intel {

class HwcLayer;
class PlaneCapabilities
{
public:
    static bool isFormatSupported(int planeType, HwcLayer *hwcLayer);
    static bool isSizeSupported(int planeType,  HwcLayer *hwcLayer);
    static bool isBlendingSupported(int planeType, HwcLayer *hwcLayer);
    static bool isScalingSupported(int planeType, HwcLayer *hwcLayer);
    static bool isTransformSupported(int planeType,  HwcLayer *hwcLayer);
};

} // namespace intel
} // namespace android

#endif /*PLANE_CAPABILITIES_H*/
