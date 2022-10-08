/*
 * portaudio.cpp, port class for audio
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

#include <stdlib.h>
#include <string.h>

#include <OMX_Core.h>
#include <OMX_Component.h>

#include <componentbase.h>
#include <portaudio.h>

#define LOG_TAG "portaudio"
#include <log.h>

PortAudio::PortAudio()
{
    memset(&audioparam, 0, sizeof(audioparam));
    ComponentBase::SetTypeHeader(&audioparam, sizeof(audioparam));
}

OMX_ERRORTYPE PortAudio::SetPortAudioParam(
    const OMX_AUDIO_PARAM_PORTFORMATTYPE *p, bool overwrite_readonly)
{
    if (!overwrite_readonly) {
        OMX_ERRORTYPE ret;

        ret = ComponentBase::CheckTypeHeader((void *)p, sizeof(*p));
        if (ret != OMX_ErrorNone)
            return ret;
        if (audioparam.nPortIndex != p->nPortIndex)
            return OMX_ErrorBadPortIndex;
    }
    else
        audioparam.nPortIndex = p->nPortIndex;

    audioparam.nIndex = p->nIndex;
    audioparam.eEncoding = p->eEncoding;

    return OMX_ErrorNone;
}

const OMX_AUDIO_PARAM_PORTFORMATTYPE *PortAudio::GetPortAudioParam(void)
{
    return &audioparam;
}

/* end of PortAudio */

PortMp3::PortMp3()
{
    OMX_AUDIO_PARAM_PORTFORMATTYPE audioparam;

    memcpy(&audioparam, GetPortAudioParam(), sizeof(audioparam));
    audioparam.eEncoding = OMX_AUDIO_CodingMP3;
    SetPortAudioParam(&audioparam, false);

    memset(&mp3param, 0, sizeof(mp3param));
    ComponentBase::SetTypeHeader(&mp3param, sizeof(mp3param));
}

OMX_ERRORTYPE PortMp3::SetPortMp3Param(const OMX_AUDIO_PARAM_MP3TYPE *p,
                                       bool overwrite_readonly)
{
    if (!overwrite_readonly) {
        OMX_ERRORTYPE ret;

        ret = ComponentBase::CheckTypeHeader((void *)p, sizeof(*p));
        if (ret != OMX_ErrorNone)
            return ret;
        if (mp3param.nPortIndex != p->nPortIndex)
            return OMX_ErrorBadPortIndex;
    }
    else {
        OMX_AUDIO_PARAM_PORTFORMATTYPE audioparam;

        memcpy(&audioparam, GetPortAudioParam(), sizeof(audioparam));
        audioparam.nPortIndex = p->nPortIndex;
        SetPortAudioParam(&audioparam, true);

        mp3param.nPortIndex = p->nPortIndex;
    }

    mp3param.nChannels = p->nChannels;
    mp3param.nBitRate = p->nBitRate;
    mp3param.nSampleRate = p->nSampleRate;
    mp3param.nAudioBandWidth = p->nAudioBandWidth;
    mp3param.eChannelMode = p->eChannelMode;
    mp3param.eFormat = p->eFormat;

    return OMX_ErrorNone;
}

const OMX_AUDIO_PARAM_MP3TYPE *PortMp3::GetPortMp3Param(void)
{
    return &mp3param;
}

/* end of PortMp3 */

PortAac::PortAac()
{
    OMX_AUDIO_PARAM_PORTFORMATTYPE audioparam;

    memcpy(&audioparam, GetPortAudioParam(), sizeof(audioparam));
    audioparam.eEncoding = OMX_AUDIO_CodingAAC;
    SetPortAudioParam(&audioparam, false);

    memset(&aacparam, 0, sizeof(aacparam));
    ComponentBase::SetTypeHeader(&aacparam, sizeof(aacparam));
}

