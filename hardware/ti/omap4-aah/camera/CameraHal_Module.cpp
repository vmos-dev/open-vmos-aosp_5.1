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

/**
* @file CameraHal.cpp
*
* This file maps the Camera Hardware Interface to V4L2.
*
*/

#include <utils/threads.h>

#include "CameraHal.h"
#include "CameraProperties.h"
#include "TICameraParameters.h"


#ifdef CAMERAHAL_DEBUG_VERBOSE
#   define CAMHAL_LOG_MODULE_FUNCTION_NAME LOG_FUNCTION_NAME
#else
#   define CAMHAL_LOG_MODULE_FUNCTION_NAME
#endif


namespace Ti {
namespace Camera {

static CameraProperties gCameraProperties;
static CameraHal* gCameraHals[MAX_CAMERAS_SUPPORTED];
static unsigned int gCamerasOpen = 0;
static android::Mutex gCameraHalDeviceLock;

static int camera_device_open(const hw_module_t* module, const char* name,
                hw_device_t** device);
static int camera_device_close(hw_device_t* device);
static int camera_get_number_of_cameras(void);
static int camera_get_camera_info(int camera_id, struct camera_info *info);

static struct hw_module_methods_t camera_module_methods = {
        open: camera_device_open
};

} // namespace Camera
} // namespace Ti


camera_module_t HAL_MODULE_INFO_SYM = {
    common: {
         tag: HARDWARE_MODULE_TAG,
         version_major: 1,
         version_minor: 0,
         id: CAMERA_HARDWARE_MODULE_ID,
         name: "TI OMAP CameraHal Module",
         author: "TI",
         methods: &Ti::Camera::camera_module_methods,
         dso: NULL, /* remove compilation warnings */
         reserved: {0}, /* remove compilation warnings */
    },
    get_number_of_cameras: Ti::Camera::camera_get_number_of_cameras,
    get_camera_info: Ti::Camera::camera_get_camera_info,
};


namespace Ti {
namespace Camera {

typedef struct ti_camera_device {
    camera_device_t base;
    /* TI specific "private" data can go here (base.priv) */
    int cameraid;
} ti_camera_device_t;


/*******************************************************************
 * implementation of camera_device_ops functions
 *******************************************************************/

int camera_set_preview_window(struct camera_device * device,
        struct preview_stream_ops *window)
{
    CAMHAL_LOG_MODULE_FUNCTION_NAME;

    int rv = -EINVAL;
    ti_camera_device_t* ti_dev = NULL;

    if(!device)
        return rv;

    ti_dev = (ti_camera_device_t*) device;

    rv = gCameraHals[ti_dev->cameraid]->setPreviewWindow(window);

    return rv;
}

#ifdef OMAP_ENHANCEMENT_CPCAM
int camera_set_extended_preview_ops(struct camera_device * device,
        preview_stream_extended_ops_t * extendedOps)
{
    CAMHAL_LOG_MODULE_FUNCTION_NAME;

    if (!device) {
        return BAD_VALUE;
    }

    ti_camera_device_t * const tiDevice = reinterpret_cast<ti_camera_device_t*>(device);
    gCameraHals[tiDevice->cameraid]->setExtendedPreviewStreamOps(extendedOps);

    return OK;
}

int camera_set_buffer_source(struct camera_device * device,
        struct preview_stream_ops *tapin,
        struct preview_stream_ops *tapout)
{
    CAMHAL_LOG_MODULE_FUNCTION_NAME;

    int rv = -EINVAL;
    ti_camera_device_t* ti_dev = NULL;

    if(!device)
        return rv;

    ti_dev = (ti_camera_device_t*) device;

    rv = gCameraHals[ti_dev->cameraid]->setBufferSource(tapin, tapout);

    return rv;
}

int camera_release_buffer_source(struct camera_device * device,
                                 struct preview_stream_ops *tapin,
                                 struct preview_stream_ops *tapout)
{
    CAMHAL_LOG_MODULE_FUNCTION_NAME;

