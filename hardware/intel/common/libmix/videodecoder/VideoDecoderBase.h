/*
* Copyright (c) 2009-2011 Intel Corporation.  All rights reserved.
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

#ifndef VIDEO_DECODER_BASE_H_
#define VIDEO_DECODER_BASE_H_

#include <va/va.h>
#include <va/va_tpi.h>
#include "VideoDecoderDefs.h"
#include "VideoDecoderInterface.h"
#include <pthread.h>
#include <dlfcn.h>

extern "C" {
#include "vbp_loader.h"
}

#ifndef Display
#ifdef USE_GEN_HW
typedef char Display;
#else
typedef unsigned int Display;
#endif
#endif

// TODO: check what is the best number. Must be at least 2 to support one backward reference frame.
// Currently set to 8 to support 7 backward reference frames. This value is used for AVC frame reordering only.
// e.g:
// POC: 4P,  8P,  10P,  6B and mNextOutputPOC = 5
#define OUTPUT_WINDOW_SIZE 8

class VideoDecoderBase : public IVideoDecoder {
public:
    VideoDecoderBase(const char *mimeType, _vbp_parser_type type);
    virtual ~VideoDecoderBase();

    virtual Decode_Status start(VideoConfigBuffer *buffer);
    virtual Decode_Status reset(VideoConfigBuffer *buffer) ;
    virtual void stop(void);
    //virtual Decode_Status decode(VideoDecodeBuffer *buffer);
    virtual void flush(void);
    virtual void freeSurfaceBuffers(void);
    virtual const VideoRenderBuffer* getOutput(bool draining = false, VideoErrorBuffer *output_buf = NULL);
    virtual Decode_Status signalRenderDone(void * graphichandler);
    virtual const VideoFormatInfo* getFormatInfo(void);
    virtual bool checkBufferAvail();
    virtual void enableErrorReport(bool enabled = false) {mErrReportEnabled = enabled; };

protected:
    // each acquireSurfaceBuffer must be followed by a corresponding outputSurfaceBuffer or releaseSurfaceBuffer.
    // Only one surface buffer can be acquired at any given time
    virtual Decode_Status acquireSurfaceBuffer(void);
    // frame is successfully decoded to the acquired surface buffer and surface is ready for output
    virtual Decode_Status outputSurfaceBuffer(void);
    // acquired surface  buffer is not used
    virtual Decode_Status releaseSurfaceBuffer(void);
    // flush all decoded but not rendered buffers
    virtual void flushSurfaceBuffers(void);
    virtual Decode_Status endDecodingFrame(bool dropFrame);
    virtual VideoSurfaceBuffer* findOutputByPoc(bool draining = false);
    virtual VideoSurfaceBuffer* findOutputByPct(bool draining = false);
    virtual VideoSurfaceBuffer* findOutputByPts();
    virtual Decode_Status setupVA(uint32_t numSurface, VAProfile profile, uint32_t numExtraSurface = 0);
    virtual Decode_Status terminateVA(void);
    virtual Decode_Status parseBuffer(uint8_t *buffer, int32_t size, bool config, void** vbpData);

    static inline uint32_t alignMB(uint32_t a) {
         return ((a + 15) & (~15));
    }

    virtual Decode_Status getRawDataFromSurface(VideoRenderBuffer *renderBuffer = NULL, uint8_t *pRawData = NULL, uint32_t *pSize = NULL, bool internal = true);

#if (defined USE_AVC_SHORT_FORMAT) || (defined USE_SLICE_HEADER_PARSING)
    Decode_Status updateBuffer(uint8_t *buffer, int32_t size, void** vbpData);
    Decode_Status queryBuffer(void **vbpData);
    Decode_Status setParserType(_vbp_parser_type type);
    virtual Decode_Status getCodecSpecificConfigs(VAProfile profile, VAConfigID *config);
#endif
    virtual Decode_Status checkHardwareCapability();
private:
    Decode_Status mapSurface(void);
    void initSurfaceBuffer(bool reset);
    void drainDecodingErrors(VideoErrorBuffer *outErrBuf, VideoRenderBuffer *currentSurface);
    void fillDecodingErrors(VideoRenderBuffer *currentSurface);

    bool mInitialized;
    pthread_mutex_t mLock;

protected:
    bool mLowDelay; // when true, decoded frame is immediately output for rendering
    VideoFormatInfo mVideoFormatInfo;
    Display *mDisplay;
    VADisplay mVADisplay;
    VAContextID mVAContext;
    VAConfigID mVAConfig;
    VASurfaceID *mExtraSurfaces; // extra surfaces array
    int32_t mNumExtraSurfaces;
    bool mVAStarted;
    uint64_t mCurrentPTS; // current presentation time stamp (unit is unknown, depend on the framework: GStreamer 100-nanosec, Android: microsecond)
    // the following three member variables should be set using
    // acquireSurfaceBuffer/outputSurfaceBuffer/releaseSurfaceBuffer
    VideoSurfaceBuffer *mAcquiredBuffer;
    VideoSurfaceBuffer *mLastReference;
    VideoSurfaceBuffer *mForwardReference;
    VideoConfigBuffer  mConfigBuffer; // only store configure meta data.
    bool mDecodingFrame; // indicate whether a frame is being decoded
    bool mSizeChanged; // indicate whether video size is changed.
    bool mShowFrame; // indicate whether the decoded frame is for display

    int32_t mOutputWindowSize; // indicate limit of number of outstanding frames for output
    int32_t mRotationDegrees;

    bool mErrReportEnabled;
    bool mWiDiOn;
    typedef uint32_t (*OpenFunc)(uint32_t, void **);
    typedef uint32_t (*CloseFunc)(void *);
    typedef uint32_t (*ParseFunc)(void *, uint8_t *, uint32_t, uint8_t);
    typedef uint32_t (*QueryFunc)(void *, void **);
    typedef uint32_t (*FlushFunc)(void *);
    typedef uint32_t (*UpdateFunc)(void *, void *, uint32_t, void **);
    void *mLibHandle;
    OpenFunc mParserOpen;
    CloseFunc mParserClose;
    ParseFunc mParserParse;
    QueryFunc mParserQuery;
    FlushFunc mParserFlush;
    UpdateFunc mParserUpdate;
    enum {
        // TODO: move this to vbp_loader.h
        VBP_INVALID = 0xFF,
        // TODO: move this to va.h
        VAProfileSoftwareDecoding = 0xFF,
    };

    enum OUTPUT_METHOD {
        // output by Picture Coding Type (I, P, B)
         OUTPUT_BY_PCT,
        // output by Picture Order Count (for AVC only)
         OUTPUT_BY_POC,
         //OUTPUT_BY_POS,
         //OUTPUT_BY_PTS,
     };

private:
    bool mRawOutput; // whether to output NV12 raw data
    bool mManageReference;  // this should stay true for VC1/MP4 decoder, and stay false for AVC decoder. AVC  handles reference frame using DPB
    OUTPUT_METHOD mOutputMethod;

    int32_t mNumSurfaces;
    VideoSurfaceBuffer *mSurfaceBuffers;
    VideoSurfaceBuffer *mOutputHead; // head of output buffer list
    VideoSurfaceBuffer *mOutputTail;  // tail of output buffer list
    VASurfaceID *mSurfaces; // surfaces array
    VASurfaceAttribExternalBuffers *mVASurfaceAttrib;
    uint8_t **mSurfaceUserPtr; // mapped user space pointer
    int32_t mSurfaceAcquirePos; // position of surface to start acquiring
    int32_t mNextOutputPOC; // Picture order count of next output
    _vbp_parser_type mParserType;
    void *mParserHandle;
    void *mSignalBufferPre[MAX_GRAPHIC_BUFFER_NUM];
    uint32 mSignalBufferSize;
    bool mUseGEN;
protected:
    void ManageReference(bool enable) {mManageReference = enable;}
    void setOutputMethod(OUTPUT_METHOD method) {mOutputMethod = method;}
    void setOutputWindowSize(int32_t size) {mOutputWindowSize = (size < OUTPUT_WINDOW_SIZE) ? size : OUTPUT_WINDOW_SIZE;}
    void querySurfaceRenderStatus(VideoSurfaceBuffer* surface);
    void enableLowDelayMode(bool enable) {mLowDelay = enable;}
    void setRotationDegrees(int32_t rotationDegrees);
    void setRenderRect(void);
};


#endif  // VIDEO_DECODER_BASE_H_
