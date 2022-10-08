/*
**
** Copyright 2008, The Android Open Source Project
** Copyright 2012, Samsung Electronics Co. LTD
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

/*!
 * \file      ExynosCameraHWInterface2.cpp
 * \brief     source file for Android Camera API 2.0 HAL
 * \author    Sungjoong Kang(sj3.kang@samsung.com)
 * \date      2012/07/10
 *
 * <b>Revision History: </b>
 * - 2012/05/31 : Sungjoong Kang(sj3.kang@samsung.com) \n
 *   Initial Release
 *
 * - 2012/07/10 : Sungjoong Kang(sj3.kang@samsung.com) \n
 *   2nd Release
 *
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "ExynosCameraHAL2"
#include <sys/time.h>
#include <utils/Log.h>
#include <math.h>

#include "ExynosCameraHWInterface2.h"
#include "exynos_format.h"

namespace android {

void m_savePostView(const char *fname, uint8_t *buf, uint32_t size)
{
    int nw;
    int cnt = 0;
    uint32_t written = 0;

    ALOGV("opening file [%s], address[%x], size(%d)", fname, (unsigned int)buf, size);
    int fd = open(fname, O_RDWR | O_CREAT, 0644);
    if (fd < 0) {
        ALOGE("failed to create file [%s]: %s", fname, strerror(errno));
        return;
    }

    ALOGV("writing %d bytes to file [%s]", size, fname);
    while (written < size) {
        nw = ::write(fd, buf + written, size - written);
        if (nw < 0) {
            ALOGE("failed to write to file %d [%s]: %s",written,fname, strerror(errno));
            break;
        }
        written += nw;
        cnt++;
    }
    ALOGV("done writing %d bytes to file [%s] in %d passes",size, fname, cnt);
    ::close(fd);
}

int get_pixel_depth(uint32_t fmt)
{
    int depth = 0;

    switch (fmt) {
    case V4L2_PIX_FMT_JPEG:
        depth = 8;
        break;

    case V4L2_PIX_FMT_NV12:
    case V4L2_PIX_FMT_NV21:
    case V4L2_PIX_FMT_YUV420:
    case V4L2_PIX_FMT_YVU420M:
    case V4L2_PIX_FMT_NV12M:
    case V4L2_PIX_FMT_NV12MT:
        depth = 12;
        break;

    case V4L2_PIX_FMT_RGB565:
    case V4L2_PIX_FMT_YUYV:
    case V4L2_PIX_FMT_YVYU:
    case V4L2_PIX_FMT_UYVY:
    case V4L2_PIX_FMT_VYUY:
    case V4L2_PIX_FMT_NV16:
    case V4L2_PIX_FMT_NV61:
    case V4L2_PIX_FMT_YUV422P:
    case V4L2_PIX_FMT_SBGGR10:
    case V4L2_PIX_FMT_SBGGR12:
    case V4L2_PIX_FMT_SBGGR16:
        depth = 16;
        break;

    case V4L2_PIX_FMT_RGB32:
        depth = 32;
        break;
    default:
        ALOGE("Get depth failed(format : %d)", fmt);
        break;
    }

    return depth;
}

int cam_int_s_fmt(node_info_t *node)
{
    struct v4l2_format v4l2_fmt;
    unsigned int framesize;
    int ret;

    memset(&v4l2_fmt, 0, sizeof(struct v4l2_format));

    v4l2_fmt.type = node->type;
    framesize = (node->width * node->height * get_pixel_depth(node->format)) / 8;

    if (node->planes >= 1) {
        v4l2_fmt.fmt.pix_mp.width       = node->width;
        v4l2_fmt.fmt.pix_mp.height      = node->height;
        v4l2_fmt.fmt.pix_mp.pixelformat = node->format;
        v4l2_fmt.fmt.pix_mp.field       = V4L2_FIELD_ANY;
    } else {
        ALOGE("%s:S_FMT, Out of bound : Number of element plane",__FUNCTION__);
    }

    /* Set up for capture */
    ret = exynos_v4l2_s_fmt(node->fd, &v4l2_fmt);

    if (ret < 0)
        ALOGE("%s: exynos_v4l2_s_fmt fail (%d)",__FUNCTION__, ret);


    return ret;
}

int cam_int_reqbufs(node_info_t *node)
{
    struct v4l2_requestbuffers req;
    int ret;

    req.count = node->buffers;
    req.type = node->type;
    req.memory = node->memory;

    ret = exynos_v4l2_reqbufs(node->fd, &req);

    if (ret < 0)
        ALOGE("%s: VIDIOC_REQBUFS (fd:%d) failed (%d)",__FUNCTION__,node->fd, ret);

    return req.count;
}

int cam_int_qbuf(node_info_t *node, int index)
{
    struct v4l2_buffer v4l2_buf;
    struct v4l2_plane planes[VIDEO_MAX_PLANES];
    int i;
    int ret = 0;

    v4l2_buf.m.planes   = planes;
    v4l2_buf.type       = node->type;
    v4l2_buf.memory     = node->memory;
    v4l2_buf.index      = index;
    v4l2_buf.length     = node->planes;

    for(i = 0; i < node->planes; i++){
        v4l2_buf.m.planes[i].m.fd = (int)(node->buffer[index].fd.extFd[i]);
        v4l2_buf.m.planes[i].length  = (unsigned long)(node->buffer[index].size.extS[i]);
    }

    ret = exynos_v4l2_qbuf(node->fd, &v4l2_buf);

    if (ret < 0)
        ALOGE("%s: cam_int_qbuf failed (index:%d)(ret:%d)",__FUNCTION__, index, ret);

    return ret;
}

int cam_int_streamon(node_info_t *node)
{
    enum v4l2_buf_type type = node->type;
    int ret;


    ret = exynos_v4l2_streamon(node->fd, type);

    if (ret < 0)
        ALOGE("%s: VIDIOC_STREAMON failed [%d] (%d)",__FUNCTION__, node->fd,ret);

    ALOGV("On streaming I/O... ... fd(%d)", node->fd);

    return ret;
}

int cam_int_streamoff(node_info_t *node)
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    int ret;


    ALOGV("Off streaming I/O... fd(%d)", node->fd);
    ret = exynos_v4l2_streamoff(node->fd, type);

    if (ret < 0)
        ALOGE("%s: VIDIOC_STREAMOFF failed (%d)",__FUNCTION__, ret);

    return ret;
}

int isp_int_streamoff(node_info_t *node)
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    int ret;

    ALOGV("Off streaming I/O... fd(%d)", node->fd);
    ret = exynos_v4l2_streamoff(node->fd, type);

    if (ret < 0)
        ALOGE("%s: VIDIOC_STREAMOFF failed (%d)",__FUNCTION__, ret);

    return ret;
}

int cam_int_dqbuf(node_info_t *node)
{
    struct v4l2_buffer v4l2_buf;
    struct v4l2_plane planes[VIDEO_MAX_PLANES];
    int ret;

    v4l2_buf.type       = node->type;
    v4l2_buf.memory     = node->memory;
    v4l2_buf.m.planes   = planes;
    v4l2_buf.length     = node->planes;

    ret = exynos_v4l2_dqbuf(node->fd, &v4l2_buf);
    if (ret < 0)
        ALOGE("%s: VIDIOC_DQBUF failed (%d)",__FUNCTION__, ret);

    return v4l2_buf.index;
}

int cam_int_dqbuf(node_info_t *node, int num_plane)
{
    struct v4l2_buffer v4l2_buf;
    struct v4l2_plane planes[VIDEO_MAX_PLANES];
    int ret;

    v4l2_buf.type       = node->type;
    v4l2_buf.memory     = node->memory;
    v4l2_buf.m.planes   = planes;
    v4l2_buf.length     = num_plane;

    ret = exynos_v4l2_dqbuf(node->fd, &v4l2_buf);
    if (ret < 0)
        ALOGE("%s: VIDIOC_DQBUF failed (%d)",__FUNCTION__, ret);

    return v4l2_buf.index;
}

int cam_int_s_input(node_info_t *node, int index)
{
    int ret;

    ret = exynos_v4l2_s_input(node->fd, index);
    if (ret < 0)
        ALOGE("%s: VIDIOC_S_INPUT failed (%d)",__FUNCTION__, ret);

    return ret;
}


gralloc_module_t const* ExynosCameraHWInterface2::m_grallocHal;

RequestManager::RequestManager(SignalDrivenThread* main_thread):
    m_vdisEnable(false),
    m_lastCompletedFrameCnt(-1),
    m_lastAeMode(0),
    m_lastAaMode(0),
    m_lastAwbMode(0),
    m_lastAeComp(0),
    m_vdisBubbleEn(false)
{
    m_metadataConverter = new MetadataConverter;
    m_mainThread = main_thread;
    ResetEntry();
    m_sensorPipelineSkipCnt = 0;
    return;
}

RequestManager::~RequestManager()
{
    ALOGV("%s", __FUNCTION__);
    if (m_metadataConverter != NULL) {
        delete m_metadataConverter;
        m_metadataConverter = NULL;
    }

    releaseSensorQ();
    return;
}

void RequestManager::ResetEntry()
{
    Mutex::Autolock lock(m_requestMutex);
    Mutex::Autolock lock2(m_numOfEntriesLock);
    for (int i=0 ; i<NUM_MAX_REQUEST_MGR_ENTRY; i++) {
        memset(&(entries[i]), 0x00, sizeof(request_manager_entry_t));
        entries[i].internal_shot.shot.ctl.request.frameCount = -1;
    }
    m_numOfEntries = 0;
    m_entryInsertionIndex = -1;
    m_entryProcessingIndex = -1;
    m_entryFrameOutputIndex = -1;
}

int RequestManager::GetNumEntries()
{
    Mutex::Autolock lock(m_numOfEntriesLock);
    return m_numOfEntries;
}

void RequestManager::SetDefaultParameters(int cropX)
{
    m_cropX = cropX;
}

bool RequestManager::IsRequestQueueFull()
{
    Mutex::Autolock lock(m_requestMutex);
    Mutex::Autolock lock2(m_numOfEntriesLock);
    if (m_numOfEntries>=NUM_MAX_REQUEST_MGR_ENTRY)
        return true;
    else
        return false;
}

void RequestManager::RegisterRequest(camera_metadata_t * new_request, int * afMode, uint32_t * afRegion)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);

    Mutex::Autolock lock(m_requestMutex);
    Mutex::Autolock lock2(m_numOfEntriesLock);

    request_manager_entry * newEntry = NULL;
    int newInsertionIndex = GetNextIndex(m_entryInsertionIndex);
    ALOGV("DEBUG(%s): got lock, new insertIndex(%d), cnt before reg(%d)", __FUNCTION__,newInsertionIndex, m_numOfEntries );


    newEntry = &(entries[newInsertionIndex]);

    if (newEntry->status!=EMPTY) {
        ALOGV("DEBUG(%s): Circular buffer abnormal ", __FUNCTION__);
        return;
    }
    newEntry->status = REGISTERED;
    newEntry->original_request = new_request;
    memset(&(newEntry->internal_shot), 0, sizeof(struct camera2_shot_ext));
    m_metadataConverter->ToInternalShot(new_request, &(newEntry->internal_shot));
    newEntry->output_stream_count = 0;
    if (newEntry->internal_shot.shot.ctl.request.outputStreams[0] & MASK_OUTPUT_SCP)
        newEntry->output_stream_count++;

    if (newEntry->internal_shot.shot.ctl.request.outputStreams[0] & MASK_OUTPUT_SCC)
        newEntry->output_stream_count++;

    m_numOfEntries++;
    m_entryInsertionIndex = newInsertionIndex;


    *afMode = (int)(newEntry->internal_shot.shot.ctl.aa.afMode);
    afRegion[0] = newEntry->internal_shot.shot.ctl.aa.afRegions[0];
    afRegion[1] = newEntry->internal_shot.shot.ctl.aa.afRegions[1];
    afRegion[2] = newEntry->internal_shot.shot.ctl.aa.afRegions[2];
    afRegion[3] = newEntry->internal_shot.shot.ctl.aa.afRegions[3];
    ALOGV("## RegisterReq DONE num(%d), insert(%d), processing(%d), frame(%d), (frameCnt(%d))",
    m_numOfEntries,m_entryInsertionIndex,m_entryProcessingIndex, m_entryFrameOutputIndex, newEntry->internal_shot.shot.ctl.request.frameCount);
}

void RequestManager::DeregisterRequest(camera_metadata_t ** deregistered_request)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    int frame_index;
    request_manager_entry * currentEntry;

    Mutex::Autolock lock(m_requestMutex);
    Mutex::Autolock lock2(m_numOfEntriesLock);

    frame_index = GetCompletedIndex();
    currentEntry =  &(entries[frame_index]);
    if (currentEntry->status != COMPLETED) {
        CAM_LOGD("DBG(%s): Circular buffer abnormal. processing(%d), frame(%d), status(%d) ", __FUNCTION__,
                       m_entryProcessingIndex, frame_index,(int)(currentEntry->status));
        return;
    }
    if (deregistered_request)  *deregistered_request = currentEntry->original_request;

    m_lastCompletedFrameCnt = currentEntry->internal_shot.shot.ctl.request.frameCount;

    currentEntry->status = EMPTY;
    currentEntry->original_request = NULL;
    memset(&(currentEntry->internal_shot), 0, sizeof(struct camera2_shot_ext));
    currentEntry->internal_shot.shot.ctl.request.frameCount = -1;
    currentEntry->output_stream_count = 0;
    m_numOfEntries--;
    ALOGV("## DeRegistReq DONE num(%d), insert(%d), processing(%d), frame(%d)",
     m_numOfEntries,m_entryInsertionIndex,m_entryProcessingIndex, m_entryFrameOutputIndex);

    CheckCompleted(GetNextIndex(frame_index));
    return;
}

bool RequestManager::PrepareFrame(size_t* num_entries, size_t* frame_size,
                camera_metadata_t ** prepared_frame, int afState)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    Mutex::Autolock lock(m_requestMutex);
    status_t res = NO_ERROR;
    int tempFrameOutputIndex = GetCompletedIndex();
    request_manager_entry * currentEntry =  &(entries[tempFrameOutputIndex]);
    ALOGV("DEBUG(%s): processing(%d), frameOut(%d), insert(%d) recentlycompleted(%d)", __FUNCTION__,
        m_entryProcessingIndex, m_entryFrameOutputIndex, m_entryInsertionIndex, m_completedIndex);

    if (currentEntry->status != COMPLETED) {
        ALOGV("DBG(%s): Circular buffer abnormal status(%d)", __FUNCTION__, (int)(currentEntry->status));

        return false;
    }
    m_entryFrameOutputIndex = tempFrameOutputIndex;
    m_tempFrameMetadata = place_camera_metadata(m_tempFrameMetadataBuf, 2000, 35, 500); //estimated
    add_camera_metadata_entry(m_tempFrameMetadata, ANDROID_CONTROL_AF_STATE, &afState, 1);
    res = m_metadataConverter->ToDynamicMetadata(&(currentEntry->internal_shot),
                m_tempFrameMetadata);
    if (res!=NO_ERROR) {
        ALOGE("ERROR(%s): ToDynamicMetadata (%d) ", __FUNCTION__, res);
        return false;
    }
    *num_entries = get_camera_metadata_entry_count(m_tempFrameMetadata);
    *frame_size = get_camera_metadata_size(m_tempFrameMetadata);
    *prepared_frame = m_tempFrameMetadata;
    ALOGV("## PrepareFrame DONE: frameOut(%d) frameCnt-req(%d) timestamp(%lld)", m_entryFrameOutputIndex,
        currentEntry->internal_shot.shot.ctl.request.frameCount, currentEntry->internal_shot.shot.dm.sensor.timeStamp);
    // Dump();
    return true;
}

int RequestManager::MarkProcessingRequest(ExynosBuffer* buf)
{
    struct camera2_shot_ext * shot_ext;
    struct camera2_shot_ext * request_shot;
    int targetStreamIndex = 0;
    request_manager_entry * newEntry = NULL;
    static int count = 0;

    Mutex::Autolock lock(m_requestMutex);
    Mutex::Autolock lock2(m_numOfEntriesLock);
    if (m_numOfEntries == 0)  {
        CAM_LOGD("DEBUG(%s): Request Manager Empty ", __FUNCTION__);
        return -1;
    }

    if ((m_entryProcessingIndex == m_entryInsertionIndex)
        && (entries[m_entryProcessingIndex].status == REQUESTED || entries[m_entryProcessingIndex].status == CAPTURED)) {
        ALOGV("## MarkProcReq skipping(request underrun) -  num(%d), insert(%d), processing(%d), frame(%d)",
         m_numOfEntries,m_entryInsertionIndex,m_entryProcessingIndex, m_entryFrameOutputIndex);
        return -1;
    }

    int newProcessingIndex = GetNextIndex(m_entryProcessingIndex);
    ALOGV("DEBUG(%s): index(%d)", __FUNCTION__, newProcessingIndex);

    newEntry = &(entries[newProcessingIndex]);
    request_shot = &(newEntry->internal_shot);
    if (newEntry->status != REGISTERED) {
        CAM_LOGD("DEBUG(%s)(%d): Circular buffer abnormal, numOfEntries(%d), status(%d)", __FUNCTION__, newProcessingIndex, m_numOfEntries, newEntry->status);
        for (int i = 0; i < NUM_MAX_REQUEST_MGR_ENTRY; i++) {
                CAM_LOGD("DBG: entrie[%d].stream output cnt = %d, framecnt(%d)", i, entries[i].output_stream_count, entries[i].internal_shot.shot.ctl.request.frameCount);
        }
        return -1;
    }

    newEntry->status = REQUESTED;

    shot_ext = (struct camera2_shot_ext *)buf->virt.extP[1];

    memset(shot_ext, 0x00, sizeof(struct camera2_shot_ext));
    shot_ext->shot.ctl.request.frameCount = request_shot->shot.ctl.request.frameCount;
    shot_ext->request_sensor = 1;
    shot_ext->dis_bypass = 1;
    shot_ext->dnr_bypass = 1;
    shot_ext->fd_bypass = 1;
    shot_ext->setfile = 0;

    targetStreamIndex = newEntry->internal_shot.shot.ctl.request.outputStreams[0];
    shot_ext->shot.ctl.request.outputStreams[0] = targetStreamIndex;
    if (targetStreamIndex & MASK_OUTPUT_SCP)
        shot_ext->request_scp = 1;

    if (targetStreamIndex & MASK_OUTPUT_SCC)
        shot_ext->request_scc = 1;

    if (shot_ext->shot.ctl.stats.faceDetectMode != FACEDETECT_MODE_OFF)
        shot_ext->fd_bypass = 0;

    if (count == 0){
        shot_ext->shot.ctl.aa.mode = AA_CONTROL_AUTO;
    } else
        shot_ext->shot.ctl.aa.mode = AA_CONTROL_NONE;

    count++;
    shot_ext->shot.ctl.request.metadataMode = METADATA_MODE_FULL;
    shot_ext->shot.ctl.stats.faceDetectMode = FACEDETECT_MODE_FULL;
    shot_ext->shot.magicNumber = 0x23456789;
    shot_ext->shot.ctl.sensor.exposureTime = 0;
    shot_ext->shot.ctl.sensor.frameDuration = 33*1000*1000;
    shot_ext->shot.ctl.sensor.sensitivity = 0;


    shot_ext->shot.ctl.scaler.cropRegion[0] = newEntry->internal_shot.shot.ctl.scaler.cropRegion[0];
    shot_ext->shot.ctl.scaler.cropRegion[1] = newEntry->internal_shot.shot.ctl.scaler.cropRegion[1];
    shot_ext->shot.ctl.scaler.cropRegion[2] = newEntry->internal_shot.shot.ctl.scaler.cropRegion[2];

    m_entryProcessingIndex = newProcessingIndex;
    return newProcessingIndex;
}

void RequestManager::NotifyStreamOutput(int frameCnt)
{
    int index;

    Mutex::Autolock lock(m_requestMutex);
    ALOGV("DEBUG(%s): frameCnt(%d)", __FUNCTION__, frameCnt);

    index = FindEntryIndexByFrameCnt(frameCnt);
    if (index == -1) {
        ALOGE("ERR(%s): Cannot find entry for frameCnt(%d)", __FUNCTION__, frameCnt);
        return;
    }
    ALOGV("DEBUG(%s): frameCnt(%d), last cnt (%d)", __FUNCTION__, frameCnt,   entries[index].output_stream_count);

    entries[index].output_stream_count--;  //TODO : match stream id also
    CheckCompleted(index);
}

void RequestManager::CheckCompleted(int index)
{
    if ((entries[index].status == METADONE || entries[index].status == COMPLETED)
        && (entries[index].output_stream_count <= 0)){
        ALOGV("(%s): Completed(index:%d)(frameCnt:%d)", __FUNCTION__,
                index, entries[index].internal_shot.shot.ctl.request.frameCount );
        entries[index].status = COMPLETED;
        if (m_lastCompletedFrameCnt + 1 == (int)entries[index].internal_shot.shot.ctl.request.frameCount)
            m_mainThread->SetSignal(SIGNAL_MAIN_STREAM_OUTPUT_DONE);
    }
}

int RequestManager::GetCompletedIndex()
{
    return FindEntryIndexByFrameCnt(m_lastCompletedFrameCnt + 1);
}

void  RequestManager::pushSensorQ(int index)
{
    Mutex::Autolock lock(m_requestMutex);
    m_sensorQ.push_back(index);
}

int RequestManager::popSensorQ()
{
   List<int>::iterator sensor_token;
   int index;

    Mutex::Autolock lock(m_requestMutex);

    if(m_sensorQ.size() == 0)
        return -1;

    sensor_token = m_sensorQ.begin()++;
    index = *sensor_token;
    m_sensorQ.erase(sensor_token);

    return (index);
}

void RequestManager::releaseSensorQ()
{
    List<int>::iterator r;

    Mutex::Autolock lock(m_requestMutex);
    ALOGV("(%s)m_sensorQ.size : %d", __FUNCTION__, m_sensorQ.size());

    while(m_sensorQ.size() > 0){
        r  = m_sensorQ.begin()++;
        m_sensorQ.erase(r);
    }
    return;
}

void RequestManager::ApplyDynamicMetadata(struct camera2_shot_ext *shot_ext)
{
    int index;
    struct camera2_shot_ext * request_shot;
    nsecs_t timeStamp;
    int i;

    Mutex::Autolock lock(m_requestMutex);
    ALOGV("DEBUG(%s): frameCnt(%d)", __FUNCTION__, shot_ext->shot.ctl.request.frameCount);

    for (i = 0 ; i < NUM_MAX_REQUEST_MGR_ENTRY ; i++) {
        if (entries[i].internal_shot.shot.ctl.request.frameCount
                == shot_ext->shot.ctl.request.frameCount) {
            if (entries[i].status == CAPTURED) {
                entries[i].status = METADONE;
                break;
            }
            if (entries[i].status == METADONE) {
                return;
            }
        }
    }

    if (i == NUM_MAX_REQUEST_MGR_ENTRY){
        ALOGE("[%s] no entry found(framecount:%d)", __FUNCTION__, shot_ext->shot.ctl.request.frameCount);
        return;
    }

    request_manager_entry * newEntry = &(entries[i]);
    request_shot = &(newEntry->internal_shot);

    timeStamp = request_shot->shot.dm.sensor.timeStamp;
    memcpy(&(request_shot->shot.dm), &(shot_ext->shot.dm), sizeof(struct camera2_dm));
    request_shot->shot.dm.sensor.timeStamp = timeStamp;
    m_lastTimeStamp = timeStamp;
    CheckCompleted(i);
}

void    RequestManager::UpdateIspParameters(struct camera2_shot_ext *shot_ext, int frameCnt, ctl_request_info_t *ctl_info)
{
    int index, targetStreamIndex;
    struct camera2_shot_ext * request_shot;

    ALOGV("DEBUG(%s): updating info with frameCnt(%d)", __FUNCTION__, frameCnt);
    if (frameCnt < 0)
        return;

    index = FindEntryIndexByFrameCnt(frameCnt);
    if (index == -1) {
        ALOGE("ERR(%s): Cannot find entry for frameCnt(%d)", __FUNCTION__, frameCnt);
        return;
    }

    request_manager_entry * newEntry = &(entries[index]);
    request_shot = &(newEntry->internal_shot);
    memcpy(&(shot_ext->shot.ctl), &(request_shot->shot.ctl), sizeof(struct camera2_ctl));
    shot_ext->shot.ctl.request.frameCount = frameCnt;
    shot_ext->request_sensor = 1;
    shot_ext->dis_bypass = 1;
    shot_ext->dnr_bypass = 1;
    shot_ext->fd_bypass = 1;
    shot_ext->drc_bypass = 1;
    shot_ext->setfile = 0;

    shot_ext->request_scc = 0;
    shot_ext->request_scp = 0;

    shot_ext->isReprocessing = request_shot->isReprocessing;
    shot_ext->reprocessInput = request_shot->reprocessInput;
    shot_ext->shot.ctl.request.outputStreams[0] = 0;

    shot_ext->awb_mode_dm = request_shot->awb_mode_dm;

    shot_ext->shot.ctl.scaler.cropRegion[0] = request_shot->shot.ctl.scaler.cropRegion[0];
    shot_ext->shot.ctl.scaler.cropRegion[1] = request_shot->shot.ctl.scaler.cropRegion[1];
    shot_ext->shot.ctl.scaler.cropRegion[2] = request_shot->shot.ctl.scaler.cropRegion[2];

    // mapping flash UI mode from aeMode
    if (request_shot->shot.ctl.aa.aeMode >= AA_AEMODE_ON) {
        if (request_shot->shot.ctl.aa.captureIntent == AA_CAPTURE_INTENT_PREVIEW)
            ctl_info->flash.i_flashMode = request_shot->shot.ctl.aa.aeMode;
        else if (request_shot->shot.ctl.aa.captureIntent == AA_CAPTURE_INTENT_VIDEO_RECORD)
            ctl_info->flash.i_flashMode = request_shot->shot.ctl.aa.aeMode;
        request_shot->shot.ctl.aa.aeMode = AA_AEMODE_ON;
    }

    // Apply ae/awb lock or unlock
    if (request_shot->ae_lock == AEMODE_LOCK_ON)
            request_shot->shot.ctl.aa.aeMode = AA_AEMODE_LOCKED;
    if (request_shot->awb_lock == AWBMODE_LOCK_ON)
            request_shot->shot.ctl.aa.awbMode = AA_AWBMODE_LOCKED;

    if (m_lastAaMode == request_shot->shot.ctl.aa.mode) {
        shot_ext->shot.ctl.aa.mode = (enum aa_mode)(0);
    }
    else {
        shot_ext->shot.ctl.aa.mode = request_shot->shot.ctl.aa.mode;
        m_lastAaMode = (int)(shot_ext->shot.ctl.aa.mode);
    }
    if (m_lastAeMode == request_shot->shot.ctl.aa.aeMode) {
        shot_ext->shot.ctl.aa.aeMode = (enum aa_aemode)(0);
    }
    else {
        shot_ext->shot.ctl.aa.aeMode = request_shot->shot.ctl.aa.aeMode;
        m_lastAeMode = (int)(shot_ext->shot.ctl.aa.aeMode);
    }
    if (m_lastAwbMode == request_shot->shot.ctl.aa.awbMode) {
        shot_ext->shot.ctl.aa.awbMode = (enum aa_awbmode)(0);
    }
    else {
        shot_ext->shot.ctl.aa.awbMode = request_shot->shot.ctl.aa.awbMode;
        m_lastAwbMode = (int)(shot_ext->shot.ctl.aa.awbMode);
    }
    if (m_lastAeComp == request_shot->shot.ctl.aa.aeExpCompensation) {
        shot_ext->shot.ctl.aa.aeExpCompensation = 0;
    }
    else {
        shot_ext->shot.ctl.aa.aeExpCompensation = request_shot->shot.ctl.aa.aeExpCompensation;
        m_lastAeComp = (int)(shot_ext->shot.ctl.aa.aeExpCompensation);
    }

    if (request_shot->shot.ctl.aa.videoStabilizationMode && m_vdisEnable) {
        m_vdisBubbleEn = true;
        shot_ext->dis_bypass = 0;
        shot_ext->dnr_bypass = 0;
    } else {
        m_vdisBubbleEn = false;
        shot_ext->dis_bypass = 1;
        shot_ext->dnr_bypass = 1;
    }

    shot_ext->shot.ctl.aa.afTrigger = 0;

    targetStreamIndex = newEntry->internal_shot.shot.ctl.request.outputStreams[0];
    shot_ext->shot.ctl.request.outputStreams[0] = targetStreamIndex;
    if (targetStreamIndex & MASK_OUTPUT_SCP)
        shot_ext->request_scp = 1;

    if (targetStreamIndex & MASK_OUTPUT_SCC)
        shot_ext->request_scc = 1;

    if (shot_ext->shot.ctl.stats.faceDetectMode != FACEDETECT_MODE_OFF)
        shot_ext->fd_bypass = 0;

    shot_ext->shot.ctl.aa.aeTargetFpsRange[0] = request_shot->shot.ctl.aa.aeTargetFpsRange[0];
    shot_ext->shot.ctl.aa.aeTargetFpsRange[1] = request_shot->shot.ctl.aa.aeTargetFpsRange[1];

    ALOGV("(%s): applied aa(%d) aemode(%d) expComp(%d), awb(%d) afmode(%d), ", __FUNCTION__,
    (int)(shot_ext->shot.ctl.aa.mode), (int)(shot_ext->shot.ctl.aa.aeMode),
    (int)(shot_ext->shot.ctl.aa.aeExpCompensation), (int)(shot_ext->shot.ctl.aa.awbMode),
    (int)(shot_ext->shot.ctl.aa.afMode));
}

bool    RequestManager::IsVdisEnable(void)
{
        return m_vdisBubbleEn;
}

int     RequestManager::FindEntryIndexByFrameCnt(int frameCnt)
{
    for (int i = 0 ; i < NUM_MAX_REQUEST_MGR_ENTRY ; i++) {
        if ((int)entries[i].internal_shot.shot.ctl.request.frameCount == frameCnt)
            return i;
    }
    return -1;
}

void    RequestManager::RegisterTimestamp(int frameCnt, nsecs_t * frameTime)
{
    int index = FindEntryIndexByFrameCnt(frameCnt);
    if (index == -1) {
        ALOGE("ERR(%s): Cannot find entry for frameCnt(%d)", __FUNCTION__, frameCnt);
        return;
    }

    request_manager_entry * currentEntry = &(entries[index]);
    if (currentEntry->internal_shot.isReprocessing == 1) {
        ALOGV("DEBUG(%s): REPROCESSING : preserving timestamp for reqIndex(%d) frameCnt(%d) (%lld)", __FUNCTION__,
        index, frameCnt, currentEntry->internal_shot.shot.dm.sensor.timeStamp);
    } else {
        currentEntry->internal_shot.shot.dm.sensor.timeStamp = *((uint64_t*)frameTime);
        ALOGV("DEBUG(%s): applied timestamp for reqIndex(%d) frameCnt(%d) (%lld)", __FUNCTION__,
            index, frameCnt, currentEntry->internal_shot.shot.dm.sensor.timeStamp);
    }
}


nsecs_t  RequestManager::GetTimestampByFrameCnt(int frameCnt)
{
    int index = FindEntryIndexByFrameCnt(frameCnt);
    if (index == -1) {
        ALOGE("ERR(%s): Cannot find entry for frameCnt(%d) returning saved time(%lld)", __FUNCTION__, frameCnt, m_lastTimeStamp);
        return m_lastTimeStamp;
    }
    else
        return GetTimestamp(index);
}

nsecs_t  RequestManager::GetTimestamp(int index)
{
    Mutex::Autolock lock(m_requestMutex);
    if (index < 0 || index >= NUM_MAX_REQUEST_MGR_ENTRY) {
        ALOGE("ERR(%s): Request entry outside of bounds (%d)", __FUNCTION__, index);
        return 0;
    }

    request_manager_entry * currentEntry = &(entries[index]);
    nsecs_t frameTime = currentEntry->internal_shot.shot.dm.sensor.timeStamp;
    if (frameTime == 0) {
        ALOGV("DEBUG(%s): timestamp null,  returning saved value", __FUNCTION__);
        frameTime = m_lastTimeStamp;
    }
    ALOGV("DEBUG(%s): Returning timestamp for reqIndex(%d) (%lld)", __FUNCTION__, index, frameTime);
    return frameTime;
}

uint8_t  RequestManager::GetOutputStreamByFrameCnt(int frameCnt)
{
    int index = FindEntryIndexByFrameCnt(frameCnt);
    if (index == -1) {
        ALOGE("ERR(%s): Cannot find entry for frameCnt(%d)", __FUNCTION__, frameCnt);
        return 0;
    }
    else
        return GetOutputStream(index);
}

uint8_t  RequestManager::GetOutputStream(int index)
{
    Mutex::Autolock lock(m_requestMutex);
    if (index < 0 || index >= NUM_MAX_REQUEST_MGR_ENTRY) {
        ALOGE("ERR(%s): Request entry outside of bounds (%d)", __FUNCTION__, index);
        return 0;
    }

    request_manager_entry * currentEntry = &(entries[index]);
    return currentEntry->internal_shot.shot.ctl.request.outputStreams[0];
}

camera2_shot_ext *  RequestManager::GetInternalShotExtByFrameCnt(int frameCnt)
{
    int index = FindEntryIndexByFrameCnt(frameCnt);
    if (index == -1) {
        ALOGE("ERR(%s): Cannot find entry for frameCnt(%d)", __FUNCTION__, frameCnt);
        return 0;
    }
    else
        return GetInternalShotExt(index);
}

camera2_shot_ext *  RequestManager::GetInternalShotExt(int index)
{
    Mutex::Autolock lock(m_requestMutex);
    if (index < 0 || index >= NUM_MAX_REQUEST_MGR_ENTRY) {
        ALOGE("ERR(%s): Request entry outside of bounds (%d)", __FUNCTION__, index);
        return 0;
    }

    request_manager_entry * currentEntry = &(entries[index]);
    return &currentEntry->internal_shot;
}

int     RequestManager::FindFrameCnt(struct camera2_shot_ext * shot_ext, bool drain)
{
    Mutex::Autolock lock(m_requestMutex);
    Mutex::Autolock lock2(m_numOfEntriesLock);
    int i;

    if (m_numOfEntries == 0) {
        CAM_LOGD("DBG(%s): No Entry found", __FUNCTION__);
        return -1;
    }

    for (i = 0 ; i < NUM_MAX_REQUEST_MGR_ENTRY ; i++) {
        if(entries[i].internal_shot.shot.ctl.request.frameCount != shot_ext->shot.ctl.request.frameCount)
            continue;

        if (entries[i].status == REQUESTED) {
            entries[i].status = CAPTURED;
            return entries[i].internal_shot.shot.ctl.request.frameCount;
        }
        if (drain && (entries[i].status >= CAPTURED)) {
            return entries[i].internal_shot.shot.ctl.request.frameCount;
        }
        CAM_LOGE("ERR(%s): frameCount(%d), index(%d), status(%d)", __FUNCTION__, shot_ext->shot.ctl.request.frameCount, i, entries[i].status);

    }
    CAM_LOGD("(%s): No Entry found frame count(%d)", __FUNCTION__, shot_ext->shot.ctl.request.frameCount);

    return -1;
}

void     RequestManager::SetInitialSkip(int count)
{
    ALOGV("(%s): Pipeline Restarting. setting cnt(%d) - current(%d)", __FUNCTION__, count, m_sensorPipelineSkipCnt);
    if (count > m_sensorPipelineSkipCnt)
        m_sensorPipelineSkipCnt = count;
}

int     RequestManager::GetSkipCnt()
{
    ALOGV("(%s): skip cnt(%d)", __FUNCTION__, m_sensorPipelineSkipCnt);
    if (m_sensorPipelineSkipCnt == 0)
        return m_sensorPipelineSkipCnt;
    else
        return --m_sensorPipelineSkipCnt;
}

void RequestManager::Dump(void)
{
    int i = 0;
    request_manager_entry * currentEntry;
    Mutex::Autolock lock(m_numOfEntriesLock);
    ALOGD("## Dump  totalentry(%d), insert(%d), processing(%d), frame(%d)",
    m_numOfEntries,m_entryInsertionIndex,m_entryProcessingIndex, m_entryFrameOutputIndex);

    for (i = 0 ; i < NUM_MAX_REQUEST_MGR_ENTRY ; i++) {
        currentEntry =  &(entries[i]);
        ALOGD("[%2d] status[%d] frameCnt[%3d] numOutput[%d] outstream[0]-%x ", i,
        currentEntry->status, currentEntry->internal_shot.shot.ctl.request.frameCount,
            currentEntry->output_stream_count,
            currentEntry->internal_shot.shot.ctl.request.outputStreams[0]);
    }
}

