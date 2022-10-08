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
#ifndef DISPLAY_QUERY_H
#define DISPLAY_QUERY_H

namespace android {
namespace intel {

class DisplayQuery
{
public:
    static bool isVideoFormat(uint32_t format);
    static int  getOverlayLumaStrideAlignment(uint32_t format);
    static uint32_t queryNV12Format();
    static bool forceFbScaling(int device);
};

} // namespace intel
} // namespace android

#endif /*DISPLAY_QUERY_H*/
