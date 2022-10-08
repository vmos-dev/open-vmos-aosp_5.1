#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <semaphore.h>
#include <pthread.h>

#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>

#include <camera/Camera.h>
#include <camera/ICamera.h>
#include <media/mediarecorder.h>

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <cutils/properties.h>
#include <camera/CameraParameters.h>
#include <system/audio.h>
#include <system/camera.h>

#include <cutils/memory.h>
#include <utils/Log.h>

#include <sys/wait.h>

#include "camera_test.h"

using namespace android;

int camera_index = 0;
int print_menu;
sp<Camera> camera;
sp<MediaRecorder> recorder;
sp<SurfaceComposerClient> client;
sp<SurfaceControl> surfaceControl;
sp<Surface> previewSurface;
CameraParameters params;
float compensation = 0.0;
double latitude = 0.0;
double longitude = 0.0;
double degree_by_step = 17.5609756;//..0975609756097;
double altitude = 0.0;
int awb_mode = 0;
int effects_mode = 0;
int scene_mode = 0;
int caf_mode = 0;
int vnf_mode = 0;
int vstab_mode = 0;

int tempBracketRange = 1;
int tempBracketIdx = 0;
int measurementIdx = 0;
int expBracketIdx = 0;
int AutoConvergenceModeIDX = 0;
int ManualConvergenceValuesIDX = 0;
int ManualConvergenceDefaultValueIDX = 2;
int gbceIDX = 0;
int glbceIDX = 0;
int rotation = 0;
bool reSizePreview = true;
bool hardwareActive = false;
bool recordingMode = false;
bool previewRunning = false;
int saturation = 0;
int zoomIDX = 0;
int videoCodecIDX = 0;
int audioCodecIDX = 0;
int outputFormatIDX = 0;
int contrast = 0;
int brightness = 0;
unsigned int burst = 0;
int sharpness = 0;
int iso_mode = 0;
int capture_mode = 0;
int exposure_mode = 0;
int ippIDX = 0;
int ippIDX_old = 0;
int previewFormat = 0;
int jpegQuality = 85;
int thumbQuality = 85;
int flashIdx = 0;
int fpsRangeIdx = 0;
timeval autofocus_start, picture_start;
char script_name[80];
int prevcnt = 0;
int videoFd = -1;
int elockidx = 0;
int wblockidx = 0;


char dir_path[80] = SDCARD_PATH;

const char *cameras[] = {"Primary Camera", "Secondary Camera 1", "Stereo Camera", "USB Camera", "Fake Camera"};
const char *measurement[] = {"disable", "enable"};
const char *expBracketing[] = {"disable", "enable"};
const char *expBracketingRange[] = {"", "-30,0,30,0,-30"};
const char *tempBracketing[] = {"disable", "enable"};
const char *faceDetection[] = {"disable", "enable"};
const char *lock[] = {"false", "true"};

#if defined(OMAP_ENHANCEMENT) && defined(TARGET_OMAP3)
const char *ipp_mode[] = { "off", "Chroma Suppression", "Edge Enhancement" };
#else
const char *ipp_mode[] = { "off", "ldc", "nsf", "ldc-nsf" };
#endif

const char *iso [] = { "auto", "100", "200", "400", "800", "1200", "1600"};

const char *effects [] = {
#if defined(OMAP_ENHANCEMENT) && defined(TARGET_OMAP3)
    "none",
    "mono",
    "negative",
    "solarize",
    "sepia",
    "whiteboard",
    "blackboard",
    "cool",
    "emboss"
#else
    "none",
    "mono",
    "negative",
    "solarize",
    "sepia",
    "vivid",
    "whiteboard",
    "blackboard",
    "cool",
    "emboss",
    "blackwhite",
    "aqua",
    "posterize"
#endif
};

const char CameraParameters::FLASH_MODE_OFF[] = "off";
const char CameraParameters::FLASH_MODE_AUTO[] = "auto";
const char CameraParameters::FLASH_MODE_ON[] = "on";
const char CameraParameters::FLASH_MODE_RED_EYE[] = "red-eye";
const char CameraParameters::FLASH_MODE_TORCH[] = "torch";

const char *flashModes[] = {
    "off",
    "auto",
    "on",
    "red-eye",
    "torch",
    "fill-in",
};

const char *caf [] = { "Off", "On" };
const char *vnf [] = { "Off", "On" };
const char *vstab [] = { "Off", "On" };


const char *scene [] = {
#if defined(OMAP_ENHANCEMENT) && defined(TARGET_OMAP3)
    "auto",
    "portrait",
    "landscape",
    "night",
    "night-portrait",
    "fireworks",
    "snow",
    "action",
#else
    "auto",
    "portrait",
    "landscape",
    "night",
    "night-portrait",
    "night-indoor",
    "fireworks",
    "sport",
    "cine",
    "beach",
    "snow",
    "mood",
    "closeup",
    "underwater",
    "document",
    "barcode",
    "oldfilm",
    "candlelight",
    "party",
    "steadyphoto",
    "sunset",
    "action",
    "theatre"
#endif
};
const char *strawb_mode[] = {
    "auto",
    "incandescent",
    "fluorescent",
    "daylight",
    "horizon",
    "shadow",
    "tungsten",
    "shade",
    "twilight",
    "warm-fluorescent",
    "facepriority",
    "sunset"
};

size_t length_cam =  ARRAY_SIZE(cameras);


preview_size previewSize [] = {
  { 0,   0,  "NULL"},
  { 128, 96, "SQCIF" },
  { 176, 144, "QCIF" },
  { 352, 288, "CIF" },
  { 320, 240, "QVGA" },
  { 352, 288, "CIF" },
  { 640, 480, "VGA" },
  { 720, 480, "NTSC" },
  { 720, 576, "PAL" },
  { 800, 480, "WVGA" },
  { 848, 480, "WVGA2"},
  { 864, 480, "WVGA3"},
  { 992, 560, "WVGA4"},
  { 1280, 720, "HD" },
  { 1920, 1080, "FULLHD"},
};

size_t length_previewSize =  ARRAY_SIZE(previewSize);

Vcapture_size VcaptureSize [] = {
  { 128, 96, "SQCIF" },
  { 176, 144, "QCIF" },
  { 352, 288, "CIF" },
  { 320, 240, "QVGA" },
  { 640, 480, "VGA" },
  { 704, 480, "TVNTSC" },
  { 704, 576, "TVPAL" },
  { 720, 480, "D1NTSC" },
  { 720, 576, "D1PAL" },
  { 800, 480, "WVGA" },
  #if defined(OMAP_ENHANCEMENT) && defined(TARGET_OMAP3)
  { 848, 480, "WVGA2"},
  { 864, 480, "WVGA3"},
  { 992, 560, "WVGA4"},
  #endif
  { 1280, 720, "HD" },
  { 1920, 1080, "FULLHD"},
};

size_t lenght_Vcapture_size = ARRAY_SIZE(VcaptureSize);

capture_Size captureSize[] = {
  {  320, 240,  "QVGA" },
  {  640, 480,  "VGA" },
  {  800, 600,  "SVGA" },
  { 1152, 864,  "1MP" },
  { 1280, 1024, "1.3MP" },
  { 1600, 1200,  "2MP" },
  { 2048, 1536,  "3MP" },
  { 2592, 1944,  "5MP" },
  { 2608, 1960,  "5MP" },
  { 3264, 2448,  "8MP" },
  { 3648, 2736, "10MP"},
  { 4032, 3024, "12MP"},
};

size_t length_capture_Size = ARRAY_SIZE(captureSize);


outformat outputFormat[] = {
        { OUTPUT_FORMAT_THREE_GPP, "3gp" },
        { OUTPUT_FORMAT_MPEG_4, "mp4" },
    };

size_t length_outformat = ARRAY_SIZE(outputFormat);

video_Codecs videoCodecs[] = {
  { VIDEO_ENCODER_H263, "H263" },
  { VIDEO_ENCODER_H264, "H264" },
  { VIDEO_ENCODER_MPEG_4_SP, "MPEG4"}
};

size_t length_video_Codecs = ARRAY_SIZE(videoCodecs);

audio_Codecs audioCodecs[] = {
  { AUDIO_ENCODER_AMR_NB, "AMR_NB" },
  { AUDIO_ENCODER_AMR_WB, "AMR_WB" },
  { AUDIO_ENCODER_AAC, "AAC" },
  { AUDIO_ENCODER_HE_AAC, "AAC+" },
  { AUDIO_ENCODER_LIST_END, "disabled"},
};

size_t length_audio_Codecs = ARRAY_SIZE(audioCodecs);

V_bitRate VbitRate[] = {
  {    64000, "64K"  },
  {   128000, "128K" },
  {   192000, "192K" },
  {   240000, "240K" },
  {   320000, "320K" },
  {   360000, "360K" },
  {   384000, "384K" },
  {   420000, "420K" },
  {   768000, "768K" },
  {  1000000, "1M"   },
  {  1500000, "1.5M" },
  {  2000000, "2M"   },
  {  4000000, "4M"   },
  {  6000000, "6M"   },
  {  8000000, "8M"   },
  { 10000000, "10M"  },
};

