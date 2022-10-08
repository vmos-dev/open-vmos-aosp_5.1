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

#include <common/utils/HwcTrace.h>
#include <common/base/Drm.h>
#include <ips/common/BlankControl.h>
#include <Hwcomposer.h>

namespace android {
namespace intel {

BlankControl::BlankControl()
    : IBlankControl()
{
}

BlankControl::~BlankControl()
{
}

bool BlankControl::blank(int disp, bool blank)
{
    // current do nothing but return true
    // use PM to trigger screen blank/unblank
    VLOGTRACE("blank is not supported yet, disp %d, blank %d", disp, blank);
    return true;
}

} // namespace intel
} // namespace android