int     RequestManager::GetNextIndex(int index)
{
    index++;
    if (index >= NUM_MAX_REQUEST_MGR_ENTRY)
        index = 0;

    return index;
}

int     RequestManager::GetPrevIndex(int index)
{
    index--;
    if (index < 0)
        index = NUM_MAX_REQUEST_MGR_ENTRY-1;

    return index;
}

ExynosCameraHWInterface2::ExynosCameraHWInterface2(int cameraId, camera2_device_t *dev, ExynosCamera2 * camera, int *openInvalid):
            m_requestQueueOps(NULL),
            m_frameQueueOps(NULL),
            m_callbackCookie(NULL),
            m_numOfRemainingReqInSvc(0),
            m_isRequestQueuePending(false),
            m_isRequestQueueNull(true),
            m_halDevice(dev),
            m_ionCameraClient(0),
            m_isIspStarted(false),
            m_sccLocalBufferValid(false),
            m_cameraId(cameraId),
            m_scp_closing(false),
            m_scp_closed(false),
            m_wideAspect(false),
            m_zoomRatio(1),
            m_vdisBubbleCnt(0),
            m_vdisDupFrame(0),
            m_jpegEncodingCount(0),
            m_scpForceSuspended(false),
            m_afState(HAL_AFSTATE_INACTIVE),
            m_afTriggerId(0),
            m_afMode(NO_CHANGE),
            m_afMode2(NO_CHANGE),
            m_IsAfModeUpdateRequired(false),
            m_IsAfTriggerRequired(false),
            m_IsAfLockRequired(false),
            m_serviceAfState(ANDROID_CONTROL_AF_STATE_INACTIVE),
            m_afPendingTriggerId(0),
            m_afModeWaitingCnt(0),
            m_scpOutputSignalCnt(0),
            m_scpOutputImageCnt(0),
            m_nightCaptureCnt(0),
            m_nightCaptureFrameCnt(0),
            m_lastSceneMode(0),
            m_thumbNailW(160),
            m_thumbNailH(120)
{
    ALOGD("(%s): ENTER", __FUNCTION__);
    int ret = 0;
    int res = 0;

    m_exynosPictureCSC = NULL;
    m_exynosVideoCSC = NULL;

    if (!m_grallocHal) {
        ret = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, (const hw_module_t **)&m_grallocHal);
        if (ret)
            ALOGE("ERR(%s):Fail on loading gralloc HAL", __FUNCTION__);
    }

    m_camera2 = camera;
    m_ionCameraClient = createIonClient(m_ionCameraClient);
    if(m_ionCameraClient == 0)
        ALOGE("ERR(%s):Fail on ion_client_create", __FUNCTION__);


    m_BayerManager = new BayerBufManager();
    m_mainThread    = new MainThread(this);
    m_requestManager = new RequestManager((SignalDrivenThread*)(m_mainThread.get()));
    *openInvalid = InitializeISPChain();
    if (*openInvalid < 0) {
        ALOGD("(%s): ISP chain init failed. exiting", __FUNCTION__);
        // clean process
        // 1. close video nodes
        // SCP
        res = exynos_v4l2_close(m_camera_info.scp.fd);
        if (res != NO_ERROR ) {
            ALOGE("ERR(%s): exynos_v4l2_close failed(%d)",__FUNCTION__ , res);
        }
        // SCC
        res = exynos_v4l2_close(m_camera_info.capture.fd);
        if (res != NO_ERROR ) {
            ALOGE("ERR(%s): exynos_v4l2_close failed(%d)",__FUNCTION__ , res);
        }
        // Sensor
        res = exynos_v4l2_close(m_camera_info.sensor.fd);
        if (res != NO_ERROR ) {
            ALOGE("ERR(%s): exynos_v4l2_close failed(%d)",__FUNCTION__ , res);
        }
        // ISP
        res = exynos_v4l2_close(m_camera_info.isp.fd);
        if (res != NO_ERROR ) {
            ALOGE("ERR(%s): exynos_v4l2_close failed(%d)",__FUNCTION__ , res);
        }
    } else {
        m_sensorThread  = new SensorThread(this);
        m_mainThread->Start("MainThread", PRIORITY_DEFAULT, 0);
        m_sensorThread->Start("SensorThread", PRIORITY_DEFAULT, 0);
        ALOGV("DEBUG(%s): created sensorthread ", __FUNCTION__);

        for (int i = 0 ; i < STREAM_ID_LAST+1 ; i++)
            m_subStreams[i].type =  SUBSTREAM_TYPE_NONE;
        CSC_METHOD cscMethod = CSC_METHOD_HW;
        m_exynosPictureCSC = csc_init(cscMethod);
        if (m_exynosPictureCSC == NULL)
            ALOGE("ERR(%s): csc_init() fail", __FUNCTION__);
        csc_set_hw_property(m_exynosPictureCSC, CSC_HW_PROPERTY_FIXED_NODE, PICTURE_GSC_NODE_NUM);
        csc_set_hw_property(m_exynosPictureCSC, CSC_HW_PROPERTY_HW_TYPE, CSC_HW_TYPE_GSCALER);

        m_exynosVideoCSC = csc_init(cscMethod);
        if (m_exynosVideoCSC == NULL)
            ALOGE("ERR(%s): csc_init() fail", __FUNCTION__);
        csc_set_hw_property(m_exynosVideoCSC, CSC_HW_PROPERTY_FIXED_NODE, VIDEO_GSC_NODE_NUM);
        csc_set_hw_property(m_exynosVideoCSC, CSC_HW_PROPERTY_HW_TYPE, CSC_HW_TYPE_GSCALER);

        m_setExifFixedAttribute();

        // contol information clear
        // flash
        m_ctlInfo.flash.i_flashMode = AA_AEMODE_ON;
        m_ctlInfo.flash.m_afFlashDoneFlg= false;
        m_ctlInfo.flash.m_flashEnableFlg = false;
        m_ctlInfo.flash.m_flashFrameCount = 0;
        m_ctlInfo.flash.m_flashCnt = IS_FLASH_STATE_NONE;
        m_ctlInfo.flash.m_flashTimeOut = 0;
        m_ctlInfo.flash.m_flashDecisionResult = false;
        m_ctlInfo.flash.m_flashTorchMode = false;
        m_ctlInfo.flash.m_precaptureState = 0;
        m_ctlInfo.flash.m_precaptureTriggerId = 0;
        // ae
        m_ctlInfo.ae.aeStateNoti = AE_STATE_INACTIVE;
        // af
        m_ctlInfo.af.m_afTriggerTimeOut = 0;
        // scene
        m_ctlInfo.scene.prevSceneMode = AA_SCENE_MODE_MAX;
    }
    ALOGD("(%s): EXIT", __FUNCTION__);
}

ExynosCameraHWInterface2::~ExynosCameraHWInterface2()
{
    ALOGD("(%s): ENTER", __FUNCTION__);
    this->release();
    ALOGD("(%s): EXIT", __FUNCTION__);
}

void ExynosCameraHWInterface2::release()
{
    int i, res;
    ALOGD("(HAL2::release): ENTER");

    if (m_streamThreads[1] != NULL) {
        m_streamThreads[1]->release();
        m_streamThreads[1]->SetSignal(SIGNAL_THREAD_TERMINATE);
    }

    if (m_streamThreads[0] != NULL) {
        m_streamThreads[0]->release();
        m_streamThreads[0]->SetSignal(SIGNAL_THREAD_TERMINATE);
    }

    if (m_sensorThread != NULL) {
        m_sensorThread->release();
    }

    if (m_mainThread != NULL) {
        m_mainThread->release();
    }

    if (m_exynosPictureCSC)
        csc_deinit(m_exynosPictureCSC);
    m_exynosPictureCSC = NULL;

    if (m_exynosVideoCSC)
        csc_deinit(m_exynosVideoCSC);
    m_exynosVideoCSC = NULL;

    if (m_streamThreads[1] != NULL) {
        ALOGD("(HAL2::release): START Waiting for (indirect) stream thread 1 termination");
        while (!m_streamThreads[1]->IsTerminated())
            usleep(SIG_WAITING_TICK);
        ALOGD("(HAL2::release): END   Waiting for (indirect) stream thread 1 termination");
        m_streamThreads[1] = NULL;
    }

    if (m_streamThreads[0] != NULL) {
        ALOGD("(HAL2::release): START Waiting for (indirect) stream thread 0 termination");
        while (!m_streamThreads[0]->IsTerminated())
            usleep(SIG_WAITING_TICK);
        ALOGD("(HAL2::release): END   Waiting for (indirect) stream thread 0 termination");
        m_streamThreads[0] = NULL;
    }

    if (m_sensorThread != NULL) {
        ALOGD("(HAL2::release): START Waiting for (indirect) sensor thread termination");
        while (!m_sensorThread->IsTerminated())
            usleep(SIG_WAITING_TICK);
        ALOGD("(HAL2::release): END   Waiting for (indirect) sensor thread termination");
        m_sensorThread = NULL;
    }

    if (m_mainThread != NULL) {
        ALOGD("(HAL2::release): START Waiting for (indirect) main thread termination");
        while (!m_mainThread->IsTerminated())
            usleep(SIG_WAITING_TICK);
        ALOGD("(HAL2::release): END   Waiting for (indirect) main thread termination");
        m_mainThread = NULL;
    }

    if (m_requestManager != NULL) {
        delete m_requestManager;
        m_requestManager = NULL;
    }

    if (m_BayerManager != NULL) {
        delete m_BayerManager;
        m_BayerManager = NULL;
    }
    for (i = 0; i < NUM_BAYER_BUFFERS; i++)
        freeCameraMemory(&m_camera_info.sensor.buffer[i], m_camera_info.sensor.planes);

    if (m_sccLocalBufferValid) {
        for (i = 0; i < NUM_SCC_BUFFERS; i++)
#ifdef ENABLE_FRAME_SYNC
            freeCameraMemory(&m_sccLocalBuffer[i], 2);
#else
            freeCameraMemory(&m_sccLocalBuffer[i], 1);
#endif
    }
    else {
        for (i = 0; i < NUM_SCC_BUFFERS; i++)
            freeCameraMemory(&m_camera_info.capture.buffer[i], m_camera_info.capture.planes);
    }

    ALOGV("DEBUG(%s): calling exynos_v4l2_close - sensor", __FUNCTION__);
    res = exynos_v4l2_close(m_camera_info.sensor.fd);
    if (res != NO_ERROR ) {
        ALOGE("ERR(%s): exynos_v4l2_close failed(%d)",__FUNCTION__ , res);
    }

    ALOGV("DEBUG(%s): calling exynos_v4l2_close - isp", __FUNCTION__);
    res = exynos_v4l2_close(m_camera_info.isp.fd);
    if (res != NO_ERROR ) {
        ALOGE("ERR(%s): exynos_v4l2_close failed(%d)",__FUNCTION__ , res);
    }

    ALOGV("DEBUG(%s): calling exynos_v4l2_close - capture", __FUNCTION__);
    res = exynos_v4l2_close(m_camera_info.capture.fd);
    if (res != NO_ERROR ) {
        ALOGE("ERR(%s): exynos_v4l2_close failed(%d)",__FUNCTION__ , res);
    }

    ALOGV("DEBUG(%s): calling exynos_v4l2_close - scp", __FUNCTION__);
    res = exynos_v4l2_close(m_camera_info.scp.fd);
    if (res != NO_ERROR ) {
        ALOGE("ERR(%s): exynos_v4l2_close failed(%d)",__FUNCTION__ , res);
    }
    ALOGV("DEBUG(%s): calling deleteIonClient", __FUNCTION__);
    deleteIonClient(m_ionCameraClient);

    ALOGD("(HAL2::release): EXIT");
}

int ExynosCameraHWInterface2::InitializeISPChain()
{
    char node_name[30];
    int fd = 0;
    int i;
    int ret = 0;

    /* Open Sensor */
    memset(&node_name, 0x00, sizeof(char[30]));
    sprintf(node_name, "%s%d", NODE_PREFIX, 40);
    fd = exynos_v4l2_open(node_name, O_RDWR, 0);

    if (fd < 0) {
        ALOGE("ERR(%s): failed to open sensor video node (%s) fd (%d)", __FUNCTION__,node_name, fd);
    }
    else {
        ALOGV("DEBUG(%s): sensor video node opened(%s) fd (%d)", __FUNCTION__,node_name, fd);
    }
    m_camera_info.sensor.fd = fd;

    /* Open ISP */
    memset(&node_name, 0x00, sizeof(char[30]));
    sprintf(node_name, "%s%d", NODE_PREFIX, 41);
    fd = exynos_v4l2_open(node_name, O_RDWR, 0);

    if (fd < 0) {
        ALOGE("ERR(%s): failed to open isp video node (%s) fd (%d)", __FUNCTION__,node_name, fd);
    }
    else {
        ALOGV("DEBUG(%s): isp video node opened(%s) fd (%d)", __FUNCTION__,node_name, fd);
    }
    m_camera_info.isp.fd = fd;

    /* Open ScalerC */
    memset(&node_name, 0x00, sizeof(char[30]));
    sprintf(node_name, "%s%d", NODE_PREFIX, 42);
    fd = exynos_v4l2_open(node_name, O_RDWR, 0);

    if (fd < 0) {
        ALOGE("ERR(%s): failed to open capture video node (%s) fd (%d)", __FUNCTION__,node_name, fd);
    }
    else {
        ALOGV("DEBUG(%s): capture video node opened(%s) fd (%d)", __FUNCTION__,node_name, fd);
    }
    m_camera_info.capture.fd = fd;

    /* Open ScalerP */
    memset(&node_name, 0x00, sizeof(char[30]));
    sprintf(node_name, "%s%d", NODE_PREFIX, 44);
    fd = exynos_v4l2_open(node_name, O_RDWR, 0);
    if (fd < 0) {
        ALOGE("DEBUG(%s): failed to open preview video node (%s) fd (%d)", __FUNCTION__,node_name, fd);
    }
    else {
        ALOGV("DEBUG(%s): preview video node opened(%s) fd (%d)", __FUNCTION__,node_name, fd);
    }
    m_camera_info.scp.fd = fd;

    if(m_cameraId == 0)
        m_camera_info.sensor_id = SENSOR_NAME_S5K4E5;
    else
        m_camera_info.sensor_id = SENSOR_NAME_S5K6A3;

    memset(&m_camera_info.dummy_shot, 0x00, sizeof(struct camera2_shot_ext));
    m_camera_info.dummy_shot.shot.ctl.request.metadataMode = METADATA_MODE_FULL;
    m_camera_info.dummy_shot.shot.magicNumber = 0x23456789;

    m_camera_info.dummy_shot.dis_bypass = 1;
    m_camera_info.dummy_shot.dnr_bypass = 1;
    m_camera_info.dummy_shot.fd_bypass = 1;

    /*sensor setting*/
    m_camera_info.dummy_shot.shot.ctl.sensor.exposureTime = 0;
    m_camera_info.dummy_shot.shot.ctl.sensor.frameDuration = 0;
    m_camera_info.dummy_shot.shot.ctl.sensor.sensitivity = 0;

    m_camera_info.dummy_shot.shot.ctl.scaler.cropRegion[0] = 0;
    m_camera_info.dummy_shot.shot.ctl.scaler.cropRegion[1] = 0;

    /*request setting*/
    m_camera_info.dummy_shot.request_sensor = 1;
    m_camera_info.dummy_shot.request_scc = 0;
    m_camera_info.dummy_shot.request_scp = 0;
    m_camera_info.dummy_shot.shot.ctl.request.outputStreams[0] = 0;

    m_camera_info.sensor.width = m_camera2->getSensorRawW();
    m_camera_info.sensor.height = m_camera2->getSensorRawH();

    m_camera_info.sensor.format = V4L2_PIX_FMT_SBGGR16;
    m_camera_info.sensor.planes = 2;
    m_camera_info.sensor.buffers = NUM_BAYER_BUFFERS;
    m_camera_info.sensor.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    m_camera_info.sensor.memory = V4L2_MEMORY_DMABUF;

    for(i = 0; i < m_camera_info.sensor.buffers; i++){
        int res;
        initCameraMemory(&m_camera_info.sensor.buffer[i], m_camera_info.sensor.planes);
        m_camera_info.sensor.buffer[i].size.extS[0] = m_camera_info.sensor.width*m_camera_info.sensor.height*2;
        m_camera_info.sensor.buffer[i].size.extS[1] = 8*1024; // HACK, driver use 8*1024, should be use predefined value
        res = allocCameraMemory(m_ionCameraClient, &m_camera_info.sensor.buffer[i], m_camera_info.sensor.planes, 1<<1);
        if (res) {
            ALOGE("ERROR(%s): failed to allocateCameraMemory for sensor buffer %d", __FUNCTION__, i);
            // Free allocated sensor buffers
            for (int j = 0; j < i; j++) {
                freeCameraMemory(&m_camera_info.sensor.buffer[j], m_camera_info.sensor.planes);
            }
            return false;
        }
    }

    m_camera_info.isp.width = m_camera_info.sensor.width;
    m_camera_info.isp.height = m_camera_info.sensor.height;
    m_camera_info.isp.format = m_camera_info.sensor.format;
    m_camera_info.isp.planes = m_camera_info.sensor.planes;
    m_camera_info.isp.buffers = m_camera_info.sensor.buffers;
    m_camera_info.isp.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    m_camera_info.isp.memory = V4L2_MEMORY_DMABUF;

    for(i = 0; i < m_camera_info.isp.buffers; i++){
        initCameraMemory(&m_camera_info.isp.buffer[i], m_camera_info.isp.planes);
        m_camera_info.isp.buffer[i].size.extS[0]    = m_camera_info.sensor.buffer[i].size.extS[0];
        m_camera_info.isp.buffer[i].size.extS[1]    = m_camera_info.sensor.buffer[i].size.extS[1];
        m_camera_info.isp.buffer[i].fd.extFd[0]     = m_camera_info.sensor.buffer[i].fd.extFd[0];
        m_camera_info.isp.buffer[i].fd.extFd[1]     = m_camera_info.sensor.buffer[i].fd.extFd[1];
        m_camera_info.isp.buffer[i].virt.extP[0]    = m_camera_info.sensor.buffer[i].virt.extP[0];
        m_camera_info.isp.buffer[i].virt.extP[1]    = m_camera_info.sensor.buffer[i].virt.extP[1];
    };

    /* init ISP */
    ret = cam_int_s_input(&(m_camera_info.isp), m_camera_info.sensor_id);
    if (ret < 0) {
        ALOGE("ERR(%s): cam_int_s_input(%d) failed!!!! ",  __FUNCTION__, m_camera_info.sensor_id);
        return false;
    }
    cam_int_s_fmt(&(m_camera_info.isp));
    ALOGV("DEBUG(%s): isp calling reqbuf", __FUNCTION__);
    cam_int_reqbufs(&(m_camera_info.isp));
    ALOGV("DEBUG(%s): isp calling querybuf", __FUNCTION__);
    ALOGV("DEBUG(%s): isp mem alloc done",  __FUNCTION__);

    /* init Sensor */
    cam_int_s_input(&(m_camera_info.sensor), m_camera_info.sensor_id);
    ALOGV("DEBUG(%s): sensor s_input done",  __FUNCTION__);
    if (cam_int_s_fmt(&(m_camera_info.sensor))< 0) {
        ALOGE("ERR(%s): sensor s_fmt fail",  __FUNCTION__);
    }
    ALOGV("DEBUG(%s): sensor s_fmt done",  __FUNCTION__);
    cam_int_reqbufs(&(m_camera_info.sensor));
    ALOGV("DEBUG(%s): sensor reqbuf done",  __FUNCTION__);
    for (i = 0; i < m_camera_info.sensor.buffers; i++) {
        ALOGV("DEBUG(%s): sensor initial QBUF [%d]",  __FUNCTION__, i);
        m_camera_info.dummy_shot.shot.ctl.sensor.frameDuration = 33*1000*1000; // apply from frame #1
        m_camera_info.dummy_shot.shot.ctl.request.frameCount = -1;
        memcpy( m_camera_info.sensor.buffer[i].virt.extP[1], &(m_camera_info.dummy_shot),
                sizeof(struct camera2_shot_ext));
    }

    for (i = 0; i < NUM_MIN_SENSOR_QBUF; i++)
        cam_int_qbuf(&(m_camera_info.sensor), i);

    for (i = NUM_MIN_SENSOR_QBUF; i < m_camera_info.sensor.buffers; i++)
        m_requestManager->pushSensorQ(i);

    ALOGV("== stream_on :: sensor");
    cam_int_streamon(&(m_camera_info.sensor));
    m_camera_info.sensor.status = true;

    /* init Capture */
    m_camera_info.capture.width = m_camera2->getSensorW();
    m_camera_info.capture.height = m_camera2->getSensorH();
    m_camera_info.capture.format = V4L2_PIX_FMT_YUYV;
#ifdef ENABLE_FRAME_SYNC
    m_camera_info.capture.planes = 2;
#else
    m_camera_info.capture.planes = 1;
#endif
    m_camera_info.capture.buffers = NUM_SCC_BUFFERS;
    m_camera_info.capture.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    m_camera_info.capture.memory = V4L2_MEMORY_DMABUF;

    m_camera_info.capture.status = false;

    return true;
}

void ExynosCameraHWInterface2::StartSCCThread(bool threadExists)
{
    ALOGV("(%s)", __FUNCTION__);
    StreamThread *AllocatedStream;
    stream_parameters_t newParameters;
    uint32_t format_actual;


    if (!threadExists) {
        m_streamThreads[1]  = new StreamThread(this, 1);
    }
    AllocatedStream = (StreamThread*)(m_streamThreads[1].get());
    if (!threadExists) {
        AllocatedStream->Start("StreamThread", PRIORITY_DEFAULT, 0);
        m_streamThreadInitialize((SignalDrivenThread*)AllocatedStream);
        AllocatedStream->m_numRegisteredStream = 1;
    }
    AllocatedStream->m_index        = 1;

    format_actual                   = HAL_PIXEL_FORMAT_YCbCr_422_I; // YUYV

    newParameters.width             = m_camera2->getSensorW();
    newParameters.height            = m_camera2->getSensorH();
    newParameters.format            = format_actual;
    newParameters.streamOps         = NULL;
    newParameters.numHwBuffers      = NUM_SCC_BUFFERS;
#ifdef ENABLE_FRAME_SYNC
    newParameters.planes            = 2;
#else
    newParameters.planes            = 1;
#endif

    newParameters.numSvcBufsInHal   = 0;

    newParameters.node              = &m_camera_info.capture;

    AllocatedStream->streamType     = STREAM_TYPE_INDIRECT;
    ALOGV("(%s): m_numRegisteredStream = %d", __FUNCTION__, AllocatedStream->m_numRegisteredStream);

    if (!threadExists) {
        if (!m_sccLocalBufferValid) {
            for (int i = 0; i < m_camera_info.capture.buffers; i++){
                initCameraMemory(&m_camera_info.capture.buffer[i], newParameters.node->planes);
                m_camera_info.capture.buffer[i].size.extS[0] = m_camera_info.capture.width*m_camera_info.capture.height*2;
#ifdef ENABLE_FRAME_SYNC
                m_camera_info.capture.buffer[i].size.extS[1] = 4*1024; // HACK, driver use 4*1024, should be use predefined value
                allocCameraMemory(m_ionCameraClient, &m_camera_info.capture.buffer[i], m_camera_info.capture.planes, 1<<1);
#else
                allocCameraMemory(m_ionCameraClient, &m_camera_info.capture.buffer[i], m_camera_info.capture.planes);
#endif
                m_sccLocalBuffer[i] = m_camera_info.capture.buffer[i];
            }
            m_sccLocalBufferValid = true;
        }
    } else {
        if (m_sccLocalBufferValid) {
             for (int i = 0; i < m_camera_info.capture.buffers; i++)
                m_camera_info.capture.buffer[i] = m_sccLocalBuffer[i];
        } else {
            ALOGE("(%s): SCC Thread starting with no buffer", __FUNCTION__);
        }
    }
    cam_int_s_input(newParameters.node, m_camera_info.sensor_id);
    m_camera_info.capture.buffers = NUM_SCC_BUFFERS;
    cam_int_s_fmt(newParameters.node);
    ALOGV("DEBUG(%s): capture calling reqbuf", __FUNCTION__);
    cam_int_reqbufs(newParameters.node);
    ALOGV("DEBUG(%s): capture calling querybuf", __FUNCTION__);

    for (int i = 0; i < newParameters.node->buffers; i++) {
        ALOGV("DEBUG(%s): capture initial QBUF [%d]",  __FUNCTION__, i);
        cam_int_qbuf(newParameters.node, i);
        newParameters.svcBufStatus[i] = ON_DRIVER;
    }

    ALOGV("== stream_on :: capture");
    if (cam_int_streamon(newParameters.node) < 0) {
        ALOGE("ERR(%s): capture stream on fail", __FUNCTION__);
    } else {
        m_camera_info.capture.status = true;
    }

    AllocatedStream->setParameter(&newParameters);
    AllocatedStream->m_activated    = true;
    AllocatedStream->m_isBufferInit = true;
}

void ExynosCameraHWInterface2::StartISP()
{
    ALOGV("== stream_on :: isp");
    cam_int_streamon(&(m_camera_info.isp));
    exynos_v4l2_s_ctrl(m_camera_info.sensor.fd, V4L2_CID_IS_S_STREAM, IS_ENABLE_STREAM);
}

int ExynosCameraHWInterface2::getCameraId() const
{
    return m_cameraId;
}

int ExynosCameraHWInterface2::setRequestQueueSrcOps(const camera2_request_queue_src_ops_t *request_src_ops)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    if ((NULL != request_src_ops) && (NULL != request_src_ops->dequeue_request)
            && (NULL != request_src_ops->free_request) && (NULL != request_src_ops->request_count)) {
        m_requestQueueOps = (camera2_request_queue_src_ops_t*)request_src_ops;
        return 0;
    }
    else {
        ALOGE("DEBUG(%s):setRequestQueueSrcOps : NULL arguments", __FUNCTION__);
        return 1;
    }
}

int ExynosCameraHWInterface2::notifyRequestQueueNotEmpty()
{
    int i = 0;

    ALOGV("DEBUG(%s):setting [SIGNAL_MAIN_REQ_Q_NOT_EMPTY] current(%d)", __FUNCTION__, m_requestManager->GetNumEntries());
    if ((NULL==m_frameQueueOps)|| (NULL==m_requestQueueOps)) {
        ALOGE("DEBUG(%s):queue ops NULL. ignoring request", __FUNCTION__);
        return 0;
    }
    m_isRequestQueueNull = false;
    if (m_requestManager->GetNumEntries() == 0)
        m_requestManager->SetInitialSkip(0);

    if (m_isIspStarted == false) {
        /* isp */
        m_camera_info.sensor.buffers = NUM_BAYER_BUFFERS;
        m_camera_info.isp.buffers = m_camera_info.sensor.buffers;
        cam_int_s_fmt(&(m_camera_info.isp));
        cam_int_reqbufs(&(m_camera_info.isp));

        /* sensor */
        if (m_camera_info.sensor.status == false) {
            cam_int_s_fmt(&(m_camera_info.sensor));
            cam_int_reqbufs(&(m_camera_info.sensor));

            for (i = 0; i < m_camera_info.sensor.buffers; i++) {
                ALOGV("DEBUG(%s): sensor initial QBUF [%d]",  __FUNCTION__, i);
                m_camera_info.dummy_shot.shot.ctl.sensor.frameDuration = 33*1000*1000; // apply from frame #1
                m_camera_info.dummy_shot.shot.ctl.request.frameCount = -1;
                memcpy( m_camera_info.sensor.buffer[i].virt.extP[1], &(m_camera_info.dummy_shot),
                        sizeof(struct camera2_shot_ext));
            }
            for (i = 0; i < NUM_MIN_SENSOR_QBUF; i++)
                cam_int_qbuf(&(m_camera_info.sensor), i);

            for (i = NUM_MIN_SENSOR_QBUF; i < m_camera_info.sensor.buffers; i++)
                m_requestManager->pushSensorQ(i);
            ALOGV("DEBUG(%s): calling sensor streamon", __FUNCTION__);
            cam_int_streamon(&(m_camera_info.sensor));
            m_camera_info.sensor.status = true;
        }
    }
    if (!(m_streamThreads[1].get())) {
        ALOGV("DEBUG(%s): stream thread 1 not exist. starting without stream", __FUNCTION__);
        StartSCCThread(false);
    } else {
        if (m_streamThreads[1]->m_activated ==  false) {
            ALOGV("DEBUG(%s): stream thread 1 suspended. restarting", __FUNCTION__);
            StartSCCThread(true);
        } else {
            if (m_camera_info.capture.status == false) {
                m_camera_info.capture.buffers = NUM_SCC_BUFFERS;
                cam_int_s_fmt(&(m_camera_info.capture));
                ALOGV("DEBUG(%s): capture calling reqbuf", __FUNCTION__);
                cam_int_reqbufs(&(m_camera_info.capture));
                ALOGV("DEBUG(%s): capture calling querybuf", __FUNCTION__);

                if (m_streamThreads[1]->streamType == STREAM_TYPE_DIRECT) {
                    StreamThread *          targetStream = m_streamThreads[1].get();
                    stream_parameters_t     *targetStreamParms = &(targetStream->m_parameters);
                    node_info_t             *currentNode = targetStreamParms->node;

                    struct v4l2_buffer v4l2_buf;
                    struct v4l2_plane  planes[VIDEO_MAX_PLANES];

                    for (i = 0 ; i < targetStreamParms->numSvcBuffers ; i++) {
                        v4l2_buf.m.planes   = planes;
                        v4l2_buf.type       = currentNode->type;
                        v4l2_buf.memory     = currentNode->memory;

                        v4l2_buf.length     = currentNode->planes;
                        v4l2_buf.index      = i;
                        ExynosBuffer metaBuf = targetStreamParms->metaBuffers[i];

                        if (i < currentNode->buffers) {
#ifdef ENABLE_FRAME_SYNC
                            v4l2_buf.m.planes[0].m.fd = targetStreamParms->svcBuffers[i].fd.extFd[0];
                            v4l2_buf.m.planes[2].m.fd = targetStreamParms->svcBuffers[i].fd.extFd[1];
                            v4l2_buf.m.planes[1].m.fd = targetStreamParms->svcBuffers[i].fd.extFd[2];
                            v4l2_buf.length += targetStreamParms->metaPlanes;
                            v4l2_buf.m.planes[v4l2_buf.length-1].m.fd = metaBuf.fd.extFd[0];
                            v4l2_buf.m.planes[v4l2_buf.length-1].length = metaBuf.size.extS[0];

                            ALOGV("Qbuf metaBuf: fd(%d), length(%d) plane(%d)", metaBuf.fd.extFd[0], metaBuf.size.extS[0], v4l2_buf.length);
#endif
                            if (exynos_v4l2_qbuf(currentNode->fd, &v4l2_buf) < 0) {
                                ALOGE("ERR(%s): exynos_v4l2_qbuf() fail fd(%d)", __FUNCTION__, currentNode->fd);
                            }
                            ALOGV("DEBUG(%s): exynos_v4l2_qbuf() success fd(%d)", __FUNCTION__, currentNode->fd);
                            targetStreamParms->svcBufStatus[i]  = REQUIRES_DQ_FROM_SVC;
                        }
                        else {
                            targetStreamParms->svcBufStatus[i]  = ON_SERVICE;
                        }

                    }

                } else {
                    for (int i = 0; i < m_camera_info.capture.buffers; i++) {
                        ALOGV("DEBUG(%s): capture initial QBUF [%d]",  __FUNCTION__, i);
                        cam_int_qbuf(&(m_camera_info.capture), i);
                    }
                }
                ALOGV("== stream_on :: capture");
                if (cam_int_streamon(&(m_camera_info.capture)) < 0) {
                    ALOGE("ERR(%s): capture stream on fail", __FUNCTION__);
                } else {
                    m_camera_info.capture.status = true;
                }
            }
            if (m_scpForceSuspended) {
                m_scpForceSuspended = false;
            }
        }
    }
    if (m_isIspStarted == false) {
        StartISP();
        ALOGV("DEBUG(%s):starting sensor thread", __FUNCTION__);
        m_requestManager->SetInitialSkip(6);
        m_sensorThread->Start("SensorThread", PRIORITY_DEFAULT, 0);
        m_isIspStarted = true;
    }
    m_mainThread->SetSignal(SIGNAL_MAIN_REQ_Q_NOT_EMPTY);
    return 0;
}

int ExynosCameraHWInterface2::setFrameQueueDstOps(const camera2_frame_queue_dst_ops_t *frame_dst_ops)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    if ((NULL != frame_dst_ops) && (NULL != frame_dst_ops->dequeue_frame)
            && (NULL != frame_dst_ops->cancel_frame) && (NULL !=frame_dst_ops->enqueue_frame)) {
        m_frameQueueOps = (camera2_frame_queue_dst_ops_t *)frame_dst_ops;
        return 0;
    }
    else {
        ALOGE("DEBUG(%s):setFrameQueueDstOps : NULL arguments", __FUNCTION__);
        return 1;
    }
}

int ExynosCameraHWInterface2::getInProgressCount()
{
    int inProgressJpeg;
    int inProgressCount;

    {
        Mutex::Autolock lock(m_jpegEncoderLock);
        inProgressJpeg = m_jpegEncodingCount;
        inProgressCount = m_requestManager->GetNumEntries();
    }
    ALOGV("DEBUG(%s): # of dequeued req (%d) jpeg(%d) = (%d)", __FUNCTION__,
        inProgressCount, inProgressJpeg, (inProgressCount + inProgressJpeg));
    return (inProgressCount + inProgressJpeg);
}

int ExynosCameraHWInterface2::flushCapturesInProgress()
{
    return 0;
}

int ExynosCameraHWInterface2::constructDefaultRequest(int request_template, camera_metadata_t **request)
{
    ALOGV("DEBUG(%s): making template (%d) ", __FUNCTION__, request_template);

    if (request == NULL) return BAD_VALUE;
    if (request_template < 0 || request_template >= CAMERA2_TEMPLATE_COUNT) {
        return BAD_VALUE;
    }
    status_t res;
    // Pass 1, calculate size and allocate
    res = m_camera2->constructDefaultRequest(request_template,
            request,
            true);
    if (res != OK) {
        return res;
    }
    // Pass 2, build request
    res = m_camera2->constructDefaultRequest(request_template,
            request,
            false);
    if (res != OK) {
        ALOGE("Unable to populate new request for template %d",
                request_template);
    }

    return res;
}