size_t length_V_bitRate = ARRAY_SIZE(VbitRate);

Zoom zoom[] = {
  { 0,  "1x"  },
  { 12, "1.5x"},
  { 20, "2x"  },
  { 27, "2.5x"},
  { 32, "3x"  },
  { 36, "3.5x"},
  { 40, "4x"  },
  { 60, "8x"  },
};

size_t length_Zoom = ARRAY_SIZE(zoom);

fps_ranges fpsRanges[] = {
  { "5000,30000", "[5:30]" },
  { "5000,10000", "[5:10]" },
  { "5000,15000", "[5:15]" },
  { "5000,20000", "[5:20]" },
};

size_t length_fps_ranges = ARRAY_SIZE(fpsRanges);

fpsConst_Ranges fpsConstRanges[] = {
  { "5000,5000", "[5:5]", 5 },
  { "10000,10000", "[10:10]", 10 },
  { "15000,15000", "[15:15]", 15 },
  { "20000,20000", "[20:20]", 20 },
  { "25000,25000", "[25:25]", 25 },
  { "30000,30000", "[30:30]", 30 },
};

size_t length_fpsConst_Ranges = ARRAY_SIZE(fpsConstRanges);

fpsConst_RangesSec fpsConstRangesSec[] = {
  { "5000,5000", "[5:5]", 5 },
  { "10000,10000", "[10:10]", 10 },
  { "15000,15000", "[15:15]", 15 },
  { "20000,20000", "[20:20]", 20 },
  { "25000,25000", "[25:25]", 25 },
  { "27000,27000", "[27:27]", 27 },
};

size_t length_fpsConst_RangesSec = ARRAY_SIZE(fpsConstRangesSec);

const char *antibanding[] = {
    "off",
    "auto",
    "50hz",
    "60hz",
};
int antibanding_mode = 0;
const char *focus[] = {
    "auto",
    "infinity",
    "macro",
    "continuous-video",
    "extended",
    "portrait",
};
int focus_mode = 0;
pixel_format pixelformat[] = {
  { HAL_PIXEL_FORMAT_YCbCr_422_I, CameraParameters::PIXEL_FORMAT_YUV422I },
  { HAL_PIXEL_FORMAT_YCrCb_420_SP, CameraParameters::PIXEL_FORMAT_YUV420SP },
  { HAL_PIXEL_FORMAT_RGB_565, CameraParameters::PIXEL_FORMAT_RGB565 },
  { -1, CameraParameters::PIXEL_FORMAT_JPEG },
  { -1, "raw" },
  };

const char *codingformat[] = {"yuv422i-yuyv", "yuv420sp", "rgb565", "jpeg", "raw", "jps", "mpo", "raw+jpeg", "raw+mpo"};
const char *gbce[] = {"disable", "enable"};
int pictureFormat = 3; // jpeg
const char *exposure[] = {"auto", "macro", "portrait", "landscape", "sports", "night", "night-portrait", "backlighting", "manual"};
const char *capture[] = { "high-performance", "high-quality", "video-mode" };
const char *autoconvergencemode[] = { "mode-disable", "mode-frame", "mode-center", "mode-fft", "mode-manual" };
const char *manualconvergencevalues[] = { "-100", "-50", "-30", "-25", "0", "25", "50", "100" };

const struct {
    int fps;
} frameRate[] = {
    {0},
    {5},
    {10},
    {15},
    {20},
    {25},
    {30}
};

int thumbSizeIDX =  3;
int previewSizeIDX = ARRAY_SIZE(previewSize) - 1;
int captureSizeIDX = ARRAY_SIZE(captureSize) - 1;
int frameRateIDX = ARRAY_SIZE(fpsConstRanges) - 1;
int frameRateIDXSec = ARRAY_SIZE(fpsConstRangesSec) - 1;
int VcaptureSizeIDX = ARRAY_SIZE(VcaptureSize) - 1;
int VbitRateIDX = ARRAY_SIZE(VbitRate) - 1;

static unsigned int recording_counter = 1;

int dump_preview = 0;
int bufferStarvationTest = 0;
bool showfps = false;

const char *metering[] = {
    "center",
    "average",
};
int meter_mode = 0;
bool bLogSysLinkTrace = true;
bool stressTest = false;
bool stopScript = false;
int restartCount = 0;

static const String16 processName("camera_test");

/** Calculate delay from a reference time */
unsigned long long timeval_delay(const timeval *ref) {
    unsigned long long st, end, delay;
    timeval current_time;

    gettimeofday(&current_time, 0);

    st = ref->tv_sec * 1000000 + ref->tv_usec;
    end = current_time.tv_sec * 1000000 + current_time.tv_usec;
    delay = end - st;

    return delay;
}

/** Callback for takePicture() */
void my_raw_callback(const sp<IMemory>& mem) {

    static int      counter = 1;
    unsigned char   *buff = NULL;
    int             size;
    int             fd = -1;
    char            fn[256];

    LOG_FUNCTION_NAME;

    if (mem == NULL)
        goto out;

    //Start preview after capture.
    camera->startPreview();

    fn[0] = 0;
    sprintf(fn, "/sdcard/img%03d.raw", counter);
    fd = open(fn, O_CREAT | O_WRONLY | O_TRUNC, 0777);

    if (fd < 0)
        goto out;

    size = mem->size();

    if (size <= 0)
        goto out;

    buff = (unsigned char *)mem->pointer();

    if (!buff)
        goto out;

    if (size != write(fd, buff, size))
        printf("Bad Write int a %s error (%d)%s\n", fn, errno, strerror(errno));

    counter++;
    printf("%s: buffer=%08X, size=%d stored at %s\n",
           __FUNCTION__, (int)buff, size, fn);

out:

    if (fd >= 0)
        close(fd);

    LOG_FUNCTION_NAME_EXIT;
}

void saveFile(const sp<IMemory>& mem) {
    static int      counter = 1;
    unsigned char   *buff = NULL;
    int             size;
    int             fd = -1;
    char            fn[256];

    LOG_FUNCTION_NAME;

    if (mem == NULL)
        goto out;

    fn[0] = 0;
    sprintf(fn, "/sdcard/preview%03d.yuv", counter);
    fd = open(fn, O_CREAT | O_WRONLY | O_TRUNC, 0777);
    if(fd < 0) {
        ALOGE("Unable to open file %s: %s", fn, strerror(fd));
        goto out;
    }

    size = mem->size();
    if (size <= 0) {
        ALOGE("IMemory object is of zero size");
        goto out;
    }

    buff = (unsigned char *)mem->pointer();
    if (!buff) {
        ALOGE("Buffer pointer is invalid");
        goto out;
    }

    if (size != write(fd, buff, size))
        printf("Bad Write int a %s error (%d)%s\n", fn, errno, strerror(errno));

    counter++;
    printf("%s: buffer=%08X, size=%d\n",
           __FUNCTION__, (int)buff, size);

out:

    if (fd >= 0)
        close(fd);

    LOG_FUNCTION_NAME_EXIT;
}


void debugShowFPS()
{
    static int mFrameCount = 0;
    static int mLastFrameCount = 0;
    static nsecs_t mLastFpsTime = 0;
    static float mFps = 0;
    mFrameCount++;
    if ( ( mFrameCount % 30 ) == 0 ) {
        nsecs_t now = systemTime();
        nsecs_t diff = now - mLastFpsTime;
        mFps =  ((mFrameCount - mLastFrameCount) * float(s2ns(1))) / diff;
        mLastFpsTime = now;
        mLastFrameCount = mFrameCount;
        printf("####### [%d] Frames, %f FPS", mFrameCount, mFps);
    }
}

/** Callback for startPreview() */
void my_preview_callback(const sp<IMemory>& mem) {

    printf("PREVIEW Callback 0x%x", ( unsigned int ) mem->pointer());
    if (dump_preview) {

        if(prevcnt==50)
        saveFile(mem);

        prevcnt++;

        uint8_t *ptr = (uint8_t*) mem->pointer();

        printf("PRV_CB: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x", ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5], ptr[6], ptr[7], ptr[8], ptr[9]);

    }

    debugShowFPS();
}

/** Callback for takePicture() */
void my_jpeg_callback(const sp<IMemory>& mem) {
    static int  counter = 1;
    unsigned char   *buff = NULL;
    int     size;
    int     fd = -1;
    char        fn[256];

    LOG_FUNCTION_NAME;

    //Start preview after capture.
    camera->startPreview();

    if (mem == NULL)
        goto out;

    fn[0] = 0;
    sprintf(fn, "%s/img%03d.jpg", dir_path,counter);
    fd = open(fn, O_CREAT | O_WRONLY | O_TRUNC, 0777);

    if(fd < 0) {
        ALOGE("Unable to open file %s: %s", fn, strerror(fd));
        goto out;
    }

    size = mem->size();
    if (size <= 0) {
        ALOGE("IMemory object is of zero size");
        goto out;
    }

    buff = (unsigned char *)mem->pointer();
    if (!buff) {
        ALOGE("Buffer pointer is invalid");
        goto out;
    }

    if (size != write(fd, buff, size))
        printf("Bad Write int a %s error (%d)%s\n", fn, errno, strerror(errno));

    counter++;
    printf("%s: buffer=%08X, size=%d stored at %s\n",
           __FUNCTION__, (int)buff, size, fn);

out:

    if (fd >= 0)
        close(fd);

    LOG_FUNCTION_NAME_EXIT;
}