OMX_ERRORTYPE PortAac::SetPortAacParam(const OMX_AUDIO_PARAM_AACPROFILETYPE *p,
                                       bool overwrite_readonly)
{
    if (!overwrite_readonly) {
        OMX_ERRORTYPE ret;

        ret = ComponentBase::CheckTypeHeader((void *)p, sizeof(*p));
        if (ret != OMX_ErrorNone)
            return ret;
        if (aacparam.nPortIndex != p->nPortIndex)
            return OMX_ErrorBadPortIndex;
    }
    else {
        OMX_AUDIO_PARAM_PORTFORMATTYPE audioparam;

        memcpy(&audioparam, GetPortAudioParam(), sizeof(audioparam));
        audioparam.nPortIndex = p->nPortIndex;
        SetPortAudioParam(&audioparam, true);

        aacparam.nPortIndex = p->nPortIndex;
    }

    aacparam.nChannels = p->nChannels;
    aacparam.nBitRate = p->nBitRate;
    aacparam.nSampleRate = p->nSampleRate;
    aacparam.nAudioBandWidth = p->nAudioBandWidth;
    aacparam.nFrameLength = p->nFrameLength;
    aacparam.nAACtools = p->nAACtools;
    aacparam.nAACERtools = p->nAACERtools;
    aacparam.eAACProfile = p->eAACProfile;
    aacparam.eAACStreamFormat = p->eAACStreamFormat;
    aacparam.eChannelMode = p->eChannelMode;

    return OMX_ErrorNone;
}

const OMX_AUDIO_PARAM_AACPROFILETYPE *PortAac::GetPortAacParam(void)
{
    return &aacparam;
}

/* end of PortAac */

PortWma::PortWma()
{
    OMX_AUDIO_PARAM_PORTFORMATTYPE audioparam;

    memcpy(&audioparam, GetPortAudioParam(), sizeof(audioparam));
    audioparam.eEncoding = OMX_AUDIO_CodingWMA;
    SetPortAudioParam(&audioparam, false);

    memset(&wmaparam, 0, sizeof(wmaparam));
    ComponentBase::SetTypeHeader(&wmaparam, sizeof(wmaparam));
}

OMX_ERRORTYPE PortWma::SetPortWmaParam(const OMX_AUDIO_PARAM_WMATYPE *p,
                                       bool overwrite_readonly)
{
    if (!overwrite_readonly) {
        OMX_ERRORTYPE ret;

        ret = ComponentBase::CheckTypeHeader((void *)p, sizeof(*p));
        if (ret != OMX_ErrorNone)
            return ret;
        if (wmaparam.nPortIndex != p->nPortIndex)
            return OMX_ErrorBadPortIndex;
    }
    else {
        OMX_AUDIO_PARAM_PORTFORMATTYPE audioparam;

        memcpy(&audioparam, GetPortAudioParam(), sizeof(audioparam));
        audioparam.nPortIndex = p->nPortIndex;
        SetPortAudioParam(&audioparam, true);

        wmaparam.nPortIndex = p->nPortIndex;
    }

    wmaparam.nChannels = p->nChannels;
    wmaparam.nBitRate = p->nBitRate;
    wmaparam.eFormat = p->eFormat;
    wmaparam.eProfile = p->eProfile;
    wmaparam.nSamplingRate = p->nSamplingRate;
    wmaparam.nBlockAlign = p->nBlockAlign;
    wmaparam.nEncodeOptions = p->nEncodeOptions;
    wmaparam.nSuperBlockAlign = p->nSuperBlockAlign;

    return OMX_ErrorNone;
}

const OMX_AUDIO_PARAM_WMATYPE *PortWma::GetPortWmaParam(void)
{
    return &wmaparam;
}

/* end of PortWma */

