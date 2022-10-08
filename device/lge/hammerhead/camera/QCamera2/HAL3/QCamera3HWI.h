/* Copyright (c) 2012-2014, The Linux Foundataion. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above
*       copyright notice, this list of conditions and the following
*       disclaimer in the documentation and/or other materials provided
*       with the distribution.
*     * Neither the name of The Linux Foundation nor the names of its
*       contributors may be used to endorse or promote products derived
*       from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
* ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
* OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
* IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/

#ifndef __QCAMERA3HARDWAREINTERFACE_H__
#define __QCAMERA3HARDWAREINTERFACE_H__

#include <pthread.h>
#include <utils/List.h>
#include <utils/KeyedVector.h>
#include <hardware/camera3.h>
#include <camera/CameraMetadata.h>
#include "QCamera3HALHeader.h"
#include "QCamera3Channel.h"

#include <hardware/power.h>

extern "C" {
#include <mm_camera_interface.h>
#include <mm_jpeg_interface.h>
}

using namespace android;

namespace qcamera {

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/* Time related macros */
typedef int64_t nsecs_t;
#define NSEC_PER_SEC 1000000000LL
#define NSEC_PER_USEC 1000
#define NSEC_PER_33MSEC 33000000LL

class QCamera3MetadataChannel;
class QCamera3PicChannel;
class QCamera3HeapMemory;
class QCamera3Exif;

typedef struct {
    camera3_stream_t *stream;
    stream_status_t status;
    QCamera3Channel *channel;
} stream_info_t;

class QCamera3HardwareInterface {
public:
    /* static variable and functions accessed by camera service */
    static camera3_device_ops_t mCameraOps;
    static int initialize(const struct camera3_device *,
                const camera3_callback_ops_t *callback_ops);
    static int configure_streams(const struct camera3_device *,
                camera3_stream_configuration_t *stream_list);
    static int register_stream_buffers(const struct camera3_device *,
                const camera3_stream_buffer_set_t *buffer_set);
    static const camera_metadata_t* construct_default_request_settings(
                                const struct camera3_device *, int type);
    static int process_capture_request(const struct camera3_device *,
                                camera3_capture_request_t *request);

    static void dump(const struct camera3_device *, int fd);
    static int flush(const struct camera3_device *);
    static int close_camera_device(struct hw_device_t* device);

public:
    QCamera3HardwareInterface(int cameraId,
            const camera_module_callbacks_t *callbacks);
    virtual ~QCamera3HardwareInterface();
    int openCamera(struct hw_device_t **hw_device);
    int getMetadata(int type);
    camera_metadata_t* translateCapabilityToMetadata(int type);

    static int getCamInfo(int cameraId, struct camera_info *info);
    static int initCapabilities(int cameraId);
    static int initStaticMetadata(int cameraId);
    static void makeTable(cam_dimension_t* dimTable, uint8_t size, int32_t* sizeTable);
    static void makeFPSTable(cam_fps_range_t* fpsTable, uint8_t size,
                                          int32_t* fpsRangesTable);
    static void makeOverridesList(cam_scene_mode_overrides_t* overridesTable, uint8_t size,
                                   uint8_t* overridesList, uint8_t* supported_indexes, int camera_id);
    static void convertToRegions(cam_rect_t rect, int32_t* region, int weight);
    static void convertFromRegions(cam_area_t* roi, const camera_metadata_t *settings,
                                   uint32_t tag);
    static bool resetIfNeededROI(cam_area_t* roi, const cam_crop_region_t* scalerCropRegion);
    static void convertLandmarks(cam_face_detection_info_t face, int32_t* landmarks);
    static void postproc_channel_cb_routine(mm_camera_super_buf_t *recvd_frame,
                                            void *userdata);
    static int32_t getScalarFormat(int32_t format);
    static int32_t getSensorSensitivity(int32_t iso_mode);

    static double computeNoiseModelEntryS(int32_t sensitivity);
    static double computeNoiseModelEntryO(int32_t sensitivity);

    static void captureResultCb(mm_camera_super_buf_t *metadata,
                camera3_stream_buffer_t *buffer, uint32_t frame_number,
                void *userdata);

    int initialize(const camera3_callback_ops_t *callback_ops);
    int configureStreams(camera3_stream_configuration_t *stream_list);
    int registerStreamBuffers(const camera3_stream_buffer_set_t *buffer_set);
    int processCaptureRequest(camera3_capture_request_t *request);
    void dump(int fd);
    int flush();

    int setFrameParameters(camera3_capture_request_t *request,
            cam_stream_ID_t streamID);
    int setReprocParameters(camera3_capture_request_t *request);
    int translateToHalMetadata(const camera3_capture_request_t *request,
            metadata_buffer_t *parm);
    camera_metadata_t* translateCbUrgentMetadataToResultMetadata (
                             metadata_buffer_t *metadata);

    camera_metadata_t* translateFromHalMetadata(metadata_buffer_t *metadata,
                            nsecs_t timestamp, int32_t request_id,
                            const CameraMetadata& jpegMetadata, uint8_t pipeline_depth);
    int getJpegSettings(const camera_metadata_t *settings);
    int initParameters();
    void deinitParameters();
    QCamera3ReprocessChannel *addOfflineReprocChannel(QCamera3Channel *pInputChannel, QCamera3PicChannel *picChHandle, metadata_buffer_t *metadata);
    bool needRotationReprocess();
    bool needReprocess();
    bool isWNREnabled();
    cam_denoise_process_type_t getWaveletDenoiseProcessPlate();

    void captureResultCb(mm_camera_super_buf_t *metadata,
                camera3_stream_buffer_t *buffer, uint32_t frame_number);