void my_face_callback(camera_frame_metadata_t *metadata) {
    int idx;

    if ( NULL == metadata ) {
        return;
    }

    for ( idx = 0 ; idx < metadata->number_of_faces ; idx++ ) {
        printf("Face %d at %d,%d %d,%d \n",
               idx,
               metadata->faces[idx].rect[0],
               metadata->faces[idx].rect[1],
               metadata->faces[idx].rect[2],
               metadata->faces[idx].rect[3]);
    }

}

void CameraHandler::notify(int32_t msgType, int32_t ext1, int32_t ext2) {

    printf("Notify cb: %d %d %d\n", msgType, ext1, ext2);

    if ( msgType & CAMERA_MSG_FOCUS )
        printf("AutoFocus %s in %llu us\n", (ext1) ? "OK" : "FAIL", timeval_delay(&autofocus_start));

    if ( msgType & CAMERA_MSG_SHUTTER )
        printf("Shutter done in %llu us\n", timeval_delay(&picture_start));

    if ( msgType & CAMERA_MSG_ERROR && (ext1 == 1))
      {
        printf("Camera Test CAMERA_MSG_ERROR.....\n");
        if (stressTest)
          {
            printf("Camera Test Notified of Error Restarting.....\n");
            stopScript = true;
          }
        else
          {
            printf("Camera Test Notified of Error Stopping.....\n");
            stopScript =false;
            stopPreview();

            if (recordingMode)
              {
                stopRecording();
                closeRecorder();
                recordingMode = false;
              }
          }
      }
}

void CameraHandler::postData(int32_t msgType,
                             const sp<IMemory>& dataPtr,
                             camera_frame_metadata_t *metadata) {
    printf("Data cb: %d\n", msgType);

    if ( msgType & CAMERA_MSG_PREVIEW_FRAME )
        my_preview_callback(dataPtr);

    if ( msgType & CAMERA_MSG_RAW_IMAGE ) {
        printf("RAW done in %llu us\n", timeval_delay(&picture_start));
        my_raw_callback(dataPtr);
    }

    if (msgType & CAMERA_MSG_POSTVIEW_FRAME) {
        printf("Postview frame %llu us\n", timeval_delay(&picture_start));
    }

    if (msgType & CAMERA_MSG_COMPRESSED_IMAGE ) {
        printf("JPEG done in %llu us\n", timeval_delay(&picture_start));
        my_jpeg_callback(dataPtr);
    }

    if ( ( msgType & CAMERA_MSG_PREVIEW_METADATA ) &&
         ( NULL != metadata ) ) {
        printf("Face detected %d \n", metadata->number_of_faces);
        my_face_callback(metadata);
    }
}

void CameraHandler::postDataTimestamp(nsecs_t timestamp, int32_t msgType, const sp<IMemory>& dataPtr)

{
    printf("Recording cb: %d %lld %p\n", msgType, timestamp, dataPtr.get());

    static uint32_t count = 0;

    //if(count==100)
    //saveFile(dataPtr);

    count++;

    uint8_t *ptr = (uint8_t*) dataPtr->pointer();

    printf("VID_CB: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x", ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5], ptr[6], ptr[7], ptr[8], ptr[9]);

    camera->releaseRecordingFrame(dataPtr);
}

int createPreviewSurface(unsigned int width, unsigned int height, int32_t pixFormat) {
    unsigned int previewWidth, previewHeight;

    if ( MAX_PREVIEW_SURFACE_WIDTH < width ) {
        previewWidth = MAX_PREVIEW_SURFACE_WIDTH;
    } else {
        previewWidth = width;
    }

    if ( MAX_PREVIEW_SURFACE_HEIGHT < height ) {
        previewHeight = MAX_PREVIEW_SURFACE_HEIGHT;
    } else {
        previewHeight = height;
    }

    client = new SurfaceComposerClient();

    if ( NULL == client.get() ) {
        printf("Unable to establish connection to Surface Composer \n");

        return -1;
    }

    surfaceControl = client->createSurface(String8("camera_test_menu"),
                                           previewWidth,
                                           previewHeight,
                                           pixFormat, 0);

    previewSurface = surfaceControl->getSurface();

    client->openGlobalTransaction();
    surfaceControl->setLayer(0x7fffffff);
    surfaceControl->setPosition(0, 0);
    surfaceControl->setSize(previewWidth, previewHeight);
    surfaceControl->show();
    client->closeGlobalTransaction();

    return 0;
}

void printSupportedParams()
{
    printf("\n\r\tSupported Cameras: %s", params.get("camera-indexes"));
    printf("\n\r\tSupported Picture Sizes: %s", params.get(CameraParameters::KEY_SUPPORTED_PICTURE_SIZES));
    printf("\n\r\tSupported Picture Formats: %s", params.get(CameraParameters::KEY_SUPPORTED_PICTURE_FORMATS));
    printf("\n\r\tSupported Preview Sizes: %s", params.get(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES));
    printf("\n\r\tSupported Preview Formats: %s", params.get(CameraParameters::KEY_SUPPORTED_PREVIEW_FORMATS));
    printf("\n\r\tSupported Preview Frame Rates: %s", params.get(CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES));
    printf("\n\r\tSupported Thumbnail Sizes: %s", params.get(CameraParameters::KEY_SUPPORTED_JPEG_THUMBNAIL_SIZES));
    printf("\n\r\tSupported Whitebalance Modes: %s", params.get(CameraParameters::KEY_SUPPORTED_WHITE_BALANCE));
    printf("\n\r\tSupported Effects: %s", params.get(CameraParameters::KEY_SUPPORTED_EFFECTS));
    printf("\n\r\tSupported Scene Modes: %s", params.get(CameraParameters::KEY_SUPPORTED_SCENE_MODES));
    printf("\n\r\tSupported Focus Modes: %s", params.get(CameraParameters::KEY_SUPPORTED_FOCUS_MODES));
    printf("\n\r\tSupported Antibanding Options: %s", params.get(CameraParameters::KEY_SUPPORTED_ANTIBANDING));
    printf("\n\r\tSupported Flash Modes: %s", params.get(CameraParameters::KEY_SUPPORTED_FLASH_MODES));
    printf("\n\r\tSupported Focus Areas: %d", params.getInt(CameraParameters::KEY_MAX_NUM_FOCUS_AREAS));

    if ( NULL != params.get(CameraParameters::KEY_FOCUS_DISTANCES) ) {
        printf("\n\r\tFocus Distances: %s \n", params.get(CameraParameters::KEY_FOCUS_DISTANCES));
    }

    return;
}


int destroyPreviewSurface() {

    if ( NULL != previewSurface.get() ) {
        previewSurface.clear();
    }

    if ( NULL != surfaceControl.get() ) {
        surfaceControl->clear();
        surfaceControl.clear();
    }

    if ( NULL != client.get() ) {
        client->dispose();
        client.clear();
    }

    return 0;
}

int openRecorder() {
    recorder = new MediaRecorder();

    if ( NULL == recorder.get() ) {
        printf("Error while creating MediaRecorder\n");

        return -1;
    }

    return 0;
}

int closeRecorder() {
    if ( NULL == recorder.get() ) {
        printf("invalid recorder reference\n");

        return -1;
    }

    if ( recorder->init() < 0 ) {
        printf("recorder failed to initialize\n");

        return -1;
    }

    if ( recorder->close() < 0 ) {
        printf("recorder failed to close\n");

        return -1;
    }

    if ( recorder->release() < 0 ) {
        printf("error while releasing recorder\n");

        return -1;
    }

    recorder.clear();

    return 0;
}