int ExynosCameraHWInterface2::allocateStream(uint32_t width, uint32_t height, int format, const camera2_stream_ops_t *stream_ops,
                                    uint32_t *stream_id, uint32_t *format_actual, uint32_t *usage, uint32_t *max_buffers)
{
    ALOGD("(%s): stream width(%d) height(%d) format(%x)", __FUNCTION__,  width, height, format);
    bool useDirectOutput = false;
    StreamThread *AllocatedStream;
    stream_parameters_t newParameters;
    substream_parameters_t *subParameters;
    StreamThread *parentStream;
    status_t res;
    int allocCase = 0;

    if ((format == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED || format == CAMERA2_HAL_PIXEL_FORMAT_OPAQUE)  &&
            m_camera2->isSupportedResolution(width, height)) {
        if (!(m_streamThreads[0].get())) {
            ALOGV("DEBUG(%s): stream 0 not exist", __FUNCTION__);
            allocCase = 0;
        }
        else {
            if ((m_streamThreads[0].get())->m_activated == true) {
                ALOGV("DEBUG(%s): stream 0 exists and activated.", __FUNCTION__);
                allocCase = 1;
            }
            else {
                ALOGV("DEBUG(%s): stream 0 exists and deactivated.", __FUNCTION__);
                allocCase = 2;
            }
        }

        // TODO : instead of that, use calculate aspect ratio and selection with calculated ratio.
        if ((width == 1920 && height == 1080) || (width == 1280 && height == 720)
                    || (width == 720 && height == 480) || (width == 1440 && height == 960)
                    || (width == 1344 && height == 896)) {
            m_wideAspect = true;
        } else {
            m_wideAspect = false;
        }
        ALOGV("DEBUG(%s): m_wideAspect (%d)", __FUNCTION__, m_wideAspect);

        if (allocCase == 0 || allocCase == 2) {
            *stream_id = STREAM_ID_PREVIEW;

            m_streamThreads[0]  = new StreamThread(this, *stream_id);

            AllocatedStream = (StreamThread*)(m_streamThreads[0].get());
            AllocatedStream->Start("StreamThread", PRIORITY_DEFAULT, 0);
            m_streamThreadInitialize((SignalDrivenThread*)AllocatedStream);

            *format_actual                      = HAL_PIXEL_FORMAT_EXYNOS_YV12;
            *usage                              = GRALLOC_USAGE_SW_WRITE_OFTEN;
            if (m_wideAspect)
                *usage                         |= GRALLOC_USAGE_PRIVATE_CHROMA;
            *max_buffers                        = 7;

            newParameters.width                 = width;
            newParameters.height                = height;
            newParameters.format                = *format_actual;
            newParameters.streamOps             = stream_ops;
            newParameters.usage                 = *usage;
            newParameters.numHwBuffers          = NUM_SCP_BUFFERS;
            newParameters.numOwnSvcBuffers      = *max_buffers;
            newParameters.planes                = NUM_PLANES(*format_actual);
            newParameters.metaPlanes            = 1;
            newParameters.numSvcBufsInHal       = 0;
            newParameters.minUndequedBuffer     = 3;
            newParameters.needsIonMap           = true;

            newParameters.node                  = &m_camera_info.scp;
            newParameters.node->type            = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
            newParameters.node->memory          = V4L2_MEMORY_DMABUF;

            AllocatedStream->streamType         = STREAM_TYPE_DIRECT;
            AllocatedStream->m_index            = 0;
            AllocatedStream->setParameter(&newParameters);
            AllocatedStream->m_activated = true;
            AllocatedStream->m_numRegisteredStream = 1;
            ALOGV("(%s): m_numRegisteredStream = %d", __FUNCTION__, AllocatedStream->m_numRegisteredStream);
            m_requestManager->SetDefaultParameters(m_camera2->getSensorW());
            m_camera_info.dummy_shot.shot.ctl.scaler.cropRegion[2] = m_camera2->getSensorW();
            if (m_subStreams[STREAM_ID_RECORD].type != SUBSTREAM_TYPE_NONE)
                AllocatedStream->attachSubStream(STREAM_ID_RECORD, 10);
            if (m_subStreams[STREAM_ID_PRVCB].type != SUBSTREAM_TYPE_NONE)
                AllocatedStream->attachSubStream(STREAM_ID_PRVCB, 70);

            // set video stabilization killswitch
            m_requestManager->m_vdisEnable = width > 352 && height > 288;

            return 0;
        } else if (allocCase == 1) {
            *stream_id = STREAM_ID_RECORD;

            subParameters = &m_subStreams[STREAM_ID_RECORD];
            memset(subParameters, 0, sizeof(substream_parameters_t));

            parentStream = (StreamThread*)(m_streamThreads[0].get());
            if (!parentStream) {
                return 1;
            }

            *format_actual = HAL_PIXEL_FORMAT_YCbCr_420_SP; // NV12M
            *usage = GRALLOC_USAGE_SW_WRITE_OFTEN;
            if (m_wideAspect)
                *usage |= GRALLOC_USAGE_PRIVATE_CHROMA;
            *max_buffers = 7;

            subParameters->type         = SUBSTREAM_TYPE_RECORD;
            subParameters->width        = width;
            subParameters->height       = height;
            subParameters->format       = *format_actual;
            subParameters->svcPlanes     = NUM_PLANES(*format_actual);
            subParameters->streamOps     = stream_ops;
            subParameters->usage         = *usage;
            subParameters->numOwnSvcBuffers = *max_buffers;
            subParameters->numSvcBufsInHal  = 0;
            subParameters->needBufferInit    = false;
            subParameters->minUndequedBuffer = 2;

            res = parentStream->attachSubStream(STREAM_ID_RECORD, 20);
            if (res != NO_ERROR) {
                ALOGE("(%s): substream attach failed. res(%d)", __FUNCTION__, res);
                return 1;
            }
            ALOGV("(%s): m_numRegisteredStream = %d", __FUNCTION__, parentStream->m_numRegisteredStream);
            ALOGV("(%s): Enabling Record", __FUNCTION__);
            return 0;
        }
    }
    else if ((format == CAMERA2_HAL_PIXEL_FORMAT_ZSL)
            && ((int32_t)width == m_camera2->getSensorW()) && ((int32_t)height == m_camera2->getSensorH())) {

        if (!(m_streamThreads[1].get())) {
            ALOGV("DEBUG(%s): stream thread 1 not exist", __FUNCTION__);
            useDirectOutput = true;
        }
        else {
            ALOGV("DEBUG(%s): stream thread 1 exists and deactivated.", __FUNCTION__);
            useDirectOutput = false;
        }
        if (useDirectOutput) {
            *stream_id = STREAM_ID_ZSL;

            m_streamThreads[1]  = new StreamThread(this, *stream_id);
            AllocatedStream = (StreamThread*)(m_streamThreads[1].get());
            AllocatedStream->Start("StreamThread", PRIORITY_DEFAULT, 0);
            m_streamThreadInitialize((SignalDrivenThread*)AllocatedStream);

            *format_actual                      = HAL_PIXEL_FORMAT_EXYNOS_YV12;

            *format_actual = HAL_PIXEL_FORMAT_YCbCr_422_I; // YUYV
            *usage = GRALLOC_USAGE_SW_WRITE_OFTEN;
            if (m_wideAspect)
                *usage |= GRALLOC_USAGE_PRIVATE_CHROMA;
            *max_buffers = 7;

            newParameters.width                 = width;
            newParameters.height                = height;
            newParameters.format                = *format_actual;
            newParameters.streamOps             = stream_ops;
            newParameters.usage                 = *usage;
            newParameters.numHwBuffers          = NUM_SCC_BUFFERS;
            newParameters.numOwnSvcBuffers      = *max_buffers;
            newParameters.planes                = NUM_PLANES(*format_actual);
            newParameters.metaPlanes            = 1;

            newParameters.numSvcBufsInHal       = 0;
            newParameters.minUndequedBuffer     = 2;
            newParameters.needsIonMap           = false;

            newParameters.node                  = &m_camera_info.capture;
            newParameters.node->type            = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
            newParameters.node->memory          = V4L2_MEMORY_DMABUF;

            AllocatedStream->streamType         = STREAM_TYPE_DIRECT;
            AllocatedStream->m_index            = 1;
            AllocatedStream->setParameter(&newParameters);
            AllocatedStream->m_activated = true;
            AllocatedStream->m_numRegisteredStream = 1;
            ALOGV("(%s): m_numRegisteredStream = %d", __FUNCTION__, AllocatedStream->m_numRegisteredStream);
            return 0;
        } else {
            bool bJpegExists = false;
            AllocatedStream = (StreamThread*)(m_streamThreads[1].get());
            subParameters = &m_subStreams[STREAM_ID_JPEG];
            if (subParameters->type == SUBSTREAM_TYPE_JPEG) {
                ALOGD("(%s): jpeg stream exists", __FUNCTION__);
                bJpegExists = true;
                AllocatedStream->detachSubStream(STREAM_ID_JPEG);
            }
            AllocatedStream->m_releasing = true;
            ALOGD("START stream thread 1 release %d", __LINE__);
            do {
                AllocatedStream->release();
                usleep(SIG_WAITING_TICK);
            } while (AllocatedStream->m_releasing);
            ALOGD("END   stream thread 1 release %d", __LINE__);

            *stream_id = STREAM_ID_ZSL;

            m_streamThreadInitialize((SignalDrivenThread*)AllocatedStream);

            *format_actual                      = HAL_PIXEL_FORMAT_EXYNOS_YV12;

            *format_actual = HAL_PIXEL_FORMAT_YCbCr_422_I; // YUYV
            *usage = GRALLOC_USAGE_SW_WRITE_OFTEN;
            if (m_wideAspect)
                *usage |= GRALLOC_USAGE_PRIVATE_CHROMA;
            *max_buffers = 7;

            newParameters.width                 = width;
            newParameters.height                = height;
            newParameters.format                = *format_actual;
            newParameters.streamOps             = stream_ops;
            newParameters.usage                 = *usage;
            newParameters.numHwBuffers          = NUM_SCC_BUFFERS;
            newParameters.numOwnSvcBuffers      = *max_buffers;
            newParameters.planes                = NUM_PLANES(*format_actual);
            newParameters.metaPlanes            = 1;

            newParameters.numSvcBufsInHal       = 0;
            newParameters.minUndequedBuffer     = 2;
            newParameters.needsIonMap           = false;

            newParameters.node                  = &m_camera_info.capture;
            newParameters.node->type            = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
            newParameters.node->memory          = V4L2_MEMORY_DMABUF;

            AllocatedStream->streamType         = STREAM_TYPE_DIRECT;
            AllocatedStream->m_index            = 1;
            AllocatedStream->setParameter(&newParameters);
            AllocatedStream->m_activated = true;
            AllocatedStream->m_numRegisteredStream = 1;
            if (bJpegExists) {
                AllocatedStream->attachSubStream(STREAM_ID_JPEG, 10);
            }
            ALOGV("(%s): m_numRegisteredStream = %d", __FUNCTION__, AllocatedStream->m_numRegisteredStream);
            return 0;

        }
    }
    else if (format == HAL_PIXEL_FORMAT_BLOB
            && m_camera2->isSupportedJpegResolution(width, height)) {
        *stream_id = STREAM_ID_JPEG;

        subParameters = &m_subStreams[*stream_id];
        memset(subParameters, 0, sizeof(substream_parameters_t));

        if (!(m_streamThreads[1].get())) {
            ALOGV("DEBUG(%s): stream thread 1 not exist", __FUNCTION__);
            StartSCCThread(false);
        }
        else if (m_streamThreads[1]->m_activated ==  false) {
            ALOGV("DEBUG(%s): stream thread 1 suspended. restarting", __FUNCTION__);
            StartSCCThread(true);
        }
        parentStream = (StreamThread*)(m_streamThreads[1].get());

        *format_actual = HAL_PIXEL_FORMAT_BLOB;
        *usage = GRALLOC_USAGE_SW_WRITE_OFTEN;
        if (m_wideAspect)
            *usage |= GRALLOC_USAGE_PRIVATE_CHROMA;
        *max_buffers = 5;

        subParameters->type          = SUBSTREAM_TYPE_JPEG;
        subParameters->width         = width;
        subParameters->height        = height;
        subParameters->format        = *format_actual;
        subParameters->svcPlanes     = 1;
        subParameters->streamOps     = stream_ops;
        subParameters->usage         = *usage;
        subParameters->numOwnSvcBuffers = *max_buffers;
        subParameters->numSvcBufsInHal  = 0;
        subParameters->needBufferInit    = false;
        subParameters->minUndequedBuffer = 2;

        res = parentStream->attachSubStream(STREAM_ID_JPEG, 10);
        if (res != NO_ERROR) {
            ALOGE("(%s): substream attach failed. res(%d)", __FUNCTION__, res);
            return 1;
        }
        ALOGV("(%s): m_numRegisteredStream = %d", __FUNCTION__, parentStream->m_numRegisteredStream);
        ALOGV("(%s): Enabling Jpeg", __FUNCTION__);
        return 0;
    }
    else if (format == HAL_PIXEL_FORMAT_YCrCb_420_SP || format == HAL_PIXEL_FORMAT_YV12) {
        *stream_id = STREAM_ID_PRVCB;

        subParameters = &m_subStreams[STREAM_ID_PRVCB];
        memset(subParameters, 0, sizeof(substream_parameters_t));

        parentStream = (StreamThread*)(m_streamThreads[0].get());
        if (!parentStream) {
            return 1;
        }

        *format_actual = format;
        *usage = GRALLOC_USAGE_SW_WRITE_OFTEN;
        if (m_wideAspect)
            *usage |= GRALLOC_USAGE_PRIVATE_CHROMA;
        *max_buffers = 7;

        subParameters->type         = SUBSTREAM_TYPE_PRVCB;
        subParameters->width        = width;
        subParameters->height       = height;
        subParameters->format       = *format_actual;
        subParameters->svcPlanes     = NUM_PLANES(*format_actual);
        subParameters->streamOps     = stream_ops;
        subParameters->usage         = *usage;
        subParameters->numOwnSvcBuffers = *max_buffers;
        subParameters->numSvcBufsInHal  = 0;
        subParameters->needBufferInit    = false;
        subParameters->minUndequedBuffer = 2;

        if (format == HAL_PIXEL_FORMAT_YCrCb_420_SP) {
            subParameters->internalFormat = HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP;
            subParameters->internalPlanes = NUM_PLANES(HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP);
        }
        else {
            subParameters->internalFormat = HAL_PIXEL_FORMAT_EXYNOS_YV12;
            subParameters->internalPlanes = NUM_PLANES(HAL_PIXEL_FORMAT_EXYNOS_YV12);
        }

        res = parentStream->attachSubStream(STREAM_ID_PRVCB, 20);
        if (res != NO_ERROR) {
            ALOGE("(%s): substream attach failed. res(%d)", __FUNCTION__, res);
            return 1;
        }
        ALOGV("(%s): m_numRegisteredStream = %d", __FUNCTION__, parentStream->m_numRegisteredStream);
        ALOGV("(%s): Enabling previewcb", __FUNCTION__);
        return 0;
    }
    ALOGE("(%s): Unsupported Pixel Format", __FUNCTION__);
    return 1;
}

int ExynosCameraHWInterface2::registerStreamBuffers(uint32_t stream_id,
        int num_buffers, buffer_handle_t *registeringBuffers)
{
    int                     i,j;
    void                    *virtAddr[3];
    int                     plane_index = 0;
    StreamThread *          targetStream;
    stream_parameters_t     *targetStreamParms;
    node_info_t             *currentNode;

    struct v4l2_buffer v4l2_buf;
    struct v4l2_plane  planes[VIDEO_MAX_PLANES];

    ALOGD("(%s): stream_id(%d), num_buff(%d), handle(%x) ", __FUNCTION__,
        stream_id, num_buffers, (uint32_t)registeringBuffers);

    if (stream_id == STREAM_ID_PREVIEW && m_streamThreads[0].get()) {
        targetStream = m_streamThreads[0].get();
        targetStreamParms = &(m_streamThreads[0]->m_parameters);

    }
    else if (stream_id == STREAM_ID_JPEG || stream_id == STREAM_ID_RECORD || stream_id == STREAM_ID_PRVCB) {
        substream_parameters_t  *targetParms;
        targetParms = &m_subStreams[stream_id];

        targetParms->numSvcBuffers = num_buffers;

        for (i = 0 ; i < targetParms->numSvcBuffers ; i++) {
            ALOGV("(%s): registering substream(%d) Buffers[%d] (%x) ", __FUNCTION__,
                i, stream_id, (uint32_t)(registeringBuffers[i]));
            if (m_grallocHal) {
                if (m_grallocHal->lock(m_grallocHal, registeringBuffers[i],
                       targetParms->usage, 0, 0,
                       targetParms->width, targetParms->height, virtAddr) != 0) {
                    ALOGE("ERR(%s): could not obtain gralloc buffer", __FUNCTION__);
                }
                else {
                    ExynosBuffer currentBuf;
                    const private_handle_t *priv_handle = reinterpret_cast<const private_handle_t *>(registeringBuffers[i]);
                    if (targetParms->svcPlanes == 1) {
                        currentBuf.fd.extFd[0] = priv_handle->fd;
                        currentBuf.size.extS[0] = priv_handle->size;
                        currentBuf.size.extS[1] = 0;
                        currentBuf.size.extS[2] = 0;
                    } else if (targetParms->svcPlanes == 2) {
                        currentBuf.fd.extFd[0] = priv_handle->fd;
                        currentBuf.fd.extFd[1] = priv_handle->fd1;

                    } else if (targetParms->svcPlanes == 3) {
                        currentBuf.fd.extFd[0] = priv_handle->fd;
                        currentBuf.fd.extFd[1] = priv_handle->fd1;
                        currentBuf.fd.extFd[2] = priv_handle->fd2;
                    }
                    for (plane_index = 0 ; plane_index < targetParms->svcPlanes ; plane_index++) {
                        currentBuf.virt.extP[plane_index] = (char *)virtAddr[plane_index];
                        CAM_LOGV("DEBUG(%s): plane(%d): fd(%d) addr(%x) size(%d)",
                             __FUNCTION__, plane_index, currentBuf.fd.extFd[plane_index],
                             (unsigned int)currentBuf.virt.extP[plane_index], currentBuf.size.extS[plane_index]);
                    }
                    targetParms->svcBufStatus[i]  = ON_SERVICE;
                    targetParms->svcBuffers[i]    = currentBuf;
                    targetParms->svcBufHandle[i]  = registeringBuffers[i];
                }
            }
        }
        targetParms->needBufferInit = true;
        return 0;
    }
    else if (stream_id == STREAM_ID_ZSL && m_streamThreads[1].get()) {
        targetStream = m_streamThreads[1].get();
        targetStreamParms = &(m_streamThreads[1]->m_parameters);
    }
    else {
        ALOGE("(%s): unregistered stream id (%d)", __FUNCTION__, stream_id);
        return 1;
    }

    if (targetStream->streamType == STREAM_TYPE_DIRECT) {
        if (num_buffers < targetStreamParms->numHwBuffers) {
            ALOGE("ERR(%s) registering insufficient num of buffers (%d) < (%d)",
                __FUNCTION__, num_buffers, targetStreamParms->numHwBuffers);
            return 1;
        }
    }
    CAM_LOGV("DEBUG(%s): format(%x) width(%d), height(%d) svcPlanes(%d)",
            __FUNCTION__, targetStreamParms->format, targetStreamParms->width,
            targetStreamParms->height, targetStreamParms->planes);
    targetStreamParms->numSvcBuffers = num_buffers;
    currentNode = targetStreamParms->node;
    currentNode->width      = targetStreamParms->width;
    currentNode->height     = targetStreamParms->height;
    currentNode->format     = HAL_PIXEL_FORMAT_2_V4L2_PIX(targetStreamParms->format);
    currentNode->planes     = targetStreamParms->planes;
    currentNode->buffers    = targetStreamParms->numHwBuffers;
    cam_int_s_input(currentNode, m_camera_info.sensor_id);
    cam_int_s_fmt(currentNode);
    cam_int_reqbufs(currentNode);
    for (i = 0 ; i < targetStreamParms->numSvcBuffers ; i++) {
        ALOGV("DEBUG(%s): registering Stream Buffers[%d] (%x) ", __FUNCTION__,
            i, (uint32_t)(registeringBuffers[i]));
                v4l2_buf.m.planes   = planes;
                v4l2_buf.type       = currentNode->type;
                v4l2_buf.memory     = currentNode->memory;
                v4l2_buf.index      = i;
                v4l2_buf.length     = currentNode->planes;

                ExynosBuffer currentBuf;
                ExynosBuffer metaBuf;
                const private_handle_t *priv_handle = reinterpret_cast<const private_handle_t *>(registeringBuffers[i]);

                m_getAlignedYUVSize(currentNode->format,
                    currentNode->width, currentNode->height, &currentBuf);

                ALOGV("DEBUG(%s):  ion_size(%d), stride(%d), ", __FUNCTION__, priv_handle->size, priv_handle->stride);
                if (currentNode->planes == 1) {
                    v4l2_buf.m.planes[0].m.fd = priv_handle->fd;
                    currentBuf.fd.extFd[0] = priv_handle->fd;
                    currentBuf.size.extS[0] = priv_handle->size;
                    currentBuf.size.extS[1] = 0;
                    currentBuf.size.extS[2] = 0;
                } else if (currentNode->planes == 2) {
                    v4l2_buf.m.planes[0].m.fd = priv_handle->fd;
                    v4l2_buf.m.planes[1].m.fd = priv_handle->fd1;
                    currentBuf.fd.extFd[0] = priv_handle->fd;
                    currentBuf.fd.extFd[1] = priv_handle->fd1;

                } else if (currentNode->planes == 3) {
                    v4l2_buf.m.planes[0].m.fd = priv_handle->fd;
                    v4l2_buf.m.planes[2].m.fd = priv_handle->fd1;
                    v4l2_buf.m.planes[1].m.fd = priv_handle->fd2;
                    currentBuf.fd.extFd[0] = priv_handle->fd;
                    currentBuf.fd.extFd[2] = priv_handle->fd1;
                    currentBuf.fd.extFd[1] = priv_handle->fd2;
                }

                for (plane_index = 0 ; plane_index < (int)v4l2_buf.length ; plane_index++) {
                    if (targetStreamParms->needsIonMap)
                        currentBuf.virt.extP[plane_index] = (char *)ion_map(currentBuf.fd.extFd[plane_index], currentBuf.size.extS[plane_index], 0);
                    v4l2_buf.m.planes[plane_index].length  = currentBuf.size.extS[plane_index];
                    ALOGV("(%s): MAPPING plane(%d): fd(%d) addr(%x), length(%d)",
                         __FUNCTION__, plane_index, v4l2_buf.m.planes[plane_index].m.fd,
                         (unsigned int)currentBuf.virt.extP[plane_index],
                         v4l2_buf.m.planes[plane_index].length);
                }

                if (i < currentNode->buffers) {


#ifdef ENABLE_FRAME_SYNC
                    /* add plane for metadata*/
                    metaBuf.size.extS[0] = 4*1024;
                    allocCameraMemory(m_ionCameraClient , &metaBuf, 1, 1<<0);

                    v4l2_buf.length += targetStreamParms->metaPlanes;
                    v4l2_buf.m.planes[v4l2_buf.length-1].m.fd = metaBuf.fd.extFd[0];
                    v4l2_buf.m.planes[v4l2_buf.length-1].length = metaBuf.size.extS[0];

                    ALOGV("Qbuf metaBuf: fd(%d), length(%d) plane(%d)", metaBuf.fd.extFd[0], metaBuf.size.extS[0], v4l2_buf.length);
#endif
                    if (exynos_v4l2_qbuf(currentNode->fd, &v4l2_buf) < 0) {
                        ALOGE("ERR(%s): stream id(%d) exynos_v4l2_qbuf() fail fd(%d)",
                            __FUNCTION__, stream_id, currentNode->fd);
                    }
                    ALOGV("DEBUG(%s): stream id(%d) exynos_v4l2_qbuf() success fd(%d)",
                            __FUNCTION__, stream_id, currentNode->fd);
                    targetStreamParms->svcBufStatus[i]  = REQUIRES_DQ_FROM_SVC;
                }
                else {
                    targetStreamParms->svcBufStatus[i]  = ON_SERVICE;
                }

                targetStreamParms->svcBuffers[i]       = currentBuf;
                targetStreamParms->metaBuffers[i] = metaBuf;
                targetStreamParms->svcBufHandle[i]     = registeringBuffers[i];
            }

    ALOGV("DEBUG(%s): calling  streamon stream id = %d", __FUNCTION__, stream_id);
    cam_int_streamon(targetStreamParms->node);
    ALOGV("DEBUG(%s): calling  streamon END", __FUNCTION__);
    currentNode->status = true;
    ALOGV("DEBUG(%s): END registerStreamBuffers", __FUNCTION__);

    return 0;
}

int ExynosCameraHWInterface2::releaseStream(uint32_t stream_id)
{
    StreamThread *targetStream;
    status_t res = NO_ERROR;
    ALOGD("(%s): stream_id(%d)", __FUNCTION__, stream_id);
    bool releasingScpMain = false;

    if (stream_id == STREAM_ID_PREVIEW) {
        targetStream = (StreamThread*)(m_streamThreads[0].get());
        if (!targetStream) {
            ALOGW("(%s): Stream Not Exists", __FUNCTION__);
            return NO_ERROR;
        }
        targetStream->m_numRegisteredStream--;
        ALOGV("(%s): m_numRegisteredStream = %d", __FUNCTION__, targetStream->m_numRegisteredStream);
        releasingScpMain = true;
        if (targetStream->m_parameters.needsIonMap) {
            for (int i = 0; i < targetStream->m_parameters.numSvcBuffers; i++) {
                for (int j = 0; j < targetStream->m_parameters.planes; j++) {
                    ion_unmap(targetStream->m_parameters.svcBuffers[i].virt.extP[j],
                                    targetStream->m_parameters.svcBuffers[i].size.extS[j]);
                    ALOGV("(%s) ummap stream buffer[%d], plane(%d), fd %d vaddr %x", __FUNCTION__, i, j,
                                  targetStream->m_parameters.svcBuffers[i].fd.extFd[j], (unsigned int)(targetStream->m_parameters.svcBuffers[i].virt.extP[j]));
                }
            }
        }
    } else if (stream_id == STREAM_ID_JPEG) {
        if (m_resizeBuf.size.s != 0) {
            freeCameraMemory(&m_resizeBuf, 1);
        }
        memset(&m_subStreams[stream_id], 0, sizeof(substream_parameters_t));

        targetStream = (StreamThread*)(m_streamThreads[1].get());
        if (!targetStream) {
            ALOGW("(%s): Stream Not Exists", __FUNCTION__);
            return NO_ERROR;
        }

        if (targetStream->detachSubStream(stream_id) != NO_ERROR) {
            ALOGE("(%s): substream detach failed. res(%d)", __FUNCTION__, res);
            return 1;
        }
        ALOGV("(%s): m_numRegisteredStream = %d", __FUNCTION__, targetStream->m_numRegisteredStream);
        return 0;
    } else if (stream_id == STREAM_ID_RECORD) {
        memset(&m_subStreams[stream_id], 0, sizeof(substream_parameters_t));

        targetStream = (StreamThread*)(m_streamThreads[0].get());
        if (!targetStream) {
            ALOGW("(%s): Stream Not Exists", __FUNCTION__);
            return NO_ERROR;
        }

        if (targetStream->detachSubStream(stream_id) != NO_ERROR) {
            ALOGE("(%s): substream detach failed. res(%d)", __FUNCTION__, res);
            return 1;
        }

        if (targetStream->m_numRegisteredStream != 0)
            return 0;
    } else if (stream_id == STREAM_ID_PRVCB) {
        if (m_previewCbBuf.size.s != 0) {
            freeCameraMemory(&m_previewCbBuf, m_subStreams[stream_id].internalPlanes);
        }
        memset(&m_subStreams[stream_id], 0, sizeof(substream_parameters_t));

        targetStream = (StreamThread*)(m_streamThreads[0].get());
        if (!targetStream) {
            ALOGW("(%s): Stream Not Exists", __FUNCTION__);
            return NO_ERROR;
        }

        if (targetStream->detachSubStream(stream_id) != NO_ERROR) {
            ALOGE("(%s): substream detach failed. res(%d)", __FUNCTION__, res);
            return 1;
        }

        if (targetStream->m_numRegisteredStream != 0)
            return 0;
    } else if (stream_id == STREAM_ID_ZSL) {
        targetStream = (StreamThread*)(m_streamThreads[1].get());
        if (!targetStream) {
            ALOGW("(%s): Stream Not Exists", __FUNCTION__);
            return NO_ERROR;
        }

        targetStream->m_numRegisteredStream--;
        ALOGV("(%s): m_numRegisteredStream = %d", __FUNCTION__, targetStream->m_numRegisteredStream);
        if (targetStream->m_parameters.needsIonMap) {
            for (int i = 0; i < targetStream->m_parameters.numSvcBuffers; i++) {
                for (int j = 0; j < targetStream->m_parameters.planes; j++) {
                    ion_unmap(targetStream->m_parameters.svcBuffers[i].virt.extP[j],
                                    targetStream->m_parameters.svcBuffers[i].size.extS[j]);
                    ALOGV("(%s) ummap stream buffer[%d], plane(%d), fd %d vaddr %x", __FUNCTION__, i, j,
                                  targetStream->m_parameters.svcBuffers[i].fd.extFd[j], (unsigned int)(targetStream->m_parameters.svcBuffers[i].virt.extP[j]));
                }
            }
        }
    } else {
        ALOGE("ERR:(%s): wrong stream id (%d)", __FUNCTION__, stream_id);
        return 1;
    }

    if (m_sensorThread != NULL && releasingScpMain) {
        m_sensorThread->release();
        ALOGD("(%s): START Waiting for (indirect) sensor thread termination", __FUNCTION__);
        while (!m_sensorThread->IsTerminated())
            usleep(SIG_WAITING_TICK);
        ALOGD("(%s): END   Waiting for (indirect) sensor thread termination", __FUNCTION__);
    }

    if (m_streamThreads[1]->m_numRegisteredStream == 0 && m_streamThreads[1]->m_activated) {
        ALOGV("(%s): deactivating stream thread 1 ", __FUNCTION__);
        targetStream = (StreamThread*)(m_streamThreads[1].get());
        targetStream->m_releasing = true;
        ALOGD("START stream thread release %d", __LINE__);
        do {
            targetStream->release();
            usleep(SIG_WAITING_TICK);
        } while (targetStream->m_releasing);
        m_camera_info.capture.status = false;
        ALOGD("END   stream thread release %d", __LINE__);
    }

    if (releasingScpMain || (m_streamThreads[0].get() != NULL && m_streamThreads[0]->m_numRegisteredStream == 0 && m_streamThreads[0]->m_activated)) {
        ALOGV("(%s): deactivating stream thread 0", __FUNCTION__);
        targetStream = (StreamThread*)(m_streamThreads[0].get());
        targetStream->m_releasing = true;
        ALOGD("(%s): START Waiting for (indirect) stream thread release - line(%d)", __FUNCTION__, __LINE__);
        do {
            targetStream->release();
            usleep(SIG_WAITING_TICK);
        } while (targetStream->m_releasing);
        ALOGD("(%s): END   Waiting for (indirect) stream thread release - line(%d)", __FUNCTION__, __LINE__);
        targetStream->SetSignal(SIGNAL_THREAD_TERMINATE);

        if (targetStream != NULL) {
            ALOGD("(%s): START Waiting for (indirect) stream thread termination", __FUNCTION__);
            while (!targetStream->IsTerminated())
                usleep(SIG_WAITING_TICK);
            ALOGD("(%s): END   Waiting for (indirect) stream thread termination", __FUNCTION__);
            m_streamThreads[0] = NULL;
        }
        if (m_camera_info.capture.status == true) {
            m_scpForceSuspended = true;
        }
        m_isIspStarted = false;
    }
    ALOGV("(%s): END", __FUNCTION__);
    return 0;
}

int ExynosCameraHWInterface2::allocateReprocessStream(
    uint32_t /*width*/, uint32_t /*height*/, uint32_t /*format*/,
    const camera2_stream_in_ops_t* /*reprocess_stream_ops*/,
    uint32_t* /*stream_id*/, uint32_t* /*consumer_usage*/, uint32_t* /*max_buffers*/)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return 0;
}

int ExynosCameraHWInterface2::allocateReprocessStreamFromStream(
            uint32_t output_stream_id,
            const camera2_stream_in_ops_t *reprocess_stream_ops,
            // outputs
            uint32_t *stream_id)
{
    ALOGD("(%s): output_stream_id(%d)", __FUNCTION__, output_stream_id);
    *stream_id = STREAM_ID_JPEG_REPROCESS;

    m_reprocessStreamId = *stream_id;
    m_reprocessOps = reprocess_stream_ops;
    m_reprocessOutputStreamId = output_stream_id;
    return 0;
}

int ExynosCameraHWInterface2::releaseReprocessStream(uint32_t stream_id)
{
    ALOGD("(%s): stream_id(%d)", __FUNCTION__, stream_id);
    if (stream_id == STREAM_ID_JPEG_REPROCESS) {
        m_reprocessStreamId = 0;
        m_reprocessOps = NULL;
        m_reprocessOutputStreamId = 0;
        return 0;
    }
    return 1;
}

int ExynosCameraHWInterface2::triggerAction(uint32_t trigger_id, int ext1, int ext2)
{
    Mutex::Autolock lock(m_afModeTriggerLock);
    ALOGV("DEBUG(%s): id(%x), %d, %d", __FUNCTION__, trigger_id, ext1, ext2);

    switch (trigger_id) {
    case CAMERA2_TRIGGER_AUTOFOCUS:
        ALOGV("DEBUG(%s):TRIGGER_AUTOFOCUS id(%d)", __FUNCTION__, ext1);
        OnAfTrigger(ext1);
        break;

    case CAMERA2_TRIGGER_CANCEL_AUTOFOCUS:
        ALOGV("DEBUG(%s):CANCEL_AUTOFOCUS id(%d)", __FUNCTION__, ext1);
        OnAfCancel(ext1);
        break;
    case CAMERA2_TRIGGER_PRECAPTURE_METERING:
        ALOGV("DEBUG(%s):CAMERA2_TRIGGER_PRECAPTURE_METERING id(%d)", __FUNCTION__, ext1);
        OnPrecaptureMeteringTriggerStart(ext1);
        break;
    default:
        break;
    }
    return 0;
}

int ExynosCameraHWInterface2::setNotifyCallback(camera2_notify_callback notify_cb, void *user)
{
    ALOGV("DEBUG(%s): cb_addr(%x)", __FUNCTION__, (unsigned int)notify_cb);
    m_notifyCb = notify_cb;
    m_callbackCookie = user;
    return 0;
}

int ExynosCameraHWInterface2::getMetadataVendorTagOps(vendor_tag_query_ops_t **ops)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    *ops = NULL;
    return 0;
}

int ExynosCameraHWInterface2::dump(int /*fd*/)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return 0;
}

void ExynosCameraHWInterface2::m_getAlignedYUVSize(int colorFormat, int w, int h, ExynosBuffer *buf)
{
    switch (colorFormat) {
    // 1p
    case V4L2_PIX_FMT_RGB565 :
    case V4L2_PIX_FMT_YUYV :
    case V4L2_PIX_FMT_UYVY :
    case V4L2_PIX_FMT_VYUY :
    case V4L2_PIX_FMT_YVYU :
        buf->size.extS[0] = FRAME_SIZE(V4L2_PIX_2_HAL_PIXEL_FORMAT(colorFormat), w, h);
        buf->size.extS[1] = 0;
        buf->size.extS[2] = 0;
        break;
    // 2p
    case V4L2_PIX_FMT_NV12 :
    case V4L2_PIX_FMT_NV12T :
    case V4L2_PIX_FMT_NV21 :
        buf->size.extS[0] = ALIGN(w,   16) * ALIGN(h,   16);
        buf->size.extS[1] = ALIGN(w/2, 16) * ALIGN(h/2, 16);
        buf->size.extS[2] = 0;
        break;
    case V4L2_PIX_FMT_NV12M :
    case V4L2_PIX_FMT_NV12MT_16X16 :
    case V4L2_PIX_FMT_NV21M:
        buf->size.extS[0] = ALIGN(w, 16) * ALIGN(h,     16);
        buf->size.extS[1] = ALIGN(buf->size.extS[0] / 2, 256);
        buf->size.extS[2] = 0;
        break;
    case V4L2_PIX_FMT_NV16 :
    case V4L2_PIX_FMT_NV61 :
        buf->size.extS[0] = ALIGN(w, 16) * ALIGN(h, 16);
        buf->size.extS[1] = ALIGN(w, 16) * ALIGN(h,  16);
        buf->size.extS[2] = 0;
        break;
     // 3p
    case V4L2_PIX_FMT_YUV420 :
    case V4L2_PIX_FMT_YVU420 :
        buf->size.extS[0] = (w * h);
        buf->size.extS[1] = (w * h) >> 2;
        buf->size.extS[2] = (w * h) >> 2;
        break;
    case V4L2_PIX_FMT_YUV420M:
    case V4L2_PIX_FMT_YVU420M :
        buf->size.extS[0] = ALIGN(w,  32) * ALIGN(h,  16);
        buf->size.extS[1] = ALIGN(w/2, 16) * ALIGN(h/2, 8);
        buf->size.extS[2] = ALIGN(w/2, 16) * ALIGN(h/2, 8);
        break;
    case V4L2_PIX_FMT_YUV422P :
        buf->size.extS[0] = ALIGN(w,  16) * ALIGN(h,  16);
        buf->size.extS[1] = ALIGN(w/2, 16) * ALIGN(h/2, 8);
        buf->size.extS[2] = ALIGN(w/2, 16) * ALIGN(h/2, 8);
        break;
    default:
        ALOGE("ERR(%s):unmatched colorFormat(%d)", __FUNCTION__, colorFormat);
        return;
        break;
    }
}

