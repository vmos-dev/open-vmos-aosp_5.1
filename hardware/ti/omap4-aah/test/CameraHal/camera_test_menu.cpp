#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <semaphore.h>
#include <pthread.h>
#include <string.h>
#include <assert.h>
#include <climits>

#include <ui/DisplayInfo.h>

#include <gui/GLConsumer.h>
#include <gui/Surface.h>
#include <ui/GraphicBuffer.h>

#include <camera/Camera.h>
#include <camera/ICamera.h>
#include <media/mediarecorder.h>

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <cutils/properties.h>
#include <camera/CameraParameters.h>
#include <camera/ShotParameters.h>
#include <system/audio.h>
#include <system/camera.h>

#include <binder/IMemory.h>
#include <binder/MemoryBase.h>
#include <binder/MemoryHeapBase.h>

#include <cutils/memory.h>
#include <utils/Log.h>

#include <sys/wait.h>

#include "camera_test.h"
#include "camera_test_surfacetexture.h"
#ifdef ANDROID_API_JB_OR_LATER
#include "camera_test_bufferqueue.h"
#endif

using namespace android;

int camera_index = 0;
int print_menu;

sp<Camera> camera;
sp<MediaRecorder> recorder;
sp<SurfaceComposerClient> client;
sp<SurfaceControl> surfaceControl;
sp<Surface> previewSurface;
sp<BufferSourceThread> bufferSourceOutputThread;
sp<BufferSourceInput> bufferSourceInput;

CameraParameters params;
ShotParameters shotParams;
float compensation = 0.0;
double latitude = 0.0;
double longitude = 0.0;
double degree_by_step = 17.5609756;
double altitude = 0.0;
int awb_mode = 0;
int effects_mode = 0;
int scene_mode = 0;
int caf_mode = 0;
int tempBracketRange = 1;
int tempBracketIdx = 0;
int measurementIdx = 0;
int expBracketIdx = BRACKETING_IDX_DEFAULT;
int AutoConvergenceModeIDX = 0;
int gbceIDX = 0;
int glbceIDX = 0;
int rotation = 0;
int previewRotation = 0;
bool reSizePreview = true;
bool hardwareActive = false;
bool recordingMode = false;
bool previewRunning = false;
bool vstabtoggle = false;
bool AutoExposureLocktoggle = false;
bool AutoWhiteBalanceLocktoggle = false;
bool vnftoggle = false;
bool faceDetectToggle = false;
bool metaDataToggle = false;
bool shotConfigFlush = false;
bool streamCapture = false;
int saturation = 0;
int zoomIDX = 0;
int videoCodecIDX = 0;
int audioCodecIDX = 0;
int outputFormatIDX = 0;
int contrast = 0;
int brightness = 0;
unsigned int burst = 0;
unsigned int burstCount = 0;
int sharpness = 0;
int iso_mode = 0;
int capture_mode = 0;
int exposure_mode = 0;
int ippIDX = 0;
int ippIDX_old = 0;
int previewFormat = 0;
int pictureFormat = 0;
int jpegQuality = 85;
int thumbQuality = 85;
int flashIdx = 0;
int fpsRangeIdx = 0;
timeval autofocus_start, picture_start;
char script_name[80];
int prevcnt = 0;
int videoFd = -1;
int afTimeoutIdx = 0;
int platformID = BLAZE_TABLET2;
int numAntibanding = 0;
int numEffects = 0;
int numcaptureSize = 0;
int nummodevalues = 0;
int numVcaptureSize = 0;
int numpreviewSize = 0;
int numthumbnailSize = 0;
int numawb = 0;
int numscene = 0;
int numfocus = 0;
int numflash = 0;
int numExposureMode = 0;
int numisoMode = 0;
int antibanding_mode = 0;
int effectsStrLenght = 0;
int numfps = 0;
int numpreviewFormat = 0;
int numpictureFormat = 0;
int *constFramerate = 0;
int rangeCnt = 0;
int constCnt = 0;
int focus_mode = 0;
int thumbSizeIDX =  0;
int previewSizeIDX = 1;
int captureSizeIDX = 0;
int VcaptureSizeIDX = 1;
int frameRateIDX = 0;
char *str;
char *param;
char *antibandStr = 0;
char *exposureModeStr = 0;
char *isoModeStr = 0;
char *effectssStr = 0;
char *captureSizeStr  = 0;
char *modevaluesstr = 0;
char *videosnapshotstr = 0;
char *autoconvergencestr = 0;
char *VcaptureSizeStr  = 0;
char *thumbnailSizeStr  = 0;
char *vstabstr = 0;
char *vnfstr = 0;
char *zoomstr = 0;
char *smoothzoomstr = 0;
char *AutoExposureLockstr = 0;
char *AutoWhiteBalanceLockstr = 0;
char *previewSizeStr = 0;
char *awbStr = 0;
char *sceneStr = 0;
char *focusStr = 0;
char *flashStr = 0;
char *fpsstr = 0;
char *previewFormatStr = 0;
char *pictureFormatStr = 0;
char **modevalues = 0;
char **elem;
char **antiband = 0;
char **effectss = 0;
char **awb = 0;
char **scene = 0;
char **focus = 0;
char **flash = 0;
char **exposureMode = 0;
char **isoMode = 0;
char **previewFormatArray = 0;
char **pictureFormatArray = 0;
char **fps_const_str = 0;
char **rangeDescription = 0;
char **fps_range_str = 0;
param_Array ** capture_Array = 0;
param_Array ** Vcapture_Array = 0;
param_Array ** preview_Array = 0;
param_Array ** thumbnail_Array = 0;
fps_Array * fpsArray = 0;

int enableMisalignmentCorrectionIdx = 0;

char **autoconvergencemode = 0;
int numAutoConvergence = 0;
const char MeteringAreas[] = "(-656,-671,188,454,1)";

char **stereoLayout;
int numLay = 0;

char **stereoCapLayout;
int numCLay = 0;

int stereoLayoutIDX = 1;
int stereoCapLayoutIDX = 0;

char *layoutstr =0;
char *capturelayoutstr =0;

char output_dir_path[256];
char videos_dir_path[256 + 8];
char images_dir_path[256 + 8];

const char *cameras[] = {"Primary Camera", "Secondary Camera 1", "Stereo Camera"};
const char *measurement[] = {"disable", "enable"};

param_NamedExpBracketList_t expBracketing[] = {
  {
    "Disabled",
    PARAM_EXP_BRACKET_PARAM_NONE,
    PARAM_EXP_BRACKET_VALUE_NONE,
    PARAM_EXP_BRACKET_APPLY_NONE,
    "0"
  },
  {
    "Relative exposure compensation",
    PARAM_EXP_BRACKET_PARAM_COMP,
    PARAM_EXP_BRACKET_VALUE_REL,
    PARAM_EXP_BRACKET_APPLY_ADJUST,
    "-300,-150,0,150,300,150,0,-150,-300"
  },
  {
    "Relative exposure compensation (forced)",
    PARAM_EXP_BRACKET_PARAM_COMP,
    PARAM_EXP_BRACKET_VALUE_REL,
    PARAM_EXP_BRACKET_APPLY_FORCED,
    "-300F,-150F,0F,150F,300F,150F,0F,-150F,-300F"
  },
  {
    "Absolute exposure and gain",
    PARAM_EXP_BRACKET_PARAM_PAIR,
    PARAM_EXP_BRACKET_VALUE_ABS,
    PARAM_EXP_BRACKET_APPLY_ADJUST,
    "(33000,10),(0,70),(33000,100),(0,130),(33000,160),(0,180),(33000,200),(0,130),(33000,200)"
  },
  {
    "Absolute exposure and gain (forced)",
    PARAM_EXP_BRACKET_PARAM_PAIR,
    PARAM_EXP_BRACKET_VALUE_ABS,
    PARAM_EXP_BRACKET_APPLY_FORCED,
    "(33000,10)F,(0,70)F,(33000,100)F,(0,130)F,(33000,160)F,(0,180)F,(33000,200)F,(0,130)F,(33000,200)F"
  },
  {
    "Relative exposure and gain",
    PARAM_EXP_BRACKET_PARAM_PAIR,
    PARAM_EXP_BRACKET_VALUE_REL,
    PARAM_EXP_BRACKET_APPLY_ADJUST,
    "(-300,-100),(-300,+0),(-100, +0),(-100,+100),(+0,+0),(+100,-100),(+100,+0),(+300,+0),(+300,+100)"
  },
  {
    "Relative exposure and gain (forced)",
    PARAM_EXP_BRACKET_PARAM_PAIR,
    PARAM_EXP_BRACKET_VALUE_REL,
    PARAM_EXP_BRACKET_APPLY_FORCED,
    "(-300,-100)F,(-300,+0)F,(-100, +0)F,(-100,+100)F,(+0,+0)F,(+100,-100)F,(+100,+0)F,(+300,+0)F,(+300,+100)F"
  },
};

const char *tempBracketing[] = {"false", "true"};
const char *faceDetection[] = {"disable", "enable"};
const char *afTimeout[] = {"enable", "disable" };

const char *misalignmentCorrection[] = {"enable", "disable" };

#if defined(OMAP_ENHANCEMENT) && defined(TARGET_OMAP3)
const char *ipp_mode[] = { "off", "Chroma Suppression", "Edge Enhancement" };
#else
const char *ipp_mode[] = { "off", "ldc", "nsf", "ldc-nsf" };
#endif


const char *caf [] = { "Off", "On" };

int numCamera = 0;
bool stereoMode = false;

int manualExp = 0;
int manualExpMin = 0;
int manualExpMax = 0;
int manualExpStep = 0;
int manualGain = 0;
int manualGainMin = 0;
int manualGainMax = 0;
int manualGainStep = 0;
int manualConv = 0;
int manualConvMin = 0;
int manualConvMax = 0;
int manualConvStep = 0;

param_Array previewSize [] = {
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
  { 240, 160,"240x160"},
  { 768, 576,  "768x576" },
  { 960, 720, "960x720"},
  { 256, 96,"SQCIF"},// stereo
  { 128, 192, "SQCIF"},
  { 352, 144,"QCIF"},
  { 176, 288, "QCIF"},
  { 480, 160, "240x160"},
  { 240, 320, "240x160"},
  { 704, 288, "CIF"},
  { 352, 576, "CIF"},
  { 640, 240,"QVGA"},
  { 320, 480, "QVGA"},
  { 1280, 480,"VGA"},
  { 640, 960, "VGA"},
  { 1536, 576,"768x576"},
  { 768, 1152, "768x576"},
  { 1440, 480,"NTSC"},
  { 720, 960,"NTSC"},
  { 1440, 576, "PAL"},
  { 720, 1152, "PAL"},
  { 1600, 480, "WVGA"},
  { 800, 960,"WVGA"},
  { 2560, 720, "HD"},
  { 1280, 1440,  "HD"}
};

size_t length_previewSize =  ARRAY_SIZE(previewSize);

param_Array thumbnailSize [] = {
  { 0,   0,  "NULL"},
  { 128, 96, "SQCIF" },
  { 176, 144, "QCIF" },
  { 352, 288, "CIF" },
  { 320, 240, "QVGA" },
  { 352, 288, "CIF" },
  { 640, 480, "VGA" },
};

size_t length_thumbnailSize =  ARRAY_SIZE(thumbnailSize);

param_Array VcaptureSize [] = {
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
  #if defined(OMAP_ENHANCEMENT) && defined(TARGET_OMAP3)
  { 848, 480, "WVGA2"},
  { 864, 480, "WVGA3"},
  { 992, 560, "WVGA4"},
  #endif
  { 1280, 720, "HD" },
  { 1920, 1080, "FULLHD"},
  { 240, 160,"240x160"},
  { 768, 576,  "768x576" },
  { 960, 720, "960x720"},
  { 256, 96,"SQCIF"},// stereo
  { 128, 192, "SQCIF"},
  { 352, 144,"QCIF"},
  { 176, 288, "QCIF"},
  { 480, 160, "240x160"},
  { 240, 320, "240x160"},
  { 704, 288, "CIF"},
  { 352, 576, "CIF"},
  { 640, 240,"QVGA"},
  { 320, 480, "QVGA"},
  { 1280, 480,"VGA"},
  { 640, 960, "VGA"},
  { 1536, 576,"768x576"},
  { 768, 1152, "768x576"},
  { 1440, 480,"NTSC"},
  { 720, 960,"NTSC"},
  { 1440, 576, "PAL"},
  { 720, 1152, "PAL"},
  { 1600, 480, "WVGA"},
  { 800, 960,"WVGA"},
  { 2560, 720, "HD"},
  { 1280, 1440,  "HD"}
};

size_t lenght_Vcapture_size = ARRAY_SIZE(VcaptureSize);

param_Array captureSize[] = {
  {  320, 240,  "QVGA" },
  {  640, 480,  "VGA" },
  {  800, 600,  "SVGA" },
  { 1152, 864,  "1MP" },
  { 1280, 1024, "1.3MP" },
  { 1600, 1200,  "2MP" },
  { 2016, 1512,  "3MP" },
  { 2592, 1944,  "5MP" },
  { 2608, 1960,  "5MP" },
  { 3264, 2448,  "8MP" },
  { 3648, 2736, "10MP"},
  { 4032, 3024, "12MP"},
  { 640, 240,   "QVGA"},   //stereo
  { 320, 480, "QVGA"},
  { 1280, 480, "VGA"},
  { 640, 960,  "VGA"},
  { 2560, 960,  "1280x960"},
  { 1280, 1920,  "1280x960"},
  { 2304, 864,  "1MP"},
  { 1152, 1728,   "1MP"},
  { 2560, 1024,  "1.3MP"},
  { 1280, 2048, "1.3MP"},
  { 3200, 1200,   "2MP"},
  { 1600, 2400,  "2MP"},
  { 4096, 1536,  "3MP"},
  { 2048, 3072,  "3MP"}
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
  { 12,  "1.5x"},
  { 20, "2x"  },
  { 28, "2.5x"},
  { 32, "3x"  },
  { 36, "3.5x"},
  { 40, "4x"  },
  { 60, "8x"  },
};

size_t length_Zoom = ARRAY_SIZE(zoom);

pixel_format pixelformat[] = {
  { HAL_PIXEL_FORMAT_YCbCr_422_I, CameraParameters::PIXEL_FORMAT_YUV422I },
  { HAL_PIXEL_FORMAT_YCrCb_420_SP, CameraParameters::PIXEL_FORMAT_YUV420SP },
  { HAL_PIXEL_FORMAT_RGB_565, CameraParameters::PIXEL_FORMAT_RGB565 },
  { -1, CameraParameters::PIXEL_FORMAT_JPEG },
  { -1, CameraParameters::PIXEL_FORMAT_BAYER_RGGB },
  };