int configureRecorder() {

    char videoFile[256],vbit_string[50];
    videoFd = -1;

    if ( ( NULL == recorder.get() ) || ( NULL == camera.get() ) ) {
        printf("invalid recorder and/or camera references\n");

        return -1;
    }

    camera->unlock();

    sprintf(vbit_string,"video-param-encoding-bitrate=%u", VbitRate[VbitRateIDX].bit_rate);
    String8 bit_rate(vbit_string);
    if ( recorder->setParameters(bit_rate) < 0 ) {
        printf("error while configuring bit rate\n");

        return -1;
    }

    if ( recorder->setCamera(camera->remote(), camera->getRecordingProxy()) < 0 ) {
        printf("error while setting the camera\n");

        return -1;
    }

    if ( recorder->setVideoSource(VIDEO_SOURCE_CAMERA) < 0 ) {
        printf("error while configuring camera video source\n");

        return -1;
    }


    if ( AUDIO_ENCODER_LIST_END != audioCodecs[audioCodecIDX].type ) {
        if ( recorder->setAudioSource(AUDIO_SOURCE_DEFAULT) < 0 ) {
            printf("error while configuring camera audio source\n");

            return -1;
        }
    }

    if ( recorder->setOutputFormat(outputFormat[outputFormatIDX].type) < 0 ) {
        printf("error while configuring output format\n");

        return -1;
    }

    if(mkdir("/mnt/sdcard/videos",0777) == -1)
         printf("\n Directory --videos-- was not created \n");
    sprintf(videoFile, "/mnt/sdcard/videos/video%d.%s", recording_counter,outputFormat[outputFormatIDX].desc);

    videoFd = open(videoFile, O_CREAT | O_RDWR);

    if(videoFd < 0){
        printf("Error while creating video filename\n");

        return -1;
    }

    if ( recorder->setOutputFile(videoFd, 0, 0) < 0 ) {
        printf("error while configuring video filename\n");

        return -1;
    }

    recording_counter++;

    if (camera_index == 0) {
        if ( recorder->setVideoFrameRate(fpsConstRanges[frameRateIDX].constFramerate) < 0 ) {
            printf("error while configuring video framerate\n");
            return -1;
        }
    }
    else  {
        if ( recorder->setVideoFrameRate(fpsConstRangesSec[frameRateIDXSec].constFramerate) < 0 ) {
            printf("error while configuring video framerate\n");
            return -1;
        }
    }

    if ( recorder->setVideoSize(VcaptureSize[VcaptureSizeIDX].width, VcaptureSize[VcaptureSizeIDX].height) < 0 ) {
        printf("error while configuring video size\n");

        return -1;
    }

    if ( recorder->setVideoEncoder(videoCodecs[videoCodecIDX].type) < 0 ) {
        printf("error while configuring video codec\n");

        return -1;
    }

    if ( AUDIO_ENCODER_LIST_END != audioCodecs[audioCodecIDX].type ) {
        if ( recorder->setAudioEncoder(audioCodecs[audioCodecIDX].type) < 0 ) {
            printf("error while configuring audio codec\n");

            return -1;
        }
    }

    if ( recorder->setPreviewSurface( surfaceControl->getSurface()->getIGraphicBufferProducer() ) < 0 ) {
        printf("error while configuring preview surface\n");

        return -1;
    }

    return 0;
}

int startRecording() {
    if ( ( NULL == recorder.get() ) || ( NULL == camera.get() ) ) {
        printf("invalid recorder and/or camera references\n");

        return -1;
    }

    camera->unlock();

    if ( recorder->prepare() < 0 ) {
        printf("recorder prepare failed\n");

        return -1;
    }

    if ( recorder->start() < 0 ) {
        printf("recorder start failed\n");

        return -1;
    }

    return 0;
}

int stopRecording() {
    if ( NULL == recorder.get() ) {
        printf("invalid recorder reference\n");

        return -1;
    }

    if ( recorder->stop() < 0 ) {
        printf("recorder failed to stop\n");

        return -1;
    }

    if ( 0 < videoFd ) {
        close(videoFd);
    }

    return 0;
}

int openCamera() {
    printf("openCamera(camera_index=%d)\n", camera_index);
    camera = Camera::connect(camera_index,
            processName,
            Camera::USE_CALLING_UID);

    if ( NULL == camera.get() ) {
        printf("Unable to connect to CameraService\n");
        printf("Retrying... \n");
        sleep(1);
        camera = Camera::connect(camera_index,
                processName,
                Camera::USE_CALLING_UID);

        if ( NULL == camera.get() ) {
            printf("Giving up!! \n");
            return -1;
        }
    }

    params = camera->getParameters();
    camera->setParameters(params.flatten());

    camera->setListener(new CameraHandler());

    hardwareActive = true;

    return 0;
}

int closeCamera() {
    if ( NULL == camera.get() ) {
        printf("invalid camera reference\n");

        return -1;
    }

    camera->disconnect();
    camera.clear();

    hardwareActive = false;

    return 0;
}

int startPreview() {
    int previewWidth, previewHeight;
    if (reSizePreview) {

        if(recordingMode)
        {
            previewWidth = VcaptureSize[VcaptureSizeIDX].width;
            previewHeight = VcaptureSize[VcaptureSizeIDX].height;
        }else
        {
            previewWidth = previewSize[previewSizeIDX].width;
            previewHeight = previewSize[previewSizeIDX].height;
        }

        if ( createPreviewSurface(previewWidth,
                                  previewHeight,
                                  pixelformat[previewFormat].pixelFormatDesc) < 0 ) {
            printf("Error while creating preview surface\n");
            return -1;
        }

        if ( !hardwareActive ) {
            openCamera();
        }

        params.setPreviewSize(previewWidth, previewHeight);
        params.setPictureSize(captureSize[captureSizeIDX].width, captureSize[captureSizeIDX].height);

        camera->setParameters(params.flatten());
        camera->setPreviewTexture(previewSurface->getIGraphicBufferProducer());

        if(!hardwareActive) prevcnt = 0;

        camera->startPreview();

        previewRunning = true;
        reSizePreview = false;

    }

    return 0;
}

void stopPreview() {
    if ( hardwareActive ) {
        camera->stopPreview();

        destroyPreviewSurface();

        previewRunning  = false;
        reSizePreview = true;
        closeCamera();
    }
}

void initDefaults() {
    camera_index = 0;
    antibanding_mode = 0;
    focus_mode = 0;
    fpsRangeIdx = 0;
    previewSizeIDX = 1;  /* Default resolution set to WVGA */
    captureSizeIDX = 3;  /* Default capture resolution is 8MP */
    frameRateIDX = ARRAY_SIZE(fpsConstRanges) - 1;      /* Default frame rate is 30 FPS */
#if defined(OMAP_ENHANCEMENT) && defined(TARGET_OMAP3)
    VcaptureSizeIDX = ARRAY_SIZE(VcaptureSize) - 6;/* Default video record is WVGA */
#else
    VcaptureSizeIDX = ARRAY_SIZE(VcaptureSize) - 2;/* Default video record is WVGA */
#endif
    VbitRateIDX = ARRAY_SIZE(VbitRate) - 4;        /*Default video bit rate is 4M */
    thumbSizeIDX = 0;
    compensation = 0.0;
    awb_mode = 0;
    effects_mode = 0;
    scene_mode = 0;
    caf_mode = 0;
    vnf_mode = 0;
    vstab_mode = 0;
    expBracketIdx = 0;
    flashIdx = 0;
    rotation = 0;
    zoomIDX = 0;
    videoCodecIDX = 0;
    gbceIDX = 0;
    glbceIDX = 0;
#ifdef TARGET_OMAP4
    ///Temporary fix until OMAP3 and OMAP4 3A values are synced
    contrast = 90;
    brightness = 50;
    sharpness = 0;
    saturation = 50;
#else
    contrast = 100;
    brightness = 100;
    sharpness = 0;
    saturation = 100;
#endif
    iso_mode = 0;
    capture_mode = 0;
    exposure_mode = 0;
    ippIDX = 0;//set the ipp to ldc-nsf as the capture mode is set to HQ by default
    ippIDX_old = ippIDX;
    jpegQuality = 85;
    bufferStarvationTest = 0;
    meter_mode = 0;
    previewFormat = 1;
    pictureFormat = 3; // jpeg
    params.setPreviewSize(previewSize[previewSizeIDX].width, previewSize[previewSizeIDX].height);
    params.setPictureSize(captureSize[captureSizeIDX].width, captureSize[captureSizeIDX].height);
    params.set(CameraParameters::KEY_ROTATION, rotation);
    params.set(KEY_COMPENSATION, (int) (compensation * 10));
    params.set(params.KEY_WHITE_BALANCE, strawb_mode[awb_mode]);
    params.set(KEY_MODE, (capture[capture_mode]));
    params.set(params.KEY_SCENE_MODE, scene[scene_mode]);
    params.set(KEY_CAF, caf_mode);
    params.set(KEY_ISO, iso_mode);
    params.set(KEY_GBCE, gbce[gbceIDX]);
    params.set(KEY_GLBCE, gbce[glbceIDX]);
    params.set(KEY_SHARPNESS, sharpness);
    params.set(KEY_CONTRAST, contrast);
    params.set(CameraParameters::KEY_ZOOM, zoom[zoomIDX].idx);
    params.set(KEY_EXPOSURE, exposure[exposure_mode]);
    params.set(KEY_BRIGHTNESS, brightness);
    params.set(KEY_SATURATION, saturation);
    params.set(params.KEY_EFFECT, effects[effects_mode]);
    params.setPreviewFrameRate(frameRate[ARRAY_SIZE(frameRate) - 1].fps);
    params.set(params.KEY_ANTIBANDING, antibanding[antibanding_mode]);
    params.set(params.KEY_FOCUS_MODE, focus[focus_mode]);
    params.set(KEY_IPP, ipp_mode[ippIDX]);
    params.set(CameraParameters::KEY_JPEG_QUALITY, jpegQuality);
    params.setPreviewFormat(pixelformat[previewFormat].pixformat);
    params.setPictureFormat(codingformat[pictureFormat]);
    params.set(KEY_BUFF_STARV, bufferStarvationTest); //enable buffer starvation
    params.set(KEY_METERING_MODE, metering[meter_mode]);
    params.set(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH, previewSize[thumbSizeIDX].width);
    params.set(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, previewSize[thumbSizeIDX].height);
    ManualConvergenceValuesIDX = ManualConvergenceDefaultValueIDX;
    params.set(KEY_MANUALCONVERGENCE_VALUES, manualconvergencevalues[ManualConvergenceValuesIDX]);
    params.set(KEY_S3D2D_PREVIEW_MODE, "off");
    params.set(KEY_STEREO_CAMERA, "false");
    params.set(KEY_EXIF_MODEL, MODEL);
    params.set(KEY_EXIF_MAKE, MAKE);
}