bool ExynosCameraHWInterface2::m_getRatioSize(int  src_w,  int   src_h,
                                             int  dst_w,  int   dst_h,
                                             int *crop_x, int *crop_y,
                                             int *crop_w, int *crop_h,
                                             int zoom)
{
    *crop_w = src_w;
    *crop_h = src_h;

    if (   src_w != dst_w
        || src_h != dst_h) {
        float src_ratio = 1.0f;
        float dst_ratio = 1.0f;

        // ex : 1024 / 768
        src_ratio = (float)src_w / (float)src_h;

        // ex : 352  / 288
        dst_ratio = (float)dst_w / (float)dst_h;

        if (dst_w * dst_h < src_w * src_h) {
            if (dst_ratio <= src_ratio) {
                // shrink w
                *crop_w = src_h * dst_ratio;
                *crop_h = src_h;
            } else {
                // shrink h
                *crop_w = src_w;
                *crop_h = src_w / dst_ratio;
            }
        } else {
            if (dst_ratio <= src_ratio) {
                // shrink w
                *crop_w = src_h * dst_ratio;
                *crop_h = src_h;
            } else {
                // shrink h
                *crop_w = src_w;
                *crop_h = src_w / dst_ratio;
            }
        }
    }

    if (zoom != 0) {
        float zoomLevel = ((float)zoom + 10.0) / 10.0;
        *crop_w = (int)((float)*crop_w / zoomLevel);
        *crop_h = (int)((float)*crop_h / zoomLevel);
    }

    #define CAMERA_CROP_WIDTH_RESTRAIN_NUM  (0x2)
    unsigned int w_align = (*crop_w & (CAMERA_CROP_WIDTH_RESTRAIN_NUM - 1));
    if (w_align != 0) {
        if (  (CAMERA_CROP_WIDTH_RESTRAIN_NUM >> 1) <= w_align
            && (int)(*crop_w + (CAMERA_CROP_WIDTH_RESTRAIN_NUM - w_align)) <= dst_w) {
            *crop_w += (CAMERA_CROP_WIDTH_RESTRAIN_NUM - w_align);
        }
        else
            *crop_w -= w_align;
    }

    #define CAMERA_CROP_HEIGHT_RESTRAIN_NUM  (0x2)
    unsigned int h_align = (*crop_h & (CAMERA_CROP_HEIGHT_RESTRAIN_NUM - 1));
    if (h_align != 0) {
        if (  (CAMERA_CROP_HEIGHT_RESTRAIN_NUM >> 1) <= h_align
            && (int)(*crop_h + (CAMERA_CROP_HEIGHT_RESTRAIN_NUM - h_align)) <= dst_h) {
            *crop_h += (CAMERA_CROP_HEIGHT_RESTRAIN_NUM - h_align);
        }
        else
            *crop_h -= h_align;
    }

    *crop_x = (src_w - *crop_w) >> 1;
    *crop_y = (src_h - *crop_h) >> 1;

    if (*crop_x & (CAMERA_CROP_WIDTH_RESTRAIN_NUM >> 1))
        *crop_x -= 1;

    if (*crop_y & (CAMERA_CROP_HEIGHT_RESTRAIN_NUM >> 1))
        *crop_y -= 1;

    return true;
}

BayerBufManager::BayerBufManager()
{
    ALOGV("DEBUG(%s): ", __FUNCTION__);
    for (int i = 0; i < NUM_BAYER_BUFFERS ; i++) {
        entries[i].status = BAYER_ON_HAL_EMPTY;
        entries[i].reqFrameCnt = 0;
    }
    sensorEnqueueHead = 0;
    sensorDequeueHead = 0;
    ispEnqueueHead = 0;
    ispDequeueHead = 0;
    numOnSensor = 0;
    numOnIsp = 0;
    numOnHalFilled = 0;
    numOnHalEmpty = NUM_BAYER_BUFFERS;
}

BayerBufManager::~BayerBufManager()
{
    ALOGV("%s", __FUNCTION__);
}

int     BayerBufManager::GetIndexForSensorEnqueue()
{
    int ret = 0;
    if (numOnHalEmpty == 0)
        ret = -1;
    else
        ret = sensorEnqueueHead;
    ALOGV("DEBUG(%s): returning (%d)", __FUNCTION__, ret);
    return ret;
}

int    BayerBufManager::MarkSensorEnqueue(int index)
{
    ALOGV("DEBUG(%s)    : BayerIndex[%d] ", __FUNCTION__, index);

    // sanity check
    if (index != sensorEnqueueHead) {
        ALOGV("DEBUG(%s)    : Abnormal BayerIndex[%d] - expected[%d]", __FUNCTION__, index, sensorEnqueueHead);
        return -1;
    }
    if (entries[index].status != BAYER_ON_HAL_EMPTY) {
        ALOGV("DEBUG(%s)    : Abnormal status in BayerIndex[%d] = (%d) expected (%d)", __FUNCTION__,
            index, entries[index].status, BAYER_ON_HAL_EMPTY);
        return -1;
    }

    entries[index].status = BAYER_ON_SENSOR;
    entries[index].reqFrameCnt = 0;
    numOnHalEmpty--;
    numOnSensor++;
    sensorEnqueueHead = GetNextIndex(index);
    ALOGV("DEBUG(%s) END: HAL-e(%d) HAL-f(%d) Sensor(%d) ISP(%d) ",
        __FUNCTION__, numOnHalEmpty, numOnHalFilled, numOnSensor, numOnIsp);
    return 0;
}

int    BayerBufManager::MarkSensorDequeue(int index, int reqFrameCnt, nsecs_t* /*timeStamp*/)
{
    ALOGV("DEBUG(%s)    : BayerIndex[%d] reqFrameCnt(%d)", __FUNCTION__, index, reqFrameCnt);

    if (entries[index].status != BAYER_ON_SENSOR) {
        ALOGE("DEBUG(%s)    : Abnormal status in BayerIndex[%d] = (%d) expected (%d)", __FUNCTION__,
            index, entries[index].status, BAYER_ON_SENSOR);
        return -1;
    }

    entries[index].status = BAYER_ON_HAL_FILLED;
    numOnHalFilled++;
    numOnSensor--;

    return 0;
}

int     BayerBufManager::GetIndexForIspEnqueue(int *reqFrameCnt)
{
    int ret = 0;
    if (numOnHalFilled == 0)
        ret = -1;
    else {
        *reqFrameCnt = entries[ispEnqueueHead].reqFrameCnt;
        ret = ispEnqueueHead;
    }
    ALOGV("DEBUG(%s): returning BayerIndex[%d]", __FUNCTION__, ret);
    return ret;
}

int     BayerBufManager::GetIndexForIspDequeue(int *reqFrameCnt)
{
    int ret = 0;
    if (numOnIsp == 0)
        ret = -1;
    else {
        *reqFrameCnt = entries[ispDequeueHead].reqFrameCnt;
        ret = ispDequeueHead;
    }
    ALOGV("DEBUG(%s): returning BayerIndex[%d]", __FUNCTION__, ret);
    return ret;
}

int    BayerBufManager::MarkIspEnqueue(int index)
{
    ALOGV("DEBUG(%s)    : BayerIndex[%d] ", __FUNCTION__, index);

    // sanity check
    if (index != ispEnqueueHead) {
        ALOGV("DEBUG(%s)    : Abnormal BayerIndex[%d] - expected[%d]", __FUNCTION__, index, ispEnqueueHead);
        return -1;
    }
    if (entries[index].status != BAYER_ON_HAL_FILLED) {
        ALOGV("DEBUG(%s)    : Abnormal status in BayerIndex[%d] = (%d) expected (%d)", __FUNCTION__,
            index, entries[index].status, BAYER_ON_HAL_FILLED);
        return -1;
    }

    entries[index].status = BAYER_ON_ISP;
    numOnHalFilled--;
    numOnIsp++;
    ispEnqueueHead = GetNextIndex(index);
    ALOGV("DEBUG(%s) END: HAL-e(%d) HAL-f(%d) Sensor(%d) ISP(%d) ",
        __FUNCTION__, numOnHalEmpty, numOnHalFilled, numOnSensor, numOnIsp);
    return 0;
}

int    BayerBufManager::MarkIspDequeue(int index)
{
    ALOGV("DEBUG(%s)    : BayerIndex[%d]", __FUNCTION__, index);

    // sanity check
    if (index != ispDequeueHead) {
        ALOGV("DEBUG(%s)    : Abnormal BayerIndex[%d] - expected[%d]", __FUNCTION__, index, ispDequeueHead);
        return -1;
    }
    if (entries[index].status != BAYER_ON_ISP) {
        ALOGV("DEBUG(%s)    : Abnormal status in BayerIndex[%d] = (%d) expected (%d)", __FUNCTION__,
            index, entries[index].status, BAYER_ON_ISP);
        return -1;
    }

    entries[index].status = BAYER_ON_HAL_EMPTY;
    entries[index].reqFrameCnt = 0;
    numOnHalEmpty++;
    numOnIsp--;
    ispDequeueHead = GetNextIndex(index);
    ALOGV("DEBUG(%s) END: HAL-e(%d) HAL-f(%d) Sensor(%d) ISP(%d) ",
        __FUNCTION__, numOnHalEmpty, numOnHalFilled, numOnSensor, numOnIsp);
    return 0;
}

int BayerBufManager::GetNumOnSensor()
{
    return numOnSensor;
}

int BayerBufManager::GetNumOnHalFilled()
{
    return numOnHalFilled;
}

int BayerBufManager::GetNumOnIsp()
{
    return numOnIsp;
}

int     BayerBufManager::GetNextIndex(int index)
{
    index++;
    if (index >= NUM_BAYER_BUFFERS)
        index = 0;

    return index;
}

void ExynosCameraHWInterface2::m_mainThreadFunc(SignalDrivenThread * self)
{
    camera_metadata_t *currentRequest = NULL;
    camera_metadata_t *currentFrame = NULL;
    size_t numEntries = 0;
    size_t frameSize = 0;
    camera_metadata_t * preparedFrame = NULL;
    camera_metadata_t *deregisteredRequest = NULL;
    uint32_t currentSignal = self->GetProcessingSignal();
    MainThread *  selfThread      = ((MainThread*)self);
    int res = 0;

    int ret;
    int afMode;
    uint32_t afRegion[4];

    ALOGV("DEBUG(%s): m_mainThreadFunc (%x)", __FUNCTION__, currentSignal);

    if (currentSignal & SIGNAL_THREAD_RELEASE) {
        ALOGV("DEBUG(%s): processing SIGNAL_THREAD_RELEASE", __FUNCTION__);

        ALOGV("DEBUG(%s): processing SIGNAL_THREAD_RELEASE DONE", __FUNCTION__);
        selfThread->SetSignal(SIGNAL_THREAD_TERMINATE);
        return;
    }

    if (currentSignal & SIGNAL_MAIN_REQ_Q_NOT_EMPTY) {
        ALOGV("DEBUG(%s): MainThread processing SIGNAL_MAIN_REQ_Q_NOT_EMPTY", __FUNCTION__);
        if (m_requestManager->IsRequestQueueFull()==false) {
            Mutex::Autolock lock(m_afModeTriggerLock);
            m_requestQueueOps->dequeue_request(m_requestQueueOps, &currentRequest);
            if (NULL == currentRequest) {
                ALOGD("DEBUG(%s)(0x%x): No more service requests left in the queue ", __FUNCTION__, currentSignal);
                m_isRequestQueueNull = true;
                if (m_requestManager->IsVdisEnable())
                    m_vdisBubbleCnt = 1;
            }
            else {
                m_requestManager->RegisterRequest(currentRequest, &afMode, afRegion);

                SetAfMode((enum aa_afmode)afMode);
                SetAfRegion(afRegion);

                m_numOfRemainingReqInSvc = m_requestQueueOps->request_count(m_requestQueueOps);
                ALOGV("DEBUG(%s): remaining req cnt (%d)", __FUNCTION__, m_numOfRemainingReqInSvc);
                if (m_requestManager->IsRequestQueueFull()==false)
                    selfThread->SetSignal(SIGNAL_MAIN_REQ_Q_NOT_EMPTY); // dequeue repeatedly

                m_sensorThread->SetSignal(SIGNAL_SENSOR_START_REQ_PROCESSING);
            }
        }
        else {
            m_isRequestQueuePending = true;
        }
    }

    if (currentSignal & SIGNAL_MAIN_STREAM_OUTPUT_DONE) {
        ALOGV("DEBUG(%s): MainThread processing SIGNAL_MAIN_STREAM_OUTPUT_DONE", __FUNCTION__);
        /*while (1)*/ {
            ret = m_requestManager->PrepareFrame(&numEntries, &frameSize, &preparedFrame, GetAfStateForService());
            if (ret == false)
                CAM_LOGE("ERR(%s): PrepareFrame ret = %d", __FUNCTION__, ret);

            m_requestManager->DeregisterRequest(&deregisteredRequest);

            ret = m_requestQueueOps->free_request(m_requestQueueOps, deregisteredRequest);
            if (ret < 0)
                CAM_LOGE("ERR(%s): free_request ret = %d", __FUNCTION__, ret);

            ret = m_frameQueueOps->dequeue_frame(m_frameQueueOps, numEntries, frameSize, &currentFrame);
            if (ret < 0)
                CAM_LOGE("ERR(%s): dequeue_frame ret = %d", __FUNCTION__, ret);

            if (currentFrame==NULL) {
                ALOGV("DBG(%s): frame dequeue returned NULL",__FUNCTION__ );
            }
            else {
                ALOGV("DEBUG(%s): frame dequeue done. numEntries(%d) frameSize(%d)",__FUNCTION__ , numEntries, frameSize);
            }
            res = append_camera_metadata(currentFrame, preparedFrame);
            if (res==0) {
                ALOGV("DEBUG(%s): frame metadata append success",__FUNCTION__);
                m_frameQueueOps->enqueue_frame(m_frameQueueOps, currentFrame);
            }
            else {
                ALOGE("ERR(%s): frame metadata append fail (%d)",__FUNCTION__, res);
            }
        }
        if (!m_isRequestQueueNull) {
            selfThread->SetSignal(SIGNAL_MAIN_REQ_Q_NOT_EMPTY);
        }

        if (getInProgressCount()>0) {
            ALOGV("DEBUG(%s): STREAM_OUTPUT_DONE and signalling REQ_PROCESSING",__FUNCTION__);
            m_sensorThread->SetSignal(SIGNAL_SENSOR_START_REQ_PROCESSING);
        }
    }
    ALOGV("DEBUG(%s): MainThread Exit", __FUNCTION__);
    return;
}

void ExynosCameraHWInterface2::DumpInfoWithShot(struct camera2_shot_ext * shot_ext)
{
    ALOGD("####  common Section");
    ALOGD("####                 magic(%x) ",
        shot_ext->shot.magicNumber);
    ALOGD("####  ctl Section");
    ALOGD("####     meta(%d) aper(%f) exp(%lld) duration(%lld) ISO(%d) AWB(%d)",
        shot_ext->shot.ctl.request.metadataMode,
        shot_ext->shot.ctl.lens.aperture,
        shot_ext->shot.ctl.sensor.exposureTime,
        shot_ext->shot.ctl.sensor.frameDuration,
        shot_ext->shot.ctl.sensor.sensitivity,
        shot_ext->shot.ctl.aa.awbMode);

    ALOGD("####                 OutputStream Sensor(%d) SCP(%d) SCC(%d) streams(%x)",
        shot_ext->request_sensor, shot_ext->request_scp, shot_ext->request_scc,
        shot_ext->shot.ctl.request.outputStreams[0]);

    ALOGD("####  DM Section");
    ALOGD("####     meta(%d) aper(%f) exp(%lld) duration(%lld) ISO(%d) timestamp(%lld) AWB(%d) cnt(%d)",
        shot_ext->shot.dm.request.metadataMode,
        shot_ext->shot.dm.lens.aperture,
        shot_ext->shot.dm.sensor.exposureTime,
        shot_ext->shot.dm.sensor.frameDuration,
        shot_ext->shot.dm.sensor.sensitivity,
        shot_ext->shot.dm.sensor.timeStamp,
        shot_ext->shot.dm.aa.awbMode,
        shot_ext->shot.dm.request.frameCount );
}

void ExynosCameraHWInterface2::m_preCaptureSetter(struct camera2_shot_ext * shot_ext)
{
    // Flash
    switch (m_ctlInfo.flash.m_flashCnt) {
    case IS_FLASH_STATE_ON:
        ALOGV("(%s): [Flash] Flash ON for Capture (%d)", __FUNCTION__, shot_ext->shot.ctl.request.frameCount);
        // check AF locked
        if (m_ctlInfo.flash.m_precaptureTriggerId > 0) {
            if (m_ctlInfo.flash.m_flashTimeOut == 0) {
                if (m_ctlInfo.flash.i_flashMode == AA_AEMODE_ON_ALWAYS_FLASH) {
                    shot_ext->shot.ctl.aa.aeflashMode = AA_FLASHMODE_ON_ALWAYS;
                    m_ctlInfo.flash.m_flashTimeOut = 5;
                } else
                    shot_ext->shot.ctl.aa.aeflashMode = AA_FLASHMODE_ON;
                m_ctlInfo.flash.m_flashCnt = IS_FLASH_STATE_ON_WAIT;
            } else {
                m_ctlInfo.flash.m_flashTimeOut--;
            }
        } else {
            if (m_ctlInfo.flash.i_flashMode == AA_AEMODE_ON_ALWAYS_FLASH) {
                shot_ext->shot.ctl.aa.aeflashMode = AA_FLASHMODE_ON_ALWAYS;
                m_ctlInfo.flash.m_flashTimeOut = 5;
            } else
                shot_ext->shot.ctl.aa.aeflashMode = AA_FLASHMODE_ON;
            m_ctlInfo.flash.m_flashCnt = IS_FLASH_STATE_ON_WAIT;
        }
        break;
    case IS_FLASH_STATE_ON_WAIT:
        break;
    case IS_FLASH_STATE_ON_DONE:
        if (!m_ctlInfo.flash.m_afFlashDoneFlg)
            // auto transition at pre-capture trigger
            m_ctlInfo.flash.m_flashCnt = IS_FLASH_STATE_AUTO_AE_AWB_LOCK;
        break;
    case IS_FLASH_STATE_AUTO_AE_AWB_LOCK:
        ALOGV("(%s): [Flash] IS_FLASH_AF_AUTO_AE_AWB_LOCK (%d)", __FUNCTION__, shot_ext->shot.ctl.request.frameCount);
        shot_ext->shot.ctl.aa.aeflashMode = AA_FLASHMODE_AUTO;
        //shot_ext->shot.ctl.aa.aeMode = AA_AEMODE_LOCKED;
        shot_ext->shot.ctl.aa.awbMode = AA_AWBMODE_LOCKED;
        m_ctlInfo.flash.m_flashCnt = IS_FLASH_STATE_AE_AWB_LOCK_WAIT;
        break;
    case IS_FLASH_STATE_AE_AWB_LOCK_WAIT:
    case IS_FLASH_STATE_AUTO_WAIT:
        shot_ext->shot.ctl.aa.aeMode =(enum aa_aemode)0;
        shot_ext->shot.ctl.aa.awbMode = (enum aa_awbmode)0;
        break;
    case IS_FLASH_STATE_AUTO_DONE:
        ALOGV("(%s): [Flash] IS_FLASH_AF_AUTO DONE (%d)", __FUNCTION__, shot_ext->shot.ctl.request.frameCount);
        shot_ext->shot.ctl.aa.aeflashMode = AA_FLASHMODE_OFF;
        break;
    case IS_FLASH_STATE_AUTO_OFF:
        ALOGV("(%s): [Flash] IS_FLASH_AF_AUTO Clear (%d)", __FUNCTION__, shot_ext->shot.ctl.request.frameCount);
        shot_ext->shot.ctl.aa.aeflashMode = AA_FLASHMODE_OFF;
        m_ctlInfo.flash.m_flashEnableFlg = false;
        break;
    case IS_FLASH_STATE_CAPTURE:
        ALOGV("(%s): [Flash] IS_FLASH_CAPTURE (%d)", __FUNCTION__, shot_ext->shot.ctl.request.frameCount);
        m_ctlInfo.flash.m_flashTimeOut = FLASH_STABLE_WAIT_TIMEOUT;
        shot_ext->shot.ctl.aa.aeflashMode = AA_FLASHMODE_CAPTURE;
        shot_ext->request_scc = 0;
        shot_ext->request_scp = 0;
        m_ctlInfo.flash.m_flashCnt = IS_FLASH_STATE_CAPTURE_WAIT; // auto transition
        break;
    case IS_FLASH_STATE_CAPTURE_WAIT:
        shot_ext->request_scc = 0;
        shot_ext->request_scp = 0;
        break;
    case IS_FLASH_STATE_CAPTURE_JPEG:
        ALOGV("(%s): [Flash] Flash Capture  (%d)!!!!!", __FUNCTION__, (FLASH_STABLE_WAIT_TIMEOUT -m_ctlInfo.flash.m_flashTimeOut));
        shot_ext->request_scc = 1;
        shot_ext->request_scp = 1;
        m_ctlInfo.flash.m_flashCnt = IS_FLASH_STATE_CAPTURE_END;  // auto transition
        break;
    case IS_FLASH_STATE_CAPTURE_END:
        ALOGV("(%s): [Flash] Flash Capture END (%d)", __FUNCTION__, shot_ext->shot.ctl.request.frameCount);
        shot_ext->shot.ctl.aa.aeflashMode = AA_FLASHMODE_OFF;
        shot_ext->request_scc = 0;
        shot_ext->request_scp = 0;
        m_ctlInfo.flash.m_flashEnableFlg = false;
        m_ctlInfo.flash.m_flashCnt = IS_FLASH_STATE_NONE;
        m_ctlInfo.flash.m_afFlashDoneFlg= false;
        break;
    case IS_FLASH_STATE_NONE:
        break;
    default:
        ALOGE("(%s): [Flash] flash state error!! (%d)", __FUNCTION__, m_ctlInfo.flash.m_flashCnt);
    }
}

void ExynosCameraHWInterface2::m_preCaptureListenerSensor(struct camera2_shot_ext * shot_ext)
{
    // Flash
    switch (m_ctlInfo.flash.m_flashCnt) {
    case IS_FLASH_STATE_AUTO_WAIT:
        if (m_ctlInfo.flash.m_flashDecisionResult) {
            if (shot_ext->shot.dm.flash.flashMode == CAM2_FLASH_MODE_OFF) {
                m_ctlInfo.flash.m_flashCnt = IS_FLASH_STATE_AUTO_DONE;
                ALOGV("(%s): [Flash] Lis :  AUTO -> OFF (%d)", __FUNCTION__, shot_ext->shot.dm.flash.flashMode);
            } else {
                ALOGV("(%s): [Flash] Waiting : AUTO -> OFF", __FUNCTION__);
            }
        } else {
            //If flash isn't activated at flash auto mode, skip flash auto control
            m_ctlInfo.flash.m_flashCnt = IS_FLASH_STATE_AUTO_DONE;
            ALOGV("(%s): [Flash] Skip :  AUTO -> OFF", __FUNCTION__);
        }
        break;
    }
}

void ExynosCameraHWInterface2::m_preCaptureListenerISP(struct camera2_shot_ext * shot_ext)
{
    // Flash
    switch (m_ctlInfo.flash.m_flashCnt) {
    case IS_FLASH_STATE_ON_WAIT:
        if (shot_ext->shot.dm.flash.decision > 0) {
            // store decision result to skip capture sequenece
            ALOGV("(%s): [Flash] IS_FLASH_ON, decision - %d", __FUNCTION__, shot_ext->shot.dm.flash.decision);
            if (shot_ext->shot.dm.flash.decision == 2)
                m_ctlInfo.flash.m_flashDecisionResult = false;
            else
                m_ctlInfo.flash.m_flashDecisionResult = true;
            m_ctlInfo.flash.m_flashCnt = IS_FLASH_STATE_ON_DONE;
        } else {
            if (m_ctlInfo.flash.m_flashTimeOut == 0) {
                ALOGV("(%s): [Flash] Timeout IS_FLASH_ON, decision is false setting", __FUNCTION__);
                m_ctlInfo.flash.m_flashCnt = IS_FLASH_STATE_ON_DONE;
                m_ctlInfo.flash.m_flashDecisionResult = false;
            } else {
                m_ctlInfo.flash.m_flashTimeOut--;
            }
        }
        break;
    case IS_FLASH_STATE_AE_AWB_LOCK_WAIT:
        if (shot_ext->shot.dm.aa.awbMode == AA_AWBMODE_LOCKED) {
            ALOGV("(%s): [Flash] FLASH_AUTO_AE_AWB_LOCK_WAIT - %d", __FUNCTION__, shot_ext->shot.dm.aa.awbMode);
            m_ctlInfo.flash.m_flashCnt = IS_FLASH_STATE_AUTO_WAIT;
        } else {
            ALOGV("(%s):  [Flash] Waiting : AA_AWBMODE_LOCKED", __FUNCTION__);
        }
        break;
    case IS_FLASH_STATE_CAPTURE_WAIT:
        if (m_ctlInfo.flash.m_flashDecisionResult) {
            if (shot_ext->shot.dm.flash.firingStable) {
                m_ctlInfo.flash.m_flashCnt = IS_FLASH_STATE_CAPTURE_JPEG;
            } else {
                if (m_ctlInfo.flash.m_flashTimeOut == 0) {
                    ALOGE("(%s): [Flash] Wait firingStable time-out!!", __FUNCTION__);
                    m_ctlInfo.flash.m_flashCnt = IS_FLASH_STATE_CAPTURE_JPEG;
                } else {
                    ALOGV("(%s): [Flash] Wait firingStable - %d", __FUNCTION__, m_ctlInfo.flash.m_flashTimeOut);
                    m_ctlInfo.flash.m_flashTimeOut--;
                }
            }
        } else {
            m_ctlInfo.flash.m_flashCnt = IS_FLASH_STATE_CAPTURE_JPEG;
        }
        break;
    }
}

void ExynosCameraHWInterface2::m_preCaptureAeState(struct camera2_shot_ext * shot_ext)
{
    switch (m_ctlInfo.flash.i_flashMode) {
    case AA_AEMODE_ON:
        // At flash off mode, capture can be done as zsl capture
        shot_ext->shot.dm.aa.aeState = AE_STATE_CONVERGED;
        break;
    case AA_AEMODE_ON_AUTO_FLASH:
        // At flash auto mode, main flash have to be done if pre-flash was done.
        if (m_ctlInfo.flash.m_flashDecisionResult && m_ctlInfo.flash.m_afFlashDoneFlg)
            shot_ext->shot.dm.aa.aeState = AE_STATE_FLASH_REQUIRED;
        // FALLTHRU
    default:
        break;
    }
}

void ExynosCameraHWInterface2::m_updateAfRegion(struct camera2_shot_ext * shot_ext)
{
    shot_ext->shot.ctl.aa.afRegions[0] = currentAfRegion[0];
    shot_ext->shot.ctl.aa.afRegions[1] = currentAfRegion[1];
    shot_ext->shot.ctl.aa.afRegions[2] = currentAfRegion[2];
    shot_ext->shot.ctl.aa.afRegions[3] = currentAfRegion[3];
}

void ExynosCameraHWInterface2::SetAfRegion(uint32_t * afRegion)
{
    currentAfRegion[0] = afRegion[0];
    currentAfRegion[1] = afRegion[1];
    currentAfRegion[2] = afRegion[2];
    currentAfRegion[3] = afRegion[3];
}

void ExynosCameraHWInterface2::m_afTrigger(struct camera2_shot_ext * shot_ext, int mode)
{
    if (m_afState == HAL_AFSTATE_SCANNING) {
        ALOGD("(%s): restarting trigger ", __FUNCTION__);
    } else if (!mode) {
        if (m_afState != HAL_AFSTATE_NEEDS_COMMAND)
            ALOGD("(%s): wrong trigger state %d", __FUNCTION__, m_afState);
        else
            m_afState = HAL_AFSTATE_STARTED;
    }
    ALOGD("### AF Triggering with mode (%d) (%d)", m_afMode, m_afState);
    shot_ext->shot.ctl.aa.afTrigger = 1;
    shot_ext->shot.ctl.aa.afMode = m_afMode;
    m_IsAfTriggerRequired = false;
}

