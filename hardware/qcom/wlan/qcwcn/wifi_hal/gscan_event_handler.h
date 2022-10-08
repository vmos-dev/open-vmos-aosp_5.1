/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __WIFI_HAL_GSCAN_EVENT_HANDLE_H__
#define __WIFI_HAL_GSCAN_EVENT_HANDLE_H__

#include "common.h"
#include "cpp_bindings.h"
#include "gscancommand.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

class GScanCommandEventHandler: public WifiVendorCommand
{
private:
    // TODO: derive 3 other command event handler classes from this base and separate
    // the data member vars
    wifi_scan_result *mHotlistApFoundResults;
    u32 mHotlistApFoundNumResults;
    bool mHotlistApFoundMoreData;
    wifi_significant_change_result **mSignificantChangeResults;
    u32 mSignificantChangeNumResults;
    bool mSignificantChangeMoreData;
    GScanCallbackHandler mHandler;
    int mRequestId;
    /* Needed because mSubcmd gets overwritten in
     * WifiVendorCommand::handleEvent()
     */
    u32 mSubCommandId;

public:
    GScanCommandEventHandler(wifi_handle handle, int id, u32 vendor_id,
                                    u32 subcmd, GScanCallbackHandler nHandler);
    virtual ~GScanCommandEventHandler();
    virtual int create();
    virtual int get_request_id();
    virtual int handleEvent(WifiEvent &event);
};

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