int menu_gps() {
    char ch;
    char coord_str[100];

    if (print_menu) {
        printf("\n\n== GPS MENU ============================\n\n");
        printf("   e. Latitude:       %.7lf\n", latitude);
        printf("   d. Longitude:      %.7lf\n", longitude);
        printf("   c. Altitude:       %.7lf\n", altitude);
        printf("\n");
        printf("   q. Return to main menu\n");
        printf("\n");
        printf("   Choice: ");
    }

    ch = getchar();
    printf("%c", ch);

    print_menu = 1;

    switch (ch) {

        case 'e':
            latitude += degree_by_step;

            if (latitude > 90.0) {
                latitude -= 180.0;
            }

            snprintf(coord_str, 7, "%.7lf", latitude);
            params.set(params.KEY_GPS_LATITUDE, coord_str);

            if ( hardwareActive )
                camera->setParameters(params.flatten());

            break;

        case 'd':
            longitude += degree_by_step;

            if (longitude > 180.0) {
                longitude -= 360.0;
            }

            snprintf(coord_str, 7, "%.7lf", longitude);
            params.set(params.KEY_GPS_LONGITUDE, coord_str);

            if ( hardwareActive )
                camera->setParameters(params.flatten());

            break;

        case 'c':
            altitude += 12345.67890123456789;

            if (altitude > 100000.0) {
                altitude -= 200000.0;
            }

            snprintf(coord_str, 100, "%.20lf", altitude);
            params.set(params.KEY_GPS_ALTITUDE, coord_str);

            if ( hardwareActive )
                camera->setParameters(params.flatten());

            break;

        case 'Q':
        case 'q':
            return -1;

        default:
            print_menu = 0;
            break;
    }

    return 0;
}