void ExynosCameraHWInterface2::m_sensorThreadFunc(SignalDrivenThread * self)
{
    uint32_t        currentSignal = self->GetProcessingSignal();
    SensorThread *  selfThread      = ((SensorThread*)self);
    int index;
    int index_isp;
    status_t res;
    nsecs_t frameTime;
    int bayersOnSensor = 0, bayersOnIsp = 0;
    int j = 0;
    bool isCapture = false;
    ALOGV("DEBUG(%s): m_sensorThreadFunc (%x)", __FUNCTION__, currentSignal);

    if (currentSignal & SIGNAL_THREAD_RELEASE) {
        CAM_LOGD("(%s): ENTER processing SIGNAL_THREAD_RELEASE", __FUNCTION__);

        ALOGV("(%s): calling sensor streamoff", __FUNCTION__);
        cam_int_streamoff(&(m_camera_info.sensor));
        ALOGV("(%s): calling sensor streamoff done", __FUNCTION__);

        m_camera_info.sensor.buffers = 0;
        ALOGV("DEBUG(%s): sensor calling reqbuf 0 ", __FUNCTION__);
        cam_int_reqbufs(&(m_camera_info.sensor));
        ALOGV("DEBUG(%s): sensor calling reqbuf 0 done", __FUNCTION__);
        m_camera_info.sensor.status = false;

        ALOGV("(%s): calling ISP streamoff", __FUNCTION__);
        isp_int_streamoff(&(m_camera_info.isp));
        ALOGV("(%s): calling ISP streamoff done", __FUNCTION__);

        m_camera_info.isp.buffers = 0;
        ALOGV("DEBUG(%s): isp calling reqbuf 0 ", __FUNCTION__);
        cam_int_reqbufs(&(m_camera_info.isp));
        ALOGV("DEBUG(%s): isp calling reqbuf 0 done", __FUNCTION__);

        exynos_v4l2_s_ctrl(m_camera_info.sensor.fd, V4L2_CID_IS_S_STREAM, IS_DISABLE_STREAM);

        m_requestManager->releaseSensorQ();
        m_requestManager->ResetEntry();
        ALOGV("(%s): EXIT processing SIGNAL_THREAD_RELEASE", __FUNCTION__);
        selfThread->SetSignal(SIGNAL_THREAD_TERMINATE);
        return;
    }

    if (currentSignal & SIGNAL_SENSOR_START_REQ_PROCESSING)
    {
        ALOGV("DEBUG(%s): SensorThread processing SIGNAL_SENSOR_START_REQ_PROCESSING", __FUNCTION__);
        int targetStreamIndex = 0, i=0;
        int matchedFrameCnt = -1, processingReqIndex;
        struct camera2_shot_ext *shot_ext;
        struct camera2_shot_ext *shot_ext_capture;
        bool triggered = false;

        /* dqbuf from sensor */
        ALOGV("Sensor DQbuf start");
        index = cam_int_dqbuf(&(m_camera_info.sensor));
        m_requestManager->pushSensorQ(index);
        ALOGV("Sensor DQbuf done(%d)", index);
        shot_ext = (struct camera2_shot_ext *)(m_camera_info.sensor.buffer[index].virt.extP[1]);

        if (m_nightCaptureCnt != 0) {
            matchedFrameCnt = m_nightCaptureFrameCnt;
        } else if (m_ctlInfo.flash.m_flashCnt >= IS_FLASH_STATE_CAPTURE) {
            matchedFrameCnt = m_ctlInfo.flash.m_flashFrameCount;
            ALOGV("Skip frame, request is fixed at %d", matchedFrameCnt);
        } else {
            matchedFrameCnt = m_requestManager->FindFrameCnt(shot_ext, m_isRequestQueueNull);
        }

        if (matchedFrameCnt == -1 && m_vdisBubbleCnt > 0) {
            matchedFrameCnt = m_vdisDupFrame;
        }

        if (matchedFrameCnt != -1) {
            if (m_vdisBubbleCnt == 0 || m_vdisDupFrame != matchedFrameCnt) {
                frameTime = systemTime();
                m_requestManager->RegisterTimestamp(matchedFrameCnt, &frameTime);
                m_requestManager->UpdateIspParameters(shot_ext, matchedFrameCnt, &m_ctlInfo);
            } else {
                ALOGV("bubble for vids: m_vdisBubbleCnt %d, matchedFrameCnt %d", m_vdisDupFrame, matchedFrameCnt);
            }

            // face af mode setting in case of face priority scene mode
            if (m_ctlInfo.scene.prevSceneMode != shot_ext->shot.ctl.aa.sceneMode) {
                ALOGV("(%s): Scene mode changed (%d)", __FUNCTION__, shot_ext->shot.ctl.aa.sceneMode);
                m_ctlInfo.scene.prevSceneMode = shot_ext->shot.ctl.aa.sceneMode;
            }

            m_zoomRatio = (float)m_camera2->getSensorW() / (float)shot_ext->shot.ctl.scaler.cropRegion[2];
            float zoomLeft, zoomTop, zoomWidth, zoomHeight;
            int crop_x = 0, crop_y = 0, crop_w = 0, crop_h = 0;

            m_getRatioSize(m_camera2->getSensorW(), m_camera2->getSensorH(),
                           m_streamThreads[0]->m_parameters.width, m_streamThreads[0]->m_parameters.height,
                           &crop_x, &crop_y,
                           &crop_w, &crop_h,
                           0);

            if (m_streamThreads[0]->m_parameters.width >= m_streamThreads[0]->m_parameters.height) {
                zoomWidth =  m_camera2->getSensorW() / m_zoomRatio;
                zoomHeight = zoomWidth *
                        m_streamThreads[0]->m_parameters.height / m_streamThreads[0]->m_parameters.width;
            } else {
                zoomHeight = m_camera2->getSensorH() / m_zoomRatio;
                zoomWidth = zoomHeight *
                        m_streamThreads[0]->m_parameters.width / m_streamThreads[0]->m_parameters.height;
            }
            zoomLeft = (crop_w - zoomWidth) / 2;
            zoomTop = (crop_h - zoomHeight) / 2;

            int32_t new_cropRegion[3] = { (int32_t)zoomLeft, (int32_t)zoomTop, (int32_t)zoomWidth };

            int cropCompensation = (new_cropRegion[0] * 2 + new_cropRegion[2]) - ALIGN(crop_w, 4);
            if (cropCompensation)
                new_cropRegion[2] -= cropCompensation;

            shot_ext->shot.ctl.scaler.cropRegion[0] = new_cropRegion[0];
            shot_ext->shot.ctl.scaler.cropRegion[1] = new_cropRegion[1];
            shot_ext->shot.ctl.scaler.cropRegion[2] = new_cropRegion[2];
            if (m_IsAfModeUpdateRequired && (m_ctlInfo.flash.m_precaptureTriggerId == 0)) {
                ALOGD("### Applying AF Mode change(Mode %d) ", m_afMode);
                shot_ext->shot.ctl.aa.afMode = m_afMode;
                if (m_afMode == AA_AFMODE_CONTINUOUS_VIDEO || m_afMode == AA_AFMODE_CONTINUOUS_PICTURE) {
                    ALOGD("### With Automatic triger for continuous modes");
                    m_afState = HAL_AFSTATE_STARTED;
                    shot_ext->shot.ctl.aa.afTrigger = 1;
                    triggered = true;
                    if ((m_ctlInfo.scene.prevSceneMode == AA_SCENE_MODE_UNSUPPORTED) ||
                            (m_ctlInfo.scene.prevSceneMode == AA_SCENE_MODE_FACE_PRIORITY)) {
                        switch (m_afMode) {
                        case AA_AFMODE_CONTINUOUS_PICTURE:
                            shot_ext->shot.ctl.aa.afMode = AA_AFMODE_CONTINUOUS_PICTURE;
                            ALOGD("### Face AF Mode change (Mode %d) ", shot_ext->shot.ctl.aa.afMode);
                            // FALLTHRU
                        default:
                            break;
                        }
                    }
                    // reset flash result
                    if (m_ctlInfo.flash.m_afFlashDoneFlg) {
                        m_ctlInfo.flash.m_flashEnableFlg = false;
                        m_ctlInfo.flash.m_afFlashDoneFlg = false;
                        m_ctlInfo.flash.m_flashDecisionResult = false;
                        m_ctlInfo.flash.m_flashCnt = IS_FLASH_STATE_NONE;
                    }
                    m_ctlInfo.af.m_afTriggerTimeOut = 1;
                }

                m_IsAfModeUpdateRequired = false;
                // support inifinity focus mode
                if ((m_afMode == AA_AFMODE_MANUAL) && ( shot_ext->shot.ctl.lens.focusDistance == 0)) {
                    shot_ext->shot.ctl.aa.afMode = AA_AFMODE_INFINITY;
                    shot_ext->shot.ctl.aa.afTrigger = 1;
                    triggered = true;
                }
                if (m_afMode2 != NO_CHANGE) {
                    enum aa_afmode tempAfMode = m_afMode2;
                    m_afMode2 = NO_CHANGE;
                    SetAfMode(tempAfMode);
                }
            }
            else {
                shot_ext->shot.ctl.aa.afMode = NO_CHANGE;
            }
            if (m_IsAfTriggerRequired) {
                if (m_ctlInfo.flash.m_flashEnableFlg && m_ctlInfo.flash.m_afFlashDoneFlg) {
                    // flash case
                    if (m_ctlInfo.flash.m_flashCnt == IS_FLASH_STATE_ON_DONE) {
                        if ((m_afMode != AA_AFMODE_AUTO) && (m_afMode != AA_AFMODE_MACRO)) {
                            // Flash is enabled and start AF
                            m_afTrigger(shot_ext, 1);
                        } else {
                            m_afTrigger(shot_ext, 0);
                        }
                    }
                } else {
                    // non-flash case
                    m_afTrigger(shot_ext, 0);
                }
            } else {
                shot_ext->shot.ctl.aa.afTrigger = 0;
            }

            if (m_wideAspect) {
                shot_ext->setfile = ISS_SUB_SCENARIO_VIDEO;
            } else {
                shot_ext->setfile = ISS_SUB_SCENARIO_STILL;
            }
            if (triggered)
                shot_ext->shot.ctl.aa.afTrigger = 1;

            // TODO : check collision with AFMode Update
            if (m_IsAfLockRequired) {
                shot_ext->shot.ctl.aa.afMode = AA_AFMODE_OFF;
                m_IsAfLockRequired = false;
            }
            ALOGV("### Isp Qbuf start(%d) count (%d), SCP(%d) SCC(%d) DIS(%d) shot_size(%d)",
                index,
                shot_ext->shot.ctl.request.frameCount,
                shot_ext->request_scp,
                shot_ext->request_scc,
                shot_ext->dis_bypass, sizeof(camera2_shot));

            // update AF region
            m_updateAfRegion(shot_ext);

            m_lastSceneMode = shot_ext->shot.ctl.aa.sceneMode;
            if (shot_ext->shot.ctl.aa.sceneMode == AA_SCENE_MODE_NIGHT
                    && shot_ext->shot.ctl.aa.aeMode == AA_AEMODE_LOCKED)
                shot_ext->shot.ctl.aa.aeMode = AA_AEMODE_ON;
            if (m_nightCaptureCnt == 0) {
                if (shot_ext->shot.ctl.aa.captureIntent == AA_CAPTURE_INTENT_STILL_CAPTURE
                        && shot_ext->shot.ctl.aa.sceneMode == AA_SCENE_MODE_NIGHT) {
                    shot_ext->shot.ctl.aa.sceneMode = AA_SCENE_MODE_NIGHT_CAPTURE;
                    shot_ext->shot.ctl.aa.aeTargetFpsRange[0] = 2;
                    shot_ext->shot.ctl.aa.aeTargetFpsRange[1] = 30;
                    m_nightCaptureCnt = 4;
                    m_nightCaptureFrameCnt = matchedFrameCnt;
                    shot_ext->request_scc = 0;
                }
            }
            else if (m_nightCaptureCnt == 1) {
                shot_ext->shot.ctl.aa.sceneMode = AA_SCENE_MODE_NIGHT_CAPTURE;
                shot_ext->shot.ctl.aa.aeTargetFpsRange[0] = 30;
                shot_ext->shot.ctl.aa.aeTargetFpsRange[1] = 30;
                m_nightCaptureCnt--;
                m_nightCaptureFrameCnt = 0;
                shot_ext->request_scc = 1;
            }
            else if (m_nightCaptureCnt == 2) {
                shot_ext->shot.ctl.aa.sceneMode = AA_SCENE_MODE_NIGHT_CAPTURE;
                shot_ext->shot.ctl.aa.aeTargetFpsRange[0] = 2;
                shot_ext->shot.ctl.aa.aeTargetFpsRange[1] = 30;
                m_nightCaptureCnt--;
                shot_ext->request_scc = 0;
            }
            else if (m_nightCaptureCnt == 3) {
                shot_ext->shot.ctl.aa.sceneMode = AA_SCENE_MODE_NIGHT_CAPTURE;
                shot_ext->shot.ctl.aa.aeTargetFpsRange[0] = 2;
                shot_ext->shot.ctl.aa.aeTargetFpsRange[1] = 30;
                m_nightCaptureCnt--;
                shot_ext->request_scc = 0;
            }
            else if (m_nightCaptureCnt == 4) {
                shot_ext->shot.ctl.aa.sceneMode = AA_SCENE_MODE_NIGHT_CAPTURE;
                shot_ext->shot.ctl.aa.aeTargetFpsRange[0] = 2;
                shot_ext->shot.ctl.aa.aeTargetFpsRange[1] = 30;
                m_nightCaptureCnt--;
                shot_ext->request_scc = 0;
            }

            switch (shot_ext->shot.ctl.aa.aeTargetFpsRange[1]) {
            case 15:
                shot_ext->shot.ctl.sensor.frameDuration = (66666 * 1000);
                break;

            case 24:
                shot_ext->shot.ctl.sensor.frameDuration = (41666 * 1000);
                break;

            case 25:
                shot_ext->shot.ctl.sensor.frameDuration = (40000 * 1000);
                break;

            case 30:
            default:
                shot_ext->shot.ctl.sensor.frameDuration = (33333 * 1000);
                break;
            }
            shot_ext->shot.ctl.aa.aeTargetFpsRange[1] = 30;

            // Flash mode
            // Keep and Skip request_scc = 1 at flash enable mode to operate flash sequence
            if ((m_ctlInfo.flash.i_flashMode >= AA_AEMODE_ON_AUTO_FLASH)
                    && (shot_ext->shot.ctl.aa.captureIntent == AA_CAPTURE_INTENT_STILL_CAPTURE)
                    && (m_cameraId == 0)) {
                if (!m_ctlInfo.flash.m_flashDecisionResult) {
                    m_ctlInfo.flash.m_flashEnableFlg = false;
                    m_ctlInfo.flash.m_afFlashDoneFlg = false;
                    m_ctlInfo.flash.m_flashCnt = IS_FLASH_STATE_NONE;
                } else if ((m_ctlInfo.flash.m_flashCnt == IS_FLASH_STATE_AUTO_DONE) ||
                                          (m_ctlInfo.flash.m_flashCnt == IS_FLASH_STATE_AUTO_OFF)) {
                    ALOGD("(%s): [Flash] Flash capture start : skip request scc 1#####", __FUNCTION__);
                    shot_ext->request_scc = 0;
                    m_ctlInfo.flash.m_flashFrameCount = matchedFrameCnt;
                    m_ctlInfo.flash.m_flashEnableFlg = true;
                    m_ctlInfo.flash.m_afFlashDoneFlg = false;
                    m_ctlInfo.flash.m_flashCnt = IS_FLASH_STATE_CAPTURE;
                } else if (m_ctlInfo.flash.m_flashCnt < IS_FLASH_STATE_AUTO_DONE) {
                    ALOGE("(%s): [Flash] Flash capture Error- wrong state !!!!!! (%d)", __FUNCTION__, m_ctlInfo.flash.m_flashCnt);
                    shot_ext->shot.ctl.aa.aeflashMode = AA_FLASHMODE_OFF;
                    m_ctlInfo.flash.m_flashEnableFlg = false;
                    m_ctlInfo.flash.m_afFlashDoneFlg= false;
                    m_ctlInfo.flash.m_flashCnt = IS_FLASH_STATE_NONE;
                }
            } else if (shot_ext->shot.ctl.aa.captureIntent == AA_CAPTURE_INTENT_STILL_CAPTURE) {
                m_ctlInfo.flash.m_flashDecisionResult = false;
            }

            if (shot_ext->shot.ctl.flash.flashMode == CAM2_FLASH_MODE_TORCH) {
                if (m_ctlInfo.flash.m_flashTorchMode == false) {
                    m_ctlInfo.flash.m_flashTorchMode = true;
                }
            } else {
                if (m_ctlInfo.flash.m_flashTorchMode == true) {
                    shot_ext->shot.ctl.flash.flashMode = CAM2_FLASH_MODE_OFF;
                    shot_ext->shot.ctl.flash.firingPower = 0;
                    m_ctlInfo.flash.m_flashTorchMode = false;
                } else {
                    shot_ext->shot.ctl.flash.flashMode = CAM2_FLASH_MODE_NOP;
                }
            }

            if (shot_ext->isReprocessing) {
                ALOGV("(%s): Sending signal for Reprocess request", __FUNCTION__);
                m_currentReprocessOutStreams = shot_ext->shot.ctl.request.outputStreams[0];
                shot_ext->request_scp = 0;
                shot_ext->request_scc = 0;
                m_reprocessingFrameCnt = shot_ext->shot.ctl.request.frameCount;
                m_ctlInfo.flash.m_flashDecisionResult = false;
                void *shot = m_requestManager->GetInternalShotExtByFrameCnt(m_reprocessingFrameCnt);
                if (!shot) { // m_isRequestQueueNull reuse current
                    ALOGD("(%s): isReprocessing: "
                        "m_reprocessingFrameCnt missing, using shot_ext",
                        __FUNCTION__);
                    shot = shot_ext;
                }
                memcpy(&m_jpegMetadata, shot, sizeof(struct camera2_shot_ext));
                m_streamThreads[1]->SetSignal(SIGNAL_STREAM_REPROCESSING_START);
                m_ctlInfo.flash.m_flashEnableFlg = false;
            }

            if (m_ctlInfo.flash.m_flashEnableFlg) {
                m_preCaptureListenerSensor(shot_ext);
                m_preCaptureSetter(shot_ext);
            }

            ALOGV("(%s): queued  aa(%d) aemode(%d) awb(%d) afmode(%d) trigger(%d)", __FUNCTION__,
            (int)(shot_ext->shot.ctl.aa.mode), (int)(shot_ext->shot.ctl.aa.aeMode),
            (int)(shot_ext->shot.ctl.aa.awbMode), (int)(shot_ext->shot.ctl.aa.afMode),
            (int)(shot_ext->shot.ctl.aa.afTrigger));

            if (m_vdisBubbleCnt > 0 && m_vdisDupFrame == matchedFrameCnt) {
                shot_ext->dis_bypass = 1;
                shot_ext->dnr_bypass = 1;
                shot_ext->request_scp = 0;
                shot_ext->request_scc = 0;
                m_vdisBubbleCnt--;
                matchedFrameCnt = -1;
            } else {
                m_vdisDupFrame = matchedFrameCnt;
            }
            if (m_scpForceSuspended)
                shot_ext->request_scc = 0;

            uint32_t current_scp = shot_ext->request_scp;
            uint32_t current_scc = shot_ext->request_scc;

            if (shot_ext->shot.dm.request.frameCount == 0) {
                CAM_LOGE("ERR(%s): dm.request.frameCount = %d", __FUNCTION__, shot_ext->shot.dm.request.frameCount);
            }

            cam_int_qbuf(&(m_camera_info.isp), index);

            ALOGV("### isp DQBUF start");
            index_isp = cam_int_dqbuf(&(m_camera_info.isp));

            shot_ext = (struct camera2_shot_ext *)(m_camera_info.isp.buffer[index_isp].virt.extP[1]);

            if (m_ctlInfo.flash.m_flashEnableFlg)
                m_preCaptureListenerISP(shot_ext);

            ALOGV("### Isp DQbuf done(%d) count (%d), SCP(%d) SCC(%d) dis_bypass(%d) dnr_bypass(%d) shot_size(%d)",
                index,
                shot_ext->shot.ctl.request.frameCount,
                shot_ext->request_scp,
                shot_ext->request_scc,
                shot_ext->dis_bypass,
                shot_ext->dnr_bypass, sizeof(camera2_shot));

            ALOGV("(%s): DM aa(%d) aemode(%d) awb(%d) afmode(%d)", __FUNCTION__,
                (int)(shot_ext->shot.dm.aa.mode), (int)(shot_ext->shot.dm.aa.aeMode),
                (int)(shot_ext->shot.dm.aa.awbMode),
                (int)(shot_ext->shot.dm.aa.afMode));

#ifndef ENABLE_FRAME_SYNC
            m_currentOutputStreams = shot_ext->shot.ctl.request.outputStreams[0];
#endif

            if (!shot_ext->fd_bypass) {
                /* FD orientation axis transformation */
                for (int i=0; i < CAMERA2_MAX_FACES; i++) {
                    if (shot_ext->shot.dm.stats.faceRectangles[i][0] > 0)
                        shot_ext->shot.dm.stats.faceRectangles[i][0] = (m_camera2->m_curCameraInfo->sensorW
                                                                                                * shot_ext->shot.dm.stats.faceRectangles[i][0])
                                                                                                / m_streamThreads[0].get()->m_parameters.width;
                    if (shot_ext->shot.dm.stats.faceRectangles[i][1] > 0)
                        shot_ext->shot.dm.stats.faceRectangles[i][1] = (m_camera2->m_curCameraInfo->sensorH
                                                                                                * shot_ext->shot.dm.stats.faceRectangles[i][1])
                                                                                                / m_streamThreads[0].get()->m_parameters.height;
                    if (shot_ext->shot.dm.stats.faceRectangles[i][2] > 0)
                        shot_ext->shot.dm.stats.faceRectangles[i][2] = (m_camera2->m_curCameraInfo->sensorW
                                                                                                * shot_ext->shot.dm.stats.faceRectangles[i][2])
                                                                                                / m_streamThreads[0].get()->m_parameters.width;
                    if (shot_ext->shot.dm.stats.faceRectangles[i][3] > 0)
                        shot_ext->shot.dm.stats.faceRectangles[i][3] = (m_camera2->m_curCameraInfo->sensorH
                                                                                                * shot_ext->shot.dm.stats.faceRectangles[i][3])
                                                                                                / m_streamThreads[0].get()->m_parameters.height;
                }
            }
            // aeState control
            if (shot_ext->shot.ctl.aa.sceneMode != AA_SCENE_MODE_NIGHT)
                m_preCaptureAeState(shot_ext);

            // At scene mode face priority
            if (shot_ext->shot.dm.aa.afMode == AA_AFMODE_CONTINUOUS_PICTURE_FACE)
                shot_ext->shot.dm.aa.afMode = AA_AFMODE_CONTINUOUS_PICTURE;

            if (matchedFrameCnt != -1 && m_nightCaptureCnt == 0 && (m_ctlInfo.flash.m_flashCnt < IS_FLASH_STATE_CAPTURE)) {
                m_requestManager->ApplyDynamicMetadata(shot_ext);
            }

            if (current_scc != shot_ext->request_scc) {
                ALOGD("(%s): scc frame drop1 request_scc(%d to %d)",
                                __FUNCTION__, current_scc, shot_ext->request_scc);
                m_requestManager->NotifyStreamOutput(shot_ext->shot.ctl.request.frameCount);
            }
            if (shot_ext->request_scc) {
                ALOGV("send SIGNAL_STREAM_DATA_COMING (SCC)");
                if (shot_ext->shot.ctl.request.outputStreams[0] & STREAM_MASK_JPEG) {
                    void *shot = shot_ext;
                    if (m_ctlInfo.flash.m_flashCnt < IS_FLASH_STATE_CAPTURE) {
                        shot = m_requestManager->GetInternalShotExtByFrameCnt(
                            shot_ext->shot.ctl.request.frameCount);
                        if (!shot) { // m_isRequestQueueNull reuse current
                            ALOGD("(%s): request_scc: "
                                "m_reprocessingFrameCnt missing, using shot_ext",
                                __FUNCTION__);
                            shot = shot_ext;
                        }
                    }
                    memcpy(&m_jpegMetadata, shot, sizeof(struct camera2_shot_ext));
                }
                m_streamThreads[1]->SetSignal(SIGNAL_STREAM_DATA_COMING);
            }
            if (current_scp != shot_ext->request_scp) {
                ALOGD("(%s): scp frame drop1 request_scp(%d to %d)",
                                __FUNCTION__, current_scp, shot_ext->request_scp);
                m_requestManager->NotifyStreamOutput(shot_ext->shot.ctl.request.frameCount);
            }
            if (shot_ext->request_scp) {
                ALOGV("send SIGNAL_STREAM_DATA_COMING (SCP)");
                m_streamThreads[0]->SetSignal(SIGNAL_STREAM_DATA_COMING);
            }

            ALOGV("(%s): SCP_CLOSING check sensor(%d) scc(%d) scp(%d) ", __FUNCTION__,
               shot_ext->request_sensor, shot_ext->request_scc, shot_ext->request_scp);
            if (shot_ext->request_scc + shot_ext->request_scp + shot_ext->request_sensor == 0) {
                ALOGV("(%s): SCP_CLOSING check OK ", __FUNCTION__);
                m_scp_closed = true;
            }
            else
                m_scp_closed = false;

            OnAfNotification(shot_ext->shot.dm.aa.afState);
            OnPrecaptureMeteringNotificationISP();
        }   else {
            memcpy(&shot_ext->shot.ctl, &m_camera_info.dummy_shot.shot.ctl, sizeof(struct camera2_ctl));
            shot_ext->shot.ctl.request.frameCount = 0xfffffffe;
            shot_ext->request_sensor = 1;
            shot_ext->dis_bypass = 1;
            shot_ext->dnr_bypass = 1;
            shot_ext->fd_bypass = 1;
            shot_ext->drc_bypass = 1;
            shot_ext->request_scc = 0;
            shot_ext->request_scp = 0;
            if (m_wideAspect) {
                shot_ext->setfile = ISS_SUB_SCENARIO_VIDEO;
            } else {
                shot_ext->setfile = ISS_SUB_SCENARIO_STILL;
            }
            shot_ext->shot.ctl.aa.sceneMode = (enum aa_scene_mode)m_lastSceneMode;
            if (shot_ext->shot.ctl.aa.sceneMode == AA_SCENE_MODE_NIGHT_CAPTURE || shot_ext->shot.ctl.aa.sceneMode == AA_SCENE_MODE_NIGHT) {
                shot_ext->shot.ctl.aa.aeTargetFpsRange[0] = 8;
                shot_ext->shot.ctl.aa.aeTargetFpsRange[1] = 30;
            }
            shot_ext->shot.ctl.aa.aeflashMode = AA_FLASHMODE_OFF;
            shot_ext->shot.ctl.flash.flashMode = CAM2_FLASH_MODE_OFF;
            ALOGV("### isp QBUF start (bubble)");
            ALOGV("bubble: queued  aa(%d) aemode(%d) awb(%d) afmode(%d) trigger(%d)",
                (int)(shot_ext->shot.ctl.aa.mode), (int)(shot_ext->shot.ctl.aa.aeMode),
                (int)(shot_ext->shot.ctl.aa.awbMode), (int)(shot_ext->shot.ctl.aa.afMode),
                (int)(shot_ext->shot.ctl.aa.afTrigger));

            cam_int_qbuf(&(m_camera_info.isp), index);
            ALOGV("### isp DQBUF start (bubble)");
            index_isp = cam_int_dqbuf(&(m_camera_info.isp));
            shot_ext = (struct camera2_shot_ext *)(m_camera_info.isp.buffer[index_isp].virt.extP[1]);
            ALOGV("bubble: DM aa(%d) aemode(%d) awb(%d) afmode(%d)",
                (int)(shot_ext->shot.dm.aa.mode), (int)(shot_ext->shot.dm.aa.aeMode),
                (int)(shot_ext->shot.dm.aa.awbMode),
                (int)(shot_ext->shot.dm.aa.afMode));

            OnAfNotification(shot_ext->shot.dm.aa.afState);
        }

        index = m_requestManager->popSensorQ();
        if(index < 0){
            ALOGE("sensorQ is empty");
            return;
        }

        processingReqIndex = m_requestManager->MarkProcessingRequest(&(m_camera_info.sensor.buffer[index]));
        shot_ext = (struct camera2_shot_ext *)(m_camera_info.sensor.buffer[index].virt.extP[1]);
        if (m_scp_closing || m_scp_closed) {
            ALOGD("(%s): SCP_CLOSING(%d) SCP_CLOSED(%d)", __FUNCTION__, m_scp_closing, m_scp_closed);
            shot_ext->request_scc = 0;
            shot_ext->request_scp = 0;
            shot_ext->request_sensor = 0;
        }
        cam_int_qbuf(&(m_camera_info.sensor), index);
        ALOGV("Sensor Qbuf done(%d)", index);

        if (!m_scp_closing
            && ((matchedFrameCnt == -1) || (processingReqIndex == -1))){
            ALOGV("make bubble shot: matchedFramcnt(%d) processingReqIndex(%d)",
                                    matchedFrameCnt, processingReqIndex);
            selfThread->SetSignal(SIGNAL_SENSOR_START_REQ_PROCESSING);
        }
    }
    return;
}

void ExynosCameraHWInterface2::m_streamBufferInit(SignalDrivenThread *self)
{
    uint32_t                currentSignal   = self->GetProcessingSignal();
    StreamThread *          selfThread      = ((StreamThread*)self);
    stream_parameters_t     *selfStreamParms =  &(selfThread->m_parameters);
    node_info_t             *currentNode    = selfStreamParms->node;
    substream_parameters_t  *subParms;
    buffer_handle_t * buf = NULL;
    status_t res;
    void *virtAddr[3];
    int i, j;
    int index;
    nsecs_t timestamp;

    if (!(selfThread->m_isBufferInit))
    {
        for ( i=0 ; i < selfStreamParms->numSvcBuffers; i++) {
            res = selfStreamParms->streamOps->dequeue_buffer(selfStreamParms->streamOps, &buf);
            if (res != NO_ERROR || buf == NULL) {
                ALOGE("ERR(%s): Init: unable to dequeue buffer : %d",__FUNCTION__ , res);
                return;
            }
            ALOGV("DEBUG(%s): got buf(%x) version(%d), numFds(%d), numInts(%d)", __FUNCTION__, (uint32_t)(*buf),
               ((native_handle_t*)(*buf))->version, ((native_handle_t*)(*buf))->numFds, ((native_handle_t*)(*buf))->numInts);

            index = selfThread->findBufferIndex(buf);
            if (index == -1) {
                ALOGE("ERR(%s): could not find buffer index", __FUNCTION__);
            }
            else {
                ALOGV("DEBUG(%s): found buffer index[%d] - status(%d)",
                    __FUNCTION__, index, selfStreamParms->svcBufStatus[index]);
                if (selfStreamParms->svcBufStatus[index]== REQUIRES_DQ_FROM_SVC)
                    selfStreamParms->svcBufStatus[index] = ON_DRIVER;
                else if (selfStreamParms->svcBufStatus[index]== ON_SERVICE)
                    selfStreamParms->svcBufStatus[index] = ON_HAL;
                else {
                    ALOGV("DBG(%s): buffer status abnormal (%d) "
                        , __FUNCTION__, selfStreamParms->svcBufStatus[index]);
                }
                selfStreamParms->numSvcBufsInHal++;
            }
            selfStreamParms->bufIndex = 0;
        }
        selfThread->m_isBufferInit = true;
    }
    for (int i = 0 ; i < NUM_MAX_SUBSTREAM ; i++) {
        if (selfThread->m_attachedSubStreams[i].streamId == -1)
            continue;

        subParms = &m_subStreams[selfThread->m_attachedSubStreams[i].streamId];
        if (subParms->type && subParms->needBufferInit) {
            ALOGV("(%s): [subStream] (id:%d) Buffer Initialization numsvcbuf(%d)",
                __FUNCTION__, selfThread->m_attachedSubStreams[i].streamId, subParms->numSvcBuffers);
            int checkingIndex = 0;
            bool found = false;
            for ( i = 0 ; i < subParms->numSvcBuffers; i++) {
                res = subParms->streamOps->dequeue_buffer(subParms->streamOps, &buf);
                if (res != NO_ERROR || buf == NULL) {
                    ALOGE("ERR(%s): Init: unable to dequeue buffer : %d",__FUNCTION__ , res);
                    return;
                }
                subParms->numSvcBufsInHal++;
                ALOGV("DEBUG(%s): [subStream] got buf(%x) bufInHal(%d) version(%d), numFds(%d), numInts(%d)", __FUNCTION__, (uint32_t)(*buf),
                   subParms->numSvcBufsInHal, ((native_handle_t*)(*buf))->version, ((native_handle_t*)(*buf))->numFds, ((native_handle_t*)(*buf))->numInts);

                if (m_grallocHal->lock(m_grallocHal, *buf,
                       subParms->usage, 0, 0,
                       subParms->width, subParms->height, virtAddr) != 0) {
                    ALOGE("ERR(%s): could not obtain gralloc buffer", __FUNCTION__);
                }
                else {
                      ALOGV("DEBUG(%s): [subStream] locked img buf plane0(%x) plane1(%x) plane2(%x)",
                        __FUNCTION__, (unsigned int)virtAddr[0], (unsigned int)virtAddr[1], (unsigned int)virtAddr[2]);
                }
                found = false;
                for (checkingIndex = 0; checkingIndex < subParms->numSvcBuffers ; checkingIndex++) {
                    if (subParms->svcBufHandle[checkingIndex] == *buf ) {
                        found = true;
                        break;
                    }
                }
                ALOGV("DEBUG(%s): [subStream] found(%d) - index[%d]", __FUNCTION__, found, checkingIndex);
                if (!found) break;

                index = checkingIndex;

                if (index == -1) {
                    ALOGV("ERR(%s): could not find buffer index", __FUNCTION__);
                }
                else {
                    ALOGV("DEBUG(%s): found buffer index[%d] - status(%d)",
                        __FUNCTION__, index, subParms->svcBufStatus[index]);
                    if (subParms->svcBufStatus[index]== ON_SERVICE)
                        subParms->svcBufStatus[index] = ON_HAL;
                    else {
                        ALOGV("DBG(%s): buffer status abnormal (%d) "
                            , __FUNCTION__, subParms->svcBufStatus[index]);
                    }
                    if (*buf != subParms->svcBufHandle[index])
                        ALOGV("DBG(%s): different buf_handle index ", __FUNCTION__);
                    else
                        ALOGV("DEBUG(%s): same buf_handle index", __FUNCTION__);
                }
                subParms->svcBufIndex = 0;
            }
            if (subParms->type == SUBSTREAM_TYPE_JPEG) {
                m_resizeBuf.size.extS[0] = ALIGN(subParms->width, 16) * ALIGN(subParms->height, 16) * 2;
                m_resizeBuf.size.extS[1] = 0;
                m_resizeBuf.size.extS[2] = 0;

                if (allocCameraMemory(m_ionCameraClient, &m_resizeBuf, 1) == -1) {
                    ALOGE("ERR(%s): Failed to allocate resize buf", __FUNCTION__);
                }
            }
            if (subParms->type == SUBSTREAM_TYPE_PRVCB) {
                m_getAlignedYUVSize(HAL_PIXEL_FORMAT_2_V4L2_PIX(subParms->internalFormat), subParms->width,
                subParms->height, &m_previewCbBuf);

                if (allocCameraMemory(m_ionCameraClient, &m_previewCbBuf, subParms->internalPlanes) == -1) {
                    ALOGE("ERR(%s): Failed to allocate prvcb buf", __FUNCTION__);
                }
            }
            subParms->needBufferInit= false;
        }
    }
}

void ExynosCameraHWInterface2::m_streamThreadInitialize(SignalDrivenThread * self)
{
    StreamThread *          selfThread      = ((StreamThread*)self);
    ALOGV("DEBUG(%s): ", __FUNCTION__ );
    memset(&(selfThread->m_parameters), 0, sizeof(stream_parameters_t));
    selfThread->m_isBufferInit = false;
    for (int i = 0 ; i < NUM_MAX_SUBSTREAM ; i++) {
        selfThread->m_attachedSubStreams[i].streamId    = -1;
        selfThread->m_attachedSubStreams[i].priority    = 0;
    }
    return;
}

int ExynosCameraHWInterface2::m_runSubStreamFunc(StreamThread *selfThread, ExynosBuffer *srcImageBuf,
    int stream_id, nsecs_t frameTimeStamp)
{
    substream_parameters_t  *subParms = &m_subStreams[stream_id];

    switch (stream_id) {

    case STREAM_ID_JPEG:
        return m_jpegCreator(selfThread, srcImageBuf, frameTimeStamp);

    case STREAM_ID_RECORD:
        return m_recordCreator(selfThread, srcImageBuf, frameTimeStamp);

    case STREAM_ID_PRVCB:
        return m_prvcbCreator(selfThread, srcImageBuf, frameTimeStamp);

    default:
        return 0;
    }
}
void ExynosCameraHWInterface2::m_streamFunc_direct(SignalDrivenThread *self)
{
    uint32_t                currentSignal   = self->GetProcessingSignal();
    StreamThread *          selfThread      = ((StreamThread*)self);
    stream_parameters_t     *selfStreamParms =  &(selfThread->m_parameters);
    node_info_t             *currentNode    = selfStreamParms->node;
    int i = 0;
    nsecs_t frameTimeStamp;

    if (currentSignal & SIGNAL_THREAD_RELEASE) {
        CAM_LOGD("(%s): [%d] START SIGNAL_THREAD_RELEASE", __FUNCTION__, selfThread->m_index);

        if (selfThread->m_isBufferInit) {
            if (!(currentNode->fd == m_camera_info.capture.fd && m_camera_info.capture.status == false)) {
                ALOGV("(%s): [%d] calling streamoff (fd:%d)", __FUNCTION__,
                    selfThread->m_index, currentNode->fd);
                if (cam_int_streamoff(currentNode) < 0 ) {
                    ALOGE("ERR(%s): stream off fail", __FUNCTION__);
                }
                ALOGV("(%s): [%d] streamoff done and calling reqbuf 0 (fd:%d)", __FUNCTION__,
                        selfThread->m_index, currentNode->fd);
                currentNode->buffers = 0;
                cam_int_reqbufs(currentNode);
                ALOGV("(%s): [%d] reqbuf 0 DONE (fd:%d)", __FUNCTION__,
                        selfThread->m_index, currentNode->fd);
            }
        }
#ifdef ENABLE_FRAME_SYNC
        // free metabuffers
        for (i = 0; i < NUM_MAX_CAMERA_BUFFERS; i++)
            if (selfStreamParms->metaBuffers[i].fd.extFd[0] != 0) {
                freeCameraMemory(&(selfStreamParms->metaBuffers[i]), 1);
                selfStreamParms->metaBuffers[i].fd.extFd[0] = 0;
                selfStreamParms->metaBuffers[i].size.extS[0] = 0;
            }
#endif
        selfThread->m_isBufferInit = false;
        selfThread->m_releasing = false;
        selfThread->m_activated = false;
        ALOGV("(%s): [%d] END  SIGNAL_THREAD_RELEASE", __FUNCTION__, selfThread->m_index);
        return;
    }
    if (currentSignal & SIGNAL_STREAM_REPROCESSING_START) {
        status_t    res;
        buffer_handle_t * buf = NULL;
        bool found = false;
        ALOGV("(%s): streamthread[%d] START SIGNAL_STREAM_REPROCESSING_START",
            __FUNCTION__, selfThread->m_index);
        res = m_reprocessOps->acquire_buffer(m_reprocessOps, &buf);
        if (res != NO_ERROR || buf == NULL) {
            ALOGE("ERR(%s): [reprocess] unable to acquire_buffer : %d",__FUNCTION__ , res);
            return;
        }
        const private_handle_t *priv_handle = reinterpret_cast<const private_handle_t *>(*buf);
        int checkingIndex = 0;
        for (checkingIndex = 0; checkingIndex < selfStreamParms->numSvcBuffers ; checkingIndex++) {
            if (priv_handle->fd == selfStreamParms->svcBuffers[checkingIndex].fd.extFd[0] ) {
                found = true;
                break;
            }
        }
        ALOGV("DEBUG(%s): dequeued buf %x => found(%d) index(%d) ",
            __FUNCTION__, (unsigned int)buf, found, checkingIndex);

        if (!found) return;

        for (int i = 0 ; i < NUM_MAX_SUBSTREAM ; i++) {
            if (selfThread->m_attachedSubStreams[i].streamId == -1)
                continue;

#ifdef ENABLE_FRAME_SYNC
            frameTimeStamp = m_requestManager->GetTimestampByFrameCnt(m_reprocessingFrameCnt);
            m_requestManager->NotifyStreamOutput(m_reprocessingFrameCnt);
#else
            frameTimeStamp = m_requestManager->GetTimestamp(m_requestManager->GetFrameIndex());
#endif
            if (m_currentReprocessOutStreams & (1<<selfThread->m_attachedSubStreams[i].streamId))
                m_runSubStreamFunc(selfThread, &(selfStreamParms->svcBuffers[checkingIndex]),
                    selfThread->m_attachedSubStreams[i].streamId, frameTimeStamp);
        }

        res = m_reprocessOps->release_buffer(m_reprocessOps, buf);
        if (res != NO_ERROR) {
            ALOGE("ERR(%s): [reprocess] unable to release_buffer : %d",__FUNCTION__ , res);
            return;
        }
        ALOGV("(%s): streamthread[%d] END   SIGNAL_STREAM_REPROCESSING_START",
            __FUNCTION__,selfThread->m_index);

        return;
    }
    if (currentSignal & SIGNAL_STREAM_DATA_COMING) {
        buffer_handle_t * buf = NULL;
        status_t res = 0;
        int i, j;
        int index;
        nsecs_t timestamp;
#ifdef ENABLE_FRAME_SYNC
        camera2_stream *frame;
        uint8_t currentOutputStreams;
        bool directOutputEnabled = false;
#endif
        int numOfUndqbuf = 0;

        ALOGV("(%s): streamthread[%d] START SIGNAL_STREAM_DATA_COMING", __FUNCTION__,selfThread->m_index);

        m_streamBufferInit(self);

        do {
            ALOGV("DEBUG(%s): streamthread[%d] type(%d) DQBUF START ",__FUNCTION__,
                selfThread->m_index, selfThread->streamType);

#ifdef ENABLE_FRAME_SYNC
            selfStreamParms->bufIndex = cam_int_dqbuf(currentNode, selfStreamParms->planes + selfStreamParms->metaPlanes);
            frame = (struct camera2_stream *)(selfStreamParms->metaBuffers[selfStreamParms->bufIndex].virt.extP[0]);
            frameTimeStamp = m_requestManager->GetTimestampByFrameCnt(frame->rcount);
            currentOutputStreams = m_requestManager->GetOutputStreamByFrameCnt(frame->rcount);
            ALOGV("frame count streamthread[%d] : %d, outputStream(%x)", selfThread->m_index, frame->rcount, currentOutputStreams);
            if (((currentOutputStreams & STREAM_MASK_PREVIEW) && selfThread->m_index == 0)||
                 ((currentOutputStreams & STREAM_MASK_ZSL) && selfThread->m_index == 1)) {
                directOutputEnabled = true;
            }
            if (!directOutputEnabled) {
                if (!m_nightCaptureFrameCnt)
                    m_requestManager->NotifyStreamOutput(frame->rcount);
            }
#else
            selfStreamParms->bufIndex = cam_int_dqbuf(currentNode);
            frameTimeStamp = m_requestManager->GetTimestamp(m_requestManager->GetFrameIndex())
#endif
            ALOGV("DEBUG(%s): streamthread[%d] DQBUF done index(%d)  sigcnt(%d)",__FUNCTION__,
                selfThread->m_index, selfStreamParms->bufIndex, m_scpOutputSignalCnt);

            if (selfStreamParms->svcBufStatus[selfStreamParms->bufIndex] !=  ON_DRIVER)
                ALOGV("DBG(%s): DQed buffer status abnormal (%d) ",
                       __FUNCTION__, selfStreamParms->svcBufStatus[selfStreamParms->bufIndex]);
            selfStreamParms->svcBufStatus[selfStreamParms->bufIndex] = ON_HAL;

            for (int i = 0 ; i < NUM_MAX_SUBSTREAM ; i++) {
                if (selfThread->m_attachedSubStreams[i].streamId == -1)
                    continue;
#ifdef ENABLE_FRAME_SYNC
                if (currentOutputStreams & (1<<selfThread->m_attachedSubStreams[i].streamId)) {
                    m_runSubStreamFunc(selfThread, &(selfStreamParms->svcBuffers[selfStreamParms->bufIndex]),
                        selfThread->m_attachedSubStreams[i].streamId, frameTimeStamp);
                }
#else
                if (m_currentOutputStreams & (1<<selfThread->m_attachedSubStreams[i].streamId)) {
                    m_runSubStreamFunc(selfThread, &(selfStreamParms->svcBuffers[selfStreamParms->bufIndex]),
                        selfThread->m_attachedSubStreams[i].streamId, frameTimeStamp);
                }
#endif
            }

            if (m_requestManager->GetSkipCnt() <= 0) {
#ifdef ENABLE_FRAME_SYNC
                if ((currentOutputStreams & STREAM_MASK_PREVIEW) && selfThread->m_index == 0) {
                    ALOGV("** Display Preview(frameCnt:%d)", frame->rcount);
                    res = selfStreamParms->streamOps->enqueue_buffer(selfStreamParms->streamOps,
                            frameTimeStamp,
                            &(selfStreamParms->svcBufHandle[selfStreamParms->bufIndex]));
                }
                else if ((currentOutputStreams & STREAM_MASK_ZSL) && selfThread->m_index == 1) {
                    ALOGV("** SCC output (frameCnt:%d)", frame->rcount);
                    res = selfStreamParms->streamOps->enqueue_buffer(selfStreamParms->streamOps,
                                frameTimeStamp,
                                &(selfStreamParms->svcBufHandle[selfStreamParms->bufIndex]));
                }
                else {
                    res = selfStreamParms->streamOps->cancel_buffer(selfStreamParms->streamOps,
                            &(selfStreamParms->svcBufHandle[selfStreamParms->bufIndex]));
                    ALOGV("DEBUG(%s): streamthread[%d] cancel_buffer to svc done res(%d)", __FUNCTION__, selfThread->m_index, res);
                }
#else
                if ((m_currentOutputStreams & STREAM_MASK_PREVIEW) && selfThread->m_index == 0) {
                    ALOGV("** Display Preview(frameCnt:%d)", m_requestManager->GetFrameIndex());
                    res = selfStreamParms->streamOps->enqueue_buffer(selfStreamParms->streamOps,
                            frameTimeStamp,
                            &(selfStreamParms->svcBufHandle[selfStreamParms->bufIndex]));
                }
                else if ((m_currentOutputStreams & STREAM_MASK_ZSL) && selfThread->m_index == 1) {
                    ALOGV("** SCC output (frameCnt:%d), last(%d)", m_requestManager->GetFrameIndex());
                    res = selfStreamParms->streamOps->enqueue_buffer(selfStreamParms->streamOps,
                                frameTimeStamp,
                                &(selfStreamParms->svcBufHandle[selfStreamParms->bufIndex]));
                }
#endif
                ALOGV("DEBUG(%s): streamthread[%d] enqueue_buffer to svc done res(%d)", __FUNCTION__, selfThread->m_index, res);
            }
            else {
                res = selfStreamParms->streamOps->cancel_buffer(selfStreamParms->streamOps,
                        &(selfStreamParms->svcBufHandle[selfStreamParms->bufIndex]));
                ALOGV("DEBUG(%s): streamthread[%d] cancel_buffer to svc done res(%d)", __FUNCTION__, selfThread->m_index, res);
            }
#ifdef ENABLE_FRAME_SYNC
            if (directOutputEnabled) {
                if (!m_nightCaptureFrameCnt)
                     m_requestManager->NotifyStreamOutput(frame->rcount);
            }
#endif
            if (res == 0) {
                selfStreamParms->svcBufStatus[selfStreamParms->bufIndex] = ON_SERVICE;
                selfStreamParms->numSvcBufsInHal--;
            }
            else {
                selfStreamParms->svcBufStatus[selfStreamParms->bufIndex] = ON_HAL;
            }

        }
        while(0);

        while ((selfStreamParms->numSvcBufsInHal - (selfStreamParms->numSvcBuffers - NUM_SCP_BUFFERS))
                    < selfStreamParms->minUndequedBuffer) {
            res = selfStreamParms->streamOps->dequeue_buffer(selfStreamParms->streamOps, &buf);
            if (res != NO_ERROR || buf == NULL) {
                ALOGV("DEBUG(%s): streamthread[%d] dequeue_buffer fail res(%d) numInHal(%d)",__FUNCTION__ , selfThread->m_index,  res, selfStreamParms->numSvcBufsInHal);
                break;
            }
            selfStreamParms->numSvcBufsInHal++;
            ALOGV("DEBUG(%s): streamthread[%d] got buf(%x) numInHal(%d) version(%d), numFds(%d), numInts(%d)", __FUNCTION__,
                selfThread->m_index, (uint32_t)(*buf), selfStreamParms->numSvcBufsInHal,
               ((native_handle_t*)(*buf))->version, ((native_handle_t*)(*buf))->numFds, ((native_handle_t*)(*buf))->numInts);
            const private_handle_t *priv_handle = reinterpret_cast<const private_handle_t *>(*buf);

            bool found = false;
            int checkingIndex = 0;
            for (checkingIndex = 0; checkingIndex < selfStreamParms->numSvcBuffers ; checkingIndex++) {
                if (priv_handle->fd == selfStreamParms->svcBuffers[checkingIndex].fd.extFd[0] ) {
                    found = true;
                    break;
                }
            }
            if (!found) break;
            selfStreamParms->bufIndex = checkingIndex;
            if (selfStreamParms->bufIndex < selfStreamParms->numHwBuffers) {
                uint32_t    plane_index = 0;
                ExynosBuffer*  currentBuf = &(selfStreamParms->svcBuffers[selfStreamParms->bufIndex]);
                struct v4l2_buffer v4l2_buf;
                struct v4l2_plane  planes[VIDEO_MAX_PLANES];

                v4l2_buf.m.planes   = planes;
                v4l2_buf.type       = currentNode->type;
                v4l2_buf.memory     = currentNode->memory;
                v4l2_buf.index      = selfStreamParms->bufIndex;
                v4l2_buf.length     = currentNode->planes;

                v4l2_buf.m.planes[0].m.fd = priv_handle->fd;
                v4l2_buf.m.planes[2].m.fd = priv_handle->fd1;
                v4l2_buf.m.planes[1].m.fd = priv_handle->fd2;
                for (plane_index=0 ; plane_index < v4l2_buf.length ; plane_index++) {
                    v4l2_buf.m.planes[plane_index].length  = currentBuf->size.extS[plane_index];
                }
#ifdef ENABLE_FRAME_SYNC
                /* add plane for metadata*/
                v4l2_buf.length += selfStreamParms->metaPlanes;
                v4l2_buf.m.planes[v4l2_buf.length-1].m.fd = selfStreamParms->metaBuffers[selfStreamParms->bufIndex].fd.extFd[0];
                v4l2_buf.m.planes[v4l2_buf.length-1].length = selfStreamParms->metaBuffers[selfStreamParms->bufIndex].size.extS[0];
#endif
                if (exynos_v4l2_qbuf(currentNode->fd, &v4l2_buf) < 0) {
                    ALOGE("ERR(%s): streamthread[%d] exynos_v4l2_qbuf() fail",
                        __FUNCTION__, selfThread->m_index);
                    return;
                }
                selfStreamParms->svcBufStatus[selfStreamParms->bufIndex] = ON_DRIVER;
                ALOGV("DEBUG(%s): streamthread[%d] QBUF done index(%d)",
                    __FUNCTION__, selfThread->m_index, selfStreamParms->bufIndex);
            }
        }

        ALOGV("(%s): streamthread[%d] END SIGNAL_STREAM_DATA_COMING", __FUNCTION__,selfThread->m_index);
    }
    return;
}

