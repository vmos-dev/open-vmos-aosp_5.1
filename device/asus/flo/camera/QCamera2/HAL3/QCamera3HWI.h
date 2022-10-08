/* Copyright (c) 2012-2013, 2015, The Linux Foundataion. All rights reserved.
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
    static void get_metadata_vendor_tag_ops(const struct camera3_device *,
                                               vendor_tag_query_ops_t* ops);
    static void dump(const struct camera3_device *, int fd);
    static int close_camera_device(struct hw_device_t* device);
public:
    QCamera3HardwareInterface(int cameraId);
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
    static void captureResultCb(mm_camera_super_buf_t *metadata,
                camera3_stream_buffer_t *buffer, uint32_t frame_number,
                void *userdata);

    int initialize(const camera3_callback_ops_t *callback_ops);
    int configureStreams(camera3_stream_configuration_t *stream_list);
    int registerStreamBuffers(const camera3_stream_buffer_set_t *buffer_set);
    int processCaptureRequest(camera3_capture_request_t *request);
    void getMetadataVendorTagOps(vendor_tag_query_ops_t* ops);
    void dump(int fd);

    int setFrameParameters(int frame_id, const camera_metadata_t *settings,
        uint32_t streamTypeMask, cam_trigger_t &aeTrigger);
    int translateMetadataToParameters(const camera_metadata_t *settings,
            cam_trigger_t &aeTrigger);
    camera_metadata_t* translateCbMetadataToResultMetadata(metadata_buffer_t *metadata,
                            nsecs_t timestamp, int32_t request_id,
                            const cam_trigger_t &aeTrigger);
    int getJpegSettings(const camera_metadata_t *settings);
    int initParameters();
    void deinitParameters();
    int getMaxUnmatchedFramesInQueue();
    QCamera3ReprocessChannel *addOnlineReprocChannel(QCamera3Channel *pInputChannel, QCamera3PicChannel *picChHandle);
    bool needRotationReprocess();
    bool needReprocess();
    bool isWNREnabled();
    cam_denoise_process_type_t getWaveletDenoiseProcessPlate();

    void captureResultCb(mm_camera_super_buf_t *metadata,
                camera3_stream_buffer_t *buffer, uint32_t frame_number);

    typedef struct {
        uint8_t fwk_name;
        uint8_t hal_name;
    } QCameraMap;

private:

    int openCamera();
    int closeCamera();
    int AddSetParmEntryToBatch(parm_buffer_t *p_table,
                               cam_intf_parm_type_t paramType,
                               uint32_t paramLength,
                               void *paramValue);
    static int8_t lookupHalName(const QCameraMap arr[],
                      int len, int fwk_name);
    static int8_t lookupFwkName(const QCameraMap arr[],
                      int len, int hal_name);

    int validateCaptureRequest(camera3_capture_request_t *request);

public:

    bool needOnlineRotation();
    void getThumbnailSize(cam_dimension_t &dim);
    int getJpegQuality();
    int calcMaxJpegSize();
    QCamera3Exif *getExifData();
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

     //First request yet to be processed after configureStreams
    bool mFirstRequest;
    QCamera3HeapMemory *mParamHeap;
    parm_buffer_t* mParameters;
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
        cam_trigger_t ae_trigger;
    } PendingRequestInfo;
    typedef KeyedVector<camera3_stream_t *, uint32_t> PendingBuffersMap;

    List<PendingRequestInfo> mPendingRequestsList;
    PendingBuffersMap mPendingBuffersMap;
    pthread_cond_t mRequestCond;
    int mPendingRequest;
    int32_t mCurrentRequestId;

    //mutex for serialized access to camera3_device_ops_t functions
    pthread_mutex_t mMutex;

    jpeg_settings_t* mJpegSettings;
    metadata_response_t mMetadataResponse;
    List<stream_info_t*> mStreamInfo;
    bool mIsZslMode;

    power_module_t *m_pPowerModule;   // power module

    int32_t mPrecaptureId;

    static const QCameraMap EFFECT_MODES_MAP[];
    static const QCameraMap WHITE_BALANCE_MODES_MAP[];
    static const QCameraMap SCENE_MODES_MAP[];
    static const QCameraMap FOCUS_MODES_MAP[];
    static const QCameraMap ANTIBANDING_MODES_MAP[];
    static const QCameraMap AE_FLASH_MODE_MAP[];
    static const QCameraMap FLASH_MODES_MAP[];

    static pthread_mutex_t mCameraSessionLock;
    static unsigned int mCameraSessionActive;
};

}; // namespace qcamera

#endif /* __QCAMERA2HARDWAREINTERFACE_H__ */