const char *gbce[] = {"disable", "enable"};

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
bool stressTest = false;
bool stopScript = false;
int restartCount = 0;
bool firstTime = true;
bool firstTimeStereo = true;

//TI extensions for enable/disable algos
const char *algoFixedGamma[] = {CameraParameters::FALSE, CameraParameters::TRUE};
const char *algoNSF1[] = {CameraParameters::FALSE, CameraParameters::TRUE};
const char *algoNSF2[] = {CameraParameters::FALSE, CameraParameters::TRUE};
const char *algoSharpening[] = {CameraParameters::FALSE, CameraParameters::TRUE};
const char *algoThreeLinColorMap[] = {CameraParameters::FALSE, CameraParameters::TRUE};
const char *algoGIC[] = {CameraParameters::FALSE, CameraParameters::TRUE};
int algoFixedGammaIDX = 1;
int algoNSF1IDX = 1;
int algoNSF2IDX = 1;
int algoSharpeningIDX = 1;
int algoThreeLinColorMapIDX = 1;
int algoGICIDX = 1;

/** Buffer source reset */
bool bufferSourceInputReset = false;
bool bufferSourceOutputReset = false;

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
    char            fn[384];

    LOG_FUNCTION_NAME;

    if (mem == NULL)
        goto out;

    if( strcmp(modevalues[capture_mode], "cp-cam") ) {
        //Start preview after capture.
        camera->startPreview();
    }

    fn[0] = 0;
    sprintf(fn, "%s/img%03d.raw", images_dir_path, counter);
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
    char            fn[384];

    LOG_FUNCTION_NAME;

    if (mem == NULL)
        goto out;

    fn[0] = 0;
    sprintf(fn, "%s/preview%03d.yuv", images_dir_path, counter);
    fd = open(fn, O_CREAT | O_WRONLY | O_TRUNC, 0777);
    if(fd < 0) {
        CAMHAL_LOGE("Unable to open file %s: %s", fn, strerror(fd));
        goto out;
    }

    size = mem->size();
    if (size <= 0) {
        CAMHAL_LOGE("IMemory object is of zero size");
        goto out;
    }

    buff = (unsigned char *)mem->pointer();
    if (!buff) {
        CAMHAL_LOGE("Buffer pointer is invalid");
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
    char        fn[384];

    LOG_FUNCTION_NAME;

    if( strcmp(modevalues[capture_mode], "cp-cam")) {
        if(burstCount > 1) {
            burstCount --;
            // Restart preview if taking a single capture
            // or after the last iteration of burstCount
        } else if(burstCount == 0 || burstCount == 1) {
            camera->startPreview();
            burstCount = burst;
        }
    }

    if (mem == NULL)
        goto out;

    fn[0] = 0;
    sprintf(fn, "%s/img%03d.jpg", images_dir_path, counter);
    fd = open(fn, O_CREAT | O_WRONLY | O_TRUNC, 0777);

    if(fd < 0) {
        CAMHAL_LOGE("Unable to open file %s: %s", fn, strerror(fd));
        goto out;
    }

    size = mem->size();
    if (size <= 0) {
        CAMHAL_LOGE("IMemory object is of zero size");
        goto out;
    }

    buff = (unsigned char *)mem->pointer();
    if (!buff) {
        CAMHAL_LOGE("Buffer pointer is invalid");
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
    int32_t msgMask;
    printf("Data cb: %d\n", msgType);

    if ( msgType & CAMERA_MSG_PREVIEW_FRAME )
        my_preview_callback(dataPtr);

    msgMask = CAMERA_MSG_RAW_IMAGE;
#ifdef OMAP_ENHANCEMENT_BURST_CAPTURE
    msgMask |= CAMERA_MSG_RAW_BURST;
#endif
    if ( msgType & msgMask) {
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
        if (metaDataToggle) {
            printf("Preview exposure: %6d    Preview gain: %4d\n",
                   metadata->exposure_time, metadata->analog_gain);
        }

        if (faceDetectToggle) {
            printf("Face detected %d \n", metadata->number_of_faces);
            my_face_callback(metadata);
        }
    }
}

void CameraHandler::postDataTimestamp(nsecs_t timestamp, int32_t msgType, const sp<IMemory>& dataPtr)

{
    static uint32_t count = 0;

    //if(count==100)
    //saveFile(dataPtr);

    count++;

    uint8_t *ptr = (uint8_t*) dataPtr->pointer();

#ifdef OMAP_ENHANCEMENT_BURST_CAPTURE
    if ( msgType & CAMERA_MSG_RAW_BURST) {
        printf("RAW done timestamp: %llu\n", timestamp);
        my_raw_callback(dataPtr);
    } else
#endif
    {
        printf("Recording cb: %d %lld %p\n", msgType, timestamp, dataPtr.get());
        printf("VID_CB: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x", ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5], ptr[6], ptr[7], ptr[8], ptr[9]);
        camera->releaseRecordingFrame(dataPtr);
    }
}

