/*
 * Copyright (C) Texas Instruments - http://www.ti.com/
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OMX_DCC_H
#define OMX_DCC_H

namespace Ti {
namespace Camera {

class DCCHandler
{
public:

    status_t loadDCC(OMX_HANDLETYPE hComponent);

private:

    OMX_ERRORTYPE initDCC(OMX_HANDLETYPE hComponent);
    OMX_ERRORTYPE sendDCCBufPtr(OMX_HANDLETYPE hComponent, CameraBuffer *dccBuffer);
    size_t readDCCdir(OMX_PTR buffer, const android::Vector<android::String8 *> &dirPaths);

private:

    static android::String8 DCCPath;
    static bool mDCCLoaded;
};

} // namespace Camera
} // namespace Ti

#endif // OMX_DCC_H