    int rv = -EINVAL;
    ti_camera_device_t* ti_dev = NULL;

    if(!device)
        return rv;

    ti_dev = (ti_camera_device_t*) device;

    rv = gCameraHals[ti_dev->cameraid]->releaseBufferSource(tapin, tapout);

    return rv;
}
#endif

void camera_set_callbacks(struct camera_device * device,
        camera_notify_callback notify_cb,
        camera_data_callback data_cb,
        camera_data_timestamp_callback data_cb_timestamp,
        camera_request_memory get_memory,
        void *user)
{
    CAMHAL_LOG_MODULE_FUNCTION_NAME;

    ti_camera_device_t* ti_dev = NULL;

    if(!device)
        return;

    ti_dev = (ti_camera_device_t*) device;

    gCameraHals[ti_dev->cameraid]->setCallbacks(notify_cb, data_cb, data_cb_timestamp, get_memory, user);
}

void camera_enable_msg_type(struct camera_device * device, int32_t msg_type)
{
    CAMHAL_LOG_MODULE_FUNCTION_NAME;

    ti_camera_device_t* ti_dev = NULL;

    if(!device)
        return;

    ti_dev = (ti_camera_device_t*) device;

    gCameraHals[ti_dev->cameraid]->enableMsgType(msg_type);
}

void camera_disable_msg_type(struct camera_device * device, int32_t msg_type)
{
    CAMHAL_LOG_MODULE_FUNCTION_NAME;

    ti_camera_device_t* ti_dev = NULL;

    if(!device)
        return;

    ti_dev = (ti_camera_device_t*) device;

    gCameraHals[ti_dev->cameraid]->disableMsgType(msg_type);
}

int camera_msg_type_enabled(struct camera_device * device, int32_t msg_type)
{
    CAMHAL_LOG_MODULE_FUNCTION_NAME;

    ti_camera_device_t* ti_dev = NULL;

    if(!device)
        return 0;

    ti_dev = (ti_camera_device_t*) device;

    return gCameraHals[ti_dev->cameraid]->msgTypeEnabled(msg_type);
}

int camera_start_preview(struct camera_device * device)
{
    CAMHAL_LOG_MODULE_FUNCTION_NAME;

    int rv = -EINVAL;
    ti_camera_device_t* ti_dev = NULL;

    if(!device)
        return rv;

    ti_dev = (ti_camera_device_t*) device;

    rv = gCameraHals[ti_dev->cameraid]->startPreview();

    return rv;
}

void camera_stop_preview(struct camera_device * device)
{
    CAMHAL_LOG_MODULE_FUNCTION_NAME;

    ti_camera_device_t* ti_dev = NULL;

    if(!device)
        return;

    ti_dev = (ti_camera_device_t*) device;

    gCameraHals[ti_dev->cameraid]->stopPreview();
}

int camera_preview_enabled(struct camera_device * device)
{
    CAMHAL_LOG_MODULE_FUNCTION_NAME;

    int rv = -EINVAL;
    ti_camera_device_t* ti_dev = NULL;

    if(!device)
        return rv;

    ti_dev = (ti_camera_device_t*) device;

    rv = gCameraHals[ti_dev->cameraid]->previewEnabled();
    return rv;
}

int camera_store_meta_data_in_buffers(struct camera_device * device, int enable)
{
    CAMHAL_LOG_MODULE_FUNCTION_NAME;

    int rv = -EINVAL;
    ti_camera_device_t* ti_dev = NULL;

    if(!device)
        return rv;

    ti_dev = (ti_camera_device_t*) device;

    //  TODO: meta data buffer not current supported
    rv = gCameraHals[ti_dev->cameraid]->storeMetaDataInBuffers(enable);
    return rv;
    //return enable ? android::INVALID_OPERATION: android::OK;
}

int camera_start_recording(struct camera_device * device)
{
    CAMHAL_LOG_MODULE_FUNCTION_NAME;

    int rv = -EINVAL;
    ti_camera_device_t* ti_dev = NULL;

    if(!device)
        return rv;

    ti_dev = (ti_camera_device_t*) device;

    rv = gCameraHals[ti_dev->cameraid]->startRecording();
    return rv;
}

void camera_stop_recording(struct camera_device * device)
{
    CAMHAL_LOG_MODULE_FUNCTION_NAME;

    ti_camera_device_t* ti_dev = NULL;

    if(!device)
        return;

    ti_dev = (ti_camera_device_t*) device;

    gCameraHals[ti_dev->cameraid]->stopRecording();
}

int camera_recording_enabled(struct camera_device * device)
{
    CAMHAL_LOG_MODULE_FUNCTION_NAME;

    int rv = -EINVAL;
    ti_camera_device_t* ti_dev = NULL;

    if(!device)
        return rv;

    ti_dev = (ti_camera_device_t*) device;

    rv = gCameraHals[ti_dev->cameraid]->recordingEnabled();
    return rv;
}

void camera_release_recording_frame(struct camera_device * device,
                const void *opaque)
{
    CAMHAL_LOG_MODULE_FUNCTION_NAME;

    ti_camera_device_t* ti_dev = NULL;

    if(!device)
        return;

    ti_dev = (ti_camera_device_t*) device;

    gCameraHals[ti_dev->cameraid]->releaseRecordingFrame(opaque);
}

int camera_auto_focus(struct camera_device * device)
{
    CAMHAL_LOG_MODULE_FUNCTION_NAME;

    int rv = -EINVAL;
    ti_camera_device_t* ti_dev = NULL;

    if(!device)
        return rv;

    ti_dev = (ti_camera_device_t*) device;

    rv = gCameraHals[ti_dev->cameraid]->autoFocus();
    return rv;
}

int camera_cancel_auto_focus(struct camera_device * device)
{
    CAMHAL_LOG_MODULE_FUNCTION_NAME;

    int rv = -EINVAL;
    ti_camera_device_t* ti_dev = NULL;

    if(!device)
        return rv;

    ti_dev = (ti_camera_device_t*) device;

    rv = gCameraHals[ti_dev->cameraid]->cancelAutoFocus();
    return rv;
}

int camera_take_picture(struct camera_device * device)
{
    CAMHAL_LOG_MODULE_FUNCTION_NAME;

    int rv = -EINVAL;
    ti_camera_device_t* ti_dev = NULL;

    if(!device)
        return rv;

    ti_dev = (ti_camera_device_t*) device;

    rv = gCameraHals[ti_dev->cameraid]->takePicture(0);
    return rv;
}

#ifdef OMAP_ENHANCEMENT_CPCAM
int camera_take_picture_with_parameters(struct camera_device * device, const char *params)
{
    CAMHAL_LOG_MODULE_FUNCTION_NAME;

    int rv = -EINVAL;
    ti_camera_device_t* ti_dev = NULL;

    if(!device)
        return rv;

    ti_dev = (ti_camera_device_t*) device;

    rv = gCameraHals[ti_dev->cameraid]->takePicture(params);
    return rv;
}
#endif

int camera_cancel_picture(struct camera_device * device)
{
    CAMHAL_LOG_MODULE_FUNCTION_NAME;

    int rv = -EINVAL;
    ti_camera_device_t* ti_dev = NULL;

    if(!device)
        return rv;

    ti_dev = (ti_camera_device_t*) device;

    rv = gCameraHals[ti_dev->cameraid]->cancelPicture();
    return rv;
}

#ifdef OMAP_ENHANCEMENT_CPCAM
int camera_reprocess(struct camera_device * device, const char *params)
{
    CAMHAL_LOG_MODULE_FUNCTION_NAME;

    int rv = -EINVAL;
    ti_camera_device_t* ti_dev = NULL;

    if(!device)
        return rv;

    ti_dev = (ti_camera_device_t*) device;

    rv = gCameraHals[ti_dev->cameraid]->reprocess(params);
    return rv;
}

int camera_cancel_reprocess(struct camera_device * device)
{
    CAMHAL_LOG_MODULE_FUNCTION_NAME;

    int rv = -EINVAL;
    ti_camera_device_t* ti_dev = NULL;

    if(!device)
        return rv;

    ti_dev = (ti_camera_device_t*) device;

    rv = gCameraHals[ti_dev->cameraid]->cancel_reprocess();
    return rv;
}
#endif

int camera_set_parameters(struct camera_device * device, const char *params)
{
    CAMHAL_LOG_MODULE_FUNCTION_NAME;

    int rv = -EINVAL;
    ti_camera_device_t* ti_dev = NULL;

    if(!device)
        return rv;

    ti_dev = (ti_camera_device_t*) device;

    rv = gCameraHals[ti_dev->cameraid]->setParameters(params);
    return rv;
}

char* camera_get_parameters(struct camera_device * device)
{
    CAMHAL_LOG_MODULE_FUNCTION_NAME;

    char* param = NULL;
    ti_camera_device_t* ti_dev = NULL;

    if(!device)
        return NULL;

    ti_dev = (ti_camera_device_t*) device;

    param = gCameraHals[ti_dev->cameraid]->getParameters();

    return param;
}

static void camera_put_parameters(struct camera_device *device, char *parms)
{
    CAMHAL_LOG_MODULE_FUNCTION_NAME;

    ti_camera_device_t* ti_dev = NULL;

    if(!device)
        return;

    ti_dev = (ti_camera_device_t*) device;

    gCameraHals[ti_dev->cameraid]->putParameters(parms);
}

int camera_send_command(struct camera_device * device,
            int32_t cmd, int32_t arg1, int32_t arg2)
{
    CAMHAL_LOG_MODULE_FUNCTION_NAME;

    int rv = -EINVAL;
    ti_camera_device_t* ti_dev = NULL;

    if(!device)
        return rv;

    ti_dev = (ti_camera_device_t*) device;

#ifdef OMAP_ENHANCEMENT
    if ( cmd == CAMERA_CMD_SETUP_EXTENDED_OPERATIONS ) {
        camera_device_extended_ops_t * const ops = static_cast<camera_device_extended_ops_t*>(
                camera_cmd_send_command_args_to_pointer(arg1, arg2));

#ifdef OMAP_ENHANCEMENT_CPCAM
        ops->set_extended_preview_ops = camera_set_extended_preview_ops;
        ops->set_buffer_source = camera_set_buffer_source;
        ops->release_buffer_source = camera_release_buffer_source;
        ops->take_picture_with_parameters = camera_take_picture_with_parameters;
        ops->reprocess = camera_reprocess;
        ops->cancel_reprocess = camera_cancel_reprocess;
#endif

        return OK;
    }
#endif

    rv = gCameraHals[ti_dev->cameraid]->sendCommand(cmd, arg1, arg2);
    return rv;
}

void camera_release(struct camera_device * device)
{
    CAMHAL_LOG_MODULE_FUNCTION_NAME;

    ti_camera_device_t* ti_dev = NULL;

    if(!device)
        return;

    ti_dev = (ti_camera_device_t*) device;

    gCameraHals[ti_dev->cameraid]->release();
}

int camera_dump(struct camera_device * device, int fd)
{
    CAMHAL_LOG_MODULE_FUNCTION_NAME;

    int rv = -EINVAL;
    ti_camera_device_t* ti_dev = NULL;

    if(!device)
        return rv;

    ti_dev = (ti_camera_device_t*) device;

    rv = gCameraHals[ti_dev->cameraid]->dump(fd);
    return rv;
}

extern "C" void heaptracker_free_leaked_memory(void);

int camera_device_close(hw_device_t* device)
{
    CAMHAL_LOG_MODULE_FUNCTION_NAME;

    int ret = 0;
    ti_camera_device_t* ti_dev = NULL;

    android::AutoMutex lock(gCameraHalDeviceLock);

    if (!device) {
        ret = -EINVAL;
        goto done;
    }

    ti_dev = (ti_camera_device_t*) device;

    if (ti_dev) {
        if (gCameraHals[ti_dev->cameraid]) {
            delete gCameraHals[ti_dev->cameraid];
            gCameraHals[ti_dev->cameraid] = NULL;
            gCamerasOpen--;
        }

        if (ti_dev->base.ops) {
            free(ti_dev->base.ops);
        }
        free(ti_dev);
    }
done:
#ifdef HEAPTRACKER
    heaptracker_free_leaked_memory();
#endif
    return ret;
}

/*******************************************************************
 * implementation of camera_module functions
 *******************************************************************/

/* open device handle to one of the cameras
 *
 * assume camera service will keep singleton of each camera
 * so this function will always only be called once per camera instance
 */

int camera_device_open(const hw_module_t* module, const char* name,
                hw_device_t** device)
{
    int rv = 0;
    int num_cameras = 0;
    int cameraid;
    ti_camera_device_t* camera_device = NULL;
    camera_device_ops_t* camera_ops = NULL;
    CameraHal* camera = NULL;
    CameraProperties::Properties* properties = NULL;

    android::AutoMutex lock(gCameraHalDeviceLock);

    CAMHAL_LOGI("camera_device open");

    if (name != NULL) {
        cameraid = atoi(name);
        num_cameras = gCameraProperties.camerasSupported();

        if(cameraid > num_cameras)
        {
            CAMHAL_LOGE("camera service provided cameraid out of bounds, "
                    "cameraid = %d, num supported = %d",
                    cameraid, num_cameras);
            rv = -EINVAL;
            goto fail;
        }

        if(gCamerasOpen >= MAX_SIMUL_CAMERAS_SUPPORTED)
        {
            CAMHAL_LOGE("maximum number of cameras already open");
            rv = -ENOMEM;
            goto fail;
        }

        camera_device = (ti_camera_device_t*)malloc(sizeof(*camera_device));
        if(!camera_device)
        {
            CAMHAL_LOGE("camera_device allocation fail");
            rv = -ENOMEM;
            goto fail;
        }

        camera_ops = (camera_device_ops_t*)malloc(sizeof(*camera_ops));
        if(!camera_ops)
        {
            CAMHAL_LOGE("camera_ops allocation fail");
            rv = -ENOMEM;
            goto fail;
        }

        memset(camera_device, 0, sizeof(*camera_device));
        memset(camera_ops, 0, sizeof(*camera_ops));

        camera_device->base.common.tag = HARDWARE_DEVICE_TAG;
        camera_device->base.common.version = 0;
        camera_device->base.common.module = (hw_module_t *)(module);
        camera_device->base.common.close = camera_device_close;
        camera_device->base.ops = camera_ops;

        camera_ops->set_preview_window = camera_set_preview_window;
        camera_ops->set_callbacks = camera_set_callbacks;
        camera_ops->enable_msg_type = camera_enable_msg_type;
        camera_ops->disable_msg_type = camera_disable_msg_type;
        camera_ops->msg_type_enabled = camera_msg_type_enabled;
        camera_ops->start_preview = camera_start_preview;
        camera_ops->stop_preview = camera_stop_preview;
        camera_ops->preview_enabled = camera_preview_enabled;
        camera_ops->store_meta_data_in_buffers = camera_store_meta_data_in_buffers;
        camera_ops->start_recording = camera_start_recording;
        camera_ops->stop_recording = camera_stop_recording;
        camera_ops->recording_enabled = camera_recording_enabled;
        camera_ops->release_recording_frame = camera_release_recording_frame;
        camera_ops->auto_focus = camera_auto_focus;
        camera_ops->cancel_auto_focus = camera_cancel_auto_focus;
        camera_ops->take_picture = camera_take_picture;
        camera_ops->cancel_picture = camera_cancel_picture;
        camera_ops->set_parameters = camera_set_parameters;
        camera_ops->get_parameters = camera_get_parameters;
        camera_ops->put_parameters = camera_put_parameters;
        camera_ops->send_command = camera_send_command;
        camera_ops->release = camera_release;
        camera_ops->dump = camera_dump;

        *device = &camera_device->base.common;

        // -------- TI specific stuff --------

        camera_device->cameraid = cameraid;

        if(gCameraProperties.getProperties(cameraid, &properties) < 0)
        {
            CAMHAL_LOGE("Couldn't get camera properties");
            rv = -ENOMEM;
            goto fail;
        }

        camera = new CameraHal(cameraid);

        if(!camera)
        {
            CAMHAL_LOGE("Couldn't create instance of CameraHal class");
            rv = -ENOMEM;
            goto fail;
        }

        if(properties && (camera->initialize(properties) != NO_ERROR))
        {
            CAMHAL_LOGE("Couldn't initialize camera instance");
            rv = -ENODEV;
            goto fail;
        }

        gCameraHals[cameraid] = camera;
        gCamerasOpen++;
    }

    return rv;

fail:
    if(camera_device) {
        free(camera_device);
        camera_device = NULL;
    }
    if(camera_ops) {
        free(camera_ops);
        camera_ops = NULL;
    }
    if(camera) {
        delete camera;
        camera = NULL;
    }
    *device = NULL;
    return rv;
}

int camera_get_number_of_cameras(void)
{
    int num_cameras = MAX_CAMERAS_SUPPORTED;

    // this going to be the first call from camera service
    // initialize camera properties here...
    if(gCameraProperties.initialize() != NO_ERROR)
    {
        CAMHAL_LOGEA("Unable to create or initialize CameraProperties");
        return NULL;
    }

    num_cameras = gCameraProperties.camerasSupported();

    return num_cameras;
}

int camera_get_camera_info(int camera_id, struct camera_info *info)
{
    int rv = 0;
    int face_value = CAMERA_FACING_BACK;
    int orientation = 0;
    const char *valstr = NULL;
    CameraProperties::Properties* properties = NULL;

    // this going to be the first call from camera service
    // initialize camera properties here...
    if(gCameraProperties.initialize() != NO_ERROR)
    {
        CAMHAL_LOGEA("Unable to create or initialize CameraProperties");
        rv = -EINVAL;
        goto end;
    }

    //Get camera properties for camera index
    if(gCameraProperties.getProperties(camera_id, &properties) < 0)
    {
        CAMHAL_LOGE("Couldn't get camera properties");
        rv = -EINVAL;
        goto end;
    }

    if(properties)
    {
        valstr = properties->get(CameraProperties::FACING_INDEX);
        if(valstr != NULL)
        {
            if (strcmp(valstr, TICameraParameters::FACING_FRONT) == 0)
            {
                face_value = CAMERA_FACING_FRONT;
            }
            else if (strcmp(valstr, TICameraParameters::FACING_BACK) == 0)
            {
                face_value = CAMERA_FACING_BACK;
            }
         }

         valstr = properties->get(CameraProperties::ORIENTATION_INDEX);
         if(valstr != NULL)
         {
             orientation = atoi(valstr);
         }
    }
    else
    {
        CAMHAL_LOGEB("getProperties() returned a NULL property set for Camera id %d", camera_id);
    }

    info->facing = face_value;
    info->orientation = orientation;

end:
    return rv;
}


} // namespace Camera
} // namespace Ti