int createPreviewSurface(unsigned int width, unsigned int height, int32_t pixFormat) {
    unsigned int previewWidth, previewHeight;
    DisplayInfo dinfo;
    SurfaceComposerClient::getDisplayInfo(0, &dinfo);

    const unsigned MAX_PREVIEW_SURFACE_WIDTH = dinfo.w;
    const unsigned MAX_PREVIEW_SURFACE_HEIGHT = dinfo.h;

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

    surfaceControl = client->createSurface(0,
                                           previewWidth,
                                           previewHeight,
                                           pixFormat);

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
    printf("\n\r\tSupported Video Formats: %s", params.get(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES));
    printf("\n\r\tSupported Preview Sizes: %s", params.get(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES));
    printf("\n\r\tSupported Preview Formats: %s", params.get(CameraParameters::KEY_SUPPORTED_PREVIEW_FORMATS));
    printf("\n\r\tSupported Preview Frame Rates: %s", params.get(CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES));
    printf("\n\r\tSupported Thumbnail Sizes: %s", params.get(CameraParameters::KEY_SUPPORTED_JPEG_THUMBNAIL_SIZES));
    printf("\n\r\tSupported Whitebalance Modes: %s", params.get(CameraParameters::KEY_SUPPORTED_WHITE_BALANCE));
    printf("\n\r\tSupported Effects: %s", params.get(CameraParameters::KEY_SUPPORTED_EFFECTS));
    printf("\n\r\tSupported Scene Modes: %s", params.get(CameraParameters::KEY_SUPPORTED_SCENE_MODES));
    printf("\n\r\tSupported ISO Modes: %s", params.get("iso-mode-values"));
    printf("\n\r\tSupported Focus Modes: %s", params.get(CameraParameters::KEY_SUPPORTED_FOCUS_MODES));
    printf("\n\r\tSupported Antibanding Options: %s", params.get(CameraParameters::KEY_SUPPORTED_ANTIBANDING));
    printf("\n\r\tSupported Flash Modes: %s", params.get(CameraParameters::KEY_SUPPORTED_FLASH_MODES));
    printf("\n\r\tSupported Focus Areas: %d", params.getInt(CameraParameters::KEY_MAX_NUM_FOCUS_AREAS));
    printf("\n\r\tSupported Metering Areas: %d", params.getInt(CameraParameters::KEY_MAX_NUM_METERING_AREAS));
    printf("\n\r\tSupported Preview FPS Range: %s", params.get(CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE));
    printf("\n\r\tSupported Exposure modes: %s", params.get("exposure-mode-values"));
    printf("\n\r\tSupported VSTAB modes: %s", params.get(CameraParameters::KEY_VIDEO_STABILIZATION_SUPPORTED));
    printf("\n\r\tSupported VNF modes: %s", params.get("vnf-supported"));
    printf("\n\r\tSupported AutoExposureLock: %s", params.get(CameraParameters::KEY_AUTO_EXPOSURE_LOCK_SUPPORTED));
    printf("\n\r\tSupported AutoWhiteBalanceLock: %s", params.get(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK_SUPPORTED));
    printf("\n\r\tSupported Zoom: %s", params.get(CameraParameters::KEY_ZOOM_SUPPORTED));
    printf("\n\r\tSupported Smooth Zoom: %s", params.get(CameraParameters::KEY_SMOOTH_ZOOM_SUPPORTED));
    printf("\n\r\tSupported Video Snapshot: %s", params.get(CameraParameters::KEY_VIDEO_SNAPSHOT_SUPPORTED));
    printf("\n\r\tSupported Capture modes: %s", params.get("mode-values"));

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

    char videoFile[384],vbit_string[50];
    videoFd = -1;
    struct CameraInfo cameraInfo;
    camera->getCameraInfo(camera_index, &cameraInfo);

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

    sprintf(videoFile, "%s/video%d.%s", videos_dir_path, recording_counter, outputFormat[outputFormatIDX].desc);

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

    if (cameraInfo.orientation == 90 || cameraInfo.orientation == 270 ) {
        if ( recorder->setVideoSize(Vcapture_Array[VcaptureSizeIDX]->height, Vcapture_Array[VcaptureSizeIDX]->width) < 0 ) {
            printf("error while configuring video size\n");
            return -1;
        }
    } else {
        if ( recorder->setVideoSize(Vcapture_Array[VcaptureSizeIDX]->width, Vcapture_Array[VcaptureSizeIDX]->height) < 0 ) {
            printf("error while configuring video size\n");
            return -1;
        }
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

    antibandStr = new char [256];
    effectssStr = new char [256];
    exposureModeStr = new char [256];
    captureSizeStr = new char [500];
    VcaptureSizeStr = new char [500];
    previewSizeStr = new char [500];
    thumbnailSizeStr = new char [500];
    awbStr = new char [400];
    sceneStr = new char [400];
    isoModeStr = new char [256];
    focusStr = new char [256];
    flashStr = new char [256];
    fpsstr = new char [256];
    previewFormatStr = new char [256];
    pictureFormatStr = new char [256];
    constFramerate = new int[32];
    vstabstr = new char[256];
    vnfstr = new char[256];
    AutoExposureLockstr = new char[256];
    AutoWhiteBalanceLockstr = new char[256];
    zoomstr = new char[256];
    smoothzoomstr = new char[256];
    modevaluesstr = new char[256];
    videosnapshotstr = new char[256];
    autoconvergencestr = new char[256];
    layoutstr = new char[256];
    capturelayoutstr = new char[256];

    requestBufferSourceReset();

    printf("openCamera(camera_index=%d)\n", camera_index);
    camera = Camera::connect(camera_index);

    if ( NULL == camera.get() ) {
        printf("Unable to connect to CameraService\n");
        printf("Retrying... \n");
        sleep(1);
        camera = Camera::connect(camera_index);

        if ( NULL == camera.get() ) {
            printf("Giving up!! \n");
            return -1;
        }
    }

    if ( firstTime ) {
        params = camera->getParameters();
        firstTime = false;
    }
    getParametersFromCapabilities();
    getSizeParametersFromCapabilities();
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

    deleteAllocatedMemory();

    camera->disconnect();
    camera.clear();

    hardwareActive = false;
    return 0;
}

void createBufferOutputSource() {
    if(bufferSourceOutputThread.get() && bufferSourceOutputReset) {
        bufferSourceOutputThread->requestExit();
        bufferSourceOutputThread.clear();
    }
    if(!bufferSourceOutputThread.get()) {
#ifdef ANDROID_API_JB_OR_LATER
        bufferSourceOutputThread = new BQ_BufferSourceThread(123, camera);
#else
        bufferSourceOutputThread = new ST_BufferSourceThread(false, 123, camera);
#endif
        bufferSourceOutputThread->run();
    }
    bufferSourceOutputReset = false;
}

void createBufferInputSource() {
    if (bufferSourceInput.get() && bufferSourceInputReset) {
        bufferSourceInput.clear();
    }
    if (!bufferSourceInput.get()) {
#ifdef ANDROID_API_JB_OR_LATER
        bufferSourceInput = new BQ_BufferSourceInput(1234, camera);
#else
        bufferSourceInput = new ST_BufferSourceInput(1234, camera);
#endif
    }
    bufferSourceInputReset = false;
}

void requestBufferSourceReset() {
    bufferSourceInputReset = true;
    bufferSourceOutputReset = true;
}

int startPreview() {
    int previewWidth, previewHeight;
    struct CameraInfo cameraInfo;
    DisplayInfo dinfo;
    int orientation;
    unsigned int correctedHeight;

    SurfaceComposerClient::getDisplayInfo(0, &dinfo);

    printf ("dinfo.orientation = %d\n", dinfo.orientation);
    printf ("dinfo.w = %d\n", dinfo.w);
    printf ("dinfo.h = %d\n", dinfo.h);

    // calculate display orientation from sensor orientation
    camera->getCameraInfo(camera_index, &cameraInfo);
    if (cameraInfo.facing == CAMERA_FACING_FRONT) {
        orientation = (cameraInfo.orientation + dinfo.orientation) % 360;
        orientation = (360 - orientation) % 360;  // compensate the mirror
    } else {  // back-facing
        orientation = (cameraInfo.orientation - dinfo.orientation + 360) % 360;
    }


    if (reSizePreview) {
        int orientedWidth, orientedHeight;

        if(recordingMode)
        {
            previewWidth = Vcapture_Array[VcaptureSizeIDX]->width;
            previewHeight = Vcapture_Array[VcaptureSizeIDX]->height;
        }else
        {
            previewWidth = preview_Array[previewSizeIDX]->width;
            previewHeight = preview_Array[previewSizeIDX]->height;
        }

        // corrected height for aspect ratio
        if ((orientation == 90) || (orientation == 270)) {
            orientedHeight = previewWidth;
            orientedWidth = previewHeight;
        } else {
            orientedHeight = previewHeight;
            orientedWidth = previewWidth;
        }
        correctedHeight = (dinfo.w * orientedHeight) / orientedWidth;
        printf("correctedHeight = %d", correctedHeight);

        if ( createPreviewSurface(dinfo.w, correctedHeight,
                                  pixelformat[previewFormat].pixelFormatDesc) < 0 ) {
            printf("Error while creating preview surface\n");
            return -1;
        }

        if ( !hardwareActive ) {
            openCamera();
        }
        if(stereoMode && firstTimeStereo)
        {
             params.set(KEY_S3D_PRV_FRAME_LAYOUT, stereoLayout[stereoLayoutIDX]);
             params.set(KEY_S3D_CAP_FRAME_LAYOUT, stereoCapLayout[stereoCapLayoutIDX]);
        }

        if ((cameraInfo.orientation == 90 || cameraInfo.orientation == 270) && recordingMode) {
            params.setPreviewSize(previewHeight, previewWidth);
        } else {
            params.setPreviewSize(previewWidth, previewHeight);
        }
        params.setPictureSize(capture_Array[captureSizeIDX]->width, capture_Array[captureSizeIDX]->height);

        // calculate display orientation from sensor orientation
        camera->getCameraInfo(camera_index, &cameraInfo);
        if (cameraInfo.facing == CAMERA_FACING_FRONT) {
            orientation = (cameraInfo.orientation + dinfo.orientation) % 360;
            orientation= (360 - orientation) % 360;  // compensate the mirror
        } else {  // back-facing
            orientation = (cameraInfo.orientation - dinfo.orientation + 360) % 360;
        }

        if(!strcmp(params.get(KEY_MODE), "video-mode") ) {
            orientation = 0;
        }

        camera->sendCommand(CAMERA_CMD_SET_DISPLAY_ORIENTATION, orientation, 0);

        camera->setParameters(params.flatten());
        camera->setPreviewTexture(previewSurface->getIGraphicBufferProducer());
    }

    if(hardwareActive) prevcnt = 0;
    camera->startPreview();
    previewRunning = true;
    reSizePreview = false;

    const char *format = params.getPictureFormat();
    if((NULL != format) && isRawPixelFormat(format)) {
        createBufferOutputSource();
        createBufferInputSource();
    }

    return 0;
}

int getParametersFromCapabilities() {
    const char *valstr = NULL;

    numCamera = camera->getNumberOfCameras();

    params.unflatten(camera->getParameters());

    valstr = params.get(KEY_AUTOCONVERGENCE_MODE_VALUES);
    if (NULL != valstr) {
        strcpy(autoconvergencestr, valstr);
        getSupportedParameters(autoconvergencestr,&numAutoConvergence,(char***)&autoconvergencemode);
    } else {
        printf("no supported parameteters for autoconvergence\n\t");
    }

    valstr = params.get(CameraParameters::KEY_SUPPORTED_EFFECTS);
    if (NULL != valstr) {
        strcpy(effectssStr, valstr);
        getSupportedParameters(effectssStr, &numEffects, (char***)&effectss);
    } else {
        printf("Color effects are not supported\n");
    }

    valstr = params.get(CameraParameters::KEY_SUPPORTED_ANTIBANDING);
    if (NULL != valstr) {
        strcpy(antibandStr, valstr);
        getSupportedParameters(antibandStr, &numAntibanding, (char***)&antiband);
    } else {
        printf("Antibanding not supported\n");
    }

    valstr = params.get(CameraParameters::KEY_SUPPORTED_WHITE_BALANCE);
    if (NULL != valstr) {
        strcpy(awbStr, valstr);
        getSupportedParameters(awbStr, &numawb, (char***)&awb);
    } else {
        printf("White balance is not supported\n");
    }

    valstr = params.get(KEY_S3D_PRV_FRAME_LAYOUT_VALUES);
    if ((NULL != valstr) && (0 != strcmp(valstr, "none"))) {
        stereoMode = true;
        strcpy(layoutstr, valstr);
        getSupportedParameters(layoutstr,&numLay,(char***)&stereoLayout);
    } else {
        stereoMode = false;
        printf("layout is not supported\n");
    }

    valstr = params.get(KEY_S3D_CAP_FRAME_LAYOUT_VALUES);
    if ((NULL != valstr) && (0 != strcmp(valstr, "none"))) {
        strcpy(capturelayoutstr, valstr);
        getSupportedParameters(capturelayoutstr,&numCLay,(char***)&stereoCapLayout);
    } else {
        printf("capture layout is not supported\n");
    }

    valstr = params.get(CameraParameters::KEY_SUPPORTED_SCENE_MODES);
    if (NULL != valstr) {
        strcpy(sceneStr, valstr);
        getSupportedParameters(sceneStr, &numscene, (char***)&scene);
    } else {
        printf("Scene modes are not supported\n");
    }

    valstr = params.get(CameraParameters::KEY_SUPPORTED_FOCUS_MODES);
    if (NULL != valstr) {
        strcpy(focusStr, valstr);
        getSupportedParameters(focusStr, &numfocus, (char***)&focus);
    } else {
        printf("Focus modes are not supported\n");
    }

    valstr = params.get(CameraParameters::KEY_SUPPORTED_FLASH_MODES);
    if (NULL != valstr) {
        strcpy(flashStr, valstr);
        getSupportedParameters(flashStr, &numflash, (char***)&flash);
    } else {
        printf("Flash modes are not supported\n");
    }

    valstr = params.get(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES);
    if (NULL != valstr) {
        strcpy(VcaptureSizeStr, valstr);
        getSupportedParametersVideoCaptureSize(VcaptureSizeStr, &numVcaptureSize, VcaptureSize, lenght_Vcapture_size);
    } else {
        printf("Preview sizes are not supported\n");
    }

    valstr = params.get(CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE);
    if (NULL != valstr) {
        strcpy(fpsstr, valstr);
        getSupportedParametersfps(fpsstr, &numfps);
    } else {
        printf("Preview fps range is not supported\n");
    }

    valstr = params.get(CameraParameters::KEY_SUPPORTED_PREVIEW_FORMATS);
    if (NULL != valstr) {
        strcpy(previewFormatStr, valstr);
        getSupportedParameters(previewFormatStr, &numpreviewFormat, (char ***)&previewFormatArray);
    } else {
        printf("Preview formats are not supported\n");
    }

    valstr = params.get(CameraParameters::KEY_SUPPORTED_PICTURE_FORMATS);
    if (NULL != valstr) {
        strcpy(pictureFormatStr, valstr);
        getSupportedParameters(pictureFormatStr, &numpictureFormat, (char ***)&pictureFormatArray);
    } else {
        printf("Picture formats are not supported\n");
    }

    valstr = params.get("exposure-mode-values");
    if (NULL != valstr) {
        strcpy(exposureModeStr, valstr);
        getSupportedParameters(exposureModeStr, &numExposureMode, (char***)&exposureMode);
    } else {
        printf("Exposure modes are not supported\n");
    }

    valstr = params.get("iso-mode-values");
    if (NULL != valstr) {
        strcpy(isoModeStr, valstr);
        getSupportedParameters(isoModeStr, &numisoMode , (char***)&isoMode);
    } else {
        printf("iso modes are not supported\n");
    }

    valstr = params.get(CameraParameters::KEY_SUPPORTED_JPEG_THUMBNAIL_SIZES);
    if (NULL != valstr) {
        strcpy(thumbnailSizeStr, valstr);
        getSupportedParametersThumbnailSize(thumbnailSizeStr, &numthumbnailSize, thumbnailSize, length_thumbnailSize);
    } else {
        printf("Thumbnail sizes are not supported\n");
    }

    valstr = params.get(CameraParameters::KEY_VIDEO_STABILIZATION_SUPPORTED);
    if (NULL != valstr) {
        strcpy(vstabstr, valstr);
    } else {
        printf("VSTAB is not supported\n");
    }

    valstr = params.get("vnf-supported");
    if (NULL != valstr) {
        strcpy(vnfstr, valstr);
    } else {
        printf("VNF is not supported\n");
    }

    valstr = params.get(CameraParameters::KEY_AUTO_EXPOSURE_LOCK_SUPPORTED);
    if (NULL != valstr) {
        strcpy(AutoExposureLockstr, valstr);
    } else {
        printf("AutoExposureLock is not supported\n");
    }

    valstr = params.get(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK_SUPPORTED);
    if (NULL != valstr) {
        strcpy(AutoWhiteBalanceLockstr, valstr);
    } else {
        printf("AutoWhiteBalanceLock is not supported\n");
    }

    valstr = params.get(CameraParameters::KEY_ZOOM_SUPPORTED);
    if (NULL != valstr) {
        strcpy(zoomstr, valstr);
    } else {
        printf("Zoom is not supported\n");
    }

    valstr = params.get(CameraParameters::KEY_SMOOTH_ZOOM_SUPPORTED);
    if (NULL != valstr) {
        strcpy(smoothzoomstr, valstr);
    } else {
        printf("SmoothZoom is not supported\n");
    }

    valstr = params.get("mode-values");
    if (NULL != valstr) {
        strcpy(modevaluesstr, valstr);
        getSupportedParameters(modevaluesstr, &nummodevalues , (char***)&modevalues);
    } else {
        printf("Mode values is not supported\n");
    }

    valstr = params.get(CameraParameters::KEY_VIDEO_SNAPSHOT_SUPPORTED);
    if (NULL != valstr) {
        strcpy(videosnapshotstr, valstr);
    } else {
        printf("Video Snapshot is not supported\n");
    }

    if (params.get(KEY_SUPPORTED_MANUAL_CONVERGENCE_MIN) != NULL) {
        manualConvMin = params.getInt(KEY_SUPPORTED_MANUAL_CONVERGENCE_MIN);
    } else {
        printf("no supported parameteters for manual convergence min\n\t");
    }

    if (params.get(KEY_SUPPORTED_MANUAL_CONVERGENCE_MAX) != NULL) {
        manualConvMax = params.getInt(KEY_SUPPORTED_MANUAL_CONVERGENCE_MAX);
    } else {
        printf("no supported parameteters for manual convergence max\n\t");
    }

    if (params.get(KEY_SUPPORTED_MANUAL_CONVERGENCE_STEP) != NULL) {
        manualConvStep = params.getInt(KEY_SUPPORTED_MANUAL_CONVERGENCE_STEP);
    } else {
        printf("no supported parameteters for manual convergence step\n\t");
    }

    if (params.get(KEY_SUPPORTED_MANUAL_EXPOSURE_MIN) != NULL) {
        manualExpMin = params.getInt(KEY_SUPPORTED_MANUAL_EXPOSURE_MIN);
    } else {
        printf("no supported parameteters for manual exposure min\n\t");
    }

    if (params.get(KEY_SUPPORTED_MANUAL_EXPOSURE_MAX) != NULL) {
        manualExpMax = params.getInt(KEY_SUPPORTED_MANUAL_EXPOSURE_MAX);
    } else {
        printf("no supported parameteters for manual exposure max\n\t");
    }

    if (params.get(KEY_SUPPORTED_MANUAL_EXPOSURE_STEP) != NULL) {
        manualExpStep = params.getInt(KEY_SUPPORTED_MANUAL_EXPOSURE_STEP);
    } else {
        printf("no supported parameteters for manual exposure step\n\t");
    }

    if (params.get(KEY_SUPPORTED_MANUAL_GAIN_ISO_MIN) != NULL) {
        manualGainMin = params.getInt(KEY_SUPPORTED_MANUAL_GAIN_ISO_MIN);
    } else {
        printf("no supported parameteters for manual gain min\n\t");
    }

    if (params.get(KEY_SUPPORTED_MANUAL_GAIN_ISO_MAX) != NULL) {
        manualGainMax = params.getInt(KEY_SUPPORTED_MANUAL_GAIN_ISO_MAX);
    } else {
        printf("no supported parameteters for manual gain max\n\t");
    }

    if (params.get(KEY_SUPPORTED_MANUAL_GAIN_ISO_STEP) != NULL) {
        manualGainStep = params.getInt(KEY_SUPPORTED_MANUAL_GAIN_ISO_STEP);
    } else {
        printf("no supported parameteters for manual gain step\n\t");
    }

    return 0;
}

void getSizeParametersFromCapabilities() {
    if(!stereoMode) {
        if (params.get(CameraParameters::KEY_SUPPORTED_PICTURE_SIZES) != NULL) {
            strcpy(captureSizeStr, params.get(CameraParameters::KEY_SUPPORTED_PICTURE_SIZES));
        } else {
            printf("Picture sizes are not supported\n");
        }

        if (params.get(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES) != NULL) {
            strcpy(previewSizeStr, params.get(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES));
            strcpy(VcaptureSizeStr, params.get(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES));
        } else {
            printf("Preview sizes are not supported\n");
        }
    } else { //stereo
        if(strcmp(stereoLayout[stereoLayoutIDX],"tb-full") == 0)
        {
                    if (params.get(KEY_SUPPORTED_PREVIEW_TOPBOTTOM_SIZES) != NULL) {
                        strcpy(previewSizeStr, params.get(KEY_SUPPORTED_PREVIEW_TOPBOTTOM_SIZES));
                        strcpy(VcaptureSizeStr, params.get(KEY_SUPPORTED_PREVIEW_TOPBOTTOM_SIZES));
                    } else {
                        printf("Preview sizes are not supported\n");
                    }
        }
        else  if(strcmp(stereoLayout[stereoLayoutIDX],"ss-full") == 0)
        {
                    if (params.get(KEY_SUPPORTED_PREVIEW_SIDEBYSIDE_SIZES) != NULL) {
                        strcpy(previewSizeStr, params.get(KEY_SUPPORTED_PREVIEW_SIDEBYSIDE_SIZES));
                        strcpy(VcaptureSizeStr, params.get(KEY_SUPPORTED_PREVIEW_SIDEBYSIDE_SIZES));
                    } else {
                        printf("Preview sizes are not supported\n");
                    }
        }
        else  if(strcmp(stereoLayout[stereoLayoutIDX],"tb-subsampled") == 0)
        {
                    if (params.get(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES) != NULL) {
                        strcpy(previewSizeStr, params.get(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES));
                        strcpy(VcaptureSizeStr, params.get(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES));
                    } else {
                        printf("Preview sizes are not supported\n");
                    }
        }
        else  if(strcmp(stereoLayout[stereoLayoutIDX],"ss-subsampled") == 0)
        {
                    if (params.get(KEY_SUPPORTED_PREVIEW_SUBSAMPLED_SIZES) != NULL) {
                        strcpy(previewSizeStr, params.get(KEY_SUPPORTED_PREVIEW_SUBSAMPLED_SIZES));
                        strcpy(VcaptureSizeStr, params.get(KEY_SUPPORTED_PREVIEW_SUBSAMPLED_SIZES));
                    } else {
                        printf("Preview sizes are not supported\n");
                    }
        }
        else
        {
                    printf("Preview sizes are not supported\n");
        }
        if(strcmp(stereoCapLayout[stereoCapLayoutIDX],"tb-full") == 0)
        {
                    if (params.get(KEY_SUPPORTED_PICTURE_TOPBOTTOM_SIZES) != NULL) {
                        strcpy(captureSizeStr, params.get(KEY_SUPPORTED_PICTURE_TOPBOTTOM_SIZES));
                    } else {
                        printf("Picture sizes are not supported\n");
                    }
        }
        else  if(strcmp(stereoCapLayout[stereoCapLayoutIDX],"ss-full") == 0)
        {
                    if (params.get(KEY_SUPPORTED_PICTURE_SIDEBYSIDE_SIZES) != NULL) {
                        strcpy(captureSizeStr, params.get(KEY_SUPPORTED_PICTURE_SIDEBYSIDE_SIZES));
                    } else {
                        printf("Picture sizes are not supported\n");
                    }
        }
        else
        {
                    printf("Picture sizes are not supported\n");
        }

    }
    getSupportedParametersCaptureSize(captureSizeStr, &numcaptureSize, captureSize, length_capture_Size);
    getSupportedParametersPreviewSize(previewSizeStr, &numpreviewSize, previewSize, length_previewSize);
    getSupportedParametersVideoCaptureSize(VcaptureSizeStr, &numVcaptureSize, VcaptureSize, lenght_Vcapture_size);
}

int getDefaultParameter(const char* val, int numOptions,  char **array) {
    int cnt = 0;

    if ((NULL == val) || (NULL == array)) {
        printf("Some default parameters are not valid");
        return 0;
    }

    for(cnt=0;cnt<numOptions;cnt++) {
        if (NULL == array[cnt]) {
            printf("Some parameter arrays are not valid");
            continue;
        }
        if (strcmp(val, array[cnt]) ==0 ) {
            return cnt;
        }
    }
    return 0;
}

int getDefaultParameterResol(const char* val, int numOptions,  param_Array **array) {
    int cnt = 0;

    for(cnt=0;cnt<numOptions;cnt++) {
        if (strcmp(val, array[cnt]->name) ==0 ) {
            return cnt;
        }
    }
    return 0;
}

int getSupportedParameters(char* parameters, int *optionsCount, char  ***elem) {
    str = new char [400];
    param = new char [400];
    int cnt = 0;

    strcpy(str, parameters);
    param = strtok(str, ",");
    *elem = new char*[30];

    while (param != NULL) {
        (*elem)[cnt] = new char[strlen(param) + 1];
        strcpy((*elem)[cnt], param);
        param = strtok (NULL, ",");
        cnt++;
    }
    *optionsCount = cnt;
    return 0;
}

int getSupportedParametersfps(char* parameters, int *optionsCount) {
    str = new char [400];
    param = new char [400];
    int cnt = 0;
    constCnt = 0;
    rangeCnt = 0;
    strcpy(str, parameters);
    fps_const_str = new char*[32];
    fps_range_str = new char*[32];
    rangeDescription = new char*[32];
    fpsArray = new fps_Array[50];
    param = strtok(str, "(,)");

    while (param != NULL) {
        fps_const_str[constCnt] = new char;
        fps_range_str[rangeCnt] = new char;
        rangeDescription[rangeCnt] = new char;
        fpsArray[cnt].rangeMin = atoi(param);
        param = strtok (NULL, "(,)");
        fpsArray[cnt].rangeMax = atoi(param);
        param = strtok (NULL, "(,)");
        if (fpsArray[cnt].rangeMin == fpsArray[cnt].rangeMax) {
            sprintf(fps_const_str[constCnt], "%d,%d", fpsArray[cnt].rangeMin, fpsArray[cnt].rangeMax);
            constFramerate[constCnt] = fpsArray[cnt].rangeMin/1000;
            sprintf(fps_range_str[rangeCnt], "%d,%d", fpsArray[cnt].rangeMin, fpsArray[cnt].rangeMax);
            sprintf(rangeDescription[rangeCnt], "[%d:%d]", fpsArray[cnt].rangeMin/1000, fpsArray[cnt].rangeMax/1000);
            constCnt ++;
            rangeCnt ++;

        } else {
            sprintf(fps_range_str[rangeCnt], "%d,%d", fpsArray[cnt].rangeMin, fpsArray[cnt].rangeMax);
            sprintf(rangeDescription[rangeCnt], "[%d:%d]", fpsArray[cnt].rangeMin/1000, fpsArray[cnt].rangeMax/1000);
            rangeCnt ++;
        }

        cnt++;
    }
    *optionsCount = cnt;
    return 0;
}


int getSupportedParametersCaptureSize(char* parameters, int *optionsCount, param_Array array[], int arraySize) {
    str = new char [400];
    param = new char [400];
    int cnt = 0;
    strcpy(str, parameters);
    param = strtok(str, ",x");
    capture_Array = new param_Array*[50];
    while (param != NULL) {

        capture_Array[cnt] = new param_Array;
        capture_Array[cnt]->width = atoi(param);
        param = strtok (NULL, ",x");
        capture_Array[cnt]->height = atoi(param);
        param = strtok (NULL, ",x");

        int x = getSupportedParametersNames(capture_Array[cnt]->width,
                capture_Array[cnt]->height, array, arraySize);

        if (x > -1) {
            strcpy(capture_Array[cnt]->name, array[x].name);
        } else {
            strcpy(capture_Array[cnt]->name, "Needs to be added/Not supported");
        }

        cnt++;
    }

    *optionsCount = cnt;
    return 0;
}

int getSupportedParametersVideoCaptureSize(char* parameters, int *optionsCount, param_Array array[], int arraySize) {
    str = new char [800];
    param = new char [800];
    int cnt = 0;
    strcpy(str, parameters);
    param = strtok(str, ",x");
    Vcapture_Array = new param_Array*[100];
    while (param != NULL) {

        Vcapture_Array[cnt] = new param_Array;
        Vcapture_Array[cnt]->width = atoi(param);
        param = strtok (NULL, ",x");
        Vcapture_Array[cnt]->height = atoi(param);
        param = strtok (NULL, ",x");

        int x = getSupportedParametersNames(Vcapture_Array[cnt]->width,
                Vcapture_Array[cnt]->height, array, arraySize);

        if (x > -1) {
            strcpy(Vcapture_Array[cnt]->name, array[x].name);
        } else {
            strcpy(Vcapture_Array[cnt]->name, "Needs to be added/Not supported");
        }

        cnt++;
    }

    *optionsCount = cnt;
    return 0;
}

int getSupportedParametersPreviewSize(char* parameters, int *optionsCount, param_Array array[], int arraySize) {
    str = new char [500];
    param = new char [500];
    int cnt = 0;
    strcpy(str, parameters);
    param = strtok(str, ",x");
    preview_Array = new param_Array*[60];
    while (param != NULL) {
        preview_Array[cnt] = new param_Array;
        preview_Array[cnt]->width = atoi(param);
        param = strtok (NULL, ",x");
        preview_Array[cnt]->height = atoi(param);
        param = strtok (NULL, ",x");

        int x = getSupportedParametersNames(preview_Array[cnt]->width,
                preview_Array[cnt]->height, array, arraySize);
        if (x > -1) {
            strcpy(preview_Array[cnt]->name, array[x].name);
        } else {
            strcpy(preview_Array[cnt]->name, "Needs to be added/Not supported");
        }

        cnt++;
    }

    *optionsCount = cnt;
    return 0;
}

int getSupportedParametersThumbnailSize(char* parameters, int *optionsCount, param_Array array[], int arraySize) {
    str = new char [500];
    param = new char [500];
    int cnt = 0;
    strcpy(str, parameters);
    param = strtok(str, ",x");
    thumbnail_Array = new param_Array*[60];
    while (param != NULL) {
        thumbnail_Array[cnt] = new param_Array;
        thumbnail_Array[cnt]->width = atoi(param);
        param = strtok (NULL, ",x");
        thumbnail_Array[cnt]->height = atoi(param);
        param = strtok (NULL, ",x");

        int x = getSupportedParametersNames(thumbnail_Array[cnt]->width,
                thumbnail_Array[cnt]->height, array, arraySize);
        if (x > -1) {
            strcpy(thumbnail_Array[cnt]->name, array[x].name);
        } else {
            strcpy(thumbnail_Array[cnt]->name, "Needs to be added/Not supported");
        }

        cnt++;
    }

    *optionsCount = cnt;
    return 0;
}

int getSupportedParametersNames(int width, int height, param_Array array[], int arraySize) {
    for (int i = 0; i<arraySize; i++) {

        if ((width == array[i].width) && (height == array[i].height)) {
            return (i);
        }
    }
    return -1;
}

int deleteAllocatedMemory() {
    int i;

    for (i=0; i<numAntibanding; i++){
        delete [] antiband[i];
    }




    for (i=0; i<numEffects; i++){
        delete [] effectss[i];
    }

    for (i=0; i<numExposureMode; i++){
        delete [] exposureMode[i];
    }

    for (i=0; i<numawb; i++) {
        delete [] awb[i];
    }

    for (i=0; i<numscene; i++){
        delete [] scene[i];
    }

    for (i=0; i<numfocus; i++){
        delete [] focus[i];
    }

    for (i=0; i<numflash; i++){
        delete [] flash[i];
    }

    for (i=0; i<numpreviewSize; i++){
        delete [] preview_Array[i];
    }

    for (i=0; i<numcaptureSize; i++){
        delete [] capture_Array[i];
    }

    for (i=0; i<numVcaptureSize; i++){
        delete [] Vcapture_Array[i];
    }

    for (i=0; i<numthumbnailSize; i++){
        delete [] thumbnail_Array[i];
    }

    for (i=0; i<constCnt; i++){
        delete [] fps_const_str[i];
    }

    for (i=0; i<rangeCnt; i++){
        delete [] fps_range_str[i];
    }

    for (i=0; i<rangeCnt; i++){
        delete [] rangeDescription[i];
    }

    for (i=0; i<numpreviewFormat; i++){
        delete [] previewFormatArray[i];
    }

    for (i=0; i<numpictureFormat; i++){
        delete [] pictureFormatArray[i];
    }

    for (i=0; i<nummodevalues; i++){
        delete [] modevalues[i];
    }

    if (numLay) {
        for (i = 0; i < numLay; i++) {
            delete [] stereoLayout[i];
        }
        numLay = 0;
    }

    if (numCLay) {
        for (i = 0; i < numCLay; i++) {
            delete [] stereoCapLayout[i];
        }
        numCLay = 0;
    }

    delete [] antibandStr;
    delete [] effectssStr;
    delete [] exposureModeStr;
    delete [] awbStr;
    delete [] sceneStr;
    delete [] focusStr;
    delete [] flashStr;
    delete [] previewSizeStr;
    delete [] captureSizeStr;
    delete [] VcaptureSizeStr;
    delete [] thumbnailSizeStr;
    delete [] fpsstr;
    delete [] previewFormatStr;
    delete [] pictureFormatStr;
    delete [] fpsArray;
    delete [] vstabstr;
    delete [] vnfstr;
    delete [] isoModeStr;
    delete [] AutoExposureLockstr;
    delete [] AutoWhiteBalanceLockstr;
    delete [] zoomstr;
    delete [] smoothzoomstr;
    delete [] modevaluesstr;
    delete [] videosnapshotstr;
    delete [] autoconvergencestr;
    delete [] layoutstr;
    delete [] capturelayoutstr;

    // Release buffer sources if any
    if (bufferSourceOutputThread.get()) {
        bufferSourceOutputThread->requestExit();
        bufferSourceOutputThread.clear();
    }
    if ( bufferSourceInput.get() ) {
        bufferSourceInput.clear();
    }

    return 0;
}

int trySetVideoStabilization(bool toggle) {
    if (strcmp(vstabstr, "true") == 0) {
        params.set(params.KEY_VIDEO_STABILIZATION, toggle ? params.TRUE : params.FALSE);
        return 0;
    }
    return 0;
}

int trySetVideoNoiseFilter(bool toggle) {
    if (strcmp(vnfstr, "true") == 0) {
        params.set("vnf", toggle ? params.TRUE : params.FALSE);
        return 0;
    }
    return 0;
}

int trySetAutoExposureLock(bool toggle) {
    if (strcmp(AutoExposureLockstr, "true") == 0) {
        params.set(KEY_AUTO_EXPOSURE_LOCK, toggle ? params.TRUE : params.FALSE);
        return 0;
    }
    return 0;
}

int trySetAutoWhiteBalanceLock(bool toggle) {
    if (strcmp(AutoWhiteBalanceLockstr, "true") == 0) {
        params.set(KEY_AUTO_WHITEBALANCE_LOCK, toggle ? params.TRUE : params.FALSE);
        return 0;
    }
    return 0;
}

bool isRawPixelFormat (const char *format) {
    bool ret = false;
    if ((0 == strcmp (format, CameraParameters::PIXEL_FORMAT_YUV422I)) ||
        (0 == strcmp (format, CameraParameters::PIXEL_FORMAT_YUV420SP)) ||
        (0 == strcmp (format, CameraParameters::PIXEL_FORMAT_RGB565)) ||
        (0 == strcmp (format, CameraParameters::PIXEL_FORMAT_BAYER_RGGB))) {
        ret = true;
    }
    return ret;
}

void stopPreview() {
    if ( hardwareActive ) {
        camera->stopPreview();

        destroyPreviewSurface();

        previewRunning  = false;
        reSizePreview = true;
    }
}

void initDefaults() {

    struct CameraInfo cameraInfo;

    camera->getCameraInfo(camera_index, &cameraInfo);
    if (cameraInfo.facing == CAMERA_FACING_FRONT) {
        rotation = cameraInfo.orientation;
    } else {  // back-facing
        rotation = cameraInfo.orientation;
    }

    antibanding_mode = getDefaultParameter("off", numAntibanding, antiband);
    focus_mode = getDefaultParameter("auto", numfocus, focus);
    fpsRangeIdx = getDefaultParameter("5000,30000", rangeCnt, fps_range_str);
    afTimeoutIdx = 0;
    previewSizeIDX = getDefaultParameterResol("VGA", numpreviewSize, preview_Array);
    captureSizeIDX = getDefaultParameterResol("12MP", numcaptureSize, capture_Array);
    frameRateIDX = getDefaultParameter("30000,30000", constCnt, fps_const_str);
    VcaptureSizeIDX = getDefaultParameterResol("HD", numVcaptureSize, Vcapture_Array);
    VbitRateIDX = 0;
    thumbSizeIDX = getDefaultParameterResol("VGA", numthumbnailSize, thumbnail_Array);
    compensation = 0.0;
    awb_mode = getDefaultParameter("auto", numawb, awb);
    effects_mode = getDefaultParameter("none", numEffects, effectss);
    scene_mode = getDefaultParameter("auto", numscene, scene);
    caf_mode = 0;

    shotConfigFlush = false;
    streamCapture = false;
    vstabtoggle = false;
    vnftoggle = false;
    AutoExposureLocktoggle = false;
    AutoWhiteBalanceLocktoggle = false;
    faceDetectToggle = false;
    metaDataToggle = false;
    expBracketIdx = BRACKETING_IDX_DEFAULT;
    flashIdx = getDefaultParameter("off", numflash, flash);
    previewRotation = 0;
    zoomIDX = 0;
    videoCodecIDX = 0;
    gbceIDX = 0;
    glbceIDX = 0;
    contrast = 100;
#ifdef TARGET_OMAP4
    ///Temporary fix until OMAP3 and OMAP4 3A values are synced
    brightness = 50;
    sharpness = 100;
#else
    brightness = 100;
    sharpness = 0;
#endif
    saturation = 100;
    iso_mode = getDefaultParameter("auto", numisoMode, isoMode);
    capture_mode = getDefaultParameter("high-quality", nummodevalues, modevalues);
    exposure_mode = getDefaultParameter("auto", numExposureMode, exposureMode);
    ippIDX = 0;
    ippIDX_old = ippIDX;
    jpegQuality = 85;
    bufferStarvationTest = 0;
    meter_mode = 0;
    previewFormat = getDefaultParameter("yuv420sp", numpreviewFormat, previewFormatArray);
    pictureFormat = getDefaultParameter("jpeg", numpictureFormat, pictureFormatArray);
    stereoCapLayoutIDX = 0;
    stereoLayoutIDX = 1;
    manualConv = 0;
    manualExp = manualExpMin;
    manualGain = manualGainMin;

    algoFixedGammaIDX = 1;
    algoNSF1IDX = 1;
    algoNSF2IDX = 1;
    algoSharpeningIDX = 1;
    algoThreeLinColorMapIDX = 1;
    algoGICIDX = 1;

    params.set(params.KEY_VIDEO_STABILIZATION, params.FALSE);
    params.set("vnf", params.FALSE);
    params.setPreviewSize(preview_Array[previewSizeIDX]->width, preview_Array[previewSizeIDX]->height);
    params.setPictureSize(capture_Array[captureSizeIDX]->width, capture_Array[captureSizeIDX]->height);
    params.set(CameraParameters::KEY_ROTATION, rotation);
    params.set(KEY_SENSOR_ORIENTATION, previewRotation);
    params.set(KEY_COMPENSATION, (int) (compensation * 10));
    params.set(params.KEY_WHITE_BALANCE, awb[awb_mode]);
    params.set(KEY_MODE, (modevalues[capture_mode]));
    params.set(params.KEY_SCENE_MODE, scene[scene_mode]);
    params.set(KEY_CAF, caf_mode);
    params.set(KEY_ISO, isoMode[iso_mode]);
    params.set(KEY_GBCE, gbce[gbceIDX]);
    params.set(KEY_GLBCE, gbce[glbceIDX]);
    params.set(KEY_SHARPNESS, sharpness);
    params.set(KEY_CONTRAST, contrast);
    params.set(CameraParameters::KEY_ZOOM, zoom[zoomIDX].idx);
    params.set(KEY_EXPOSURE, exposureMode[exposure_mode]);
    params.set(KEY_BRIGHTNESS, brightness);
    params.set(KEY_SATURATION, saturation);
    params.set(params.KEY_EFFECT, effectss[effects_mode]);
    params.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, fps_const_str[frameRateIDX]);
    params.set(params.KEY_ANTIBANDING, antiband[antibanding_mode]);
    params.set(params.KEY_FOCUS_MODE, focus[focus_mode]);
    params.set(KEY_IPP, ipp_mode[ippIDX]);
    params.set(CameraParameters::KEY_JPEG_QUALITY, jpegQuality);
    params.setPreviewFormat(previewFormatArray[previewFormat]);
    params.setPictureFormat(pictureFormatArray[pictureFormat]);
    params.set(KEY_BUFF_STARV, bufferStarvationTest);
    params.set(KEY_METERING_MODE, metering[meter_mode]);
    params.set(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH, thumbnail_Array[thumbSizeIDX]->width);
    params.set(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, thumbnail_Array[thumbSizeIDX]->height);
    params.set(KEY_MANUAL_CONVERGENCE, manualConv);
    params.set(KEY_MANUAL_EXPOSURE, manualExp);
    params.set(KEY_MANUAL_GAIN_ISO, manualGain);
    params.set(KEY_MANUAL_EXPOSURE_RIGHT, manualExp);
    params.set(KEY_MANUAL_GAIN_ISO_RIGHT, manualGain);
    params.set(KEY_S3D2D_PREVIEW_MODE, "off");
    params.set(KEY_EXIF_MODEL, MODEL);
    params.set(KEY_EXIF_MAKE, MAKE);

    setDefaultExpGainPreset(shotParams, expBracketIdx);
}

