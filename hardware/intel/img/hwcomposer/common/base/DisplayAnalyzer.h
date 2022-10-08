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
#ifndef DISPLAY_ANALYZER_H
#define DISPLAY_ANALYZER_H

#include <utils/threads.h>
#include <utils/Vector.h>


namespace android {
namespace intel {


class DisplayAnalyzer {
public:
    DisplayAnalyzer();
    virtual ~DisplayAnalyzer();

public:
    bool initialize();
    void deinitialize();
    void analyzeContents(size_t numDisplays, hwc_display_contents_1_t** displays);
    void postHotplugEvent(bool connected);

private:
    enum DisplayEventType {
        HOTPLUG_EVENT,
    };

    struct Event {
        int type;

        union {
            bool bValue;
            int  nValue;
        };
    };
    inline void postEvent(Event& e);
    inline bool getEvent(Event& e);
    void handlePendingEvents();
    void handleHotplugEvent(bool connected);
    inline void setCompositionType(hwc_display_contents_1_t *content, int type);
    inline void setCompositionType(int device, int type, bool reset);


private:
    bool mInitialized;
    int mCachedNumDisplays;
    hwc_display_contents_1_t** mCachedDisplays;
    Vector<Event> mPendingEvents;
    Mutex mEventMutex;
};

} // namespace intel
} // namespace android



#endif /* DISPLAY_ANALYZER_H */
