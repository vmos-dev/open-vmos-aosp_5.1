/* Copyright (c) 2012-2013, The Linux Foundataion. All rights reserved.
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

#ifndef __QCAMERA_POSTPROC_H__
#define __QCAMERA_POSTPROC_H__

extern "C" {
#include <mm_camera_interface.h>
#include <mm_jpeg_interface.h>
}
#include "QCamera2HWI.h"

namespace qcamera {

class QCameraExif;

typedef struct {
    uint32_t jobId;                  // job ID
    uint32_t client_hdl;             // handle of jpeg client (obtained when open jpeg)
    mm_camera_super_buf_t *src_frame;// source frame (need to be returned back to kernel after done)
    mm_camera_super_buf_t *src_reproc_frame; // original source frame for reproc if not NULL
} qcamera_jpeg_data_t;

typedef struct {
    uint32_t jobId;                  // job ID
    mm_camera_super_buf_t *src_frame;// source frame (need to be returned back to kernel after done)
} qcamera_pp_data_t;

typedef struct {
    mm_camera_super_buf_t *frame;    // source frame that needs post process
} qcamera_pp_request_t;

typedef struct {
    uint32_t jobId;                  // job ID (obtained when start_jpeg_job)
    jpeg_job_status_t status;        // jpeg encoding status
    mm_jpeg_output_t out_data;         // ptr to jpeg output buf
} qcamera_jpeg_evt_payload_t;

typedef struct {
    camera_memory_t *        data;     // ptr to data memory struct
    mm_camera_super_buf_t *  frame;    // ptr to frame
} qcamera_release_data_t;

typedef struct {
    int32_t                  msg_type; // msg type of data notify
    camera_memory_t *        data;     // ptr to data memory struct
    unsigned int             index;    // index of the buf in the whole buffer
    camera_frame_metadata_t *metadata; // ptr to meta data
    qcamera_release_data_t   release_data; // any data needs to be release after notify
} qcamera_data_argm_t;

#define MAX_EXIF_TABLE_ENTRIES 17
class QCameraExif
{
public:
    QCameraExif();
    virtual ~QCameraExif();

    int32_t addEntry(exif_tag_id_t tagid,
                     exif_tag_type_t type,
                     uint32_t count,
                     void *data);
    uint32_t getNumOfEntries() {return m_nNumEntries;};
    QEXIF_INFO_DATA *getEntries() {return m_Entries;};

private:
    QEXIF_INFO_DATA m_Entries[MAX_EXIF_TABLE_ENTRIES];  // exif tags for JPEG encoder
    uint32_t  m_nNumEntries;                            // number of valid entries
};

class QCameraPostProcessor
{
public:
    QCameraPostProcessor(QCamera2HardwareInterface *cam_ctrl);
    virtual ~QCameraPostProcessor();

    int32_t init(jpeg_encode_callback_t jpeg_cb, void *user_data);
    int32_t deinit();
    int32_t start(QCameraChannel *pSrcChannel);
    int32_t stop();
    int32_t processData(mm_camera_super_buf_t *frame);
    int32_t processRawData(mm_camera_super_buf_t *frame);
    int32_t processPPData(mm_camera_super_buf_t *frame);
    int32_t processJpegEvt(qcamera_jpeg_evt_payload_t *evt);
    int32_t getJpegPaddingReq(cam_padding_info_t &padding_info);

private:
    int32_t sendDataNotify(int32_t msg_type,
                           camera_memory_t *data,
                           uint8_t index,
                           camera_frame_metadata_t *metadata,
                           qcamera_release_data_t *release_data);
    int32_t sendEvtNotify(int32_t msg_type, int32_t ext1, int32_t ext2);
    qcamera_jpeg_data_t *findJpegJobByJobId(uint32_t jobId);
    mm_jpeg_color_format getColorfmtFromImgFmt(cam_format_t img_fmt);
    mm_jpeg_format_t getJpegImgTypeFromImgFmt(cam_format_t img_fmt);
    int32_t getJpegEncodingConfig(mm_jpeg_encode_params_t& encode_parm,
                                  QCameraStream *main_stream,
                                  QCameraStream *thumb_stream);
    int32_t encodeData(qcamera_jpeg_data_t *jpeg_job_data,
                       uint8_t &needNewSess);
    void releaseSuperBuf(mm_camera_super_buf_t *super_buf);
    static void releaseNotifyData(void *user_data, void *cookie);
    void releaseJpegJobData(qcamera_jpeg_data_t *job);
    int32_t processRawImageImpl(mm_camera_super_buf_t *recvd_frame);

    static void releaseJpegData(void *data, void *user_data);
    static void releasePPInputData(void *data, void *user_data);
    static void releaseOngoingPPData(void *data, void *user_data);

    static void *dataProcessRoutine(void *data);

private:
    QCamera2HardwareInterface *m_parent;
    jpeg_encode_callback_t     mJpegCB;
    void *                     mJpegUserData;
    mm_jpeg_ops_t              mJpegHandle;
    uint32_t                   mJpegClientHandle;
    uint32_t                   mJpegSessionId;

    QCameraStreamMemory *      m_pJpegOutputMem;
    QCameraExif *              m_pJpegExifObj;
    int8_t                     m_bThumbnailNeeded;
    QCameraReprocessChannel *  m_pReprocChannel;

    QCameraQueue m_inputPPQ;            // input queue for postproc
    QCameraQueue m_ongoingPPQ;          // ongoing postproc queue
    QCameraQueue m_inputJpegQ;          // input jpeg job queue
    QCameraQueue m_ongoingJpegQ;        // ongoing jpeg job queue
    QCameraQueue m_inputRawQ;           // input raw job queue
    QCameraCmdThread m_dataProcTh;      // thread for data processing
};

}; // namespace qcamera

#endif /* __QCAMERA_POSTPROC_H__ */