void setDefaultExpGainPreset(ShotParameters &params, int idx) {
    if ( ((int)ARRAY_SIZE(expBracketing) > idx) && (0 <= idx) ) {
        setExpGainPreset(params, expBracketing[idx].value, false, expBracketing[idx].param_type, shotConfigFlush);
    } else {
        printf("setDefaultExpGainPreset: Index (%d) is out of range 0 ~ %u\n", idx, ARRAY_SIZE(expBracketing) - 1);
    }
}

void setSingleExpGainPreset(ShotParameters &params, int idx, int exp, int gain) {
    String8 val;

    if (PARAM_EXP_BRACKET_PARAM_PAIR == expBracketing[idx].param_type) {
        val.append("(");
    }

    if (PARAM_EXP_BRACKET_VALUE_REL == expBracketing[idx].value_type) {
        val.appendFormat("%+d", exp);
    } else {
        val.appendFormat("%u", (unsigned int) exp);
    }

    if (PARAM_EXP_BRACKET_PARAM_PAIR == expBracketing[idx].param_type) {
        if (PARAM_EXP_BRACKET_VALUE_REL == expBracketing[idx].value_type) {
            val.appendFormat(",%+d)", gain);
        } else {
            val.appendFormat(",%u)", (unsigned int) gain);
        }
    }

    if (PARAM_EXP_BRACKET_APPLY_FORCED == expBracketing[idx].apply_type) {
        val.append("F");
    }

    setExpGainPreset(params, val, false, expBracketing[idx].param_type, false);
}