PortPcm::PortPcm()
{
    OMX_AUDIO_PARAM_PORTFORMATTYPE audioparam;

    memcpy(&audioparam, GetPortAudioParam(), sizeof(audioparam));
    audioparam.eEncoding = OMX_AUDIO_CodingPCM;
    SetPortAudioParam(&audioparam, false);

    memset(&pcmparam, 0, sizeof(pcmparam));
    ComponentBase::SetTypeHeader(&pcmparam, sizeof(pcmparam));
}

OMX_ERRORTYPE PortPcm::SetPortPcmParam(const OMX_AUDIO_PARAM_PCMMODETYPE *p,
                                       bool overwrite_readonly)
{
    if (!overwrite_readonly) {
        OMX_ERRORTYPE ret;

        ret = ComponentBase::CheckTypeHeader((void *)p, sizeof(*p));
        if (ret != OMX_ErrorNone)
            return ret;
        if (pcmparam.nPortIndex != p->nPortIndex)
            return OMX_ErrorBadPortIndex;
    }
    else {
        OMX_AUDIO_PARAM_PORTFORMATTYPE audioparam;

        memcpy(&audioparam, GetPortAudioParam(), sizeof(audioparam));
        audioparam.nPortIndex = p->nPortIndex;
        SetPortAudioParam(&audioparam, true);

        pcmparam.nPortIndex = p->nPortIndex;
    }

    pcmparam.nChannels = p->nChannels;
    pcmparam.eNumData = p->eNumData;
    pcmparam.eEndian = p->eEndian;
    pcmparam.bInterleaved = p->bInterleaved;
    pcmparam.nBitPerSample = p->nBitPerSample;
    pcmparam.nSamplingRate = p->nSamplingRate;
    pcmparam.ePCMMode = p->ePCMMode;
    memcpy(&pcmparam.eChannelMapping[0], &p->eChannelMapping[0],
           sizeof(OMX_U32) * p->nChannels);

    return OMX_ErrorNone;
}

const OMX_AUDIO_PARAM_PCMMODETYPE *PortPcm::GetPortPcmParam(void)
{
    return &pcmparam;
}

/* end of PortPcm */

PortAmr::PortAmr()
{
    OMX_AUDIO_PARAM_PORTFORMATTYPE audioparam;

    memcpy(&audioparam, GetPortAudioParam(), sizeof(audioparam));
    audioparam.eEncoding = OMX_AUDIO_CodingAMR;
    SetPortAudioParam(&audioparam, false);

    memset(&amrparam, 0, sizeof(amrparam));
    ComponentBase::SetTypeHeader(&amrparam, sizeof(amrparam));
}

OMX_ERRORTYPE PortAmr::SetPortAmrParam(const OMX_AUDIO_PARAM_AMRTYPE *p,
                                       bool overwrite_readonly)
{
    if (!overwrite_readonly) {
        OMX_ERRORTYPE ret;

        ret = ComponentBase::CheckTypeHeader((void *)p, sizeof(*p));
        if (ret != OMX_ErrorNone)
            return ret;
        if (amrparam.nPortIndex != p->nPortIndex)
            return OMX_ErrorBadPortIndex;
    }
    else {
        OMX_AUDIO_PARAM_PORTFORMATTYPE audioparam;

        memcpy(&audioparam, GetPortAudioParam(), sizeof(audioparam));
        audioparam.nPortIndex = p->nPortIndex;
        SetPortAudioParam(&audioparam, true);

        amrparam.nPortIndex = p->nPortIndex;
    }

    amrparam.nChannels       = p->nChannels;
    amrparam.nBitRate        = p->nBitRate;
    amrparam.eAMRBandMode    = p->eAMRBandMode;
    amrparam.eAMRDTXMode     = p->eAMRDTXMode;
    amrparam.eAMRFrameFormat = p->eAMRFrameFormat;

    return OMX_ErrorNone;
}

const OMX_AUDIO_PARAM_AMRTYPE *PortAmr::GetPortAmrParam(void)
{
    return &amrparam;
}

/* end of PortAmr */