void ExynosCameraHWInterface2::m_streamFunc_indirect(SignalDrivenThread *self)
{
    uint32_t                currentSignal   = self->GetProcessingSignal();
    StreamThread *          selfThread      = ((StreamThread*)self);
    stream_parameters_t     *selfStreamParms =  &(selfThread->m_parameters);
    node_info_t             *currentNode    = selfStreamParms->node;


    if (currentSignal & SIGNAL_THREAD_RELEASE) {
        CAM_LOGV("(%s): [%d] START SIGNAL_THREAD_RELEASE", __FUNCTION__, selfThread->m_index);

        if (selfThread->m_isBufferInit) {
            if (currentNode->fd == m_camera_info.capture.fd) {
                if (m_camera_info.capture.status == true) {
                    ALOGV("DEBUG(%s): calling streamthread[%d] streamoff (fd:%d)", __FUNCTION__,
                    selfThread->m_index, currentNode->fd);
                    if (cam_int_streamoff(currentNode) < 0 ){
                        ALOGE("ERR(%s): stream off fail", __FUNCTION__);
                    } else {
                        m_camera_info.capture.status = false;
                    }
                }
            } else {
                ALOGV("DEBUG(%s): calling streamthread[%d] streamoff (fd:%d)", __FUNCTION__,
                selfThread->m_index, currentNode->fd);
                if (cam_int_streamoff(currentNode) < 0 ){
                    ALOGE("ERR(%s): stream off fail", __FUNCTION__);
                }
            }
            ALOGV("DEBUG(%s): calling streamthread[%d] streamoff done", __FUNCTION__, selfThread->m_index);
            ALOGV("DEBUG(%s): calling streamthread[%d] reqbuf 0 (fd:%d)", __FUNCTION__,
                    selfThread->m_index, currentNode->fd);
            currentNode->buffers = 0;
            cam_int_reqbufs(currentNode);
            ALOGV("DEBUG(%s): calling streamthread[%d] reqbuf 0 DONE(fd:%d)", __FUNCTION__,
                    selfThread->m_index, currentNode->fd);
        }

        selfThread->m_isBufferInit = false;
        selfThread->m_releasing = false;
        selfThread->m_activated = false;
        ALOGV("(%s): [%d] END SIGNAL_THREAD_RELEASE", __FUNCTION__, selfThread->m_index);
        return;
    }

    if (currentSignal & SIGNAL_STREAM_DATA_COMING) {
#ifdef ENABLE_FRAME_SYNC
        camera2_stream *frame;
        uint8_t currentOutputStreams;
#endif
        nsecs_t frameTimeStamp;

        ALOGV("DEBUG(%s): streamthread[%d] processing SIGNAL_STREAM_DATA_COMING",
            __FUNCTION__,selfThread->m_index);

        m_streamBufferInit(self);

        ALOGV("DEBUG(%s): streamthread[%d] DQBUF START", __FUNCTION__, selfThread->m_index);
        selfStreamParms->bufIndex = cam_int_dqbuf(currentNode);
        ALOGV("DEBUG(%s): streamthread[%d] DQBUF done index(%d)",__FUNCTION__,
            selfThread->m_index, selfStreamParms->bufIndex);

#ifdef ENABLE_FRAME_SYNC
        frame = (struct camera2_stream *)(currentNode->buffer[selfStreamParms->bufIndex].virt.extP[selfStreamParms->planes -1]);
        frameTimeStamp = m_requestManager->GetTimestampByFrameCnt(frame->rcount);
        currentOutputStreams = m_requestManager->GetOutputStreamByFrameCnt(frame->rcount);
        ALOGV("frame count(SCC) : %d outputStream(%x)",  frame->rcount, currentOutputStreams);
#else
        frameTimeStamp = m_requestManager->GetTimestamp(m_requestManager->GetFrameIndex());
#endif

        for (int i = 0 ; i < NUM_MAX_SUBSTREAM ; i++) {
            if (selfThread->m_attachedSubStreams[i].streamId == -1)
                continue;
#ifdef ENABLE_FRAME_SYNC
            if (currentOutputStreams & (1<<selfThread->m_attachedSubStreams[i].streamId)) {
                m_requestManager->NotifyStreamOutput(frame->rcount);
                m_runSubStreamFunc(selfThread, &(currentNode->buffer[selfStreamParms->bufIndex]),
                    selfThread->m_attachedSubStreams[i].streamId, frameTimeStamp);
            }
#else
            if (m_currentOutputStreams & (1<<selfThread->m_attachedSubStreams[i].streamId)) {
                m_runSubStreamFunc(selfThread, &(currentNode->buffer[selfStreamParms->bufIndex]),
                    selfThread->m_attachedSubStreams[i].streamId, frameTimeStamp);
            }
#endif
        }
        cam_int_qbuf(currentNode, selfStreamParms->bufIndex);
        ALOGV("DEBUG(%s): streamthread[%d] QBUF DONE", __FUNCTION__, selfThread->m_index);



        ALOGV("DEBUG(%s): streamthread[%d] processing SIGNAL_STREAM_DATA_COMING DONE",
            __FUNCTION__, selfThread->m_index);
    }


    return;
}

void ExynosCameraHWInterface2::m_streamThreadFunc(SignalDrivenThread * self)
{
    uint32_t                currentSignal   = self->GetProcessingSignal();
    StreamThread *          selfThread      = ((StreamThread*)self);
    stream_parameters_t     *selfStreamParms =  &(selfThread->m_parameters);
    node_info_t             *currentNode    = selfStreamParms->node;

    ALOGV("DEBUG(%s): m_streamThreadFunc[%d] (%x)", __FUNCTION__, selfThread->m_index, currentSignal);

    // Do something in Child thread handler
    // Should change function to class that inherited StreamThread class to support dynamic stream allocation
    if (selfThread->streamType == STREAM_TYPE_DIRECT) {
        m_streamFunc_direct(self);
    } else if (selfThread->streamType == STREAM_TYPE_INDIRECT) {
        m_streamFunc_indirect(self);
    }

    return;
}
int ExynosCameraHWInterface2::m_jpegCreator(StreamThread *selfThread, ExynosBuffer *srcImageBuf, nsecs_t frameTimeStamp)
{
    stream_parameters_t     *selfStreamParms = &(selfThread->m_parameters);
    substream_parameters_t  *subParms        = &m_subStreams[STREAM_ID_JPEG];
    status_t    res;
    ExynosRect jpegRect;
    bool found = false;
    int srcW, srcH, srcCropX, srcCropY;
    int pictureW, pictureH, pictureFramesize = 0;
    int pictureFormat;
    int cropX, cropY, cropW, cropH = 0;
    ExynosBuffer resizeBufInfo;
    ExynosRect   m_jpegPictureRect;
    buffer_handle_t * buf = NULL;
    camera2_jpeg_blob * jpegBlob = NULL;
    int jpegBufSize = 0;

    ALOGV("DEBUG(%s): index(%d)",__FUNCTION__, subParms->svcBufIndex);
    for (int i = 0 ; subParms->numSvcBuffers ; i++) {
        if (subParms->svcBufStatus[subParms->svcBufIndex] == ON_HAL) {
            found = true;
            break;
        }
        subParms->svcBufIndex++;
        if (subParms->svcBufIndex >= subParms->numSvcBuffers)
            subParms->svcBufIndex = 0;
    }
    if (!found) {
        ALOGE("(%s): cannot find free svc buffer", __FUNCTION__);
        subParms->svcBufIndex++;
        return 1;
    }

    {
        Mutex::Autolock lock(m_jpegEncoderLock);
        m_jpegEncodingCount++;
    }

    m_getRatioSize(selfStreamParms->width, selfStreamParms->height,
                    m_streamThreads[0]->m_parameters.width, m_streamThreads[0]->m_parameters.height,
                    &srcCropX, &srcCropY,
                    &srcW, &srcH,
                    0);

    m_jpegPictureRect.w = subParms->width;
    m_jpegPictureRect.h = subParms->height;

     ALOGV("DEBUG(%s):w = %d, h = %d, w = %d, h = %d",
              __FUNCTION__, selfStreamParms->width, selfStreamParms->height,
                   m_jpegPictureRect.w, m_jpegPictureRect.h);

    m_getRatioSize(srcW, srcH,
                   m_jpegPictureRect.w, m_jpegPictureRect.h,
                   &cropX, &cropY,
                   &pictureW, &pictureH,
                   0);
    pictureFormat = V4L2_PIX_FMT_YUYV;
    pictureFramesize = FRAME_SIZE(V4L2_PIX_2_HAL_PIXEL_FORMAT(pictureFormat), pictureW, pictureH);

    if (m_exynosPictureCSC) {
        float zoom_w = 0, zoom_h = 0;
        if (m_zoomRatio == 0)
            m_zoomRatio = 1;

        if (m_jpegPictureRect.w >= m_jpegPictureRect.h) {
            zoom_w =  pictureW / m_zoomRatio;
            zoom_h = zoom_w * m_jpegPictureRect.h / m_jpegPictureRect.w;
        } else {
            zoom_h = pictureH / m_zoomRatio;
            zoom_w = zoom_h * m_jpegPictureRect.w / m_jpegPictureRect.h;
        }
        cropX = (srcW - zoom_w) / 2;
        cropY = (srcH - zoom_h) / 2;
        cropW = zoom_w;
        cropH = zoom_h;

        ALOGV("DEBUG(%s):cropX = %d, cropY = %d, cropW = %d, cropH = %d",
              __FUNCTION__, cropX, cropY, cropW, cropH);

        csc_set_src_format(m_exynosPictureCSC,
                           ALIGN(srcW, 16), ALIGN(srcH, 16),
                           cropX, cropY, cropW, cropH,
                           V4L2_PIX_2_HAL_PIXEL_FORMAT(pictureFormat),
                           0);

        csc_set_dst_format(m_exynosPictureCSC,
                           m_jpegPictureRect.w, m_jpegPictureRect.h,
                           0, 0, m_jpegPictureRect.w, m_jpegPictureRect.h,
                           V4L2_PIX_2_HAL_PIXEL_FORMAT(V4L2_PIX_FMT_NV16),
                           0);
        for (int i = 0 ; i < 3 ; i++)
            ALOGV("DEBUG(%s): m_pictureBuf.fd.extFd[%d]=%d ",
                __FUNCTION__, i, srcImageBuf->fd.extFd[i]);
        csc_set_src_buffer(m_exynosPictureCSC,
                           (void **)&srcImageBuf->fd.fd);

        csc_set_dst_buffer(m_exynosPictureCSC,
                           (void **)&m_resizeBuf.fd.fd);
        for (int i = 0 ; i < 3 ; i++)
            ALOGV("DEBUG(%s): m_resizeBuf.virt.extP[%d]=%d m_resizeBuf.size.extS[%d]=%d",
                __FUNCTION__, i, m_resizeBuf.fd.extFd[i], i, m_resizeBuf.size.extS[i]);

        if (csc_convert(m_exynosPictureCSC) != 0)
            ALOGE("ERR(%s): csc_convert() fail", __FUNCTION__);

    }
    else {
        ALOGE("ERR(%s): m_exynosPictureCSC == NULL", __FUNCTION__);
    }

    resizeBufInfo = m_resizeBuf;

    m_getAlignedYUVSize(V4L2_PIX_FMT_NV16, m_jpegPictureRect.w, m_jpegPictureRect.h, &m_resizeBuf);

    for (int i = 1; i < 3; i++) {
        if (m_resizeBuf.size.extS[i] != 0)
            m_resizeBuf.fd.extFd[i] = m_resizeBuf.fd.extFd[i-1] + m_resizeBuf.size.extS[i-1];

        ALOGV("(%s): m_resizeBuf.size.extS[%d] = %d", __FUNCTION__, i, m_resizeBuf.size.extS[i]);
    }

    jpegRect.w = m_jpegPictureRect.w;
    jpegRect.h = m_jpegPictureRect.h;
    jpegRect.colorFormat = V4L2_PIX_FMT_NV16;

    for (int j = 0 ; j < 3 ; j++)
        ALOGV("DEBUG(%s): dest buf node  fd.extFd[%d]=%d size=%d virt=%x ",
            __FUNCTION__, j, subParms->svcBuffers[subParms->svcBufIndex].fd.extFd[j],
            (unsigned int)subParms->svcBuffers[subParms->svcBufIndex].size.extS[j],
            (unsigned int)subParms->svcBuffers[subParms->svcBufIndex].virt.extP[j]);

    jpegBufSize = subParms->svcBuffers[subParms->svcBufIndex].size.extS[0];
    if (yuv2Jpeg(&m_resizeBuf, &subParms->svcBuffers[subParms->svcBufIndex], &jpegRect) == false) {
        ALOGE("ERR(%s):yuv2Jpeg() fail", __FUNCTION__);
    } else {
        m_resizeBuf = resizeBufInfo;

        int jpegSize = subParms->svcBuffers[subParms->svcBufIndex].size.s;
        ALOGD("(%s): (%d x %d) jpegbuf size(%d) encoded size(%d)", __FUNCTION__,
            m_jpegPictureRect.w, m_jpegPictureRect.h, jpegBufSize, jpegSize);
        char * jpegBuffer = (char*)(subParms->svcBuffers[subParms->svcBufIndex].virt.extP[0]);
        jpegBlob = (camera2_jpeg_blob*)(&jpegBuffer[jpegBufSize - sizeof(camera2_jpeg_blob)]);

        if (jpegBuffer[jpegSize-1] == 0)
            jpegSize--;
        jpegBlob->jpeg_size = jpegSize;
        jpegBlob->jpeg_blob_id = CAMERA2_JPEG_BLOB_ID;
    }
    subParms->svcBuffers[subParms->svcBufIndex].size.extS[0] = jpegBufSize;
    res = subParms->streamOps->enqueue_buffer(subParms->streamOps, frameTimeStamp, &(subParms->svcBufHandle[subParms->svcBufIndex]));

    ALOGV("DEBUG(%s): streamthread[%d] enqueue_buffer index(%d) to svc done res(%d)",
            __FUNCTION__, selfThread->m_index, subParms->svcBufIndex, res);
    if (res == 0) {
        subParms->svcBufStatus[subParms->svcBufIndex] = ON_SERVICE;
        subParms->numSvcBufsInHal--;
    }
    else {
        subParms->svcBufStatus[subParms->svcBufIndex] = ON_HAL;
    }

    while (subParms->numSvcBufsInHal <= subParms->minUndequedBuffer)
    {
        bool found = false;
        int checkingIndex = 0;

        ALOGV("DEBUG(%s): jpeg currentBuf#(%d)", __FUNCTION__ , subParms->numSvcBufsInHal);

        res = subParms->streamOps->dequeue_buffer(subParms->streamOps, &buf);
        if (res != NO_ERROR || buf == NULL) {
            ALOGV("DEBUG(%s): jpeg stream(%d) dequeue_buffer fail res(%d)",__FUNCTION__ , selfThread->m_index,  res);
            break;
        }
        const private_handle_t *priv_handle = reinterpret_cast<const private_handle_t *>(*buf);
        subParms->numSvcBufsInHal ++;
        ALOGV("DEBUG(%s): jpeg got buf(%x) numBufInHal(%d) version(%d), numFds(%d), numInts(%d)", __FUNCTION__, (uint32_t)(*buf),
           subParms->numSvcBufsInHal, ((native_handle_t*)(*buf))->version, ((native_handle_t*)(*buf))->numFds, ((native_handle_t*)(*buf))->numInts);


        for (checkingIndex = 0; checkingIndex < subParms->numSvcBuffers ; checkingIndex++) {
            if (priv_handle->fd == subParms->svcBuffers[checkingIndex].fd.extFd[0] ) {
                found = true;
                break;
            }
        }
        ALOGV("DEBUG(%s): jpeg dequeueed_buffer found index(%d)", __FUNCTION__, found);

        if (!found) {
             break;
        }

        subParms->svcBufIndex = checkingIndex;
        if (subParms->svcBufStatus[subParms->svcBufIndex] == ON_SERVICE) {
            subParms->svcBufStatus[subParms->svcBufIndex] = ON_HAL;
        }
        else {
            ALOGV("DEBUG(%s): jpeg bufstatus abnormal [%d]  status = %d", __FUNCTION__,
                subParms->svcBufIndex,  subParms->svcBufStatus[subParms->svcBufIndex]);
        }
    }
    {
        Mutex::Autolock lock(m_jpegEncoderLock);
        m_jpegEncodingCount--;
    }
    return 0;
}

int ExynosCameraHWInterface2::m_recordCreator(StreamThread *selfThread, ExynosBuffer *srcImageBuf, nsecs_t frameTimeStamp)
{
    stream_parameters_t     *selfStreamParms = &(selfThread->m_parameters);
    substream_parameters_t  *subParms        = &m_subStreams[STREAM_ID_RECORD];
    status_t    res;
    ExynosRect jpegRect;
    bool found = false;
    int cropX, cropY, cropW, cropH = 0;
    buffer_handle_t * buf = NULL;

    ALOGV("DEBUG(%s): index(%d)",__FUNCTION__, subParms->svcBufIndex);
    for (int i = 0 ; subParms->numSvcBuffers ; i++) {
        if (subParms->svcBufStatus[subParms->svcBufIndex] == ON_HAL) {
            found = true;
            break;
        }
        subParms->svcBufIndex++;
        if (subParms->svcBufIndex >= subParms->numSvcBuffers)
            subParms->svcBufIndex = 0;
    }
    if (!found) {
        ALOGE("(%s): cannot find free svc buffer", __FUNCTION__);
        subParms->svcBufIndex++;
        return 1;
    }

    if (m_exynosVideoCSC) {
        int videoW = subParms->width, videoH = subParms->height;
        int cropX, cropY, cropW, cropH = 0;
        int previewW = selfStreamParms->width, previewH = selfStreamParms->height;
        m_getRatioSize(previewW, previewH,
                       videoW, videoH,
                       &cropX, &cropY,
                       &cropW, &cropH,
                       0);

        ALOGV("DEBUG(%s):cropX = %d, cropY = %d, cropW = %d, cropH = %d",
                 __FUNCTION__, cropX, cropY, cropW, cropH);

        csc_set_src_format(m_exynosVideoCSC,
                           ALIGN(previewW, 32), previewH,
                           cropX, cropY, cropW, cropH,
                           selfStreamParms->format,
                           0);

        csc_set_dst_format(m_exynosVideoCSC,
                           videoW, videoH,
                           0, 0, videoW, videoH,
                           subParms->format,
                           1);

        csc_set_src_buffer(m_exynosVideoCSC,
                        (void **)&srcImageBuf->fd.fd);

        csc_set_dst_buffer(m_exynosVideoCSC,
            (void **)(&(subParms->svcBuffers[subParms->svcBufIndex].fd.fd)));

        if (csc_convert(m_exynosVideoCSC) != 0) {
            ALOGE("ERR(%s):csc_convert() fail", __FUNCTION__);
        }
        else {
            ALOGV("(%s):csc_convert() SUCCESS", __FUNCTION__);
        }
    }
    else {
        ALOGE("ERR(%s):m_exynosVideoCSC == NULL", __FUNCTION__);
    }

    res = subParms->streamOps->enqueue_buffer(subParms->streamOps, frameTimeStamp, &(subParms->svcBufHandle[subParms->svcBufIndex]));

    ALOGV("DEBUG(%s): streamthread[%d] enqueue_buffer index(%d) to svc done res(%d)",
            __FUNCTION__, selfThread->m_index, subParms->svcBufIndex, res);
    if (res == 0) {
        subParms->svcBufStatus[subParms->svcBufIndex] = ON_SERVICE;
        subParms->numSvcBufsInHal--;
    }
    else {
        subParms->svcBufStatus[subParms->svcBufIndex] = ON_HAL;
    }

    while (subParms->numSvcBufsInHal <= subParms->minUndequedBuffer)
    {
        bool found = false;
        int checkingIndex = 0;

        ALOGV("DEBUG(%s): record currentBuf#(%d)", __FUNCTION__ , subParms->numSvcBufsInHal);

        res = subParms->streamOps->dequeue_buffer(subParms->streamOps, &buf);
        if (res != NO_ERROR || buf == NULL) {
            ALOGV("DEBUG(%s): record stream(%d) dequeue_buffer fail res(%d)",__FUNCTION__ , selfThread->m_index,  res);
            break;
        }
        const private_handle_t *priv_handle = reinterpret_cast<const private_handle_t *>(*buf);
        subParms->numSvcBufsInHal ++;
        ALOGV("DEBUG(%s): record got buf(%x) numBufInHal(%d) version(%d), numFds(%d), numInts(%d)", __FUNCTION__, (uint32_t)(*buf),
           subParms->numSvcBufsInHal, ((native_handle_t*)(*buf))->version, ((native_handle_t*)(*buf))->numFds, ((native_handle_t*)(*buf))->numInts);

        for (checkingIndex = 0; checkingIndex < subParms->numSvcBuffers ; checkingIndex++) {
            if (priv_handle->fd == subParms->svcBuffers[checkingIndex].fd.extFd[0] ) {
                found = true;
                break;
            }
        }
        ALOGV("DEBUG(%s): record dequeueed_buffer found(%d) index = %d", __FUNCTION__, found, checkingIndex);

        if (!found) {
             break;
        }

        subParms->svcBufIndex = checkingIndex;
        if (subParms->svcBufStatus[subParms->svcBufIndex] == ON_SERVICE) {
            subParms->svcBufStatus[subParms->svcBufIndex] = ON_HAL;
        }
        else {
            ALOGV("DEBUG(%s): record bufstatus abnormal [%d]  status = %d", __FUNCTION__,
                subParms->svcBufIndex,  subParms->svcBufStatus[subParms->svcBufIndex]);
        }
    }
    return 0;
}

int ExynosCameraHWInterface2::m_prvcbCreator(StreamThread *selfThread, ExynosBuffer *srcImageBuf, nsecs_t frameTimeStamp)
{
    stream_parameters_t     *selfStreamParms = &(selfThread->m_parameters);
    substream_parameters_t  *subParms        = &m_subStreams[STREAM_ID_PRVCB];
    status_t    res;
    bool found = false;
    int cropX, cropY, cropW, cropH = 0;
    buffer_handle_t * buf = NULL;

    ALOGV("DEBUG(%s): index(%d)",__FUNCTION__, subParms->svcBufIndex);
    for (int i = 0 ; subParms->numSvcBuffers ; i++) {
        if (subParms->svcBufStatus[subParms->svcBufIndex] == ON_HAL) {
            found = true;
            break;
        }
        subParms->svcBufIndex++;
        if (subParms->svcBufIndex >= subParms->numSvcBuffers)
            subParms->svcBufIndex = 0;
    }
    if (!found) {
        ALOGE("(%s): cannot find free svc buffer", __FUNCTION__);
        subParms->svcBufIndex++;
        return 1;
    }

    if (subParms->format == HAL_PIXEL_FORMAT_YCrCb_420_SP) {
        if (m_exynosVideoCSC) {
            int previewCbW = subParms->width, previewCbH = subParms->height;
            int cropX, cropY, cropW, cropH = 0;
            int previewW = selfStreamParms->width, previewH = selfStreamParms->height;
            m_getRatioSize(previewW, previewH,
                           previewCbW, previewCbH,
                           &cropX, &cropY,
                           &cropW, &cropH,
                           0);

            ALOGV("DEBUG(%s):cropX = %d, cropY = %d, cropW = %d, cropH = %d",
                     __FUNCTION__, cropX, cropY, cropW, cropH);
            csc_set_src_format(m_exynosVideoCSC,
                               ALIGN(previewW, 32), previewH,
                               cropX, cropY, cropW, cropH,
                               selfStreamParms->format,
                               0);

            csc_set_dst_format(m_exynosVideoCSC,
                               previewCbW, previewCbH,
                               0, 0, previewCbW, previewCbH,
                               subParms->internalFormat,
                               1);

            csc_set_src_buffer(m_exynosVideoCSC,
                        (void **)&srcImageBuf->fd.fd);

            csc_set_dst_buffer(m_exynosVideoCSC,
                (void **)(&(m_previewCbBuf.fd.fd)));

            if (csc_convert(m_exynosVideoCSC) != 0) {
                ALOGE("ERR(%s):previewcb csc_convert() fail", __FUNCTION__);
            }
            else {
                ALOGV("(%s):previewcb csc_convert() SUCCESS", __FUNCTION__);
            }
            if (previewCbW == ALIGN(previewCbW, 16)) {
                memcpy(subParms->svcBuffers[subParms->svcBufIndex].virt.extP[0],
                    m_previewCbBuf.virt.extP[0], previewCbW * previewCbH);
                memcpy(subParms->svcBuffers[subParms->svcBufIndex].virt.extP[0] + previewCbW * previewCbH,
                    m_previewCbBuf.virt.extP[1], previewCbW * previewCbH / 2 );
            }
            else {
                // TODO : copy line by line ?
            }
        }
        else {
            ALOGE("ERR(%s):m_exynosVideoCSC == NULL", __FUNCTION__);
        }
    }
    else if (subParms->format == HAL_PIXEL_FORMAT_YV12) {
        int previewCbW = subParms->width, previewCbH = subParms->height;
        int stride = ALIGN(previewCbW, 16);
        int uv_stride = ALIGN(previewCbW/2, 16);
        int c_stride = ALIGN(stride / 2, 16);

        if (previewCbW == ALIGN(previewCbW, 32)) {
            memcpy(subParms->svcBuffers[subParms->svcBufIndex].virt.extP[0],
                srcImageBuf->virt.extP[0], stride * previewCbH);
            memcpy(subParms->svcBuffers[subParms->svcBufIndex].virt.extP[0] + stride * previewCbH,
                srcImageBuf->virt.extP[1], c_stride * previewCbH / 2 );
            memcpy(subParms->svcBuffers[subParms->svcBufIndex].virt.extP[0] + (stride * previewCbH) + (c_stride * previewCbH / 2),
                srcImageBuf->virt.extP[2], c_stride * previewCbH / 2 );
        } else {
            char * dstAddr = (char *)(subParms->svcBuffers[subParms->svcBufIndex].virt.extP[0]);
            char * srcAddr = (char *)(srcImageBuf->virt.extP[0]);
            for (int i = 0 ; i < previewCbH ; i++) {
                memcpy(dstAddr, srcAddr, previewCbW);
                dstAddr += stride;
                srcAddr += ALIGN(stride, 32);
            }
            dstAddr = (char *)(subParms->svcBuffers[subParms->svcBufIndex].virt.extP[0] + stride * previewCbH);
            srcAddr = (char *)(srcImageBuf->virt.extP[1]);
            for (int i = 0 ; i < previewCbH/2 ; i++) {
                memcpy(dstAddr, srcAddr, previewCbW/2);
                dstAddr += c_stride;
                srcAddr += uv_stride;
            }
            srcAddr = (char *)(srcImageBuf->virt.extP[2]);
            for (int i = 0 ; i < previewCbH/2 ; i++) {
                memcpy(dstAddr, srcAddr, previewCbW/2);
                dstAddr += c_stride;
                srcAddr += uv_stride;
            }
        }
    }
    res = subParms->streamOps->enqueue_buffer(subParms->streamOps, frameTimeStamp, &(subParms->svcBufHandle[subParms->svcBufIndex]));

    ALOGV("DEBUG(%s): streamthread[%d] enqueue_buffer index(%d) to svc done res(%d)",
            __FUNCTION__, selfThread->m_index, subParms->svcBufIndex, res);
    if (res == 0) {
        subParms->svcBufStatus[subParms->svcBufIndex] = ON_SERVICE;
        subParms->numSvcBufsInHal--;
    }
    else {
        subParms->svcBufStatus[subParms->svcBufIndex] = ON_HAL;
    }

    while (subParms->numSvcBufsInHal <= subParms->minUndequedBuffer)
    {
        bool found = false;
        int checkingIndex = 0;

        ALOGV("DEBUG(%s): prvcb currentBuf#(%d)", __FUNCTION__ , subParms->numSvcBufsInHal);

        res = subParms->streamOps->dequeue_buffer(subParms->streamOps, &buf);
        if (res != NO_ERROR || buf == NULL) {
            ALOGV("DEBUG(%s): prvcb stream(%d) dequeue_buffer fail res(%d)",__FUNCTION__ , selfThread->m_index,  res);
            break;
        }
        const private_handle_t *priv_handle = reinterpret_cast<const private_handle_t *>(*buf);
        subParms->numSvcBufsInHal ++;
        ALOGV("DEBUG(%s): prvcb got buf(%x) numBufInHal(%d) version(%d), numFds(%d), numInts(%d)", __FUNCTION__, (uint32_t)(*buf),
           subParms->numSvcBufsInHal, ((native_handle_t*)(*buf))->version, ((native_handle_t*)(*buf))->numFds, ((native_handle_t*)(*buf))->numInts);


        for (checkingIndex = 0; checkingIndex < subParms->numSvcBuffers ; checkingIndex++) {
            if (priv_handle->fd == subParms->svcBuffers[checkingIndex].fd.extFd[0] ) {
                found = true;
                break;
            }
        }
        ALOGV("DEBUG(%s): prvcb dequeueed_buffer found(%d) index = %d", __FUNCTION__, found, checkingIndex);

        if (!found) {
             break;
        }

        subParms->svcBufIndex = checkingIndex;
        if (subParms->svcBufStatus[subParms->svcBufIndex] == ON_SERVICE) {
            subParms->svcBufStatus[subParms->svcBufIndex] = ON_HAL;
        }
        else {
            ALOGV("DEBUG(%s): prvcb bufstatus abnormal [%d]  status = %d", __FUNCTION__,
                subParms->svcBufIndex,  subParms->svcBufStatus[subParms->svcBufIndex]);
        }
    }
    return 0;
}