void setExpGainPreset(ShotParameters &params, const char *input, bool force, param_ExpBracketParamType_t type, bool flush) {
    const char *startPtr = NULL;
    size_t i = 0;

    if (NULL == input) {
        printf("setExpGainPreset: missing input string\n");
    } else if ( (force && (NULL == strpbrk(input, "()"))) ||
         (PARAM_EXP_BRACKET_PARAM_COMP == type) ) {
        // parse for the number of inputs (count the number of ',' + 1)
        startPtr = strchr(input, ',');
        while (startPtr != NULL) {
            i++;
            startPtr = strchr(startPtr + 1, ',');
        }
        i++;
        printf("relative EV input: \"%s\"\nnumber of relative EV values: %d (%s)\n",
               input, i, flush ? "reset" : "append");
        burst = i;
        burstCount = i;
        params.set(ShotParameters::KEY_BURST, burst);
        params.set(ShotParameters::KEY_EXP_COMPENSATION, input);
        params.remove(ShotParameters::KEY_EXP_GAIN_PAIRS);
        params.set(ShotParameters::KEY_FLUSH_CONFIG,
                       flush ? ShotParameters::TRUE : ShotParameters::FALSE);
    } else if ( force || (PARAM_EXP_BRACKET_PARAM_PAIR == type) ) {
        // parse for the number of inputs (count the number of '(')
        startPtr = strchr(input, '(');
        while (startPtr != NULL) {
            i++;
            startPtr = strchr(startPtr + 1, '(');
        }
        printf("absolute exposure,gain input: \"%s\"\nNumber of brackets: %d (%s)\n",
               input, i, flush ? "reset" : "append");
        burst = i;
        burstCount = i;
        params.set(ShotParameters::KEY_BURST, burst);
        params.set(ShotParameters::KEY_EXP_GAIN_PAIRS, input);
        params.remove(ShotParameters::KEY_EXP_COMPENSATION);
        params.set(ShotParameters::KEY_FLUSH_CONFIG,
                       flush ? ShotParameters::TRUE : ShotParameters::FALSE);
    } else {
        printf("no bracketing input: \"%s\"\n", input);
        params.remove(ShotParameters::KEY_EXP_GAIN_PAIRS);
        params.remove(ShotParameters::KEY_EXP_COMPENSATION);
        params.remove(ShotParameters::KEY_BURST);
        params.remove(ShotParameters::KEY_FLUSH_CONFIG);
    }
}

void calcNextSingleExpGainPreset(int idx, int &exp, int &gain) {
    if (PARAM_EXP_BRACKET_VALUE_ABS == expBracketing[idx].value_type) {
        // absolute
        if ( (0 == exp) && (0 == gain) ) {
            exp=100;
            gain = 150;
            printf("Streaming: Init default absolute exp./gain: %d,%d\n", exp, gain);
        }

        exp *= 2;
        if (1000000 < exp) {
            exp = 100;
            gain += 50;
            if(400 < gain) {
                gain = 50;
            }
        }
    } else {
        // relative
        exp += 50;
        if (200 < exp) {
            exp = -200;
            gain += 50;
            if(200 < gain) {
                gain = -200;
            }
        }
    }
}

void updateShotConfigFlushParam() {
    // Will update flush shot config parameter if already present
    // Otherwise, keep empty (will be set later in setExpGainPreset())
    if (NULL != shotParams.get(ShotParameters::KEY_FLUSH_CONFIG)) {
        shotParams.set(ShotParameters::KEY_FLUSH_CONFIG,
                       shotConfigFlush ? ShotParameters::TRUE : ShotParameters::FALSE);
    }
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

        case 'q':
            return -1;

        default:
            print_menu = 0;
            break;
    }

    return 0;
}

