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
#ifndef __VSYNC_EVENT_OBSERVER_H__
#define __VSYNC_EVENT_OBSERVER_H__

#include <common/base/SimpleThread.h>
#include <IVsyncControl.h>

namespace android {
namespace intel {

class PhysicalDevice;

class VsyncEventObserver {
public:
    VsyncEventObserver(PhysicalDevice& disp);
    virtual ~VsyncEventObserver();

public:
    virtual bool initialize();
    virtual void deinitialize();
    bool control(bool enabled);

private:
    mutable Mutex mLock;
    Condition mCondition;
    PhysicalDevice& mDisplayDevice;
    IVsyncControl *mVsyncControl;
    int  mDevice;
    bool mEnabled;
    bool mExitThread;
    bool mInitialized;

private:
    DECLARE_THREAD(VsyncEventPollThread, VsyncEventObserver);
};

} // namespace intel
} // namespace android



#endif /* __VSYNC_EVENT_OBSERVER_H__ */