    typedef struct {
        uint32_t fwk_name;
        uint8_t hal_name;
    } QCameraMap;

private:

    int openCamera();
    int closeCamera();
    int AddSetMetaEntryToBatch(metadata_buffer_t *p_table,
                               unsigned int paramType,
                               uint32_t paramLength,
                               void *paramValue);
    static int8_t lookupHalName(const QCameraMap arr[],
                      int len, unsigned int fwk_name);
    static int32_t lookupFwkName(const QCameraMap arr[],
                      int len, int hal_name);

    int validateCaptureRequest(camera3_capture_request_t *request);
    int validateStreamDimensions(camera3_stream_configuration_t *streamList);
    void deriveMinFrameDuration();
    int64_t getMinFrameDuration(const camera3_capture_request_t *request);

    void handleMetadataWithLock(mm_camera_super_buf_t *metadata_buf);
    void handleBufferWithLock(camera3_stream_buffer_t *buffer,
        uint32_t frame_number);
    void unblockRequestIfNecessary();
    void dumpMetadataToFile(tuning_params_t &meta,
                            uint32_t &dumpFrameCount,
                            int32_t enabled,
                            const char *type,
                            uint32_t frameNumber);

    void cleanAndSortStreamInfo();
    void extractJpegMetadata(CameraMetadata& jpegMetadata,
            const camera3_capture_request_t *request);
public:

    bool needOnlineRotation();
    int getJpegQuality();
    int calcMaxJpegSize();
    QCamera3Exif *getExifData();
public:
    static int kMaxInFlight;
private:
    camera3_device_t   mCameraDevice;
    uint8_t            mCameraId;
    mm_camera_vtbl_t  *mCameraHandle;
    bool               mCameraOpened;
    bool               mCameraInitialized;
    camera_metadata_t *mDefaultMetadata[CAMERA3_TEMPLATE_COUNT];
    int mBlobRequest;

    const camera3_callback_ops_t *mCallbackOps;

    camera3_stream_t *mInputStream;
    QCamera3MetadataChannel *mMetadataChannel;
    QCamera3PicChannel *mPictureChannel;
    QCamera3RawChannel *mRawChannel;
    QCamera3SupportChannel *mSupportChannel;

     //First request yet to be processed after configureStreams
    bool mFirstRequest;
    bool mRepeatingRequest;
    QCamera3HeapMemory *mParamHeap;
    metadata_buffer_t* mParameters;
    metadata_buffer_t* mPrevParameters;
    bool m_bWNROn;

    /* Data structure to store pending request */
    typedef struct {
        camera3_stream_t *stream;
        camera3_stream_buffer_t *buffer;
    } RequestedBufferInfo;
    typedef struct {
        uint32_t frame_number;
        uint32_t num_buffers;
        int32_t request_id;
        List<RequestedBufferInfo> buffers;
        int blob_request;
        nsecs_t timestamp;
        uint8_t bNotified;
        int input_buffer_present;
        CameraMetadata jpegMetadata;
        uint8_t pipeline_depth;
        uint32_t partial_result_cnt;
    } PendingRequestInfo;
    typedef struct {
        uint32_t frame_number;
        uint32_t stream_ID;
    } PendingFrameDropInfo;

    // Store the Pending buffers for Flushing
    typedef struct {
        // Frame number pertaining to the buffer
        uint32_t frame_number;
        camera3_stream_t *stream;
        // Buffer handle
        buffer_handle_t *buffer;
    } PendingBufferInfo;

    typedef struct {
        // Total number of buffer requests pending
        uint32_t num_buffers;
        // List of pending buffers
        List<PendingBufferInfo> mPendingBufferList;
    } PendingBuffersMap;

    typedef KeyedVector<uint32_t, Vector<PendingBufferInfo> > FlushMap;

    List<PendingRequestInfo> mPendingRequestsList;
    List<PendingFrameDropInfo> mPendingFrameDropList;
    PendingBuffersMap mPendingBuffersMap;
    pthread_cond_t mRequestCond;
    int mPendingRequest;
    int32_t mCurrentRequestId;
    camera3_capture_result_t *mLoopBackResult;
    nsecs_t mLoopBackTimestamp;
    bool mFlush;

    //mutex for serialized access to camera3_device_ops_t functions
    pthread_mutex_t mMutex;

    List<stream_info_t*> mStreamInfo;

    int64_t mMinProcessedFrameDuration;
    int64_t mMinJpegFrameDuration;
    int64_t mMinRawFrameDuration;

    power_module_t *m_pPowerModule;   // power module

#ifdef HAS_MULTIMEDIA_HINTS
    bool mHdrHint;
#endif
    uint32_t mMetaFrameCount;
    const camera_module_callbacks_t *mCallbacks;

    static const QCameraMap EFFECT_MODES_MAP[];
    static const QCameraMap WHITE_BALANCE_MODES_MAP[];
    static const QCameraMap SCENE_MODES_MAP[];
    static const QCameraMap FOCUS_MODES_MAP[];
    static const QCameraMap ANTIBANDING_MODES_MAP[];
    static const QCameraMap AE_FLASH_MODE_MAP[];
    static const QCameraMap FLASH_MODES_MAP[];
    static const QCameraMap FACEDETECT_MODES_MAP[];
    static const QCameraMap FOCUS_CALIBRATION_MAP[];
    static const QCameraMap TEST_PATTERN_MAP[];
    static const QCameraMap REFERENCE_ILLUMINANT_MAP[];

    static pthread_mutex_t mCameraSessionLock;
    static unsigned int mCameraSessionActive;
};

}; // namespace qcamera

#endif /* __QCAMERA2HARDWAREINTERFACE_H__ */
