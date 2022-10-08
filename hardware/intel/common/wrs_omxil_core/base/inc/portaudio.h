/*
 * portaudio.h, port class for audio
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

#ifndef __PORTAUDIO_H
#define __PORTAUDIO_H

#include <OMX_Core.h>
#include <OMX_Component.h>

#include <portbase.h>

class PortAudio : public PortBase
{
public:
    PortAudio();

    OMX_ERRORTYPE SetPortAudioParam(
        const OMX_AUDIO_PARAM_PORTFORMATTYPE *audioparam,
        bool overwrite_readonly);
    const OMX_AUDIO_PARAM_PORTFORMATTYPE *GetPortAudioParam(void);

private:
    OMX_AUDIO_PARAM_PORTFORMATTYPE audioparam;
};

/* end of PortAudio */

class PortMp3 : public PortAudio
{
public:
    PortMp3();

    OMX_ERRORTYPE SetPortMp3Param(const OMX_AUDIO_PARAM_MP3TYPE *p,
                                  bool overwrite_readonly);
    const OMX_AUDIO_PARAM_MP3TYPE *GetPortMp3Param(void);

private:
    OMX_AUDIO_PARAM_MP3TYPE mp3param;
};

/* end of PortMp3 */

class PortAac : public PortAudio
{
public:
    PortAac();

    OMX_ERRORTYPE SetPortAacParam(const OMX_AUDIO_PARAM_AACPROFILETYPE *p,
                                  bool overwrite_readonly);
    const OMX_AUDIO_PARAM_AACPROFILETYPE *GetPortAacParam(void);

private:
    OMX_AUDIO_PARAM_AACPROFILETYPE aacparam;
};

/* end of PortAac */

class PortWma : public PortAudio
{
public:
    PortWma();

    OMX_ERRORTYPE SetPortWmaParam(const OMX_AUDIO_PARAM_WMATYPE *p,
                                  bool overwrite_readonly);
    const OMX_AUDIO_PARAM_WMATYPE *GetPortWmaParam(void);

private:
    OMX_AUDIO_PARAM_WMATYPE wmaparam;
};

/* end of PortWma */

class PortPcm : public PortAudio
{
public:
    PortPcm();

    OMX_ERRORTYPE SetPortPcmParam(const OMX_AUDIO_PARAM_PCMMODETYPE *p,
                                  bool overwrite_readonly);
    const OMX_AUDIO_PARAM_PCMMODETYPE*GetPortPcmParam(void);

private:
    OMX_AUDIO_PARAM_PCMMODETYPE pcmparam;
};

/* end of PortPcm */

class PortAmr : public PortAudio
{
public:
    PortAmr();

    OMX_ERRORTYPE SetPortAmrParam(const OMX_AUDIO_PARAM_AMRTYPE *p,
                                  bool overwrite_readonly);
    const OMX_AUDIO_PARAM_AMRTYPE*GetPortAmrParam(void);

private:
    OMX_AUDIO_PARAM_AMRTYPE amrparam;
};

/* end of PortAmr */


#endif /* __PORTAUDIO_H */