bool ExynosCameraHWInterface2::m_checkThumbnailSize(int w, int h)
{
    int sizeOfSupportList;

    //REAR Camera
    if(this->getCameraId() == 0) {
        sizeOfSupportList = sizeof(SUPPORT_THUMBNAIL_REAR_SIZE) / (sizeof(int32_t)*2);

        for(int i = 0; i < sizeOfSupportList; i++) {
            if((SUPPORT_THUMBNAIL_REAR_SIZE[i][0] == w) &&(SUPPORT_THUMBNAIL_REAR_SIZE[i][1] == h))
                return true;
        }

    }
    else {
        sizeOfSupportList = sizeof(SUPPORT_THUMBNAIL_FRONT_SIZE) / (sizeof(int32_t)*2);

        for(int i = 0; i < sizeOfSupportList; i++) {
            if((SUPPORT_THUMBNAIL_FRONT_SIZE[i][0] == w) &&(SUPPORT_THUMBNAIL_FRONT_SIZE[i][1] == h))
                return true;
        }
    }

    return false;
}
bool ExynosCameraHWInterface2::yuv2Jpeg(ExynosBuffer *yuvBuf,
                            ExynosBuffer *jpegBuf,
                            ExynosRect *rect)
{
    unsigned char *addr;

    ExynosJpegEncoderForCamera jpegEnc;
    bool ret = false;
    int res = 0;

    unsigned int *yuvSize = yuvBuf->size.extS;

    if (jpegEnc.create()) {
        ALOGE("ERR(%s):jpegEnc.create() fail", __FUNCTION__);
        goto jpeg_encode_done;
    }

    if (jpegEnc.setQuality(m_jpegMetadata.shot.ctl.jpeg.quality)) {
        ALOGE("ERR(%s):jpegEnc.setQuality() fail", __FUNCTION__);
        goto jpeg_encode_done;
    }

    if (jpegEnc.setSize(rect->w, rect->h)) {
        ALOGE("ERR(%s):jpegEnc.setSize() fail", __FUNCTION__);
        goto jpeg_encode_done;
    }
    ALOGV("%s : width = %d , height = %d\n", __FUNCTION__, rect->w, rect->h);

    if (jpegEnc.setColorFormat(rect->colorFormat)) {
        ALOGE("ERR(%s):jpegEnc.setColorFormat() fail", __FUNCTION__);
        goto jpeg_encode_done;
    }

    if (jpegEnc.setJpegFormat(V4L2_PIX_FMT_JPEG_422)) {
        ALOGE("ERR(%s):jpegEnc.setJpegFormat() fail", __FUNCTION__);
        goto jpeg_encode_done;
    }

    if((m_jpegMetadata.shot.ctl.jpeg.thumbnailSize[0] != 0) && (m_jpegMetadata.shot.ctl.jpeg.thumbnailSize[1] != 0)) {
        mExifInfo.enableThumb = true;
        if(!m_checkThumbnailSize(m_jpegMetadata.shot.ctl.jpeg.thumbnailSize[0], m_jpegMetadata.shot.ctl.jpeg.thumbnailSize[1])) {
            // in the case of unsupported parameter, disable thumbnail
            mExifInfo.enableThumb = false;
        } else {
            m_thumbNailW = m_jpegMetadata.shot.ctl.jpeg.thumbnailSize[0];
            m_thumbNailH = m_jpegMetadata.shot.ctl.jpeg.thumbnailSize[1];
        }

        ALOGV("(%s) m_thumbNailW = %d, m_thumbNailH = %d", __FUNCTION__, m_thumbNailW, m_thumbNailH);

    } else {
        mExifInfo.enableThumb = false;
    }

    if (jpegEnc.setThumbnailSize(m_thumbNailW, m_thumbNailH)) {
        ALOGE("ERR(%s):jpegEnc.setThumbnailSize(%d, %d) fail", __FUNCTION__, m_thumbNailH, m_thumbNailH);
        goto jpeg_encode_done;
    }

    ALOGV("(%s):jpegEnc.setThumbnailSize(%d, %d) ", __FUNCTION__, m_thumbNailW, m_thumbNailW);
    if (jpegEnc.setThumbnailQuality(m_jpegMetadata.shot.ctl.jpeg.thumbnailQuality)) {
        ALOGE("ERR(%s):jpegEnc.setThumbnailQuality fail", __FUNCTION__);
        goto jpeg_encode_done;
    }

    m_setExifChangedAttribute(&mExifInfo, rect, &m_jpegMetadata);
    ALOGV("DEBUG(%s):calling jpegEnc.setInBuf() yuvSize(%d)", __FUNCTION__, *yuvSize);
    if (jpegEnc.setInBuf((int *)&(yuvBuf->fd.fd), &(yuvBuf->virt.p), (int *)yuvSize)) {
        ALOGE("ERR(%s):jpegEnc.setInBuf() fail", __FUNCTION__);
        goto jpeg_encode_done;
    }
    if (jpegEnc.setOutBuf(jpegBuf->fd.fd, jpegBuf->virt.p, jpegBuf->size.extS[0] + jpegBuf->size.extS[1] + jpegBuf->size.extS[2])) {
        ALOGE("ERR(%s):jpegEnc.setOutBuf() fail", __FUNCTION__);
        goto jpeg_encode_done;
    }

    if (jpegEnc.updateConfig()) {
        ALOGE("ERR(%s):jpegEnc.updateConfig() fail", __FUNCTION__);
        goto jpeg_encode_done;
    }

    if ((res = jpegEnc.encode((int *)&jpegBuf->size.s, &mExifInfo))) {
        ALOGE("ERR(%s):jpegEnc.encode() fail ret(%d)", __FUNCTION__, res);
        goto jpeg_encode_done;
    }

    ret = true;

jpeg_encode_done:

    if (jpegEnc.flagCreate() == true)
        jpegEnc.destroy();

    return ret;
}

void ExynosCameraHWInterface2::OnPrecaptureMeteringTriggerStart(int id)
{
    m_ctlInfo.flash.m_precaptureTriggerId = id;
    m_ctlInfo.ae.aeStateNoti = AE_STATE_INACTIVE;
    if ((m_ctlInfo.flash.i_flashMode >= AA_AEMODE_ON_AUTO_FLASH) && (m_cameraId == 0)) {
        // flash is required
        switch (m_ctlInfo.flash.m_flashCnt) {
        case IS_FLASH_STATE_AUTO_DONE:
        case IS_FLASH_STATE_AUTO_OFF:
            // Flash capture sequence, AF flash was executed before
            break;
        default:
            // Full flash sequence
            m_ctlInfo.flash.m_flashCnt = IS_FLASH_STATE_ON;
            m_ctlInfo.flash.m_flashEnableFlg = true;
            m_ctlInfo.flash.m_flashTimeOut = 0;
        }
    } else {
        // Skip pre-capture in case of non-flash.
        ALOGV("[PreCap] Flash OFF mode ");
        m_ctlInfo.flash.m_flashEnableFlg = false;
        m_ctlInfo.flash.m_flashCnt = IS_FLASH_STATE_NONE;
    }
    ALOGV("[PreCap] OnPrecaptureMeteringTriggerStart (ID %d) (flag : %d) (cnt : %d)", id, m_ctlInfo.flash.m_flashEnableFlg, m_ctlInfo.flash.m_flashCnt);
    OnPrecaptureMeteringNotificationSensor();
}

void ExynosCameraHWInterface2::OnAfTrigger(int id)
{
    m_afTriggerId = id;

    switch (m_afMode) {
    case AA_AFMODE_AUTO:
    case AA_AFMODE_MACRO:
    case AA_AFMODE_MANUAL:
        ALOGV("[AF] OnAfTrigger - AUTO,MACRO,OFF (Mode %d) ", m_afMode);
        // If flash is enable, Flash operation is executed before triggering AF
        if ((m_ctlInfo.flash.i_flashMode >= AA_AEMODE_ON_AUTO_FLASH)
                && (m_ctlInfo.flash.m_flashEnableFlg == false)
                && (m_cameraId == 0)) {
            ALOGV("[Flash] AF Flash start with Mode (%d)", m_afMode);
            m_ctlInfo.flash.m_flashEnableFlg = true;
            m_ctlInfo.flash.m_flashCnt = IS_FLASH_STATE_ON;
            m_ctlInfo.flash.m_flashDecisionResult = false;
            m_ctlInfo.flash.m_afFlashDoneFlg = true;
        }
        OnAfTriggerAutoMacro(id);
        break;
    case AA_AFMODE_CONTINUOUS_VIDEO:
        ALOGV("[AF] OnAfTrigger - AA_AFMODE_CONTINUOUS_VIDEO (Mode %d) ", m_afMode);
        OnAfTriggerCAFVideo(id);
        break;
    case AA_AFMODE_CONTINUOUS_PICTURE:
        ALOGV("[AF] OnAfTrigger - AA_AFMODE_CONTINUOUS_PICTURE (Mode %d) ", m_afMode);
        OnAfTriggerCAFPicture(id);
        break;

    case AA_AFMODE_OFF:
    default:
        break;
    }
}

void ExynosCameraHWInterface2::OnAfTriggerAutoMacro(int /*id*/)
{
    int nextState = NO_TRANSITION;

    switch (m_afState) {
    case HAL_AFSTATE_INACTIVE:
    case HAL_AFSTATE_PASSIVE_FOCUSED:
    case HAL_AFSTATE_SCANNING:
        nextState = HAL_AFSTATE_NEEDS_COMMAND;
        m_IsAfTriggerRequired = true;
        break;
    case HAL_AFSTATE_NEEDS_COMMAND:
        nextState = NO_TRANSITION;
        break;
    case HAL_AFSTATE_STARTED:
        nextState = NO_TRANSITION;
        break;
    case HAL_AFSTATE_LOCKED:
        nextState = HAL_AFSTATE_NEEDS_COMMAND;
        m_IsAfTriggerRequired = true;
        break;
    case HAL_AFSTATE_FAILED:
        nextState = HAL_AFSTATE_NEEDS_COMMAND;
        m_IsAfTriggerRequired = true;
        break;
    default:
        break;
    }
    ALOGV("(%s): State (%d) -> (%d)", __FUNCTION__, m_afState, nextState);
    if (nextState != NO_TRANSITION)
        m_afState = nextState;
}

void ExynosCameraHWInterface2::OnAfTriggerCAFPicture(int id)
{
    int nextState = NO_TRANSITION;

    switch (m_afState) {
    case HAL_AFSTATE_INACTIVE:
        nextState = HAL_AFSTATE_FAILED;
        SetAfStateForService(ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED);
        break;
    case HAL_AFSTATE_NEEDS_COMMAND:
        // not used
        break;
    case HAL_AFSTATE_STARTED:
        nextState = HAL_AFSTATE_NEEDS_DETERMINATION;
        m_AfHwStateFailed = false;
        break;
    case HAL_AFSTATE_SCANNING:
        nextState = HAL_AFSTATE_NEEDS_DETERMINATION;
        m_AfHwStateFailed = false;
        // If flash is enable, Flash operation is executed before triggering AF
        if ((m_ctlInfo.flash.i_flashMode >= AA_AEMODE_ON_AUTO_FLASH)
                && (m_ctlInfo.flash.m_flashEnableFlg == false)
                && (m_cameraId == 0)) {
            ALOGV("[AF Flash] AF Flash start with Mode (%d) state (%d) id (%d)", m_afMode, m_afState, id);
            m_ctlInfo.flash.m_flashEnableFlg = true;
            m_ctlInfo.flash.m_flashCnt = IS_FLASH_STATE_ON;
            m_ctlInfo.flash.m_flashDecisionResult = false;
            m_ctlInfo.flash.m_afFlashDoneFlg = true;
        }
        break;
    case HAL_AFSTATE_NEEDS_DETERMINATION:
        nextState = NO_TRANSITION;
        break;
    case HAL_AFSTATE_PASSIVE_FOCUSED:
        m_IsAfLockRequired = true;
        if (m_AfHwStateFailed) {
            ALOGE("(%s): [CAF] LAST : fail", __FUNCTION__);
            SetAfStateForService(ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED);
            nextState = HAL_AFSTATE_FAILED;
        }
        else {
            ALOGV("(%s): [CAF] LAST : success", __FUNCTION__);
            SetAfStateForService(ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED);
            nextState = HAL_AFSTATE_LOCKED;
        }
        m_AfHwStateFailed = false;
        break;
    case HAL_AFSTATE_LOCKED:
        nextState = NO_TRANSITION;
        break;
    case HAL_AFSTATE_FAILED:
        nextState = NO_TRANSITION;
        break;
    default:
        break;
    }
    ALOGV("(%s): State (%d) -> (%d)", __FUNCTION__, m_afState, nextState);
    if (nextState != NO_TRANSITION)
        m_afState = nextState;
}


void ExynosCameraHWInterface2::OnAfTriggerCAFVideo(int /*id*/)
{
    int nextState = NO_TRANSITION;

    switch (m_afState) {
    case HAL_AFSTATE_INACTIVE:
        nextState = HAL_AFSTATE_FAILED;
        SetAfStateForService(ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED);
        break;
    case HAL_AFSTATE_NEEDS_COMMAND:
        // not used
        break;
    case HAL_AFSTATE_STARTED:
        m_IsAfLockRequired = true;
        nextState = HAL_AFSTATE_FAILED;
        SetAfStateForService(ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED);
        break;
    case HAL_AFSTATE_SCANNING:
        m_IsAfLockRequired = true;
        nextState = HAL_AFSTATE_FAILED;
        SetAfStateForService(ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED);
        break;
    case HAL_AFSTATE_NEEDS_DETERMINATION:
        // not used
        break;
    case HAL_AFSTATE_PASSIVE_FOCUSED:
        m_IsAfLockRequired = true;
        SetAfStateForService(ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED);
        nextState = HAL_AFSTATE_LOCKED;
        break;
    case HAL_AFSTATE_LOCKED:
        nextState = NO_TRANSITION;
        break;
    case HAL_AFSTATE_FAILED:
        nextState = NO_TRANSITION;
        break;
    default:
        break;
    }
    ALOGV("(%s): State (%d) -> (%d)", __FUNCTION__, m_afState, nextState);
    if (nextState != NO_TRANSITION)
        m_afState = nextState;
}

void ExynosCameraHWInterface2::OnPrecaptureMeteringNotificationSensor()
{
    if (m_ctlInfo.flash.m_precaptureTriggerId > 0) {
        // Just noti of pre-capture start
        if (m_ctlInfo.ae.aeStateNoti != AE_STATE_PRECAPTURE) {
            m_notifyCb(CAMERA2_MSG_AUTOEXPOSURE,
                        ANDROID_CONTROL_AE_STATE_PRECAPTURE,
                        m_ctlInfo.flash.m_precaptureTriggerId, 0, m_callbackCookie);
            ALOGV("(%s) ANDROID_CONTROL_AE_STATE_PRECAPTURE (%d)", __FUNCTION__, m_ctlInfo.flash.m_flashCnt);
            m_notifyCb(CAMERA2_MSG_AUTOWB,
                        ANDROID_CONTROL_AWB_STATE_CONVERGED,
                        m_ctlInfo.flash.m_precaptureTriggerId, 0, m_callbackCookie);
            m_ctlInfo.ae.aeStateNoti = AE_STATE_PRECAPTURE;
        }
    }
}

void ExynosCameraHWInterface2::OnPrecaptureMeteringNotificationISP()
{
    if (m_ctlInfo.flash.m_precaptureTriggerId > 0) {
        if (m_ctlInfo.flash.m_flashEnableFlg) {
            // flash case
            switch (m_ctlInfo.flash.m_flashCnt) {
            case IS_FLASH_STATE_AUTO_DONE:
            case IS_FLASH_STATE_AUTO_OFF:
                if (m_ctlInfo.ae.aeStateNoti == AE_STATE_PRECAPTURE) {
                    // End notification
                    m_notifyCb(CAMERA2_MSG_AUTOEXPOSURE,
                                    ANDROID_CONTROL_AE_STATE_CONVERGED,
                                    m_ctlInfo.flash.m_precaptureTriggerId, 0, m_callbackCookie);
                    ALOGV("(%s) ANDROID_CONTROL_AE_STATE_CONVERGED (%d)", __FUNCTION__, m_ctlInfo.flash.m_flashCnt);
                    m_notifyCb(CAMERA2_MSG_AUTOWB,
                                    ANDROID_CONTROL_AWB_STATE_CONVERGED,
                                    m_ctlInfo.flash.m_precaptureTriggerId, 0, m_callbackCookie);
                    m_ctlInfo.flash.m_precaptureTriggerId = 0;
                } else {
                    m_notifyCb(CAMERA2_MSG_AUTOEXPOSURE,
                                    ANDROID_CONTROL_AE_STATE_PRECAPTURE,
                                    m_ctlInfo.flash.m_precaptureTriggerId, 0, m_callbackCookie);
                    ALOGV("(%s) ANDROID_CONTROL_AE_STATE_PRECAPTURE (%d)", __FUNCTION__, m_ctlInfo.flash.m_flashCnt);
                    m_notifyCb(CAMERA2_MSG_AUTOWB,
                                    ANDROID_CONTROL_AWB_STATE_CONVERGED,
                                    m_ctlInfo.flash.m_precaptureTriggerId, 0, m_callbackCookie);
                    m_ctlInfo.ae.aeStateNoti = AE_STATE_PRECAPTURE;
                }
                break;
            case IS_FLASH_STATE_CAPTURE:
            case IS_FLASH_STATE_CAPTURE_WAIT:
            case IS_FLASH_STATE_CAPTURE_JPEG:
            case IS_FLASH_STATE_CAPTURE_END:
                ALOGV("(%s) INVALID flash state count. (%d)", __FUNCTION__, (int)m_ctlInfo.flash.m_flashCnt);
                m_ctlInfo.flash.m_flashCnt = IS_FLASH_STATE_AUTO_DONE;
                m_notifyCb(CAMERA2_MSG_AUTOEXPOSURE,
                        ANDROID_CONTROL_AE_STATE_CONVERGED,
                        m_ctlInfo.flash.m_precaptureTriggerId, 0, m_callbackCookie);
                m_notifyCb(CAMERA2_MSG_AUTOWB,
                        ANDROID_CONTROL_AWB_STATE_CONVERGED,
                        m_ctlInfo.flash.m_precaptureTriggerId, 0, m_callbackCookie);
                m_ctlInfo.flash.m_precaptureTriggerId = 0;
                break;
            }
        } else {
            // non-flash case
            if (m_ctlInfo.ae.aeStateNoti == AE_STATE_PRECAPTURE) {
                m_notifyCb(CAMERA2_MSG_AUTOEXPOSURE,
                                ANDROID_CONTROL_AE_STATE_CONVERGED,
                                m_ctlInfo.flash.m_precaptureTriggerId, 0, m_callbackCookie);
                ALOGV("(%s) ANDROID_CONTROL_AE_STATE_CONVERGED (%d)", __FUNCTION__, m_ctlInfo.flash.m_flashCnt);
                m_notifyCb(CAMERA2_MSG_AUTOWB,
                                ANDROID_CONTROL_AWB_STATE_CONVERGED,
                                m_ctlInfo.flash.m_precaptureTriggerId, 0, m_callbackCookie);
                m_ctlInfo.flash.m_precaptureTriggerId = 0;
            }
        }
    }
}

void ExynosCameraHWInterface2::OnAfNotification(enum aa_afstate noti)
{
    switch (m_afMode) {
    case AA_AFMODE_AUTO:
    case AA_AFMODE_MACRO:
        OnAfNotificationAutoMacro(noti);
        break;
    case AA_AFMODE_CONTINUOUS_VIDEO:
        OnAfNotificationCAFVideo(noti);
        break;
    case AA_AFMODE_CONTINUOUS_PICTURE:
        OnAfNotificationCAFPicture(noti);
        break;
    case AA_AFMODE_OFF:
    default:
        break;
    }
}

void ExynosCameraHWInterface2::OnAfNotificationAutoMacro(enum aa_afstate noti)
{
    int nextState = NO_TRANSITION;
    bool bWrongTransition = false;

    if (m_afState == HAL_AFSTATE_INACTIVE || m_afState == HAL_AFSTATE_NEEDS_COMMAND) {
        switch (noti) {
        case AA_AFSTATE_INACTIVE:
        case AA_AFSTATE_ACTIVE_SCAN:
        case AA_AFSTATE_AF_ACQUIRED_FOCUS:
        case AA_AFSTATE_AF_FAILED_FOCUS:
        default:
            nextState = NO_TRANSITION;
            break;
        }
    }
    else if (m_afState == HAL_AFSTATE_STARTED) {
        switch (noti) {
        case AA_AFSTATE_INACTIVE:
            nextState = NO_TRANSITION;
            break;
        case AA_AFSTATE_ACTIVE_SCAN:
            nextState = HAL_AFSTATE_SCANNING;
            SetAfStateForService(ANDROID_CONTROL_AF_STATE_ACTIVE_SCAN);
            break;
        case AA_AFSTATE_AF_ACQUIRED_FOCUS:
            nextState = NO_TRANSITION;
            break;
        case AA_AFSTATE_AF_FAILED_FOCUS:
            nextState = NO_TRANSITION;
            break;
        default:
            bWrongTransition = true;
            break;
        }
    }
    else if (m_afState == HAL_AFSTATE_SCANNING) {
        switch (noti) {
        case AA_AFSTATE_INACTIVE:
            bWrongTransition = true;
            break;
        case AA_AFSTATE_ACTIVE_SCAN:
            nextState = NO_TRANSITION;
            break;
        case AA_AFSTATE_AF_ACQUIRED_FOCUS:
            // If Flash mode is enable, after AF execute pre-capture metering
            if (m_ctlInfo.flash.m_flashEnableFlg && m_ctlInfo.flash.m_afFlashDoneFlg) {
                switch (m_ctlInfo.flash.m_flashCnt) {
                case IS_FLASH_STATE_ON_DONE:
                    m_ctlInfo.flash.m_flashCnt = IS_FLASH_STATE_AUTO_AE_AWB_LOCK;
                    nextState = NO_TRANSITION;
                    break;
                case IS_FLASH_STATE_AUTO_DONE:
                    m_ctlInfo.flash.m_flashCnt = IS_FLASH_STATE_AUTO_OFF;
                    nextState = HAL_AFSTATE_LOCKED;
                    SetAfStateForService(ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED);
                    break;
                default:
                    nextState = NO_TRANSITION;
                }
            } else {
                nextState = HAL_AFSTATE_LOCKED;
                SetAfStateForService(ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED);
            }
            break;
        case AA_AFSTATE_AF_FAILED_FOCUS:
            // If Flash mode is enable, after AF execute pre-capture metering
            if (m_ctlInfo.flash.m_flashEnableFlg && m_ctlInfo.flash.m_afFlashDoneFlg) {
                switch (m_ctlInfo.flash.m_flashCnt) {
                case IS_FLASH_STATE_ON_DONE:
                    m_ctlInfo.flash.m_flashCnt = IS_FLASH_STATE_AUTO_AE_AWB_LOCK;
                    nextState = NO_TRANSITION;
                    break;
                case IS_FLASH_STATE_AUTO_DONE:
                    m_ctlInfo.flash.m_flashCnt = IS_FLASH_STATE_AUTO_OFF;
                    nextState = HAL_AFSTATE_FAILED;
                    SetAfStateForService(ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED);
                    break;
                default:
                    nextState = NO_TRANSITION;
                }
            } else {
                nextState = HAL_AFSTATE_FAILED;
                SetAfStateForService(ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED);
            }
            break;
        default:
            bWrongTransition = true;
            break;
        }
    }
    else if (m_afState == HAL_AFSTATE_LOCKED) {
        switch (noti) {
            case AA_AFSTATE_INACTIVE:
            case AA_AFSTATE_ACTIVE_SCAN:
                bWrongTransition = true;
                break;
            case AA_AFSTATE_AF_ACQUIRED_FOCUS:
                nextState = NO_TRANSITION;
                break;
            case AA_AFSTATE_AF_FAILED_FOCUS:
            default:
                bWrongTransition = true;
                break;
        }
    }
    else if (m_afState == HAL_AFSTATE_FAILED) {
        switch (noti) {
            case AA_AFSTATE_INACTIVE:
            case AA_AFSTATE_ACTIVE_SCAN:
            case AA_AFSTATE_AF_ACQUIRED_FOCUS:
                bWrongTransition = true;
                break;
            case AA_AFSTATE_AF_FAILED_FOCUS:
                nextState = NO_TRANSITION;
                break;
            default:
                bWrongTransition = true;
                break;
        }
    }
    if (bWrongTransition) {
        ALOGV("(%s): Wrong Transition state(%d) noti(%d)", __FUNCTION__, m_afState, noti);
        return;
    }
    ALOGV("(%s): State (%d) -> (%d) by (%d)", __FUNCTION__, m_afState, nextState, noti);
    if (nextState != NO_TRANSITION)
        m_afState = nextState;
}

void ExynosCameraHWInterface2::OnAfNotificationCAFPicture(enum aa_afstate noti)
{
    int nextState = NO_TRANSITION;
    bool bWrongTransition = false;

    if (m_afState == HAL_AFSTATE_INACTIVE) {
        switch (noti) {
        case AA_AFSTATE_INACTIVE:
        case AA_AFSTATE_ACTIVE_SCAN:
        case AA_AFSTATE_AF_ACQUIRED_FOCUS:
        case AA_AFSTATE_AF_FAILED_FOCUS:
        default:
            nextState = NO_TRANSITION;
            break;
        }
        // Check AF notification after triggering
        if (m_ctlInfo.af.m_afTriggerTimeOut > 0) {
            if (m_ctlInfo.af.m_afTriggerTimeOut > 5) {
                ALOGE("(%s) AF notification error - try to re-trigger mode (%)", __FUNCTION__, m_afMode);
                SetAfMode(AA_AFMODE_OFF);
                SetAfMode(m_afMode);
                m_ctlInfo.af.m_afTriggerTimeOut = 0;
            } else {
                m_ctlInfo.af.m_afTriggerTimeOut++;
            }
        }
    }
    else if (m_afState == HAL_AFSTATE_STARTED) {
        switch (noti) {
        case AA_AFSTATE_INACTIVE:
            nextState = NO_TRANSITION;
            break;
        case AA_AFSTATE_ACTIVE_SCAN:
            nextState = HAL_AFSTATE_SCANNING;
            SetAfStateForService(ANDROID_CONTROL_AF_STATE_PASSIVE_SCAN);
            m_ctlInfo.af.m_afTriggerTimeOut = 0;
            break;
        case AA_AFSTATE_AF_ACQUIRED_FOCUS:
            nextState = HAL_AFSTATE_PASSIVE_FOCUSED;
            SetAfStateForService(ANDROID_CONTROL_AF_STATE_PASSIVE_FOCUSED);
            m_ctlInfo.af.m_afTriggerTimeOut = 0;
            break;
        case AA_AFSTATE_AF_FAILED_FOCUS:
            //nextState = HAL_AFSTATE_FAILED;
            //SetAfStateForService(ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED);
            nextState = NO_TRANSITION;
            break;
        default:
            bWrongTransition = true;
            break;
        }
    }
    else if (m_afState == HAL_AFSTATE_SCANNING) {
        switch (noti) {
        case AA_AFSTATE_INACTIVE:
            nextState = NO_TRANSITION;
            break;
        case AA_AFSTATE_ACTIVE_SCAN:
            nextState = NO_TRANSITION;
            m_AfHwStateFailed = false;
            break;
        case AA_AFSTATE_AF_ACQUIRED_FOCUS:
            nextState = HAL_AFSTATE_PASSIVE_FOCUSED;
            m_AfHwStateFailed = false;
            SetAfStateForService(ANDROID_CONTROL_AF_STATE_PASSIVE_FOCUSED);
            break;
        case AA_AFSTATE_AF_FAILED_FOCUS:
            nextState = HAL_AFSTATE_PASSIVE_FOCUSED;
            m_AfHwStateFailed = true;
            SetAfStateForService(ANDROID_CONTROL_AF_STATE_PASSIVE_FOCUSED);
            break;
        default:
            bWrongTransition = true;
            break;
        }
    }
    else if (m_afState == HAL_AFSTATE_PASSIVE_FOCUSED) {
        switch (noti) {
        case AA_AFSTATE_INACTIVE:
            nextState = NO_TRANSITION;
            break;
        case AA_AFSTATE_ACTIVE_SCAN:
            nextState = HAL_AFSTATE_SCANNING;
            m_AfHwStateFailed = false;
            SetAfStateForService(ANDROID_CONTROL_AF_STATE_PASSIVE_SCAN);
            break;
        case AA_AFSTATE_AF_ACQUIRED_FOCUS:
            nextState = NO_TRANSITION;
            m_AfHwStateFailed = false;
            break;
        case AA_AFSTATE_AF_FAILED_FOCUS:
            nextState = NO_TRANSITION;
            m_AfHwStateFailed = true;
            break;
        default:
            bWrongTransition = true;
            break;
        }
    }
    else if (m_afState == HAL_AFSTATE_NEEDS_DETERMINATION) {
        //Skip notification in case of flash, wait the end of flash on
        if (m_ctlInfo.flash.m_flashEnableFlg && m_ctlInfo.flash.m_afFlashDoneFlg) {
            if (m_ctlInfo.flash.m_flashCnt < IS_FLASH_STATE_ON_DONE)
                return;
        }
        switch (noti) {
        case AA_AFSTATE_INACTIVE:
            nextState = NO_TRANSITION;
            break;
        case AA_AFSTATE_ACTIVE_SCAN:
            nextState = NO_TRANSITION;
            break;
        case AA_AFSTATE_AF_ACQUIRED_FOCUS:
            // If Flash mode is enable, after AF execute pre-capture metering
            if (m_ctlInfo.flash.m_flashEnableFlg && m_ctlInfo.flash.m_afFlashDoneFlg) {
                switch (m_ctlInfo.flash.m_flashCnt) {
                case IS_FLASH_STATE_ON_DONE:
                    ALOGV("[AF Flash] AUTO start with Mode (%d) state (%d) noti (%d)", m_afMode, m_afState, (int)noti);
                    m_ctlInfo.flash.m_flashCnt = IS_FLASH_STATE_AUTO_AE_AWB_LOCK;
                    nextState = NO_TRANSITION;
                    break;
                case IS_FLASH_STATE_AUTO_DONE:
                    ALOGV("[AF Flash] AUTO end with Mode (%d) state (%d) noti (%d)", m_afMode, m_afState, (int)noti);
                    m_ctlInfo.flash.m_flashCnt = IS_FLASH_STATE_AUTO_OFF;
                    m_IsAfLockRequired = true;
                    nextState = HAL_AFSTATE_LOCKED;
                    SetAfStateForService(ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED);
                    break;
                default:
                    nextState = NO_TRANSITION;
                }
            } else {
                m_IsAfLockRequired = true;
                nextState = HAL_AFSTATE_LOCKED;
                SetAfStateForService(ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED);
            }
            break;
        case AA_AFSTATE_AF_FAILED_FOCUS:
            // If Flash mode is enable, after AF execute pre-capture metering
            if (m_ctlInfo.flash.m_flashEnableFlg && m_ctlInfo.flash.m_afFlashDoneFlg) {
                switch (m_ctlInfo.flash.m_flashCnt) {
                case IS_FLASH_STATE_ON_DONE:
                    ALOGV("[AF Flash] AUTO start with Mode (%d) state (%d) noti (%d)", m_afMode, m_afState, (int)noti);
                    m_ctlInfo.flash.m_flashCnt = IS_FLASH_STATE_AUTO_AE_AWB_LOCK;
                    nextState = NO_TRANSITION;
                    break;
                case IS_FLASH_STATE_AUTO_DONE:
                    ALOGV("[AF Flash] AUTO end with Mode (%d) state (%d) noti (%d)", m_afMode, m_afState, (int)noti);
                    m_ctlInfo.flash.m_flashCnt = IS_FLASH_STATE_AUTO_OFF;
                    m_IsAfLockRequired = true;
                    nextState = HAL_AFSTATE_FAILED;
                    SetAfStateForService(ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED);
                    break;
                default:
                    nextState = NO_TRANSITION;
                }
            } else {
                m_IsAfLockRequired = true;
                nextState = HAL_AFSTATE_FAILED;
                SetAfStateForService(ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED);
            }
            break;
        default:
            bWrongTransition = true;
            break;
        }
    }
    else if (m_afState == HAL_AFSTATE_LOCKED) {
        switch (noti) {
            case AA_AFSTATE_INACTIVE:
                nextState = NO_TRANSITION;
                break;
            case AA_AFSTATE_ACTIVE_SCAN:
                bWrongTransition = true;
                break;
            case AA_AFSTATE_AF_ACQUIRED_FOCUS:
                nextState = NO_TRANSITION;
                break;
            case AA_AFSTATE_AF_FAILED_FOCUS:
            default:
                bWrongTransition = true;
                break;
        }
    }
    else if (m_afState == HAL_AFSTATE_FAILED) {
        switch (noti) {
            case AA_AFSTATE_INACTIVE:
                bWrongTransition = true;
                break;
            case AA_AFSTATE_ACTIVE_SCAN:
                nextState = HAL_AFSTATE_SCANNING;
                break;
            case AA_AFSTATE_AF_ACQUIRED_FOCUS:
                bWrongTransition = true;
                break;
            case AA_AFSTATE_AF_FAILED_FOCUS:
                nextState = NO_TRANSITION;
                break;
            default:
                bWrongTransition = true;
                break;
        }
    }
    if (bWrongTransition) {
        ALOGV("(%s): Wrong Transition state(%d) noti(%d)", __FUNCTION__, m_afState, noti);
        return;
    }
    ALOGV("(%s): State (%d) -> (%d) by (%d)", __FUNCTION__, m_afState, nextState, noti);
    if (nextState != NO_TRANSITION)
        m_afState = nextState;
}

void ExynosCameraHWInterface2::OnAfNotificationCAFVideo(enum aa_afstate noti)
{
    int nextState = NO_TRANSITION;
    bool bWrongTransition = false;

    if (m_afState == HAL_AFSTATE_INACTIVE) {
        switch (noti) {
        case AA_AFSTATE_INACTIVE:
        case AA_AFSTATE_ACTIVE_SCAN:
        case AA_AFSTATE_AF_ACQUIRED_FOCUS:
        case AA_AFSTATE_AF_FAILED_FOCUS:
        default:
            nextState = NO_TRANSITION;
            break;
        }
    }
    else if (m_afState == HAL_AFSTATE_STARTED) {
        switch (noti) {
        case AA_AFSTATE_INACTIVE:
            nextState = NO_TRANSITION;
            break;
        case AA_AFSTATE_ACTIVE_SCAN:
            nextState = HAL_AFSTATE_SCANNING;
            SetAfStateForService(ANDROID_CONTROL_AF_STATE_PASSIVE_SCAN);
            break;
        case AA_AFSTATE_AF_ACQUIRED_FOCUS:
            nextState = HAL_AFSTATE_PASSIVE_FOCUSED;
            SetAfStateForService(ANDROID_CONTROL_AF_STATE_PASSIVE_FOCUSED);
            break;
        case AA_AFSTATE_AF_FAILED_FOCUS:
            nextState = HAL_AFSTATE_FAILED;
            SetAfStateForService(ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED);
            break;
        default:
            bWrongTransition = true;
            break;
        }
    }
    else if (m_afState == HAL_AFSTATE_SCANNING) {
        switch (noti) {
        case AA_AFSTATE_INACTIVE:
            bWrongTransition = true;
            break;
        case AA_AFSTATE_ACTIVE_SCAN:
            nextState = NO_TRANSITION;
            break;
        case AA_AFSTATE_AF_ACQUIRED_FOCUS:
            nextState = HAL_AFSTATE_PASSIVE_FOCUSED;
            SetAfStateForService(ANDROID_CONTROL_AF_STATE_PASSIVE_FOCUSED);
            break;
        case AA_AFSTATE_AF_FAILED_FOCUS:
            nextState = NO_TRANSITION;
            break;
        default:
            bWrongTransition = true;
            break;
        }
    }
    else if (m_afState == HAL_AFSTATE_PASSIVE_FOCUSED) {
        switch (noti) {
        case AA_AFSTATE_INACTIVE:
            bWrongTransition = true;
            break;
        case AA_AFSTATE_ACTIVE_SCAN:
            nextState = HAL_AFSTATE_SCANNING;
            SetAfStateForService(ANDROID_CONTROL_AF_STATE_PASSIVE_SCAN);
            break;
        case AA_AFSTATE_AF_ACQUIRED_FOCUS:
            nextState = NO_TRANSITION;
            break;
        case AA_AFSTATE_AF_FAILED_FOCUS:
            nextState = HAL_AFSTATE_FAILED;
            SetAfStateForService(ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED);
            // TODO : needs NO_TRANSITION ?
            break;
        default:
            bWrongTransition = true;
            break;
        }
    }
    else if (m_afState == HAL_AFSTATE_NEEDS_DETERMINATION) {
        switch (noti) {
        case AA_AFSTATE_INACTIVE:
            bWrongTransition = true;
            break;
        case AA_AFSTATE_ACTIVE_SCAN:
            nextState = NO_TRANSITION;
            break;
        case AA_AFSTATE_AF_ACQUIRED_FOCUS:
            m_IsAfLockRequired = true;
            nextState = HAL_AFSTATE_LOCKED;
            SetAfStateForService(ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED);
            break;
        case AA_AFSTATE_AF_FAILED_FOCUS:
            nextState = HAL_AFSTATE_FAILED;
            SetAfStateForService(ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED);
            break;
        default:
            bWrongTransition = true;
            break;
        }
    }
    else if (m_afState == HAL_AFSTATE_LOCKED) {
        switch (noti) {
            case AA_AFSTATE_INACTIVE:
                nextState = NO_TRANSITION;
                break;
            case AA_AFSTATE_ACTIVE_SCAN:
                bWrongTransition = true;
                break;
            case AA_AFSTATE_AF_ACQUIRED_FOCUS:
                nextState = NO_TRANSITION;
                break;
            case AA_AFSTATE_AF_FAILED_FOCUS:
            default:
                bWrongTransition = true;
                break;
        }
    }
    else if (m_afState == HAL_AFSTATE_FAILED) {
        switch (noti) {
            case AA_AFSTATE_INACTIVE:
            case AA_AFSTATE_ACTIVE_SCAN:
            case AA_AFSTATE_AF_ACQUIRED_FOCUS:
                bWrongTransition = true;
                break;
            case AA_AFSTATE_AF_FAILED_FOCUS:
                nextState = NO_TRANSITION;
                break;
            default:
                bWrongTransition = true;
                break;
        }
    }
    if (bWrongTransition) {
        ALOGV("(%s): Wrong Transition state(%d) noti(%d)", __FUNCTION__, m_afState, noti);
        return;
    }
    ALOGV("(%s): State (%d) -> (%d) by (%d)", __FUNCTION__, m_afState, nextState, noti);
    if (nextState != NO_TRANSITION)
        m_afState = nextState;
}

