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
#ifndef PLATF_HWCOMPOSER_H
#define PLATF_HWCOMPOSER_H

#include <hal_public.h>
#include <Hwcomposer.h>

namespace android {
namespace intel {

class PlatfHwcomposer : public Hwcomposer {
public:
    PlatfHwcomposer();
    virtual ~PlatfHwcomposer();

protected:
    DisplayPlaneManager* createDisplayPlaneManager();
    BufferManager* createBufferManager();
    IDisplayDevice* createDisplayDevice(int disp, DisplayPlaneManager& dpm);
    IDisplayContext* createDisplayContext();
};

} //namespace intel
} //namespace android

#endif /* PLATF_HWCOMPOSER_H */
