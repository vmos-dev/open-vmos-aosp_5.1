#ifndef CAMERA_TEST_H
#define CAMERA_TEST_H

#define PRINTOVER(arg...)     ALOGD(#arg)
#define LOG_FUNCTION_NAME         ALOGD("%d: %s() ENTER", __LINE__, __FUNCTION__);
#define LOG_FUNCTION_NAME_EXIT    ALOGD("%d: %s() EXIT", __LINE__, __FUNCTION__);
#define KEY_GBCE            "gbce"
#define KEY_GLBCE           "glbce"
#define KEY_CAMERA          "camera-index"
#define KEY_SATURATION      "saturation"
#define KEY_BRIGHTNESS      "brightness"
#define KEY_BURST           "burst-capture"
#define KEY_EXPOSURE        "exposure"
#define KEY_CONTRAST        "contrast"
#define KEY_SHARPNESS       "sharpness"
#define KEY_ISO             "iso"
#define KEY_CAF             "caf"
#define KEY_MODE            "mode"
#define KEY_VNF             "vnf"
#define KEY_VSTAB           "vstab"
#define KEY_COMPENSATION    "exposure-compensation"

#define KEY_IPP             "ipp"

#define KEY_BUFF_STARV      "buff-starvation"
#define KEY_METERING_MODE   "meter-mode"
#define KEY_AUTOCONVERGENCE "auto-convergence"
#define KEY_MANUALCONVERGENCE_VALUES "manual-convergence-values"
#define AUTOCONVERGENCE_MODE_MANUAL "mode-manual"
#define KEY_EXP_BRACKETING_RANGE "exp-bracketing-range"
#define KEY_TEMP_BRACKETING "temporal-bracketing"
#define KEY_TEMP_BRACKETING_POS "temporal-bracketing-range-positive"
#define KEY_TEMP_BRACKETING_NEG "temporal-bracketing-range-negative"
#define KEY_MEASUREMENT "measurement"
#define KEY_S3D2D_PREVIEW_MODE "s3d2d-preview"
#define KEY_STEREO_CAMERA "s3d-supported"
#define KEY_EXIF_MODEL "exif-model"
#define KEY_EXIF_MAKE "exif-make"

#define KEY_AUTO_EXPOSURE_LOCK "auto-exposure-lock"
#define KEY_AUTO_WHITEBALANCE_LOCK "auto-whitebalance-lock"

#define SDCARD_PATH "/sdcard/"

#define MAX_BURST   15
#define BURST_INC     5
#define TEMP_BRACKETING_MAX_RANGE 4

#define MEDIASERVER_DUMP "procmem -w $(ps | grep mediaserver | grep -Eo '[0-9]+' | head -n 1) | grep \"\\(Name\\|libcamera.so\\|libOMX\\|libomxcameraadapter.so\\|librcm.so\\|libnotify.so\\|libipcutils.so\\|libipc.so\\|libsysmgr.so\\|TOTAL\\)\""
#define MEMORY_DUMP "procrank -u"
#define KEY_METERING_MODE   "meter-mode"

#define TEST_FOCUS_AREA "(0,0,1000,1000,300),(-1000,-1000,1000,1000,300),(0,0,0,0,0)"

#define COMPENSATION_OFFSET 20
#define DELIMITER           "|"

#define MAX_PREVIEW_SURFACE_WIDTH   800
#define MAX_PREVIEW_SURFACE_HEIGHT  480

#define MODEL "camera_test"
#define MAKE "camera_test"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

namespace android {
    class CameraHandler: public CameraListener {
        public:
            virtual void notify(int32_t msgType, int32_t ext1, int32_t ext2);
            virtual void postData(int32_t msgType,
                                  const sp<IMemory>& dataPtr,
                                  camera_frame_metadata_t *metadata);

            virtual void postDataTimestamp(nsecs_t timestamp, int32_t msgType, const sp<IMemory>& dataPtr);
    };

};

using namespace android;

char * get_cycle_cmd(const char *aSrc);
int execute_functional_script(char *script);
status_t dump_mem_status();
int openCamera();
int closeCamera();
void initDefaults();
int startPreview();
void stopPreview();
int startRecording();
int stopRecording();
int closeRecorder();
int openRecorder();
int configureRecorder();
void printSupportedParams();
char *load_script(char *config);
int start_logging(char *config, int &pid);
int stop_logging(int &pid);
int execute_error_script(char *script);

typedef struct pixel_format_t {
    int32_t pixelFormatDesc;
    const char *pixformat;
}pixel_format;

typedef struct output_format_t {
    output_format type;
    const char *desc;
} outformat;

typedef struct preview_size_t {
    int width, height;
    const char *desc;
} preview_size;

typedef struct Vcapture_size_t {
    int width, height;
    const char *desc;
} Vcapture_size;

typedef struct capture_Size_t {
    int width, height;
    const char *name;
} capture_Size;

typedef struct video_Codecs_t {
    video_encoder type;
    const char *desc;
} video_Codecs;

typedef struct audio_Codecs_t {
    audio_encoder type;
    const char *desc;
} audio_Codecs;

typedef struct V_bitRate_t {
    uint32_t bit_rate;
    const char *desc;
} V_bitRate;

typedef struct zoom_t {
    int idx;
    const char *zoom_description;
} Zoom;

typedef struct fps_ranges_t {
    const char *range;
    const char *rangeDescription;
} fps_ranges;

typedef struct fpsConst_Ranges_t {
    const char *range;
    const char *rangeDescription;
    int constFramerate;
} fpsConst_Ranges;

typedef struct fpsConst_RangesSec_t {
    const char *range;
    const char *rangeDescription;
    int constFramerate;
} fpsConst_RangesSec;

#endif