void ExynosCameraHWInterface2::OnAfCancel(int id)
{
    m_afTriggerId = id;

    switch (m_afMode) {
    case AA_AFMODE_AUTO:
    case AA_AFMODE_MACRO:
    case AA_AFMODE_OFF:
    case AA_AFMODE_MANUAL:
        OnAfCancelAutoMacro(id);
        break;
    case AA_AFMODE_CONTINUOUS_VIDEO:
        OnAfCancelCAFVideo(id);
        break;
    case AA_AFMODE_CONTINUOUS_PICTURE:
        OnAfCancelCAFPicture(id);
        break;
    default:
        break;
    }
}

void ExynosCameraHWInterface2::OnAfCancelAutoMacro(int /*id*/)
{
    int nextState = NO_TRANSITION;

    if (m_ctlInfo.flash.m_flashEnableFlg  && m_ctlInfo.flash.m_afFlashDoneFlg) {
        m_ctlInfo.flash.m_flashCnt = IS_FLASH_STATE_AUTO_OFF;
    }
    switch (m_afState) {
    case HAL_AFSTATE_INACTIVE:
        nextState = NO_TRANSITION;
        SetAfStateForService(ANDROID_CONTROL_AF_STATE_INACTIVE);
        break;
    case HAL_AFSTATE_NEEDS_COMMAND:
    case HAL_AFSTATE_STARTED:
    case HAL_AFSTATE_SCANNING:
    case HAL_AFSTATE_LOCKED:
    case HAL_AFSTATE_FAILED:
        SetAfMode(AA_AFMODE_OFF);
        SetAfStateForService(ANDROID_CONTROL_AF_STATE_INACTIVE);
        nextState = HAL_AFSTATE_INACTIVE;
        break;
    default:
        break;
    }
    ALOGV("(%s): State (%d) -> (%d)", __FUNCTION__, m_afState, nextState);
    if (nextState != NO_TRANSITION)
        m_afState = nextState;
}

void ExynosCameraHWInterface2::OnAfCancelCAFPicture(int /*id*/)
{
    int nextState = NO_TRANSITION;

    switch (m_afState) {
    case HAL_AFSTATE_INACTIVE:
        nextState = NO_TRANSITION;
        break;
    case HAL_AFSTATE_NEEDS_COMMAND:
    case HAL_AFSTATE_STARTED:
    case HAL_AFSTATE_SCANNING:
    case HAL_AFSTATE_LOCKED:
    case HAL_AFSTATE_FAILED:
    case HAL_AFSTATE_NEEDS_DETERMINATION:
    case HAL_AFSTATE_PASSIVE_FOCUSED:
        SetAfMode(AA_AFMODE_OFF);
        SetAfStateForService(ANDROID_CONTROL_AF_STATE_INACTIVE);
        SetAfMode(AA_AFMODE_CONTINUOUS_PICTURE);
        nextState = HAL_AFSTATE_INACTIVE;
        break;
    default:
        break;
    }
    ALOGV("(%s): State (%d) -> (%d)", __FUNCTION__, m_afState, nextState);
    if (nextState != NO_TRANSITION)
        m_afState = nextState;
}

void ExynosCameraHWInterface2::OnAfCancelCAFVideo(int /*id*/)
{
    int nextState = NO_TRANSITION;

    switch (m_afState) {
    case HAL_AFSTATE_INACTIVE:
        nextState = NO_TRANSITION;
        break;
    case HAL_AFSTATE_NEEDS_COMMAND:
    case HAL_AFSTATE_STARTED:
    case HAL_AFSTATE_SCANNING:
    case HAL_AFSTATE_LOCKED:
    case HAL_AFSTATE_FAILED:
    case HAL_AFSTATE_NEEDS_DETERMINATION:
    case HAL_AFSTATE_PASSIVE_FOCUSED:
        SetAfMode(AA_AFMODE_OFF);
        SetAfStateForService(ANDROID_CONTROL_AF_STATE_INACTIVE);
        SetAfMode(AA_AFMODE_CONTINUOUS_VIDEO);
        nextState = HAL_AFSTATE_INACTIVE;
        break;
    default:
        break;
    }
    ALOGV("(%s): State (%d) -> (%d)", __FUNCTION__, m_afState, nextState);
    if (nextState != NO_TRANSITION)
        m_afState = nextState;
}

void ExynosCameraHWInterface2::SetAfStateForService(int newState)
{
    if (m_serviceAfState != newState || newState == 0)
        m_notifyCb(CAMERA2_MSG_AUTOFOCUS, newState, m_afTriggerId, 0, m_callbackCookie);
    m_serviceAfState = newState;
}

int ExynosCameraHWInterface2::GetAfStateForService()
{
   return m_serviceAfState;
}

void ExynosCameraHWInterface2::SetAfMode(enum aa_afmode afMode)
{
    if (m_afMode != afMode) {
        if (m_IsAfModeUpdateRequired && m_afMode != AA_AFMODE_OFF) {
            m_afMode2 = afMode;
            ALOGV("(%s): pending(%d) and new(%d)", __FUNCTION__, m_afMode, afMode);
        }
        else {
            ALOGV("(%s): current(%d) new(%d)", __FUNCTION__, m_afMode, afMode);
            m_IsAfModeUpdateRequired = true;
            m_afMode = afMode;
            SetAfStateForService(ANDROID_CONTROL_AF_STATE_INACTIVE);
            m_afState = HAL_AFSTATE_INACTIVE;
        }
    }
}

void ExynosCameraHWInterface2::m_setExifFixedAttribute(void)
{
    char property[PROPERTY_VALUE_MAX];

    //2 0th IFD TIFF Tags
    //3 Maker
    property_get("ro.product.brand", property, EXIF_DEF_MAKER);
    strncpy((char *)mExifInfo.maker, property,
                sizeof(mExifInfo.maker) - 1);
    mExifInfo.maker[sizeof(mExifInfo.maker) - 1] = '\0';
    //3 Model
    property_get("ro.product.model", property, EXIF_DEF_MODEL);
    strncpy((char *)mExifInfo.model, property,
                sizeof(mExifInfo.model) - 1);
    mExifInfo.model[sizeof(mExifInfo.model) - 1] = '\0';
    //3 Software
    property_get("ro.build.id", property, EXIF_DEF_SOFTWARE);
    strncpy((char *)mExifInfo.software, property,
                sizeof(mExifInfo.software) - 1);
    mExifInfo.software[sizeof(mExifInfo.software) - 1] = '\0';

    //3 YCbCr Positioning
    mExifInfo.ycbcr_positioning = EXIF_DEF_YCBCR_POSITIONING;

    //2 0th IFD Exif Private Tags
    //3 F Number
    mExifInfo.fnumber.num = (uint32_t)(m_camera2->m_curCameraInfo->fnumber * EXIF_DEF_FNUMBER_DEN);
    mExifInfo.fnumber.den = EXIF_DEF_FNUMBER_DEN;
    //3 Exposure Program
    mExifInfo.exposure_program = EXIF_DEF_EXPOSURE_PROGRAM;
    //3 Exif Version
    memcpy(mExifInfo.exif_version, EXIF_DEF_EXIF_VERSION, sizeof(mExifInfo.exif_version));
    //3 Aperture
    double av = APEX_FNUM_TO_APERTURE((double)mExifInfo.fnumber.num/mExifInfo.fnumber.den);
    mExifInfo.aperture.num = (uint32_t)(av*EXIF_DEF_APEX_DEN);
    mExifInfo.aperture.den = EXIF_DEF_APEX_DEN;
    //3 Maximum lens aperture
    mExifInfo.max_aperture.num = mExifInfo.aperture.num;
    mExifInfo.max_aperture.den = mExifInfo.aperture.den;
    //3 Lens Focal Length
    mExifInfo.focal_length.num = (uint32_t)(m_camera2->m_curCameraInfo->focalLength * 100);

    mExifInfo.focal_length.den = EXIF_DEF_FOCAL_LEN_DEN;
    //3 User Comments
    strcpy((char *)mExifInfo.user_comment, EXIF_DEF_USERCOMMENTS);
    //3 Color Space information
    mExifInfo.color_space = EXIF_DEF_COLOR_SPACE;
    //3 Exposure Mode
    mExifInfo.exposure_mode = EXIF_DEF_EXPOSURE_MODE;

    //2 0th IFD GPS Info Tags
    unsigned char gps_version[4] = { 0x02, 0x02, 0x00, 0x00 };
    memcpy(mExifInfo.gps_version_id, gps_version, sizeof(gps_version));

    //2 1th IFD TIFF Tags
    mExifInfo.compression_scheme = EXIF_DEF_COMPRESSION;
    mExifInfo.x_resolution.num = EXIF_DEF_RESOLUTION_NUM;
    mExifInfo.x_resolution.den = EXIF_DEF_RESOLUTION_DEN;
    mExifInfo.y_resolution.num = EXIF_DEF_RESOLUTION_NUM;
    mExifInfo.y_resolution.den = EXIF_DEF_RESOLUTION_DEN;
    mExifInfo.resolution_unit = EXIF_DEF_RESOLUTION_UNIT;
}

void ExynosCameraHWInterface2::m_setExifChangedAttribute(exif_attribute_t *exifInfo, ExynosRect *rect,
        camera2_shot_ext *currentEntry)
{
    camera2_dm *dm = &(currentEntry->shot.dm);
    camera2_ctl *ctl = &(currentEntry->shot.ctl);

    ALOGV("(%s): framecnt(%d) exp(%lld) iso(%d)", __FUNCTION__, ctl->request.frameCount, dm->sensor.exposureTime,dm->aa.isoValue );
    if (!ctl->request.frameCount)
       return;
    //2 0th IFD TIFF Tags
    //3 Width
    exifInfo->width = rect->w;
    //3 Height
    exifInfo->height = rect->h;
    //3 Orientation
    switch (ctl->jpeg.orientation) {
    case 90:
        exifInfo->orientation = EXIF_ORIENTATION_90;
        break;
    case 180:
        exifInfo->orientation = EXIF_ORIENTATION_180;
        break;
    case 270:
        exifInfo->orientation = EXIF_ORIENTATION_270;
        break;
    case 0:
    default:
        exifInfo->orientation = EXIF_ORIENTATION_UP;
        break;
    }

    //3 Date time
    struct timeval rawtime;
    struct tm *timeinfo;
    gettimeofday(&rawtime, NULL);
    timeinfo = localtime(&rawtime.tv_sec);
    strftime((char *)exifInfo->date_time, 20, "%Y:%m:%d %H:%M:%S", timeinfo);
    snprintf((char *)exifInfo->sub_sec, sizeof(exifInfo->sub_sec), "%03lu",
        (unsigned long)rawtime.tv_usec / 1000UL);

    //2 0th IFD Exif Private Tags
    //3 Exposure Time
    int shutterSpeed = (dm->sensor.exposureTime/1000);

    // To display exposure time just above 500ms as 1/2sec, not 1 sec.
    if (shutterSpeed > 500000)
        shutterSpeed -=  100000;

    if (shutterSpeed < 0) {
        shutterSpeed = 100;
    }

    exifInfo->exposure_time.num = 1;
    // x us -> 1/x s */
    //exifInfo->exposure_time.den = (uint32_t)(1000000 / shutterSpeed);
    exifInfo->exposure_time.den = (uint32_t)((double)1000000 / shutterSpeed);

    //3 ISO Speed Rating
    exifInfo->iso_speed_rating = dm->aa.isoValue;

    uint32_t av, tv, bv, sv, ev;
    av = APEX_FNUM_TO_APERTURE((double)exifInfo->fnumber.num / exifInfo->fnumber.den);
    tv = APEX_EXPOSURE_TO_SHUTTER((double)exifInfo->exposure_time.num / exifInfo->exposure_time.den);
    sv = APEX_ISO_TO_FILMSENSITIVITY(exifInfo->iso_speed_rating);
    bv = av + tv - sv;
    ev = av + tv;
    //ALOGD("Shutter speed=%d us, iso=%d", shutterSpeed, exifInfo->iso_speed_rating);
    ALOGV("AV=%d, TV=%d, SV=%d", av, tv, sv);

    //3 Shutter Speed
    exifInfo->shutter_speed.num = tv * EXIF_DEF_APEX_DEN;
    exifInfo->shutter_speed.den = EXIF_DEF_APEX_DEN;
    //3 Brightness
    exifInfo->brightness.num = bv*EXIF_DEF_APEX_DEN;
    exifInfo->brightness.den = EXIF_DEF_APEX_DEN;
    //3 Exposure Bias
    if (ctl->aa.sceneMode== AA_SCENE_MODE_BEACH||
        ctl->aa.sceneMode== AA_SCENE_MODE_SNOW) {
        exifInfo->exposure_bias.num = EXIF_DEF_APEX_DEN;
        exifInfo->exposure_bias.den = EXIF_DEF_APEX_DEN;
    } else {
        exifInfo->exposure_bias.num = 0;
        exifInfo->exposure_bias.den = 0;
    }
    //3 Metering Mode
    /*switch (m_curCameraInfo->metering) {
    case METERING_MODE_CENTER:
        exifInfo->metering_mode = EXIF_METERING_CENTER;
        break;
    case METERING_MODE_MATRIX:
        exifInfo->metering_mode = EXIF_METERING_MULTISPOT;
        break;
    case METERING_MODE_SPOT:
        exifInfo->metering_mode = EXIF_METERING_SPOT;
        break;
    case METERING_MODE_AVERAGE:
    default:
        exifInfo->metering_mode = EXIF_METERING_AVERAGE;
        break;
    }*/
    exifInfo->metering_mode = EXIF_METERING_CENTER;

    //3 Flash
    if (m_ctlInfo.flash.m_flashDecisionResult)
        exifInfo->flash = 1;
    else
        exifInfo->flash = EXIF_DEF_FLASH;

    //3 White Balance
    if (currentEntry->awb_mode_dm == AA_AWBMODE_WB_AUTO)
        exifInfo->white_balance = EXIF_WB_AUTO;
    else
        exifInfo->white_balance = EXIF_WB_MANUAL;

    //3 Scene Capture Type
    switch (ctl->aa.sceneMode) {
    case AA_SCENE_MODE_PORTRAIT:
        exifInfo->scene_capture_type = EXIF_SCENE_PORTRAIT;
        break;
    case AA_SCENE_MODE_LANDSCAPE:
        exifInfo->scene_capture_type = EXIF_SCENE_LANDSCAPE;
        break;
    case AA_SCENE_MODE_NIGHT_PORTRAIT:
        exifInfo->scene_capture_type = EXIF_SCENE_NIGHT;
        break;
    default:
        exifInfo->scene_capture_type = EXIF_SCENE_STANDARD;
        break;
    }

    //2 0th IFD GPS Info Tags
    if (ctl->jpeg.gpsCoordinates[0] != 0 && ctl->jpeg.gpsCoordinates[1] != 0) {

        if (ctl->jpeg.gpsCoordinates[0] > 0)
            strcpy((char *)exifInfo->gps_latitude_ref, "N");
        else
            strcpy((char *)exifInfo->gps_latitude_ref, "S");

        if (ctl->jpeg.gpsCoordinates[1] > 0)
            strcpy((char *)exifInfo->gps_longitude_ref, "E");
        else
            strcpy((char *)exifInfo->gps_longitude_ref, "W");

        if (ctl->jpeg.gpsCoordinates[2] > 0)
            exifInfo->gps_altitude_ref = 0;
        else
            exifInfo->gps_altitude_ref = 1;

        double latitude = fabs(ctl->jpeg.gpsCoordinates[0]);
        double longitude = fabs(ctl->jpeg.gpsCoordinates[1]);
        double altitude = fabs(ctl->jpeg.gpsCoordinates[2]);

        exifInfo->gps_latitude[0].num = (uint32_t)latitude;
        exifInfo->gps_latitude[0].den = 1;
        exifInfo->gps_latitude[1].num = (uint32_t)((latitude - exifInfo->gps_latitude[0].num) * 60);
        exifInfo->gps_latitude[1].den = 1;
        exifInfo->gps_latitude[2].num = (uint32_t)round((((latitude - exifInfo->gps_latitude[0].num) * 60)
                                        - exifInfo->gps_latitude[1].num) * 60);
        exifInfo->gps_latitude[2].den = 1;

        exifInfo->gps_longitude[0].num = (uint32_t)longitude;
        exifInfo->gps_longitude[0].den = 1;
        exifInfo->gps_longitude[1].num = (uint32_t)((longitude - exifInfo->gps_longitude[0].num) * 60);
        exifInfo->gps_longitude[1].den = 1;
        exifInfo->gps_longitude[2].num = (uint32_t)round((((longitude - exifInfo->gps_longitude[0].num) * 60)
                                        - exifInfo->gps_longitude[1].num) * 60);
        exifInfo->gps_longitude[2].den = 1;

        exifInfo->gps_altitude.num = (uint32_t)round(altitude);
        exifInfo->gps_altitude.den = 1;

        struct tm tm_data;
        long timestamp;
        timestamp = (long)ctl->jpeg.gpsTimestamp;
        gmtime_r(&timestamp, &tm_data);
        exifInfo->gps_timestamp[0].num = tm_data.tm_hour;
        exifInfo->gps_timestamp[0].den = 1;
        exifInfo->gps_timestamp[1].num = tm_data.tm_min;
        exifInfo->gps_timestamp[1].den = 1;
        exifInfo->gps_timestamp[2].num = tm_data.tm_sec;
        exifInfo->gps_timestamp[2].den = 1;
        snprintf((char*)exifInfo->gps_datestamp, sizeof(exifInfo->gps_datestamp),
                "%04d:%02d:%02d", tm_data.tm_year + 1900, tm_data.tm_mon + 1, tm_data.tm_mday);

        memset(exifInfo->gps_processing_method, 0, 100);
        memcpy(exifInfo->gps_processing_method, currentEntry->gpsProcessingMethod, 32);
        exifInfo->enableGps = true;
    } else {
        exifInfo->enableGps = false;
    }

    //2 1th IFD TIFF Tags
    exifInfo->widthThumb = ctl->jpeg.thumbnailSize[0];
    exifInfo->heightThumb = ctl->jpeg.thumbnailSize[1];
}

ExynosCameraHWInterface2::MainThread::~MainThread()
{
    ALOGV("(%s):", __FUNCTION__);
}

void ExynosCameraHWInterface2::MainThread::release()
{
    ALOGV("(%s):", __func__);
    SetSignal(SIGNAL_THREAD_RELEASE);
}

ExynosCameraHWInterface2::SensorThread::~SensorThread()
{
    ALOGV("(%s):", __FUNCTION__);
}

void ExynosCameraHWInterface2::SensorThread::release()
{
    ALOGV("(%s):", __func__);
    SetSignal(SIGNAL_THREAD_RELEASE);
}

ExynosCameraHWInterface2::StreamThread::~StreamThread()
{
    ALOGV("(%s):", __FUNCTION__);
}

void ExynosCameraHWInterface2::StreamThread::setParameter(stream_parameters_t * new_parameters)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    memcpy(&m_parameters, new_parameters, sizeof(stream_parameters_t));
}

void ExynosCameraHWInterface2::StreamThread::release()
{
    ALOGV("(%s):", __func__);
    SetSignal(SIGNAL_THREAD_RELEASE);
}

int ExynosCameraHWInterface2::StreamThread::findBufferIndex(void * bufAddr)
{
    int index;
    for (index = 0 ; index < m_parameters.numSvcBuffers ; index++) {
        if (m_parameters.svcBuffers[index].virt.extP[0] == bufAddr)
            return index;
    }
    return -1;
}

int ExynosCameraHWInterface2::StreamThread::findBufferIndex(buffer_handle_t * bufHandle)
{
    int index;
    for (index = 0 ; index < m_parameters.numSvcBuffers ; index++) {
        if (m_parameters.svcBufHandle[index] == *bufHandle)
            return index;
    }
    return -1;
}

status_t ExynosCameraHWInterface2::StreamThread::attachSubStream(int stream_id, int priority)
{
    ALOGV("(%s): substream_id(%d)", __FUNCTION__, stream_id);
    int index, vacantIndex;
    bool vacancy = false;

    for (index = 0 ; index < NUM_MAX_SUBSTREAM ; index++) {
        if (!vacancy && m_attachedSubStreams[index].streamId == -1) {
            vacancy = true;
            vacantIndex = index;
        } else if (m_attachedSubStreams[index].streamId == stream_id) {
            return BAD_VALUE;
        }
    }
    if (!vacancy)
        return NO_MEMORY;
    m_attachedSubStreams[vacantIndex].streamId = stream_id;
    m_attachedSubStreams[vacantIndex].priority = priority;
    m_numRegisteredStream++;
    return NO_ERROR;
}

status_t ExynosCameraHWInterface2::StreamThread::detachSubStream(int stream_id)
{
    ALOGV("(%s): substream_id(%d)", __FUNCTION__, stream_id);
    int index;
    bool found = false;

    for (index = 0 ; index < NUM_MAX_SUBSTREAM ; index++) {
        if (m_attachedSubStreams[index].streamId == stream_id) {
            found = true;
            break;
        }
    }
    if (!found)
        return BAD_VALUE;
    m_attachedSubStreams[index].streamId = -1;
    m_attachedSubStreams[index].priority = 0;
    m_numRegisteredStream--;
    return NO_ERROR;
}

int ExynosCameraHWInterface2::createIonClient(ion_client ionClient)
{
    if (ionClient == 0) {
        ionClient = ion_client_create();
        if (ionClient < 0) {
            ALOGE("[%s]src ion client create failed, value = %d\n", __FUNCTION__, ionClient);
            return 0;
        }
    }
    return ionClient;
}

int ExynosCameraHWInterface2::deleteIonClient(ion_client ionClient)
{
    if (ionClient != 0) {
        if (ionClient > 0) {
            ion_client_destroy(ionClient);
        }
        ionClient = 0;
    }
    return ionClient;
}

int ExynosCameraHWInterface2::allocCameraMemory(ion_client ionClient, ExynosBuffer *buf, int iMemoryNum)
{
    return allocCameraMemory(ionClient, buf, iMemoryNum, 0);
}

int ExynosCameraHWInterface2::allocCameraMemory(ion_client ionClient, ExynosBuffer *buf, int iMemoryNum, int cacheFlag)
{
    int ret = 0;
    int i = 0;
    int flag = 0;

    if (ionClient == 0) {
        ALOGE("[%s] ionClient is zero (%d)\n", __FUNCTION__, ionClient);
        return -1;
    }

    for (i = 0 ; i < iMemoryNum ; i++) {
        if (buf->size.extS[i] == 0) {
            break;
        }
        if (1 << i & cacheFlag)
            flag = ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC;
        else
            flag = 0;
        buf->fd.extFd[i] = ion_alloc(ionClient, \
                                      buf->size.extS[i], 0, ION_HEAP_SYSTEM_MASK, flag);
        if ((buf->fd.extFd[i] == -1) ||(buf->fd.extFd[i] == 0)) {
            ALOGE("[%s]ion_alloc(%d) failed\n", __FUNCTION__, buf->size.extS[i]);
            buf->fd.extFd[i] = -1;
            freeCameraMemory(buf, iMemoryNum);
            return -1;
        }

        buf->virt.extP[i] = (char *)ion_map(buf->fd.extFd[i], \
                                        buf->size.extS[i], 0);
        if ((buf->virt.extP[i] == (char *)MAP_FAILED) || (buf->virt.extP[i] == NULL)) {
            ALOGE("[%s]src ion map failed(%d)\n", __FUNCTION__, buf->size.extS[i]);
            buf->virt.extP[i] = (char *)MAP_FAILED;
            freeCameraMemory(buf, iMemoryNum);
            return -1;
        }
        ALOGV("allocCameraMem : [%d][0x%08x] size(%d) flag(%d)", i, (unsigned int)(buf->virt.extP[i]), buf->size.extS[i], flag);
    }

    return ret;
}

void ExynosCameraHWInterface2::freeCameraMemory(ExynosBuffer *buf, int iMemoryNum)
{

    int i = 0 ;
    int ret = 0;

    for (i=0;i<iMemoryNum;i++) {
        if (buf->fd.extFd[i] != -1) {
            if (buf->virt.extP[i] != (char *)MAP_FAILED) {
                ret = ion_unmap(buf->virt.extP[i], buf->size.extS[i]);
                if (ret < 0)
                    ALOGE("ERR(%s)", __FUNCTION__);
            }
            ion_free(buf->fd.extFd[i]);
        ALOGV("freeCameraMemory : [%d][0x%08x] size(%d)", i, (unsigned int)(buf->virt.extP[i]), buf->size.extS[i]);
        }
        buf->fd.extFd[i] = -1;
        buf->virt.extP[i] = (char *)MAP_FAILED;
        buf->size.extS[i] = 0;
    }
}

void ExynosCameraHWInterface2::initCameraMemory(ExynosBuffer *buf, int iMemoryNum)
{
    int i =0 ;
    for (i=0;i<iMemoryNum;i++) {
        buf->virt.extP[i] = (char *)MAP_FAILED;
        buf->fd.extFd[i] = -1;
        buf->size.extS[i] = 0;
    }
}




static camera2_device_t *g_cam2_device = NULL;
static bool g_camera_vaild = false;
static Mutex g_camera_mutex;
ExynosCamera2 * g_camera2[2] = { NULL, NULL };

static int HAL2_camera_device_close(struct hw_device_t* device)
{
    Mutex::Autolock lock(g_camera_mutex);
    ALOGD("(%s): ENTER", __FUNCTION__);
    if (device) {

        camera2_device_t *cam_device = (camera2_device_t *)device;
        ALOGV("cam_device(0x%08x):", (unsigned int)cam_device);
        ALOGV("g_cam2_device(0x%08x):", (unsigned int)g_cam2_device);
        delete static_cast<ExynosCameraHWInterface2 *>(cam_device->priv);
        free(cam_device);
        g_camera_vaild = false;
        g_cam2_device = NULL;
    }

    ALOGD("(%s): EXIT", __FUNCTION__);
    return 0;
}

static inline ExynosCameraHWInterface2 *obj(const struct camera2_device *dev)
{
    return reinterpret_cast<ExynosCameraHWInterface2 *>(dev->priv);
}

static int HAL2_device_set_request_queue_src_ops(const struct camera2_device *dev,
            const camera2_request_queue_src_ops_t *request_src_ops)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->setRequestQueueSrcOps(request_src_ops);
}

static int HAL2_device_notify_request_queue_not_empty(const struct camera2_device *dev)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->notifyRequestQueueNotEmpty();
}

static int HAL2_device_set_frame_queue_dst_ops(const struct camera2_device *dev,
            const camera2_frame_queue_dst_ops_t *frame_dst_ops)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->setFrameQueueDstOps(frame_dst_ops);
}

static int HAL2_device_get_in_progress_count(const struct camera2_device *dev)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->getInProgressCount();
}

static int HAL2_device_flush_captures_in_progress(const struct camera2_device *dev)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->flushCapturesInProgress();
}

static int HAL2_device_construct_default_request(const struct camera2_device *dev,
            int request_template, camera_metadata_t **request)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->constructDefaultRequest(request_template, request);
}

static int HAL2_device_allocate_stream(
            const struct camera2_device *dev,
            // inputs
            uint32_t width,
            uint32_t height,
            int      format,
            const camera2_stream_ops_t *stream_ops,
            // outputs
            uint32_t *stream_id,
            uint32_t *format_actual,
            uint32_t *usage,
            uint32_t *max_buffers)
{
    ALOGV("(%s): ", __FUNCTION__);
    return obj(dev)->allocateStream(width, height, format, stream_ops,
                                    stream_id, format_actual, usage, max_buffers);
}

static int HAL2_device_register_stream_buffers(const struct camera2_device *dev,
            uint32_t stream_id,
            int num_buffers,
            buffer_handle_t *buffers)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->registerStreamBuffers(stream_id, num_buffers, buffers);
}

static int HAL2_device_release_stream(
        const struct camera2_device *dev,
            uint32_t stream_id)
{
    ALOGV("DEBUG(%s)(id: %d):", __FUNCTION__, stream_id);
    if (!g_camera_vaild)
        return 0;
    return obj(dev)->releaseStream(stream_id);
}

static int HAL2_device_allocate_reprocess_stream(
           const struct camera2_device *dev,
            uint32_t width,
            uint32_t height,
            uint32_t format,
            const camera2_stream_in_ops_t *reprocess_stream_ops,
            // outputs
            uint32_t *stream_id,
            uint32_t *consumer_usage,
            uint32_t *max_buffers)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->allocateReprocessStream(width, height, format, reprocess_stream_ops,
                                    stream_id, consumer_usage, max_buffers);
}

static int HAL2_device_allocate_reprocess_stream_from_stream(
           const struct camera2_device *dev,
            uint32_t output_stream_id,
            const camera2_stream_in_ops_t *reprocess_stream_ops,
            // outputs
            uint32_t *stream_id)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->allocateReprocessStreamFromStream(output_stream_id,
                                    reprocess_stream_ops, stream_id);
}

static int HAL2_device_release_reprocess_stream(
        const struct camera2_device *dev,
            uint32_t stream_id)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->releaseReprocessStream(stream_id);
}

static int HAL2_device_trigger_action(const struct camera2_device *dev,
           uint32_t trigger_id,
            int ext1,
            int ext2)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    if (!g_camera_vaild)
        return 0;
    return obj(dev)->triggerAction(trigger_id, ext1, ext2);
}

static int HAL2_device_set_notify_callback(const struct camera2_device *dev,
            camera2_notify_callback notify_cb,
            void *user)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->setNotifyCallback(notify_cb, user);
}

static int HAL2_device_get_metadata_vendor_tag_ops(const struct camera2_device*dev,
            vendor_tag_query_ops_t **ops)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->getMetadataVendorTagOps(ops);
}

static int HAL2_device_dump(const struct camera2_device *dev, int fd)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->dump(fd);
}





static int HAL2_getNumberOfCameras()
{
    ALOGV("(%s): returning 2", __FUNCTION__);
    return 2;
}


static int HAL2_getCameraInfo(int cameraId, struct camera_info *info)
{
    ALOGV("DEBUG(%s): cameraID: %d", __FUNCTION__, cameraId);
    static camera_metadata_t * mCameraInfo[2] = {NULL, NULL};

    status_t res;

    if (cameraId == 0) {
        info->facing = CAMERA_FACING_BACK;
        if (!g_camera2[0])
            g_camera2[0] = new ExynosCamera2(0);
    }
    else if (cameraId == 1) {
        info->facing = CAMERA_FACING_FRONT;
        if (!g_camera2[1])
            g_camera2[1] = new ExynosCamera2(1);
    }
    else
        return BAD_VALUE;

    info->orientation = 0;
    info->device_version = HARDWARE_DEVICE_API_VERSION(2, 0);
    if (mCameraInfo[cameraId] == NULL) {
        res = g_camera2[cameraId]->constructStaticInfo(&(mCameraInfo[cameraId]), cameraId, true);
        if (res != OK) {
            ALOGE("%s: Unable to allocate static info: %s (%d)",
                    __FUNCTION__, strerror(-res), res);
            return res;
        }
        res = g_camera2[cameraId]->constructStaticInfo(&(mCameraInfo[cameraId]), cameraId, false);
        if (res != OK) {
            ALOGE("%s: Unable to fill in static info: %s (%d)",
                    __FUNCTION__, strerror(-res), res);
            return res;
        }
    }
    info->static_camera_characteristics = mCameraInfo[cameraId];
    return NO_ERROR;
}

#define SET_METHOD(m) m : HAL2_device_##m

static camera2_device_ops_t camera2_device_ops = {
        SET_METHOD(set_request_queue_src_ops),
        SET_METHOD(notify_request_queue_not_empty),
        SET_METHOD(set_frame_queue_dst_ops),
        SET_METHOD(get_in_progress_count),
        SET_METHOD(flush_captures_in_progress),
        SET_METHOD(construct_default_request),
        SET_METHOD(allocate_stream),
        SET_METHOD(register_stream_buffers),
        SET_METHOD(release_stream),
        SET_METHOD(allocate_reprocess_stream),
        SET_METHOD(allocate_reprocess_stream_from_stream),
        SET_METHOD(release_reprocess_stream),
        SET_METHOD(trigger_action),
        SET_METHOD(set_notify_callback),
        SET_METHOD(get_metadata_vendor_tag_ops),
        SET_METHOD(dump),
        get_instance_metadata : NULL
};

#undef SET_METHOD


static int HAL2_camera_device_open(const struct hw_module_t* module,
                                  const char *id,
                                  struct hw_device_t** device)
{
    int cameraId = atoi(id);
    int openInvalid = 0;

    Mutex::Autolock lock(g_camera_mutex);
    if (g_camera_vaild) {
        ALOGE("ERR(%s): Can't open, other camera is in use", __FUNCTION__);
        return -EUSERS;
    }
    g_camera_vaild = false;
    ALOGD("\n\n>>> I'm Samsung's CameraHAL_2(ID:%d) <<<\n\n", cameraId);
    if (cameraId < 0 || cameraId >= HAL2_getNumberOfCameras()) {
        ALOGE("ERR(%s):Invalid camera ID %s", __FUNCTION__, id);
        return -EINVAL;
    }

    ALOGD("g_cam2_device : 0x%08x", (unsigned int)g_cam2_device);
    if (g_cam2_device) {
        if (obj(g_cam2_device)->getCameraId() == cameraId) {
            ALOGD("DEBUG(%s):returning existing camera ID %s", __FUNCTION__, id);
            goto done;
        } else {
            ALOGD("(%s): START waiting for cam device free", __FUNCTION__);
            while (g_cam2_device)
                usleep(SIG_WAITING_TICK);
            ALOGD("(%s): END   waiting for cam device free", __FUNCTION__);
        }
    }

    g_cam2_device = (camera2_device_t *)malloc(sizeof(camera2_device_t));
    ALOGV("g_cam2_device : 0x%08x", (unsigned int)g_cam2_device);

    if (!g_cam2_device)
        return -ENOMEM;

    g_cam2_device->common.tag     = HARDWARE_DEVICE_TAG;
    g_cam2_device->common.version = CAMERA_DEVICE_API_VERSION_2_0;
    g_cam2_device->common.module  = const_cast<hw_module_t *>(module);
    g_cam2_device->common.close   = HAL2_camera_device_close;

    g_cam2_device->ops = &camera2_device_ops;

    ALOGV("DEBUG(%s):open camera2 %s", __FUNCTION__, id);

    g_cam2_device->priv = new ExynosCameraHWInterface2(cameraId, g_cam2_device, g_camera2[cameraId], &openInvalid);
    if (!openInvalid) {
        ALOGE("DEBUG(%s): ExynosCameraHWInterface2 creation failed", __FUNCTION__);
        return -ENODEV;
    }
done:
    *device = (hw_device_t *)g_cam2_device;
    ALOGV("DEBUG(%s):opened camera2 %s (%p)", __FUNCTION__, id, *device);
    g_camera_vaild = true;

    return 0;
}


static hw_module_methods_t camera_module_methods = {
            open : HAL2_camera_device_open
};

extern "C" {
    struct camera_module HAL_MODULE_INFO_SYM = {
      common : {
          tag                : HARDWARE_MODULE_TAG,
          module_api_version : CAMERA_MODULE_API_VERSION_2_0,
          hal_api_version    : HARDWARE_HAL_API_VERSION,
          id                 : CAMERA_HARDWARE_MODULE_ID,
          name               : "Exynos Camera HAL2",
          author             : "Samsung Corporation",
          methods            : &camera_module_methods,
          dso                : NULL,
          reserved           : {0},
      },
      get_number_of_cameras : HAL2_getNumberOfCameras,
      get_camera_info       : HAL2_getCameraInfo,
      set_callbacks         : NULL,
      get_vendor_tag_ops    : NULL,
      open_legacy           : NULL,
      reserved              : {0}
    };
}

}; // namespace android