int functional_menu() {
    char ch;

    if (print_menu) {

        printf("\n\n=========== FUNCTIONAL TEST MENU ===================\n\n");

        printf(" \n\nSTART / STOP / GENERAL SERVICES \n");
        printf(" -----------------------------\n");
        printf("   A  Select Camera %s\n", cameras[camera_index]);
        printf("   [. Resume Preview after capture\n");
        printf("   0. Reset to defaults\n");
        printf("   q. Quit\n");
        printf("   @. Disconnect and Reconnect to CameraService \n");
        printf("   /. Enable/Disable showfps: %s\n", ((showfps)? "Enabled":"Disabled"));
        printf("   a. GEO tagging settings menu\n");
        printf("   E.  Camera Capability Dump");


        printf(" \n\n PREVIEW SUB MENU \n");
        printf(" -----------------------------\n");
        printf("   1. Start Preview\n");
        printf("   2. Stop Preview\n");
        printf("   ~. Preview format %s\n", pixelformat[previewFormat].pixformat);
#if defined(OMAP_ENHANCEMENT) && defined(TARGET_OMAP3)
        printf("   4. Preview size:   %4d x %4d - %s\n",previewSize[previewSizeIDX].width,  previewSize[previewSizeIDX].height, previewSize[previewSizeIDX].desc);
#else
        printf("   4. Preview size:   %4d x %4d - %s\n",previewSize[previewSizeIDX].width, camera_index == 2 ? previewSize[previewSizeIDX].height*2 : previewSize[previewSizeIDX].height, previewSize[previewSizeIDX].desc);
#endif
        printf("   R. Preview framerate range: %s\n", fpsRanges[fpsRangeIdx].rangeDescription);
        printf("   &. Dump a preview frame\n");
        printf("   _. Auto Convergence mode: %s\n", autoconvergencemode[AutoConvergenceModeIDX]);
        printf("   ^. Manual Convergence Value: %s\n", manualconvergencevalues[ManualConvergenceValuesIDX]);
        printf("   {. 2D Preview in 3D Stereo Mode: %s\n", params.get(KEY_S3D2D_PREVIEW_MODE));

        printf(" \n\n IMAGE CAPTURE SUB MENU \n");
        printf(" -----------------------------\n");
        printf("   p. Take picture/Full Press\n");
        printf("   H. Exposure Bracketing: %s\n", expBracketing[expBracketIdx]);
        printf("   U. Temporal Bracketing:   %s\n", tempBracketing[tempBracketIdx]);
        printf("   W. Temporal Bracketing Range: [-%d;+%d]\n", tempBracketRange, tempBracketRange);
        printf("   $. Picture Format: %s\n", codingformat[pictureFormat]);
        printf("   3. Picture Rotation:       %3d degree\n", rotation );
        printf("   5. Picture size:   %4d x %4d - %s\n",captureSize[captureSizeIDX].width, captureSize[captureSizeIDX].height,              captureSize[captureSizeIDX].name);
        printf("   i. ISO mode:       %s\n", iso[iso_mode]);
        printf("   u. Capture Mode:   %s\n", capture[capture_mode]);
        printf("   k. IPP Mode:       %s\n", ipp_mode[ippIDX]);
        printf("   K. GBCE: %s\n", gbce[gbceIDX]);
        printf("   O. GLBCE %s\n", gbce[glbceIDX]);
        printf("   o. Jpeg Quality:   %d\n", jpegQuality);
        printf("   #. Burst Images:  %3d\n", burst);
        printf("   :. Thumbnail Size:  %4d x %4d - %s\n",previewSize[thumbSizeIDX].width, previewSize[thumbSizeIDX].height, previewSize[thumbSizeIDX].desc);
        printf("   ': Thumbnail Quality %d\n", thumbQuality);

        printf(" \n\n VIDEO CAPTURE SUB MENU \n");
        printf(" -----------------------------\n");

        printf("   6. Start Video Recording\n");
        printf("   2. Stop Recording\n");
        printf("   l. Video Capture resolution:   %4d x %4d - %s\n",VcaptureSize[VcaptureSizeIDX].width,VcaptureSize[VcaptureSizeIDX].height, VcaptureSize[VcaptureSizeIDX].desc);
        printf("   ]. Video Bit rate :  %s\n", VbitRate[VbitRateIDX].desc);
        printf("   9. Video Codec:    %s\n", videoCodecs[videoCodecIDX].desc);
        printf("   D. Audio Codec:    %s\n", audioCodecs[audioCodecIDX].desc);
        printf("   v. Output Format:  %s\n", outputFormat[outputFormatIDX].desc);

        if  (camera_index == 1) {
            printf("   r. Framerate:     %d\n", fpsConstRangesSec[frameRateIDXSec].constFramerate);
        }
        else {
            printf("   r. Framerate:     %d\n", fpsConstRanges[frameRateIDX].constFramerate);
        }
        printf("   *. Start Video Recording dump ( 1 raw frame ) \n");
        printf("   B  VNF              %s \n", vnf[vnf_mode]);
        printf("   C  VSTAB              %s", vstab[vstab_mode]);

        printf(" \n\n 3A SETTING SUB MENU \n");
        printf(" -----------------------------\n");

        printf("   M. Measurement Data: %s\n", measurement[measurementIdx]);
        printf("   F. Start face detection \n");
        printf("   T. Stop face detection \n");
        printf("   G. Touch/Focus area AF\n");
        printf("   f. Auto Focus/Half Press\n");
        printf("   J.Flash:              %s\n", flashModes[flashIdx]);
        printf("   7. EV offset:      %4.1f\n", compensation);
        printf("   8. AWB mode:       %s\n", strawb_mode[awb_mode]);
        printf("   z. Zoom            %s\n", zoom[zoomIDX].zoom_description);
        printf("   j. Exposure        %s\n", exposure[exposure_mode]);
        printf("   e. Effect:         %s\n", effects[effects_mode]);
        printf("   w. Scene:          %s\n", scene[scene_mode]);
        printf("   s. Saturation:     %d\n", saturation);
        printf("   c. Contrast:       %d\n", contrast);
        printf("   h. Sharpness:      %d\n", sharpness);
        printf("   b. Brightness:     %d\n", brightness);
        printf("   x. Antibanding:    %s\n", antibanding[antibanding_mode]);
        printf("   g. Focus mode:     %s\n", focus[focus_mode]);
        printf("   m. Metering mode:     %s\n" , metering[meter_mode]);
        printf("   <. Exposure Lock:     %s\n", lock[elockidx]);
        printf("   >. WhiteBalance Lock:  %s\n",lock[wblockidx]);
        printf("\n");
        printf("   Choice: ");
    }

    ch = getchar();
    printf("%c", ch);

    print_menu = 1;

    switch (ch) {

    case '_':
        AutoConvergenceModeIDX++;
        AutoConvergenceModeIDX %= ARRAY_SIZE(autoconvergencemode);
        params.set(KEY_AUTOCONVERGENCE, autoconvergencemode[AutoConvergenceModeIDX]);
        if ( strcmp (autoconvergencemode[AutoConvergenceModeIDX], AUTOCONVERGENCE_MODE_MANUAL) == 0) {
            params.set(KEY_MANUALCONVERGENCE_VALUES, manualconvergencevalues[ManualConvergenceValuesIDX]);
        }
        else {
            params.set(KEY_MANUALCONVERGENCE_VALUES, manualconvergencevalues[ManualConvergenceDefaultValueIDX]);
            ManualConvergenceValuesIDX = ManualConvergenceDefaultValueIDX;
        }
        camera->setParameters(params.flatten());

        break;
    case '^':
        if ( strcmp (autoconvergencemode[AutoConvergenceModeIDX], AUTOCONVERGENCE_MODE_MANUAL) == 0) {
            ManualConvergenceValuesIDX++;
            ManualConvergenceValuesIDX %= ARRAY_SIZE(manualconvergencevalues);
            params.set(KEY_MANUALCONVERGENCE_VALUES, manualconvergencevalues[ManualConvergenceValuesIDX]);
            camera->setParameters(params.flatten());
        }
        break;
    case 'A':
        camera_index++;
        camera_index %= ARRAY_SIZE(cameras);
        if ( camera_index == 2) {
            params.set(KEY_STEREO_CAMERA, "true");
        } else {
            params.set(KEY_STEREO_CAMERA, "false");
        }
        closeCamera();

        openCamera();

        if (camera_index == 0) {
            params.setPreviewFrameRate(30);
        } else {
            params.setPreviewFrameRate(27);
        }


        break;
    case '[':
        if ( hardwareActive ) {
            camera->setParameters(params.flatten());
            camera->startPreview();
        }
        break;

    case '0':
        initDefaults();
        break;

        case '1':

            if ( startPreview() < 0 ) {
                printf("Error while starting preview\n");

                return -1;
            }

            break;

        case '2':
            stopPreview();

            if ( recordingMode ) {
                camera->disconnect();
                camera.clear();
                stopRecording();
                closeRecorder();

                camera = Camera::connect(camera_index,
                        processName,
                        Camera::USE_CALLING_UID);
                  if ( NULL == camera.get() ) {
                      sleep(1);
                      camera = Camera::connect(camera_index,
                              processName,
                              Camera::USE_CALLING_UID);
                      if ( NULL == camera.get() ) {
                          return -1;
                      }
                  }
                  camera->setListener(new CameraHandler());
                  camera->setParameters(params.flatten());
                  recordingMode = false;
            }

            break;

        case '3':
            rotation += 90;
            rotation %= 360;
            params.set(CameraParameters::KEY_ROTATION, rotation);
            if ( hardwareActive )
                camera->setParameters(params.flatten());

            break;

        case '4':
            previewSizeIDX += 1;
            previewSizeIDX %= ARRAY_SIZE(previewSize);
            if ( NULL != params.get(KEY_STEREO_CAMERA) ) {
                if ( strcmp(params.get(KEY_STEREO_CAMERA), "false") == 0 ) {
                    params.setPreviewSize(previewSize[previewSizeIDX].width, previewSize[previewSizeIDX].height);
                } else {
                    params.setPreviewSize(previewSize[previewSizeIDX].width, previewSize[previewSizeIDX].height*2);
                }
            }
            reSizePreview = true;

            if ( hardwareActive && previewRunning ) {
                camera->stopPreview();
                camera->setParameters(params.flatten());
                camera->startPreview();
            } else if ( hardwareActive ) {
                camera->setParameters(params.flatten());
            }

            break;

        case '5':
            captureSizeIDX += 1;
            captureSizeIDX %= ARRAY_SIZE(captureSize);
            params.setPictureSize(captureSize[captureSizeIDX].width, captureSize[captureSizeIDX].height);

            if ( hardwareActive )
                camera->setParameters(params.flatten());
            break;

        case 'l':
        case 'L':
            VcaptureSizeIDX++;
            VcaptureSizeIDX %= ARRAY_SIZE(VcaptureSize);
            break;

        case ']':
            VbitRateIDX++;
            VbitRateIDX %= ARRAY_SIZE(VbitRate);
            break;


        case '6':

            if ( !recordingMode ) {

                recordingMode = true;

                if ( startPreview() < 0 ) {
                    printf("Error while starting preview\n");

                    return -1;
                }

                if ( openRecorder() < 0 ) {
                    printf("Error while openning video recorder\n");

                    return -1;
                }

                if ( configureRecorder() < 0 ) {
                    printf("Error while configuring video recorder\n");

                    return -1;
                }

                if ( startRecording() < 0 ) {
                    printf("Error while starting video recording\n");

                    return -1;
                }
            }

            break;

        case '7':

            if ( compensation > 2.0) {
                compensation = -2.0;
            } else {
                compensation += 0.1;
            }

            params.set(KEY_COMPENSATION, (int) (compensation * 10));

            if ( hardwareActive )
                camera->setParameters(params.flatten());

            break;

        case '8':
            awb_mode++;
            awb_mode %= ARRAY_SIZE(strawb_mode);
            params.set(params.KEY_WHITE_BALANCE, strawb_mode[awb_mode]);

            if ( hardwareActive )
                camera->setParameters(params.flatten());

            break;

        case '9':
            videoCodecIDX++;
            videoCodecIDX %= ARRAY_SIZE(videoCodecs);
            break;
        case '~':
            previewFormat += 1;
            previewFormat %= ARRAY_SIZE(pixelformat) - 1;
            params.setPreviewFormat(pixelformat[previewFormat].pixformat);

            if ( hardwareActive )
                camera->setParameters(params.flatten());

            break;
        case '$':
            pictureFormat += 1;
            if ( NULL != params.get(KEY_STEREO_CAMERA) ) {
                if ( strcmp(params.get(KEY_STEREO_CAMERA), "false") == 0 && pictureFormat > 4 )
                    pictureFormat = 0;
            }
            pictureFormat %= ARRAY_SIZE(codingformat);
            params.setPictureFormat(codingformat[pictureFormat]);
            if ( hardwareActive )
                camera->setParameters(params.flatten());

            break;

        case '?' :
            ///Set mode=3 to select video mode
            params.set(KEY_MODE, 3);
            params.set(KEY_VNF, 1);
            params.set(KEY_VSTAB, 1);
            break;

        case ':':
            thumbSizeIDX += 1;
            thumbSizeIDX %= ARRAY_SIZE(previewSize);
            params.set(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH, previewSize[thumbSizeIDX].width);
            params.set(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, previewSize[thumbSizeIDX].height);

            if ( hardwareActive )
                camera->setParameters(params.flatten());

            break;

        case '\'':
            if ( thumbQuality >= 100) {
                thumbQuality = 0;
            } else {
                thumbQuality += 5;
            }

            params.set(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY, thumbQuality);
            if ( hardwareActive )
                camera->setParameters(params.flatten());
            break;

        case 'B' :
            vnf_mode++;
            vnf_mode %= ARRAY_SIZE(vnf);
            params.set(KEY_VNF, vnf_mode);

            if ( hardwareActive )
                camera->setParameters(params.flatten());
            break;

        case 'C' :
            vstab_mode++;
            vstab_mode %= ARRAY_SIZE(vstab);
            params.set(KEY_VSTAB, vstab_mode);

            if ( hardwareActive )
                camera->setParameters(params.flatten());
            break;

        case 'E':
            if(hardwareActive)
                params.unflatten(camera->getParameters());
            printSupportedParams();
            break;

        case '*':
            if ( hardwareActive )
                camera->startRecording();
            break;

        case 'o':
            if ( jpegQuality >= 100) {
                jpegQuality = 0;
            } else {
                jpegQuality += 5;
            }

            params.set(CameraParameters::KEY_JPEG_QUALITY, jpegQuality);
            if ( hardwareActive )
                camera->setParameters(params.flatten());
            break;

        case 'M':
            measurementIdx = (measurementIdx + 1)%ARRAY_SIZE(measurement);
            params.set(KEY_MEASUREMENT, measurement[measurementIdx]);
            if ( hardwareActive )
                camera->setParameters(params.flatten());
            break;
        case 'm':
        {
            meter_mode = (meter_mode + 1)%ARRAY_SIZE(metering);
            params.set(KEY_METERING_MODE, metering[meter_mode]);
            if ( hardwareActive )
                camera->setParameters(params.flatten());
            break;
        }

        case 'k':
            ippIDX += 1;
            ippIDX %= ARRAY_SIZE(ipp_mode);
            ippIDX_old = ippIDX;

            params.set(KEY_IPP, ipp_mode[ippIDX]);

            if ( hardwareActive )
                camera->setParameters(params.flatten());

            break;

        case 'K':
            gbceIDX+= 1;
            gbceIDX %= ARRAY_SIZE(gbce);
            params.set(KEY_GBCE, gbce[gbceIDX]);

            if ( hardwareActive )
                camera->setParameters(params.flatten());
            break;

        case 'O':
            glbceIDX+= 1;
            glbceIDX %= ARRAY_SIZE(gbce);
            params.set(KEY_GLBCE, gbce[glbceIDX]);

            if ( hardwareActive )
                camera->setParameters(params.flatten());
            break;

        case 'F':
            if ( hardwareActive )
                camera->sendCommand(CAMERA_CMD_START_FACE_DETECTION, 0, 0);

            break;

        case 'T':

            if ( hardwareActive )
                camera->sendCommand(CAMERA_CMD_STOP_FACE_DETECTION, 0, 0);

            break;

        case '@':
            if ( hardwareActive ) {

                closeCamera();

                if ( 0 >= openCamera() ) {
                    printf( "Reconnected to CameraService \n");
                }
            }

            break;

        case '#':

            if ( burst >= MAX_BURST ) {
                burst = 0;
            } else {
                burst += BURST_INC;
            }
            params.set(KEY_BURST, burst);

            if ( hardwareActive )
                camera->setParameters(params.flatten());

            break;

        case 'J':
            flashIdx++;
            flashIdx %= ARRAY_SIZE(flashModes);
            params.set(CameraParameters::KEY_FLASH_MODE, (flashModes[flashIdx]));

            if ( hardwareActive )
                camera->setParameters(params.flatten());

            break;

        case 'u':
            capture_mode++;
            capture_mode %= ARRAY_SIZE(capture);

            // HQ should always be in ldc-nsf
            // if not HQ, then return the ipp to its previous state
            if( !strcmp(capture[capture_mode], "high-quality") ) {
                ippIDX_old = ippIDX;
                ippIDX = 3;
                params.set(KEY_IPP, ipp_mode[ippIDX]);
            } else {
                ippIDX = ippIDX_old;
            }

            params.set(KEY_MODE, (capture[capture_mode]));

            if ( hardwareActive )
                camera->setParameters(params.flatten());

            break;

        case 'U':
            tempBracketIdx++;
            tempBracketIdx %= ARRAY_SIZE(tempBracketing);
            params.set(KEY_TEMP_BRACKETING, tempBracketing[tempBracketIdx]);

            if ( hardwareActive )
                camera->setParameters(params.flatten());

            break;

        case 'H':
            expBracketIdx++;
            expBracketIdx %= ARRAY_SIZE(expBracketing);

            params.set(KEY_EXP_BRACKETING_RANGE, expBracketingRange[expBracketIdx]);

            if ( hardwareActive )
                camera->setParameters(params.flatten());

            break;

        case 'W':
            tempBracketRange++;
            tempBracketRange %= TEMP_BRACKETING_MAX_RANGE;
            if ( 0 == tempBracketRange ) {
                tempBracketRange = 1;
            }

            params.set(KEY_TEMP_BRACKETING_NEG, tempBracketRange);
            params.set(KEY_TEMP_BRACKETING_POS, tempBracketRange);

            if ( hardwareActive )
                camera->setParameters(params.flatten());

            break;

        case 'w':
            scene_mode++;
            scene_mode %= ARRAY_SIZE(scene);
            params.set(params.KEY_SCENE_MODE, scene[scene_mode]);

            if ( hardwareActive )
                camera->setParameters(params.flatten());

            break;

        case 'i':
            iso_mode++;
            iso_mode %= ARRAY_SIZE(iso);
            params.set(KEY_ISO, iso[iso_mode]);

            if ( hardwareActive )
                camera->setParameters(params.flatten());
            break;

        case 'h':
            if ( sharpness >= 100) {
                sharpness = 0;
            } else {
                sharpness += 10;
            }
            params.set(KEY_SHARPNESS, sharpness);
            if ( hardwareActive )
                camera->setParameters(params.flatten());
            break;

        case 'D':
        {
            audioCodecIDX++;
            audioCodecIDX %= ARRAY_SIZE(audioCodecs);
            break;
        }

        case 'v':
        {
            outputFormatIDX++;
            outputFormatIDX %= ARRAY_SIZE(outputFormat);
            break;
        }

        case 'z':
            zoomIDX++;
            zoomIDX %= ARRAY_SIZE(zoom);
            params.set(CameraParameters::KEY_ZOOM, zoom[zoomIDX].idx);

            if ( hardwareActive )
                camera->setParameters(params.flatten());

            break;

        case 'j':
            exposure_mode++;
            exposure_mode %= ARRAY_SIZE(exposure);
            params.set(KEY_EXPOSURE, exposure[exposure_mode]);

            if ( hardwareActive )
                camera->setParameters(params.flatten());

            break;

        case 'c':
            if( contrast >= 200){
                contrast = 0;
            } else {
                contrast += 10;
            }
            params.set(KEY_CONTRAST, contrast);
            if ( hardwareActive )
                camera->setParameters(params.flatten());
            break;
        case 'b':
            if ( brightness >= 200) {
                brightness = 0;
            } else {
                brightness += 10;
            }

            params.set(KEY_BRIGHTNESS, brightness);

            if ( hardwareActive )
                camera->setParameters(params.flatten());

            break;

        case 's':
        case 'S':
            if ( saturation >= 100) {
                saturation = 0;
            } else {
                saturation += 10;
            }

            params.set(KEY_SATURATION, saturation);

            if ( hardwareActive )
                camera->setParameters(params.flatten());

            break;

        case 'e':
            effects_mode++;
            effects_mode %= ARRAY_SIZE(effects);
            params.set(params.KEY_EFFECT, effects[effects_mode]);

            if ( hardwareActive )
                camera->setParameters(params.flatten());

            break;

        case 'r':


            if (camera_index == 0) {
                frameRateIDX += 1;
                frameRateIDX %= ARRAY_SIZE(fpsConstRanges);
                params.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, fpsConstRanges[frameRateIDX].range);
            } else
            {
                frameRateIDXSec += 1;
                frameRateIDXSec %= ARRAY_SIZE(fpsConstRangesSec);
                params.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, fpsConstRangesSec[frameRateIDXSec].range);


            }

            if ( hardwareActive ) {
                camera->setParameters(params.flatten());
            }

            break;

        case 'R':
            fpsRangeIdx += 1;
            fpsRangeIdx %= ARRAY_SIZE(fpsRanges);
            params.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, fpsRanges[fpsRangeIdx].range);

            if ( hardwareActive ) {
                camera->setParameters(params.flatten());
            }

            break;

        case 'x':
            antibanding_mode++;
            antibanding_mode %= ARRAY_SIZE(antibanding);
            params.set(params.KEY_ANTIBANDING, antibanding[antibanding_mode]);

            if ( hardwareActive )
                camera->setParameters(params.flatten());

            break;

        case 'g':
            focus_mode++;
            focus_mode %= ARRAY_SIZE(focus);
            params.set(params.KEY_FOCUS_MODE, focus[focus_mode]);

            if ( hardwareActive )
                camera->setParameters(params.flatten());

            break;

        case 'G':

            params.set(CameraParameters::KEY_FOCUS_AREAS, TEST_FOCUS_AREA);

            if ( hardwareActive )
                camera->setParameters(params.flatten());

            params.remove(CameraParameters::KEY_FOCUS_AREAS);

        case 'f':

            gettimeofday(&autofocus_start, 0);

            if ( hardwareActive )
                camera->autoFocus();

            break;

        case 'p':

            gettimeofday(&picture_start, 0);

            if ( hardwareActive )
                camera->takePicture(CAMERA_MSG_COMPRESSED_IMAGE|CAMERA_MSG_RAW_IMAGE);

            break;

        case '&':
            printf("Enabling Preview Callback");
            dump_preview = 1;
            if ( hardwareActive )
            camera->setPreviewCallbackFlags(CAMERA_FRAME_CALLBACK_FLAG_ENABLE_MASK);
            break;

        case '{':
            if ( strcmp(params.get(KEY_S3D2D_PREVIEW_MODE), "off") == 0 )
                {
                params.set(KEY_S3D2D_PREVIEW_MODE, "on");
                }
            else
                {
                params.set(KEY_S3D2D_PREVIEW_MODE, "off");
                }
            if ( hardwareActive )
                camera->setParameters(params.flatten());
            break;

        case 'a':

            while (1) {
                if ( menu_gps() < 0)
                    break;
            };

            break;

        case 'q':

            stopPreview();

            return -1;

        case '/':
        {
            if (showfps)
            {
                property_set("debug.image.showfps", "0");
                showfps = false;
            }
            else
            {
                property_set("debug.image.showfps", "1");
                showfps = true;
            }
            break;
        }

    case '<':
      elockidx += 1;
      elockidx %= ARRAY_SIZE(lock);
      params.set(KEY_AUTO_EXPOSURE_LOCK, lock[elockidx]);
      if ( hardwareActive )
        camera->setParameters(params.flatten());
      break;

    case '>':
      wblockidx += 1;
      wblockidx %= ARRAY_SIZE(lock);
      params.set(KEY_AUTO_WHITEBALANCE_LOCK, lock[wblockidx]);
      if ( hardwareActive )
        camera->setParameters(params.flatten());
      break;

        default:
            print_menu = 0;

            break;
    }

    return 0;
}