int menu_algo() {
    char ch;

    if (print_menu) {
        printf("\n\n== ALGO ENABLE/DISABLE MENU ============\n\n");
        printf("   a.                      Fixed Gamma: %s\n", algoFixedGamma[algoFixedGammaIDX]);
        printf("   s.                             NSF1: %s\n", algoNSF1[algoNSF1IDX]);
        printf("   d.                             NSF2: %s\n", algoNSF2[algoNSF2IDX]);
        printf("   f.                       Sharpening: %s\n", algoSharpening[algoSharpeningIDX]);
        printf("   g.                 Color Conversion: %s\n", algoThreeLinColorMap[algoThreeLinColorMapIDX]);
        printf("   h.      Green Inballance Correction: %s\n", algoGIC[algoGICIDX]);
        printf("\n");
        printf("   q. Return to main menu\n");
        printf("\n");
        printf("   Choice: ");
    }

    ch = getchar();
    printf("%c", ch);

    print_menu = 1;

    switch (ch) {

        case 'a':
        case 'A':
            algoFixedGammaIDX++;
            algoFixedGammaIDX %= ARRAY_SIZE(algoFixedGamma);
            params.set(KEY_ALGO_FIXED_GAMMA, (algoFixedGamma[algoFixedGammaIDX]));

            if ( hardwareActive )
                camera->setParameters(params.flatten());

            break;

        case 's':
        case 'S':
            algoNSF1IDX++;
            algoNSF1IDX %= ARRAY_SIZE(algoNSF1);
            params.set(KEY_ALGO_NSF1, (algoNSF1[algoNSF1IDX]));

            if ( hardwareActive )
                camera->setParameters(params.flatten());

            break;

        case 'd':
        case 'D':
            algoNSF2IDX++;
            algoNSF2IDX %= ARRAY_SIZE(algoNSF2);
            params.set(KEY_ALGO_NSF2, (algoNSF2[algoNSF2IDX]));

            if ( hardwareActive )
                camera->setParameters(params.flatten());

            break;

        case 'f':
        case 'F':
            algoSharpeningIDX++;
            algoSharpeningIDX %= ARRAY_SIZE(algoSharpening);
            params.set(KEY_ALGO_SHARPENING, (algoSharpening[algoSharpeningIDX]));

            if ( hardwareActive )
                camera->setParameters(params.flatten());

            break;

        case 'g':
        case 'G':
            algoThreeLinColorMapIDX++;
            algoThreeLinColorMapIDX %= ARRAY_SIZE(algoThreeLinColorMap);
            params.set(KEY_ALGO_THREELINCOLORMAP, (algoThreeLinColorMap[algoThreeLinColorMapIDX]));

            if ( hardwareActive )
                camera->setParameters(params.flatten());

            break;

        case 'h':
        case 'H':
            algoGICIDX++;
            algoGICIDX %= ARRAY_SIZE(algoGIC);
            params.set(KEY_ALGO_GIC, (algoGIC[algoGICIDX]));

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
    char area1[MAX_LINES][MAX_SYMBOLS+1];
    char area2[MAX_LINES][MAX_SYMBOLS+1];
    int j = 0;
    int k = 0;
    const char *valstr = NULL;
    struct CameraInfo cameraInfo;
    bool queueEmpty = true;

    memset(area1, '\0', MAX_LINES*(MAX_SYMBOLS+1));
    memset(area2, '\0', MAX_LINES*(MAX_SYMBOLS+1));

    if (print_menu) {

        printf("\n========================================= FUNCTIONAL TEST MENU =========================================\n");

        snprintf(area1[j++], MAX_SYMBOLS, "   START / STOP / GENERAL SERVICES");
        snprintf(area1[j++], MAX_SYMBOLS, "   -----------------------------");
        snprintf(area1[j++], MAX_SYMBOLS, "A  Select Camera %s", cameras[camera_index]);
        snprintf(area1[j++], MAX_SYMBOLS, "[. Resume Preview after capture");
        snprintf(area1[j++], MAX_SYMBOLS, "0. Reset to defaults");
        snprintf(area1[j++], MAX_SYMBOLS, "q. Quit");
        snprintf(area1[j++], MAX_SYMBOLS, "@. Disconnect and Reconnect to CameraService");
        snprintf(area1[j++], MAX_SYMBOLS, "/. Enable/Disable showfps: %s", ((showfps)? "Enabled":"Disabled"));
        snprintf(area1[j++], MAX_SYMBOLS, "a. GEO tagging settings menu");
        snprintf(area1[j++], MAX_SYMBOLS, "E. Camera Capability Dump");

        snprintf(area1[j++], MAX_SYMBOLS, "        PREVIEW SUB MENU");
        snprintf(area1[j++], MAX_SYMBOLS, "   -----------------------------");
        snprintf(area1[j++], MAX_SYMBOLS, "1. Start Preview");
        snprintf(area1[j++], MAX_SYMBOLS, "2. Stop Preview");
        snprintf(area1[j++], MAX_SYMBOLS, "~. Preview format %s", previewFormatArray[previewFormat]);
#if defined(OMAP_ENHANCEMENT) && defined(TARGET_OMAP3)
        snprintf(area1[j++], MAX_SYMBOLS, "4. Preview size: %4d x %4d - %s",preview_Array[previewSizeIDX]->width,  preview_Array[previewSizeIDX]->height, preview_Array[previewSizeIDX]->name);
#else
        snprintf(area1[j++], MAX_SYMBOLS, "4. Preview size: %4d x %4d - %s",preview_Array[previewSizeIDX]->width, stereoMode ? preview_Array[previewSizeIDX]->height*2 : preview_Array[previewSizeIDX]->height, preview_Array[previewSizeIDX]->name);
#endif
        snprintf(area1[j++], MAX_SYMBOLS, "R. Preview framerate range: %s", rangeDescription[fpsRangeIdx]);
        snprintf(area1[j++], MAX_SYMBOLS, "&. Dump a preview frame");
        if (stereoMode) {
            snprintf(area1[j++], MAX_SYMBOLS, "_. Auto Convergence mode: %s", autoconvergencemode[AutoConvergenceModeIDX]);
            snprintf(area1[j++], MAX_SYMBOLS, "^. Manual Convergence Value: %d\n", manualConv);
            snprintf(area1[j++], MAX_SYMBOLS, "L. Stereo Preview Layout: %s\n", stereoLayout[stereoLayoutIDX]);
            snprintf(area1[j++], MAX_SYMBOLS, ".  Stereo Capture Layout: %s\n", stereoCapLayout[stereoCapLayoutIDX]);
        }
        snprintf(area1[j++], MAX_SYMBOLS, "{. 2D Preview in 3D Stereo Mode: %s", params.get(KEY_S3D2D_PREVIEW_MODE));

        snprintf(area1[j++], MAX_SYMBOLS, "     IMAGE CAPTURE SUB MENU");
        snprintf(area1[j++], MAX_SYMBOLS, "   -----------------------------");
        snprintf(area1[j++], MAX_SYMBOLS, "p. Take picture/Full Press");
        snprintf(area1[j++], MAX_SYMBOLS, "n. Flush shot config queue: %s", shotConfigFlush ? "On" : "Off");
        snprintf(area1[j++], MAX_SYMBOLS, "H. Exposure Bracketing: %s", expBracketing[expBracketIdx].desc);
        snprintf(area1[j++], MAX_SYMBOLS, "U. Temporal Bracketing:   %s", tempBracketing[tempBracketIdx]);
        snprintf(area1[j++], MAX_SYMBOLS, "W. Temporal Bracketing Range: [-%d;+%d]", tempBracketRange, tempBracketRange);
        snprintf(area1[j++], MAX_SYMBOLS, "$. Picture Format: %s", pictureFormatArray[pictureFormat]);
        snprintf(area1[j++], MAX_SYMBOLS, "3. Picture Rotation:       %3d degree", rotation );
        snprintf(area1[j++], MAX_SYMBOLS, "V. Preview Rotation:       %3d degree", previewRotation );
        snprintf(area1[j++], MAX_SYMBOLS, "5. Picture size:   %4d x %4d - %s",capture_Array[captureSizeIDX]->width, capture_Array[captureSizeIDX]->height,              capture_Array[captureSizeIDX]->name);
        snprintf(area1[j++], MAX_SYMBOLS, "i. ISO mode:       %s", isoMode[iso_mode]);
        snprintf(area1[j++], MAX_SYMBOLS, ",  Manual gain iso value  = %d\n", manualGain);
        snprintf(area1[j++], MAX_SYMBOLS, "u. Capture Mode:   %s", modevalues[capture_mode]);
        snprintf(area1[j++], MAX_SYMBOLS, "k. IPP Mode:       %s", ipp_mode[ippIDX]);
        snprintf(area1[j++], MAX_SYMBOLS, "K. GBCE: %s", gbce[gbceIDX]);
        snprintf(area1[j++], MAX_SYMBOLS, "O. GLBCE %s", gbce[glbceIDX]);
        snprintf(area1[j++], MAX_SYMBOLS, "o. Jpeg Quality:   %d", jpegQuality);
        snprintf(area1[j++], MAX_SYMBOLS, "#. Burst Images:  %3d", burst);
        snprintf(area1[j++], MAX_SYMBOLS, ":. Thumbnail Size:  %4d x %4d - %s",thumbnail_Array[thumbSizeIDX]->width, thumbnail_Array[thumbSizeIDX]->height, thumbnail_Array[thumbSizeIDX]->name);
        snprintf(area1[j++], MAX_SYMBOLS, "': Thumbnail Quality %d", thumbQuality);

        snprintf(area2[k++], MAX_SYMBOLS, "     VIDEO CAPTURE SUB MENU");
        snprintf(area2[k++], MAX_SYMBOLS, "   -----------------------------");
        snprintf(area2[k++], MAX_SYMBOLS, "6. Start Video Recording");
        snprintf(area2[k++], MAX_SYMBOLS, "2. Stop Recording");
        snprintf(area2[k++], MAX_SYMBOLS, "l. Video Capture resolution:   %4d x %4d - %s",Vcapture_Array[VcaptureSizeIDX]->width,Vcapture_Array[VcaptureSizeIDX]->height, Vcapture_Array[VcaptureSizeIDX]->name);
        snprintf(area2[k++], MAX_SYMBOLS, "]. Video Bit rate :  %s", VbitRate[VbitRateIDX].desc);
        snprintf(area2[k++], MAX_SYMBOLS, "9. Video Codec:    %s", videoCodecs[videoCodecIDX].desc);
        snprintf(area2[k++], MAX_SYMBOLS, "D. Audio Codec:    %s", audioCodecs[audioCodecIDX].desc);
        snprintf(area2[k++], MAX_SYMBOLS, "v. Output Format:  %s", outputFormat[outputFormatIDX].desc);
        snprintf(area2[k++], MAX_SYMBOLS, "r. Framerate:     %d", constFramerate[frameRateIDX]);
        snprintf(area2[k++], MAX_SYMBOLS, "*. Start Video Recording dump ( 1 raw frame )");
        snprintf(area2[k++], MAX_SYMBOLS, "B  VNF              %s", vnftoggle? "On" : "Off");
        snprintf(area2[k++], MAX_SYMBOLS, "C  VSTAB              %s", vstabtoggle? "On" : "Off");

        snprintf(area2[k++], MAX_SYMBOLS, "       3A SETTING SUB MENU");
        snprintf(area2[k++], MAX_SYMBOLS, "   -----------------------------");
        snprintf(area2[k++], MAX_SYMBOLS, "M. Measurement Data: %s", measurement[measurementIdx]);
        snprintf(area2[k++], MAX_SYMBOLS, "F. Toggle face detection: %s", faceDetectToggle ? "On" : "Off");
        snprintf(area2[k++], MAX_SYMBOLS, "T. Toggle metadata: %s", metaDataToggle ? "On" : "Off");
        snprintf(area2[k++], MAX_SYMBOLS, "G. Touch/Focus area AF");
        snprintf(area2[k++], MAX_SYMBOLS, "y. Metering area");
        snprintf(area2[k++], MAX_SYMBOLS, "Y. Metering area center");
        snprintf(area2[k++], MAX_SYMBOLS, "N. Metering area average");
        snprintf(area2[k++], MAX_SYMBOLS, "f. Auto Focus/Half Press");
        snprintf(area2[k++], MAX_SYMBOLS, "I. AF Timeout       %s", afTimeout[afTimeoutIdx]);
        snprintf(area2[k++], MAX_SYMBOLS, "J.Flash:              %s", flash[flashIdx]);
        snprintf(area2[k++], MAX_SYMBOLS, "7. EV offset:      %4.1f", compensation);
        snprintf(area2[k++], MAX_SYMBOLS, "8. AWB mode:       %s", awb[awb_mode]);
        snprintf(area2[k++], MAX_SYMBOLS, "z. Zoom            %s", zoom[zoomIDX].zoom_description);
        snprintf(area2[k++], MAX_SYMBOLS, "Z. Smooth Zoom     %s", zoom[zoomIDX].zoom_description);
        snprintf(area2[k++], MAX_SYMBOLS, "j. Exposure        %s", exposureMode[exposure_mode]);
        snprintf(area2[k++], MAX_SYMBOLS, "Q. manual exposure value  =  %d\n", manualExp);
        snprintf(area2[k++], MAX_SYMBOLS, "e. Effect:         %s", effectss[effects_mode]);
        snprintf(area2[k++], MAX_SYMBOLS, "w. Scene:          %s", scene[scene_mode]);
        snprintf(area2[k++], MAX_SYMBOLS, "s. Saturation:     %d", saturation);
        snprintf(area2[k++], MAX_SYMBOLS, "c. Contrast:       %d", contrast);
        snprintf(area2[k++], MAX_SYMBOLS, "h. Sharpness:      %d", sharpness);
        snprintf(area2[k++], MAX_SYMBOLS, "b. Brightness:     %d", brightness);
        snprintf(area2[k++], MAX_SYMBOLS, "x. Antibanding:    %s", antiband[antibanding_mode]);
        snprintf(area2[k++], MAX_SYMBOLS, "g. Focus mode:     %s", focus[focus_mode]);
        snprintf(area2[k++], MAX_SYMBOLS, "m. Metering mode:     %s" , metering[meter_mode]);
        snprintf(area2[k++], MAX_SYMBOLS, "<. Exposure Lock:     %s", AutoExposureLocktoggle ? "On" : "Off");
        snprintf(area2[k++], MAX_SYMBOLS, ">. WhiteBalance Lock:  %s",AutoWhiteBalanceLocktoggle ? "On": "Off");
        snprintf(area2[k++], MAX_SYMBOLS, "). Mechanical Misalignment Correction:  %s",misalignmentCorrection[enableMisalignmentCorrectionIdx]);
        snprintf(area2[k++], MAX_SYMBOLS, "d. Algo enable/disable functions menu");

        printf("\n");
        for (int i=0; (i<j || i < k) && i<MAX_LINES; i++) {
            printf("%-65s \t %-65s\n", area1[i], area2[i]);
        }
        printf("   Choice:");
    }

    ch = getchar();
    printf("%c", ch);

    print_menu = 1;

    switch (ch) {

    case '_':
        AutoConvergenceModeIDX++;
        AutoConvergenceModeIDX %= numAutoConvergence;
        params.set(KEY_AUTOCONVERGENCE, autoconvergencemode[AutoConvergenceModeIDX]);
        if ( strcmp (autoconvergencemode[AutoConvergenceModeIDX], "manual") == 0) {
            params.set(KEY_MANUAL_CONVERGENCE, manualConv);
        } else {
            if ( strcmp (autoconvergencemode[AutoConvergenceModeIDX], "touch") == 0) {
                params.set(CameraParameters::KEY_METERING_AREAS, MeteringAreas);
            }
            manualConv = 0;
            params.set(KEY_MANUAL_CONVERGENCE, manualConv);
        }
        camera->setParameters(params.flatten());

        break;
    case '^':
        if ( strcmp (autoconvergencemode[AutoConvergenceModeIDX], "manual") == 0) {
            manualConv += manualConvStep;
            if( manualConv > manualConvMax) {
               manualConv = manualConvMin;
            }
            params.set(KEY_MANUAL_CONVERGENCE, manualConv);
            camera->setParameters(params.flatten());
        }
        break;
    case 'A':
        camera_index++;
        camera_index %= numCamera;
        firstTime = true;
        closeCamera();
        openCamera();
        initDefaults();


        break;
    case '[':
        if ( hardwareActive ) {
            camera->setParameters(params.flatten());
            camera->startPreview();
        }
        break;

    case '0':
        initDefaults();
        camera_index = 0;
        break;

        case '1':

            if ( startPreview() < 0 ) {
                printf("Error while starting preview\n");

                return -1;
            }

            break;

        case '2':
            if ( recordingMode ) {
                stopRecording();
                stopPreview();
                closeRecorder();
                camera->disconnect();
                camera.clear();
                camera = Camera::connect(camera_index);
                  if ( NULL == camera.get() ) {
                      sleep(1);
                      camera = Camera::connect(camera_index);
                      if ( NULL == camera.get() ) {
                          return -1;
                      }
                  }
                  camera->setListener(new CameraHandler());
                  camera->setParameters(params.flatten());
                  recordingMode = false;
            } else {
                stopPreview();
            }

            break;

        case '3':
            rotation += 90;
            rotation %= 360;
            params.set(CameraParameters::KEY_ROTATION, rotation);
            if ( hardwareActive )
                camera->setParameters(params.flatten());

            break;

        case 'V':
            previewRotation += 90;
            previewRotation %= 360;
            params.set(KEY_SENSOR_ORIENTATION, previewRotation);

            if ( hardwareActive )
                camera->setParameters(params.flatten());

            break;

        case '4':
            previewSizeIDX += 1;
            previewSizeIDX %= numpreviewSize;
            params.setPreviewSize(preview_Array[previewSizeIDX]->width, preview_Array[previewSizeIDX]->height);

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
            captureSizeIDX %= numcaptureSize;
            printf("CaptureSizeIDX %d \n", captureSizeIDX);
            params.setPictureSize(capture_Array[captureSizeIDX]->width, capture_Array[captureSizeIDX]->height);

            if ( hardwareActive )
                camera->setParameters(params.flatten());

            requestBufferSourceReset();

            break;

        case 'l':

            VcaptureSizeIDX++;
            VcaptureSizeIDX %= numVcaptureSize;
            break;

        case 'L' :
            stereoLayoutIDX++;
            stereoLayoutIDX %= numLay;

            if (stereoMode) {
                firstTimeStereo = false;
                params.set(KEY_S3D_PRV_FRAME_LAYOUT, stereoLayout[stereoLayoutIDX]);
            }

            getSizeParametersFromCapabilities();

            if (hardwareActive && previewRunning) {
                stopPreview();
                camera->setParameters(params.flatten());
                startPreview();
            } else if (hardwareActive) {
                camera->setParameters(params.flatten());
            }

            break;

        case '.' :
            stereoCapLayoutIDX++;
            stereoCapLayoutIDX %= numCLay;

            if (stereoMode) {
                firstTimeStereo = false;
                params.set(KEY_S3D_CAP_FRAME_LAYOUT, stereoCapLayout[stereoCapLayoutIDX]);
            }

            getSizeParametersFromCapabilities();

            if (hardwareActive && previewRunning) {
                stopPreview();
                camera->setParameters(params.flatten());
                startPreview();
            } else if (hardwareActive) {
                camera->setParameters(params.flatten());
            }

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
            awb_mode %= numawb;
            params.set(params.KEY_WHITE_BALANCE, awb[awb_mode]);

            if ( hardwareActive )
                camera->setParameters(params.flatten());

            break;

        case '9':
            videoCodecIDX++;
            videoCodecIDX %= ARRAY_SIZE(videoCodecs);
            break;
        case '~':
            previewFormat += 1;
            previewFormat %= numpreviewFormat;
            params.setPreviewFormat(previewFormatArray[previewFormat]);

            if ( hardwareActive )
                camera->setParameters(params.flatten());

            break;
        case '$':
            pictureFormat += 1;
            pictureFormat %= numpictureFormat;
            printf("pictureFormat %d\n", pictureFormat);
            printf("numpreviewFormat %d\n", numpictureFormat);
            params.setPictureFormat(pictureFormatArray[pictureFormat]);

            queueEmpty = true;
            if ( bufferSourceOutputThread.get() ) {
                if ( 0 < bufferSourceOutputThread->hasBuffer() ) {
                    queueEmpty = false;
                }
            }
            if ( hardwareActive && queueEmpty )
                camera->setParameters(params.flatten());

            break;

        case ':':
            thumbSizeIDX += 1;
            thumbSizeIDX %= numthumbnailSize;
            printf("ThumbnailSizeIDX %d \n", thumbSizeIDX);

            params.set(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH, thumbnail_Array[thumbSizeIDX]->width);
            params.set(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT,thumbnail_Array[thumbSizeIDX]->height);

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
            if(strcmp(vnfstr, "true") == 0) {
                if(vnftoggle == false) {
                    trySetVideoNoiseFilter(true);
                    vnftoggle = true;
                } else {
                    trySetVideoNoiseFilter(false);
                    vnftoggle = false;
                }

            }else {
                trySetVideoNoiseFilter(false);
                vnftoggle = false;
                printf("VNF is not supported\n");
            }
            if ( hardwareActive )
                camera->setParameters(params.flatten());

            break;

        case 'C' :
            if(strcmp(vstabstr, "true") == 0) {
                if(vstabtoggle == false) {
                    trySetVideoStabilization(true);
                    vstabtoggle = true;
                } else {
                    trySetVideoStabilization(false);
                    vstabtoggle = false;
                }

            } else {
                trySetVideoStabilization(false);
                vstabtoggle = false;
                printf("VSTAB is not supported\n");
            }
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
            meter_mode = (meter_mode + 1)%ARRAY_SIZE(metering);
            params.set(KEY_METERING_MODE, metering[meter_mode]);
            if ( hardwareActive )
                camera->setParameters(params.flatten());
            break;

        case 'k':
            ippIDX += 1;
            ippIDX %= ARRAY_SIZE(ipp_mode);
            ippIDX_old = ippIDX;

            params.set(KEY_IPP, ipp_mode[ippIDX]);

            if ( hardwareActive )
                camera->setParameters(params.flatten());

            requestBufferSourceReset();

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
            faceDetectToggle = !faceDetectToggle;
            if ( hardwareActive ) {
                if (faceDetectToggle)
                    camera->sendCommand(CAMERA_CMD_START_FACE_DETECTION, 0, 0);
                else
                    camera->sendCommand(CAMERA_CMD_STOP_FACE_DETECTION, 0, 0);
            }
            break;

        case 'I':
            afTimeoutIdx++;
            afTimeoutIdx %= ARRAY_SIZE(afTimeout);
            params.set(KEY_AF_TIMEOUT, afTimeout[afTimeoutIdx]);
            if ( hardwareActive )
                camera->setParameters(params.flatten());

            break;

        case 'T':
            metaDataToggle = !metaDataToggle;
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
            burstCount = burst;
            params.set(KEY_TI_BURST, burst);

            if ( hardwareActive )
                camera->setParameters(params.flatten());

            break;

        case 'J':
            flashIdx++;
            flashIdx %= numflash;
            params.set(CameraParameters::KEY_FLASH_MODE, (flash[flashIdx]));

            if ( hardwareActive )
                camera->setParameters(params.flatten());

            break;

        case 'u':
            capture_mode++;
            capture_mode %= nummodevalues;

            // HQ should always be in ldc-nsf
            // if not HQ, then return the ipp to its previous state
            if( !strcmp(modevalues[capture_mode], "high-quality") ) {
                ippIDX_old = ippIDX;
                ippIDX = 3;
                params.set(KEY_IPP, ipp_mode[ippIDX]);
                params.set(CameraParameters::KEY_RECORDING_HINT, CameraParameters::FALSE);
                previewRotation = 0;
                params.set(KEY_SENSOR_ORIENTATION, previewRotation);
            } else if ( !strcmp(modevalues[capture_mode], "video-mode") ) {
                params.set(CameraParameters::KEY_RECORDING_HINT, CameraParameters::TRUE);
                camera->getCameraInfo(camera_index, &cameraInfo);
                previewRotation = ((360-cameraInfo.orientation)%360);
                if (previewRotation >= 0 || previewRotation <=360) {
                    params.set(KEY_SENSOR_ORIENTATION, previewRotation);
                }
            } else {
                ippIDX = ippIDX_old;
                params.set(CameraParameters::KEY_RECORDING_HINT, CameraParameters::FALSE);
                previewRotation = 0;
                params.set(KEY_SENSOR_ORIENTATION, previewRotation);
            }

            params.set(KEY_MODE, (modevalues[capture_mode]));

            if ( hardwareActive ) {
                if (previewRunning) {
                    stopPreview();
                }
                camera->setParameters(params.flatten());
                // Get parameters from capabilities for the new capture mode
                params = camera->getParameters();
                getSizeParametersFromCapabilities();
                getParametersFromCapabilities();
                // Set framerate 30fps and 12MP capture resolution if available for the new capture mode.
                // If not available set framerate and capture mode under index 0 from fps_const_str and capture_Array.
                frameRateIDX = getDefaultParameter("30000,30000", constCnt, fps_const_str);
                captureSizeIDX = getDefaultParameterResol("12MP", numcaptureSize, capture_Array);
                params.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, fps_const_str[frameRateIDX]);
                params.setPictureSize(capture_Array[captureSizeIDX]->width, capture_Array[captureSizeIDX]->height);
                camera->setParameters(params.flatten());
            }

            requestBufferSourceReset();

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
            setDefaultExpGainPreset(shotParams, expBracketIdx);

            break;

        case 'n':
            if (shotConfigFlush)
                shotConfigFlush = false;
            else
                shotConfigFlush = true;

            updateShotConfigFlushParam();

            break;

        case '(':
        {
            char input[256];
            input[0] = ch;
            scanf("%254s", input+1);
            setExpGainPreset(shotParams, input, true, PARAM_EXP_BRACKET_PARAM_NONE, shotConfigFlush);
            break;
        }
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
            scene_mode %= numscene;
            params.set(params.KEY_SCENE_MODE, scene[scene_mode]);

            if ( hardwareActive )
                camera->setParameters(params.flatten());

            break;

        case 'i':
            iso_mode++;
            iso_mode %= numisoMode;
            params.set(KEY_ISO, isoMode[iso_mode]);
            if ( hardwareActive )
                camera->setParameters(params.flatten());
            break;


        case 'h':
#ifdef TARGET_OMAP4
            if ( sharpness >= 200)
#else
            if ( sharpness >= 100)
#endif
            {
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
            if(strcmp(zoomstr, "true") == 0) {
                zoomIDX++;
                zoomIDX %= ARRAY_SIZE(zoom);
                params.set(CameraParameters::KEY_ZOOM, zoom[zoomIDX].idx);

                if ( hardwareActive )
                    camera->setParameters(params.flatten());
            }
            break;

        case 'Z':
            if(strcmp(smoothzoomstr, "true") == 0) {
                zoomIDX++;
                zoomIDX %= ARRAY_SIZE(zoom);

                if ( hardwareActive )
                    camera->sendCommand(CAMERA_CMD_START_SMOOTH_ZOOM, zoom[zoomIDX].idx, 0);
            }
            break;

        case 'j':
            exposure_mode++;
            exposure_mode %= numExposureMode;
            params.set(KEY_EXPOSURE, exposureMode[exposure_mode]);
            if ( strcmp (exposureMode[exposure_mode], "manual") == 0) {
                params.set(KEY_MANUAL_EXPOSURE, manualExp);
                params.set(KEY_MANUAL_GAIN_ISO, manualGain);
                params.set(KEY_MANUAL_EXPOSURE_RIGHT, manualExp);
                params.set(KEY_MANUAL_GAIN_ISO_RIGHT, manualGain);
            }
            else
            {
                manualExp = manualExpMin;
                params.set(KEY_MANUAL_EXPOSURE, manualExp);
                params.set(KEY_MANUAL_EXPOSURE_RIGHT, manualExp);
                manualGain = manualGainMin;
                params.set(KEY_MANUAL_GAIN_ISO, manualGain);
                params.set(KEY_MANUAL_GAIN_ISO_RIGHT, manualGain);
            }

            if ( hardwareActive ) {
                camera->setParameters(params.flatten());
            }

            break;

        case 'Q':
            if ( strcmp (exposureMode[exposure_mode], "manual") == 0) {
                manualExp += manualExpStep;
                if( manualExp > manualExpMax) {
                    manualExp = manualExpMin;
                }
                params.set(KEY_MANUAL_EXPOSURE, manualExp);
                params.set(KEY_MANUAL_EXPOSURE_RIGHT, manualExp);
                camera->setParameters(params.flatten());
            }
            break;

        case ',':
            if ( strcmp (exposureMode[exposure_mode], "manual") == 0) {
                manualGain += manualGainStep;
                if( manualGain > manualGainMax) {
                   manualGain = manualGainMin;
                }
                params.set(KEY_MANUAL_GAIN_ISO, manualGain);
                params.set(KEY_MANUAL_GAIN_ISO_RIGHT, manualGain);
                camera->setParameters(params.flatten());
            }
            break;

        case 'c':
            if( contrast >= 200){
                contrast = 0;
            } else {
                contrast += 10;
            }
            params.set(KEY_CONTRAST, contrast);
            if ( hardwareActive ) {
                camera->setParameters(params.flatten());
            }
            break;
        case 'b':
#ifdef TARGET_OMAP4
            if ( brightness >= 100)
#else
            if ( brightness >= 200)
#endif
            {
                brightness = 0;
            } else {
                brightness += 10;
            }

            params.set(KEY_BRIGHTNESS, brightness);

            if ( hardwareActive ) {
                camera->setParameters(params.flatten());
            }

            break;

        case 's':
            if ( saturation >= 200) {
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
            effects_mode %= numEffects;
            printf("%d", numEffects);
            params.set(params.KEY_EFFECT, effectss[effects_mode]);
            printf("Effects_mode %d", effects_mode);

            if ( hardwareActive )
                camera->setParameters(params.flatten());

            break;

        case 'r':
            frameRateIDX++;
            frameRateIDX %= constCnt;
            params.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, fps_const_str[frameRateIDX]);
            printf("fps_const_str[frameRateIDX] %s\n", fps_const_str[frameRateIDX]);

            if ( hardwareActive ) {
                camera->setParameters(params.flatten());
            }

            break;

        case 'R':
            fpsRangeIdx += 1;
            fpsRangeIdx %= rangeCnt;
            params.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, fps_range_str[fpsRangeIdx]);
            printf("fps_range_str[fpsRangeIdx] %s\n", fps_range_str[fpsRangeIdx]);

            if ( hardwareActive ) {
                camera->setParameters(params.flatten());
            }

            break;

        case 'x':
            antibanding_mode++;
            antibanding_mode %= numAntibanding;
            printf("%d", numAntibanding);
            params.set(params.KEY_ANTIBANDING, antiband[antibanding_mode]);

            if ( hardwareActive )
                camera->setParameters(params.flatten());

            break;

        case 'g':
            focus_mode++;
            focus_mode %= numfocus;
            params.set(params.KEY_FOCUS_MODE, focus[focus_mode]);

            if ( hardwareActive )
                camera->setParameters(params.flatten());

            break;

        case 'G':
            params.set(CameraParameters::KEY_FOCUS_AREAS, TEST_FOCUS_AREA);

            if ( hardwareActive )
                camera->setParameters(params.flatten());

            break;

        case 'y':
            params.set(CameraParameters::KEY_METERING_AREAS, TEST_METERING_AREA);

            if ( hardwareActive ) {
                camera->setParameters(params.flatten());
            }

            break;

        case 'Y':

            params.set(CameraParameters::KEY_METERING_AREAS, TEST_METERING_AREA_CENTER);

            if ( hardwareActive ) {
                camera->setParameters(params.flatten());
            }

            break;

        case 'N':

            params.set(CameraParameters::KEY_METERING_AREAS, TEST_METERING_AREA_AVERAGE);

            if ( hardwareActive ) {
                camera->setParameters(params.flatten());
            }

            break;

        case 'f':
            gettimeofday(&autofocus_start, 0);

            if ( hardwareActive )
                camera->autoFocus();

            break;

        case 'p':
        {
            int msgType = 0;

            if((0 == strcmp(modevalues[capture_mode], "video-mode")) &&
               (0 != strcmp(videosnapshotstr, "true"))) {
                printf("Video Snapshot is not supported\n");
            } else if ( hardwareActive ) {
                if(isRawPixelFormat(pictureFormatArray[pictureFormat])) {
                    createBufferOutputSource();
                    if (bufferSourceOutputThread.get()) {
                        bufferSourceOutputThread->setBuffer(shotParams);
                        bufferSourceOutputThread->setStreamCapture(streamCapture, expBracketIdx);
                    }
                } else {
                    msgType = CAMERA_MSG_COMPRESSED_IMAGE |
                              CAMERA_MSG_RAW_IMAGE;
#ifdef OMAP_ENHANCEMENT_BURST_CAPTURE
                    msgType |= CAMERA_MSG_RAW_BURST;
#endif
                }

                gettimeofday(&picture_start, 0);
                camera->setParameters(params.flatten());
                camera->takePictureWithParameters(msgType, shotParams.flatten());
            }
            break;
        }

        case 'S':
        {
            if (streamCapture) {
                streamCapture = false;
                setDefaultExpGainPreset(shotParams, expBracketIdx);
                // Stop streaming
                if (bufferSourceOutputThread.get()) {
                    bufferSourceOutputThread->setStreamCapture(streamCapture, expBracketIdx);
                }
            } else {
                streamCapture = true;
                setSingleExpGainPreset(shotParams, expBracketIdx, 0, 0);
                // Queue more frames initially
                shotParams.set(ShotParameters::KEY_BURST, BRACKETING_STREAM_BUFFERS);
            }
            break;
        }

        case 'P':
        {
            int msgType = CAMERA_MSG_COMPRESSED_IMAGE;
            ShotParameters reprocParams;

            gettimeofday(&picture_start, 0);
            createBufferInputSource();
            if (bufferSourceOutputThread.get() &&
                bufferSourceOutputThread->hasBuffer())
            {
                bufferSourceOutputThread->setStreamCapture(false, expBracketIdx);
                if (hardwareActive) camera->setParameters(params.flatten());

                if (bufferSourceInput.get()) {
                    buffer_info_t info = bufferSourceOutputThread->popBuffer();
                    bufferSourceInput->setInput(info, pictureFormatArray[pictureFormat], reprocParams);
                    if (hardwareActive) camera->reprocess(msgType, reprocParams.flatten());
                }
            }
            break;
        }

        case '&':
            printf("Enabling Preview Callback");
            dump_preview = 1;
            if ( hardwareActive )
            camera->setPreviewCallbackFlags(CAMERA_FRAME_CALLBACK_FLAG_ENABLE_MASK);
            break;

        case '{':
            valstr = params.get(KEY_S3D2D_PREVIEW_MODE);
            if ( (NULL != valstr) && (0 == strcmp(valstr, "on")) )
                {
                params.set(KEY_S3D2D_PREVIEW_MODE, "off");
                }
            else
                {
                params.set(KEY_S3D2D_PREVIEW_MODE, "on");
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
            deleteAllocatedMemory();

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
            if(strcmp(AutoExposureLockstr, "true") == 0) {
                if(AutoExposureLocktoggle == false) {
                    trySetAutoExposureLock(true);
                    AutoExposureLocktoggle = true;
                } else {
                    trySetAutoExposureLock(false);
                    AutoExposureLocktoggle = false;
                    printf("ExposureLock is not supported\n");
                }
            }

            if ( hardwareActive )
                camera->setParameters(params.flatten());

            break;

        case '>':
            if(strcmp(AutoWhiteBalanceLockstr, "true") == 0) {
                if(AutoWhiteBalanceLocktoggle == false) {
                    trySetAutoWhiteBalanceLock(true);
                    AutoWhiteBalanceLocktoggle = true;
                } else {
                    trySetAutoWhiteBalanceLock(false);
                    AutoWhiteBalanceLocktoggle = false;
                    printf("ExposureLock is not supported\n");
                }
            }

            if ( hardwareActive ) {
                camera->setParameters(params.flatten());
            }

            break;

    case ')':
      enableMisalignmentCorrectionIdx++;
      enableMisalignmentCorrectionIdx %= ARRAY_SIZE(misalignmentCorrection);
      params.set(KEY_MECHANICAL_MISALIGNMENT_CORRECTION, misalignmentCorrection[enableMisalignmentCorrectionIdx]);
      if ( hardwareActive ) {
        camera->setParameters(params.flatten());
      }
      break;

    case 'd':
        while (1) {
            if ( menu_algo() < 0)
                break;
        }
        break;

    default:
      print_menu = 0;

      break;
    }

    return 0;
}

void print_usage() {
    printf(" USAGE: camera_test <options>\n");
    printf(" <options> (case insensitive)\n");
    printf("-----------\n");
    printf(" -f            -> Functional tests.\n");
    printf(" -a            -> API tests.\n");
    printf(" -e [<script>] -> Error scenario tests. If no script file is provided\n");
    printf("                  the test is run in interactive mode.\n");
    printf(" -s <script> -c <sensorID>  -> Stress / regression tests.\n");
    printf(" -l [<flags>]  -> Enable different kinds of logging capture. Multiple flags\n");
    printf("                  should be combined into a string. If flags are not provided\n");
    printf("                  no logs are captured.\n");
    printf("                   <flags>\n");
    printf("                  ---------\n");
    printf("                   l -> logcat [default]\n");
    printf("                   s -> syslink [default]\n");
    printf(" -o <path>     -> Output directory to store the test results. Image and video\n");
    printf("                  files are stored in corresponding sub-directories.\n");
    printf(" -p <platform> -> Target platform. Only for stress tests.\n");
    printf("                   <platform>\n");
    printf("                  ------------\n");
    printf("                   blaze or B    -> BLAZE\n");
    printf("                   tablet1 or T1 -> Blaze TABLET-1\n");
    printf("                   tablet2 or T2 -> Blaze TABLET-2 [default]\n\n");
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

  printf("+++Restarting Camera After Error+++\n");
  stopPreview();

  if (recordingMode) {
    stopRecording();
    closeRecorder();

    recordingMode = false;
  }

  sleep(3); //Wait a bit before restarting

  restartCount++;

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

int parseCommandLine(int argc, char *argv[], cmd_args_t *cmd_args) {
    if (argc < 2) {
        printf("Please enter at least 1 argument\n");
        return -2;
    }

    // Set defaults
    memset(cmd_args, 0, sizeof(*cmd_args));
    cmd_args->logging = LOGGING_LOGCAT | LOGGING_SYSLINK;
    cmd_args->platform_id = BLAZE_TABLET2;

    for (int a = 1; a < argc; a++) {
        const char * const arg = argv[a];
        if (arg[0] != '-') {
            printf("Error: Invalid argument \"%s\"\n", arg);
            return -2;
        }

        switch (arg[1]) {
            case 's':
                cmd_args->test_type = TEST_TYPE_REGRESSION;
                if (a < argc - 1) {
                    cmd_args->script_file_name = argv[++a];
                } else {
                    printf("Error: No script is specified for stress / regression test.\n");
                    return -2;
                }
                break;

            case 'f':
                cmd_args->test_type = TEST_TYPE_FUNCTIONAL;
                break;

            case 'a':
                cmd_args->test_type = TEST_TYPE_API;
                break;

            case 'e':
                cmd_args->test_type = TEST_TYPE_ERROR;
                if (a < argc - 1) {
                    cmd_args->script_file_name = argv[++a];
                }
                break;

            case 'l':
                cmd_args->logging = 0;

                if (a < argc - 1 && argv[a + 1][0] != '-') {
                    const char *flags = argv[++a];
                    while (*flags) {
                        char flag = *flags++;
                        switch (flag) {
                            case 'l':
                                cmd_args->logging |= LOGGING_LOGCAT;
                                break;

                            case 's':
                                cmd_args->logging |= LOGGING_SYSLINK;
                                break;

                            default:
                                printf("Error: Unknown logging type \"%c\"\n", flag);
                                return -2;
                        }
                    }
                }
                break;

            case 'p':
                if (a < argc - 1) {
                    const char *platform = argv[++a];
                    if( strcasecmp(platform,"blaze") == 0 || strcasecmp(platform,"B") == 0 ){
                        cmd_args->platform_id = BLAZE;
                    }
                    else if( (strcasecmp(platform,"tablet1") == 0) || (strcasecmp(platform,"T1") == 0) ) {
                        cmd_args->platform_id = BLAZE_TABLET1;
                    }
                    else if( (strcasecmp(platform,"tablet2") == 0) || (strcasecmp(platform,"T2") == 0) ) {
                        cmd_args->platform_id = BLAZE_TABLET2;
                    }
                    else {
                        printf("Error: Unknown argument for platform ID.\n");
                        return -2;
                    }
                } else {
                    printf("Error: No argument is specified for platform ID.\n");
                    return -2;
                }
                break;

            case 'o':
                if (a < argc - 1) {
                    cmd_args->output_path = argv[++a];
                } else {
                    printf("Error: No output path is specified.\n");
                    return -2;
                }
                break;

            case 'c':
                if (a < argc -1) {
                    camera_index = atoi(argv[++a]);
                } else {
                    printf("Error: No sensorID is specified.\n");
                    return -2;
                }
                break;

            default:
                printf("Error: Unknown option \"%s\"\n", argv[a]);
                return -2;
        }
    }

    return 0;
}

int setOutputDirPath(cmd_args_t *cmd_args, int restart_count) {
    if ((cmd_args->output_path != NULL) &&
            (strlen(cmd_args->output_path) < sizeof(output_dir_path))) {
        strcpy(output_dir_path, cmd_args->output_path);
    } else {
        strcpy(output_dir_path, SDCARD_PATH);

        if (cmd_args->script_file_name != NULL) {
            const char *config = cmd_args->script_file_name;
            char dir_name[40];
            size_t count = 0;
            char *p;

            // remove just the '.txt' part of the config
            while ((config[count] != '.') && ((count + 1) < sizeof(dir_name))) {
                count++;
            }

            strncpy(dir_name, config, count);

            dir_name[count] = NULL;
            p = dir_name;
            while (*p != '\0') {
                if (*p == '/') {
                    printf("SDCARD_PATH is not added to the output directory.\n");
                    // Needed when camera_test script is executed using the OTC
                    strcpy(output_dir_path, "");
                    break;
                }
            }

            strcat(output_dir_path, dir_name);
            if (camera_index == 1) {
                strcat(output_dir_path, SECONDARY_SENSOR);
            }else if (camera_index == 2) {
                strcat(output_dir_path, S3D_SENSOR);
            }
        }
    }

    if (restart_count && (strlen(output_dir_path) + 16) < sizeof(output_dir_path)) {
        char count[16];
        sprintf(count, "_%d", restart_count);
        strcat(output_dir_path, count);
    }

    if (access(output_dir_path, F_OK) == -1) {
        if (mkdir(output_dir_path, 0777) == -1) {
             printf("\nError: Output directory \"%s\" was not created\n", output_dir_path);
             return -1;
        }
    }

    sprintf(videos_dir_path, "%s/videos", output_dir_path);

    if (access(videos_dir_path, F_OK) == -1) {
        if (mkdir(videos_dir_path, 0777) == -1) {
             printf("\nError: Videos directory \"%s\" was not created\n", videos_dir_path);
             return -1;
        }
    }

    sprintf(images_dir_path, "%s/images", output_dir_path);

    if (access(images_dir_path, F_OK) == -1) {
        if (mkdir(images_dir_path, 0777) == -1) {
             printf("\nError: Images directory \"%s\" was not created\n", images_dir_path);
             return -1;
        }
    }

    return 0;
}

int startTest() {
    ProcessState::self()->startThreadPool();

    if (openCamera() < 0) {
        printf("Camera initialization failed\n");
        return -1;
    }

    initDefaults();

    return 0;
}

int runRegressionTest(cmd_args_t *cmd_args) {
    char *cmd;
    int pid;

    platformID = cmd_args->platform_id;

    int res = startTest();
    if (res != 0) {
        return res;
    }

    cmd = load_script(cmd_args->script_file_name);

    if (cmd != NULL) {
        start_logging(cmd_args->logging, pid);
        stressTest = true;

        while (1) {
            if (execute_functional_script(cmd) == 0) {
                break;
            }

            printf("CameraTest Restarting Camera...\n");

            free(cmd);
            cmd = NULL;

            if ( (restartCamera() != 0)  || ((cmd = load_script(cmd_args->script_file_name)) == NULL) ) {
                printf("ERROR::CameraTest Restarting Camera...\n");
                res = -1;
                break;
            }

            res = setOutputDirPath(cmd_args, restartCount);
            if (res != 0) {
                break;
            }
        }

        free(cmd);
        stop_logging(cmd_args->logging, pid);
    }

    return 0;
}

int runFunctionalTest() {
    int res = startTest();
    if (res != 0) {
        return res;
    }

    print_menu = 1;

    while (1) {
        if (functional_menu() < 0) {
            break;
        }
    }

    return 0;
}

int runApiTest() {
    printf("API level test cases coming soon ... \n");
    return 0;
}

int runErrorTest(cmd_args_t *cmd_args) {
    int res = startTest();
    if (res != 0) {
        return res;
    }

    if (cmd_args->script_file_name != NULL) {
        char *cmd;
        int pid;

        cmd = load_script(cmd_args->script_file_name);

        if (cmd != NULL) {
            start_logging(cmd_args->logging, pid);
            execute_error_script(cmd);
            free(cmd);
            stop_logging(cmd_args->logging, pid);
        }
    } else {
        print_menu = 1;

        while (1) {
            if (error_scenario() < 0) {
                break;
            }
        }
    }

    return 0;
}

int main(int argc, char *argv[]) {
    sp<ProcessState> proc(ProcessState::self());

    unsigned long long st, end, delay;
    timeval current_time;
    cmd_args_t cmd_args;
    int res;

    res = parseCommandLine(argc, argv, &cmd_args);
    if (res != 0) {
        print_usage();
        return res;
    }

    res = setOutputDirPath(&cmd_args, 0);
    if (res != 0) {
        return res;
    }

    gettimeofday(&current_time, 0);

    st = current_time.tv_sec * 1000000 + current_time.tv_usec;

    system("echo camerahal_test > /sys/power/wake_lock");

    switch (cmd_args.test_type) {
        case TEST_TYPE_REGRESSION:
            res = runRegressionTest(&cmd_args);
            break;

        case TEST_TYPE_FUNCTIONAL:
            res = runFunctionalTest();
            break;

        case TEST_TYPE_API:
            res = runApiTest();
            break;

        case TEST_TYPE_ERROR:
            res = runErrorTest(&cmd_args);
            break;
    }

    system("echo camerahal_test > /sys/power/wake_unlock");

    gettimeofday(&current_time, 0);
    end = current_time.tv_sec * 1000000 + current_time.tv_usec;
    delay = end - st;
    printf("Application closed after: %llu ms\n", delay);

    return res;
}
