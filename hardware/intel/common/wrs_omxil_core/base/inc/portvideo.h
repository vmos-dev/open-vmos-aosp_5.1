/*
 * portvideo.h, port class for video
 *
 * Copyright (c) 2009-2010 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __PORTVIDEO_H
#define __PORTVIDEO_H

#include <OMX_Core.h>
#include <OMX_Component.h>
#include <OMX_IntelVideoExt.h>
#include <portbase.h>

class PortVideo : public PortBase
{
public:
    PortVideo();
    //~PortVideo();

    OMX_ERRORTYPE SetPortVideoParam(
        const OMX_VIDEO_PARAM_PORTFORMATTYPE *videoparam, bool internal);
    const OMX_VIDEO_PARAM_PORTFORMATTYPE *GetPortVideoParam(void);

    OMX_ERRORTYPE SetPortBitrateParam(
        const OMX_VIDEO_PARAM_BITRATETYPE *bitrateparam, bool internal);
    const OMX_VIDEO_PARAM_BITRATETYPE *GetPortBitrateParam(void);
    
    OMX_ERRORTYPE SetPortPrivateInfoParam(
         const struct OMX_VIDEO_CONFIG_PRI_INFOTYPE *privateinfoparam, bool internal);
    const struct OMX_VIDEO_CONFIG_PRI_INFOTYPE *GetPortPrivateInfoParam(void);

    OMX_ERRORTYPE SetPortBufferSharingInfo(OMX_BOOL isbufsharing);
    const OMX_BOOL *GetPortBufferSharingInfo(void);


private:
    OMX_VIDEO_PARAM_PORTFORMATTYPE videoparam;

    OMX_VIDEO_PARAM_BITRATETYPE bitrateparam;
    
    struct OMX_VIDEO_CONFIG_PRI_INFOTYPE  privateinfoparam;

    OMX_BOOL mbufsharing;
};
/* end of PortVideo */
#if 0
class PortAvc : public PortVideo
{
public:
    PortAvc();

    OMX_ERRORTYPE SetPortAvcParam(const OMX_VIDEO_PARAM_AVCTYPE *p,
                                  bool overwrite_readonly);
    const OMX_VIDEO_PARAM_AVCTYPE *GetPortAvcParam(void);

private:
    OMX_VIDEO_PARAM_AVCTYPE avcparam;
};

/* end of PortAvc */
#endif
#endif /* __PORTVIDEO_H */