void print_usage() {
    printf(" USAGE: camera_test  <param>  <script>\n");
    printf(" <param>\n-----------\n\n");
    printf(" F or f -> Functional tests \n");
    printf(" A or a -> API tests \n");
    printf(" E or e -> Error scenario tests \n");
    printf(" S or s -> Stress tests; with syslink trace \n");
    printf(" SN or sn -> Stress tests; No syslink trace \n\n");
    printf(" <script>\n----------\n");
    printf("Script name (Only for stress tests)\n\n");
    return;
}

int error_scenario() {
    char ch;
    status_t stat = NO_ERROR;

    if (print_menu) {
        printf("   0. Buffer need\n");
        printf("   1. Not enough memory\n");
        printf("   2. Media server crash\n");
        printf("   3. Overlay object request\n");
        printf("   4. Pass unsupported preview&picture format\n");
        printf("   5. Pass unsupported preview&picture resolution\n");
        printf("   6. Pass unsupported preview framerate\n");

        printf("   q. Quit\n");
        printf("   Choice: ");
    }

    print_menu = 1;
    ch = getchar();
    printf("%c\n", ch);

    switch (ch) {
        case '0': {
            printf("Case0:Buffer need\n");
            bufferStarvationTest = 1;
            params.set(KEY_BUFF_STARV, bufferStarvationTest); //enable buffer starvation

            if ( !recordingMode ) {
                recordingMode = true;
                if ( startPreview() < 0 ) {
                    printf("Error while starting preview\n");

                    return -1;
                }

                if ( openRecorder() < 0 ) {
                    printf("Error while openning video recorder\n");

                    return -1;
                }

                if ( configureRecorder() < 0 ) {
                    printf("Error while configuring video recorder\n");

                    return -1;
                }

                if ( startRecording() < 0 ) {
                    printf("Error while starting video recording\n");

                    return -1;
                }

            }

            usleep(1000000);//1s

            stopPreview();

            if ( recordingMode ) {
                stopRecording();
                closeRecorder();

                recordingMode = false;
            }

            break;
        }

        case '1': {
            printf("Case1:Not enough memory\n");
            int* tMemoryEater = new int[999999999];

            if (!tMemoryEater) {
                printf("Not enough memory\n");
                return -1;
            } else {
                delete tMemoryEater;
            }

            break;
        }

        case '2': {
            printf("Case2:Media server crash\n");
            //camera = Camera::connect();

            if ( NULL == camera.get() ) {
                printf("Unable to connect to CameraService\n");
                return -1;
            }

            break;
        }

        case '3': {
            printf("Case3:Overlay object request\n");
            int err = 0;

            err = open("/dev/video5", O_RDWR);

            if (err < 0) {
                printf("Could not open the camera device5: %d\n",  err );
                return err;
            }

            if ( startPreview() < 0 ) {
                printf("Error while starting preview\n");
                return -1;
            }

            usleep(1000000);//1s

            stopPreview();

            close(err);
            break;
        }

        case '4': {

            if ( hardwareActive ) {

                params.setPictureFormat("invalid-format");
                params.setPreviewFormat("invalid-format");

                stat = camera->setParameters(params.flatten());

                if ( NO_ERROR != stat ) {
                    printf("Test passed!\n");
                } else {
                    printf("Test failed!\n");
                }

                initDefaults();
            }

            break;
        }

        case '5': {

            if ( hardwareActive ) {

                params.setPictureSize(-1, -1);
                params.setPreviewSize(-1, -1);

                stat = camera->setParameters(params.flatten());

                if ( NO_ERROR != stat ) {
                    printf("Test passed!\n");
                } else {
                    printf("Test failed!\n");
                }

                initDefaults();
            }

            break;
        }

        case '6': {

            if ( hardwareActive ) {

                params.setPreviewFrameRate(-1);

                stat = camera->setParameters(params.flatten());

                if ( NO_ERROR != stat ) {
                    printf("Test passed!\n");
                } else {
                    printf("Test failed!\n");
                }

                initDefaults();
            }


            break;
        }

        case 'q': {
            return -1;
        }

        default: {
            print_menu = 0;
            break;
        }
    }

    return 0;
}

int restartCamera() {

  const char dir_path_name[80] = SDCARD_PATH;

  printf("+++Restarting Camera After Error+++\n");
  stopPreview();

  if (recordingMode) {
    stopRecording();
    closeRecorder();

    recordingMode = false;
  }

  sleep(3); //Wait a bit before restarting

  restartCount++;

  if (strcpy(dir_path, dir_path_name) == NULL)
  {
    printf("Error reseting dir name");
    return -1;
  }

  if ( openCamera() < 0 )
  {
    printf("+++Camera Restarted Failed+++\n");
    system("echo camerahal_test > /sys/power/wake_unlock");
    return -1;
  }

  initDefaults();

  stopScript = false;

  printf("+++Camera Restarted Successfully+++\n");
  return 0;
}

int main(int argc, char *argv[]) {
    char *cmd;
    int pid;
    sp<ProcessState> proc(ProcessState::self());

    unsigned long long st, end, delay;
    timeval current_time;

    gettimeofday(&current_time, 0);

    st = current_time.tv_sec * 1000000 + current_time.tv_usec;

    cmd = NULL;

    if ( argc < 2 ) {
        printf(" Please enter atleast 1 argument\n");
        print_usage();

        return 0;
    }
    system("echo camerahal_test > /sys/power/wake_lock");
    if ( argc < 3 ) {
        switch (*argv[1]) {
            case 'S':
            case 's':
                printf("This is stress / regression tests \n");
                printf("Provide script file as 2nd argument\n");

                break;

            case 'F':
            case 'f':
                ProcessState::self()->startThreadPool();

                if ( openCamera() < 0 ) {
                    printf("Camera initialization failed\n");
                    system("echo camerahal_test > /sys/power/wake_unlock");
                    return -1;
                }

                initDefaults();
                print_menu = 1;

                while ( 1 ) {
                    if ( functional_menu() < 0 )
                        break;
                };

                break;

            case 'A':
            case 'a':
                printf("API level test cases coming soon ... \n");

                break;

            case 'E':
            case 'e': {
                ProcessState::self()->startThreadPool();

                if ( openCamera() < 0 ) {
                    printf("Camera initialization failed\n");
                    system("echo camerahal_test > /sys/power/wake_unlock");
                    return -1;
                }

                initDefaults();
                print_menu = 1;

                while (1) {
                    if (error_scenario() < 0) {
                        break;
                    }
                }

                break;
            }

            default:
                printf("INVALID OPTION USED\n");
                print_usage();

                break;
        }
    } else if ( ( argc == 3) && ( ( *argv[1] == 'S' ) || ( *argv[1] == 's') ) ) {

        if((argv[1][1] == 'N') || (argv[1][1] == 'n')) {
            bLogSysLinkTrace = false;
        }

        ProcessState::self()->startThreadPool();

        if ( openCamera() < 0 ) {
            printf("Camera initialization failed\n");
            system("echo camerahal_test > /sys/power/wake_unlock");
            return -1;
        }

        initDefaults();

        cmd = load_script(argv[2]);

        if ( cmd != NULL) {
            start_logging(argv[2], pid);
            stressTest = true;

            while (1)
              {
                if ( execute_functional_script(cmd) == 0 )
                  {
                    break;
                  }
                else
                  {
                    printf("CameraTest Restarting Camera...\n");

                    free(cmd);
                    cmd = NULL;

                    if ( (restartCamera() != 0)  || ((cmd = load_script(argv[2])) == NULL) )
                      {
                        printf("ERROR::CameraTest Restarting Camera...\n");
                        break;
                      }
                  }
              }
            free(cmd);
            stop_logging(pid);
        }
    } else if ( ( argc == 3) && ( ( *argv[1] == 'E' ) || ( *argv[1] == 'e') ) ) {

        ProcessState::self()->startThreadPool();

        if ( openCamera() < 0 ) {
            printf("Camera initialization failed\n");
            system("echo camerahal_test > /sys/power/wake_unlock");
            return -1;
        }

        initDefaults();

        cmd = load_script(argv[2]);

        if ( cmd != NULL) {
            start_logging(argv[2], pid);
            execute_error_script(cmd);
            free(cmd);
            stop_logging(pid);
        }

    } else {
        printf("INVALID OPTION USED\n");
        print_usage();
    }

    gettimeofday(&current_time, 0);
    end = current_time.tv_sec * 1000000 + current_time.tv_usec;
    delay = end - st;
    printf("Application clossed after: %llu ms\n", delay);
    system("echo camerahal_test > /sys/power/wake_unlock");
    return 0;
}
