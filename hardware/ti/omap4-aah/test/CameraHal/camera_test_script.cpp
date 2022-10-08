#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <semaphore.h>
#include <pthread.h>

#include <camera/Camera.h>
#include <camera/ICamera.h>
#include <media/mediarecorder.h>

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <cutils/properties.h>
#include <camera/CameraParameters.h>
#include <camera/ShotParameters.h>

#include <sys/wait.h>

#include "camera_test.h"
#include "camera_test_surfacetexture.h"
#ifdef ANDROID_API_JB_OR_LATER
#include "camera_test_bufferqueue.h"
#endif

using namespace android;

extern bool stopScript;
extern bool hardwareActive;
extern sp<Camera> camera;
extern sp<BufferSourceThread> bufferSourceOutputThread;
extern sp<BufferSourceInput> bufferSourceInput;
extern CameraParameters params;
extern ShotParameters shotParams;
extern bool shotConfigFlush;
extern bool streamCapture;
extern bool recordingMode;
extern int camera_index;
extern int rotation;
extern int previewRotation;
extern const param_Array captureSize[];
extern const param_Array VcaptureSize[];
extern const outformat outputFormat[];
extern const video_Codecs videoCodecs[];
extern const audio_Codecs audioCodecs[];
extern const V_bitRate VbitRate[];
extern const Zoom zoom [];
extern int previewSizeIDX;
extern bool reSizePreview;
extern bool previewRunning;
extern int captureSizeIDX;
extern float compensation;
extern int videoCodecIDX;
extern int outputFormatIDX;
extern int audioCodecIDX;
extern int VcaptureSizeIDX;
extern int VbitRateIDX;
extern int thumbSizeIDX;
extern int thumbQuality;
extern int jpegQuality;
extern int dump_preview;
extern int ippIDX_old;
extern const char *capture[];
extern int capture_mode;
extern int ippIDX;
extern const char *ipp_mode[];
extern int tempBracketRange;
extern int iso_mode;
extern int sharpness;
extern int contrast;
extern int zoomIDX;
extern int brightness;
extern int saturation;
extern int fpsRangeIdx;
extern int numAntibanding;
extern int numEffects;
extern int numawb;
extern int numExposureMode;
extern int numscene;
extern int numisoMode;
extern int numflash;
extern int numcaptureSize;
extern int numVcaptureSize;
extern int numpreviewSize;
extern int numthumbnailSize;
extern int numfocus;
extern int numpreviewFormat;
extern int numpictureFormat;
extern int nummodevalues;
extern int numLay;
extern int numCLay;
extern int constCnt;
extern int rangeCnt;
extern int * constFramerate;
extern int frameRateIDX;
extern int fpsRangeIdx;
extern int stereoLayoutIDX;
extern int stereoCapLayoutIDX;
extern int expBracketIdx;
int resol_index = 0;
int a = 0;
extern char * vstabstr;
extern char * vnfstr;
extern char * zoomstr;
extern char * smoothzoomstr;
extern char * videosnapshotstr;
extern char ** antiband;
extern char **effectss;
extern bool firstTime;
extern char **exposureMode;
extern char **awb;
extern char **scene;
extern char ** isoMode;
extern char ** modevalues;
extern char **focus;
extern char **flash;
extern char **previewFormatArray;
extern char **pictureFormatArray;
extern char ** fps_const_str;
extern char ** fps_range_str;
extern char ** rangeDescription;
extern param_Array ** capture_Array;
extern param_Array ** Vcapture_Array;
extern param_Array ** preview_Array;
extern param_Array ** thumbnail_Array;
extern timeval autofocus_start, picture_start;
extern const char *cameras[];
extern double latitude;
extern double degree_by_step;
extern double longitude;
extern double altitude;
extern char output_dir_path[];
extern char images_dir_path[];
extern int AutoConvergenceModeIDX;
extern const char *autoconvergencemode[];
extern int numCamera;
extern bool stereoMode;
extern char script_name[];
extern int bufferStarvationTest;
extern size_t length_previewSize;
extern size_t length_thumbnailSize;
extern size_t lenght_Vcapture_size;
extern size_t length_outformat;
extern size_t length_capture_Size;
extern size_t length_video_Codecs;
extern size_t length_audio_Codecs;
extern size_t length_V_bitRate;
extern size_t length_Zoom;
extern size_t length_fps_ranges;
extern size_t length_fpsConst_Ranges;
extern size_t length_fpsConst_RangesSec;
extern int platformID;
extern char **stereoLayout;
extern char **stereoCapLayout;
extern void getSizeParametersFromCapabilities();
extern int exposure_mode;
int manE = 0;
extern int manualExp ;
extern int manualExpMin ;
extern int manualExpMax ;
int manG = 0;
extern int manualGain ;
extern int manualGainMin ;
extern int manualGainMax ;
int manC = 0;
extern int manualConv ;
extern int manualConvMin ;
extern int manualConvMax ;
extern bool faceDetectToggle;
extern unsigned int burstCount;

/** Buffer source reset */
extern bool bufferSourceInputReset;
extern bool bufferSourceOutputReset;

void trim_script_cmd(char *cmd) {
    char *nl, *cr;

    // first remove all carriage return symbols
    while ( NULL != (cr = strchr(cmd, '\r'))) {
        for (char *c = cr; '\0' != *c; c++) {
            *c = *(c+1);
        }
    }

    // then remove all single line feed symbols
    while ( NULL != (nl = strchr(cmd, '\n'))) {
        if (*nl == *(nl+1)) {
            // two or more concatenated newlines:
            // end of script found
            break;
        }
        // clip the newline
        for (char *c = nl; '\0' != *c; c++) {
            *c = *(c+1);
        }
    }
}

int execute_functional_script(char *script) {
    char *cmd, *ctx, *cycle_cmd, *temp_cmd;
    char id;
    unsigned int i;
    int dly;
    int cycleCounter = 1;
    int tLen = 0;
    unsigned int iteration = 0;
    bool zoomtoggle = false;
    bool smoothzoomtoggle = false;
    status_t ret = NO_ERROR;
    //int frameR = 20;
    int frameRConst = 0;
    int frameRRange = 0;
    struct CameraInfo cameraInfo;
    bool queueEmpty = true;

    LOG_FUNCTION_NAME;

    dump_mem_status();

    cmd = strtok_r((char *) script, DELIMITER, &ctx);

    while ( NULL != cmd && (stopScript == false)) {
        trim_script_cmd(cmd);
        id = cmd[0];
        printf("Full Command: %s \n", cmd);
        printf("Command: %c \n", cmd[0]);

        switch (id) {

            // Case for Suspend-Resume Feature
            case '!': {
                // STEP 1: Mount Debugfs
                system("mkdir /debug");
                system("mount -t debugfs debugfs /debug");

                // STEP 2: Set up wake up Timer - wake up happens after 5 seconds
                system("echo 10 > /debug/pm_debug/wakeup_timer_seconds");

                // STEP 3: Make system ready for Suspend
                system("echo camerahal_test > /sys/power/wake_unlock");
                // Release wake lock held by test app
                printf(" Wake lock released ");
                system("cat /sys/power/wake_lock");
                system("sendevent /dev/input/event0 1 60 1");
                system("sendevent /dev/input/event0 1 60 0");
                // Simulate F2 key press to make display OFF
                printf(" F2 event simulation complete ");

                //STEP 4: Wait for system Resume and then simuate F1 key
                sleep(50);//50s  // This delay is not related to suspend resume timer
                printf(" After 30 seconds of sleep");
                system("sendevent /dev/input/event0 1 59 0");
                system("sendevent /dev/input/event0 1 59 1");
                // Simulate F1 key press to make display ON
                system("echo camerahal_test > /sys/power/wake_lock");
                // Acquire wake lock for test app

                break;
            }

            case '[':
                if ( hardwareActive )
                    {

                    camera->setParameters(params.flatten());

                    printf("starting camera preview..");
                    status_t ret = camera->startPreview();
                    if(ret !=NO_ERROR)
                        {
                        printf("startPreview failed %d..", ret);
                        }
                    }
                break;
            case '+': {
                cycleCounter = atoi(cmd + 1);
                cycle_cmd = get_cycle_cmd(ctx);
                tLen = strlen(cycle_cmd);
                temp_cmd = new char[tLen+1];

                for (int ind = 0; ind < cycleCounter; ind++) {
                    strcpy(temp_cmd, cycle_cmd);
                    if ( execute_functional_script(temp_cmd) != 0 )
                      return -1;
                    temp_cmd[0] = '\0';

                    //patch for image capture
                    //[
                    if (ind < cycleCounter - 1) {
                        if (hardwareActive == false) {
                            if ( openCamera() < 0 ) {
                                printf("Camera initialization failed\n");

                                return -1;
                            }

                            initDefaults();
                        }
                    }

                    //]
                }

                ctx += tLen + 1;

                if (temp_cmd) {
                    delete temp_cmd;
                    temp_cmd = NULL;
                }

                if (cycle_cmd) {
                    delete cycle_cmd;
                    cycle_cmd = NULL;
                }

                break;
            }

            case '0':
            {
                initDefaults();
                break;
            }

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
                rotation = atoi(cmd + 1);
                params.set(CameraParameters::KEY_ROTATION, rotation);

                if ( hardwareActive )
                    camera->setParameters(params.flatten());

                break;

            case 'V':
                previewRotation = atoi(cmd + 1);
                params.set(KEY_SENSOR_ORIENTATION, previewRotation);

                if ( hardwareActive )
                    camera->setParameters(params.flatten());

                break;

            case '4':
                printf("Setting resolution...");

                a = checkSupportedParamScriptResol(preview_Array, numpreviewSize, cmd, &resol_index);
                if (a > -1) {
                    params.setPreviewSize(preview_Array[resol_index]->width, preview_Array[resol_index]->height);
                    previewSizeIDX = resol_index;
                } else {
                    printf("\nNot supported parameter %s from sensor %d\n\n", cmd + 1, camera_index);
                }
                if ( hardwareActive && previewRunning ) {
                    camera->stopPreview();
                    camera->setParameters(params.flatten());
                    camera->startPreview();
                } else if ( hardwareActive ) {
                    camera->setParameters(params.flatten());
                }
                break;

            case '5':
                if( strcmp((cmd + 1), "MAX_CAPTURE_SIZE") == 0) {
                    resol_index = 0;
                    for (int i=0; i<numcaptureSize; i++) {
                        if ((capture_Array[resol_index]->width * capture_Array[resol_index]->height) < (capture_Array[i]->width * capture_Array[i]->height)) {
                            resol_index = i;
                        }
                    }
                    if ((0 < capture_Array[resol_index]->width) && (0 < capture_Array[resol_index]->height)) {
                        params.setPictureSize(capture_Array[resol_index]->width, capture_Array[resol_index]->height);
                        captureSizeIDX = resol_index;
                        printf("Capture Size set: %dx%d\n", capture_Array[resol_index]->width, capture_Array[resol_index]->height);
                    } else {
                        printf("\nCapture size is 0!\n");
                    }
                } else {
                    a = checkSupportedParamScriptResol(capture_Array, numcaptureSize, cmd, &resol_index);
                    if (camera_index != 2) {
                        if (a > -1) {
                            params.setPictureSize(capture_Array[resol_index]->width, capture_Array[resol_index]->height);
                            captureSizeIDX = resol_index;
                        } else {
                            printf("\nNot supported parameter %s from sensor %d\n\n", cmd + 1, camera_index);
                        }
                    } else {
                        int widthC, heightC;
                        char *resC = NULL;
                        resC = strtok(cmd + 1, "x");
                        widthC = atoi(resC);
                        resC = strtok(NULL, "x");
                        heightC = atoi(resC);
                        params.setPictureSize(widthC,heightC);
                    a = checkSupportedParamScriptResol(capture_Array, numcaptureSize,
                                                       widthC, heightC, &resol_index);
                    if (a > -1) captureSizeIDX = resol_index;
                    }

                    if ( hardwareActive ) {
                        camera->setParameters(params.flatten());
                    }
                }

                requestBufferSourceReset();

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
                compensation = atof(cmd + 1);
                params.set(KEY_COMPENSATION, (int) (compensation * 10));

                if ( hardwareActive )
                    camera->setParameters(params.flatten());

                break;

            case '8':

                a = checkSupportedParamScript(awb, numawb, cmd);
                if (a > -1) {
                    params.set(params.KEY_WHITE_BALANCE, (cmd + 1));
                } else {
                    printf("\nNot supported parameter %s from sensor %d\n\n", cmd + 1, camera_index);
                }

                if ( hardwareActive )
                    camera->setParameters(params.flatten());

                break;

            case '9':
                for(i = 0; i < length_video_Codecs; i++)
                {
                    if( strcmp((cmd + 1), videoCodecs[i].desc) == 0)
                    {
                        videoCodecIDX = i;
                        printf("Video Codec Selected: %s\n",
                                videoCodecs[i].desc);
                        break;
                    }
                }
                break;

            case 'v':
                for(i = 0; i < length_outformat; i++)

                {
                    if( strcmp((cmd + 1), outputFormat[i].desc) == 0)
                    {
                        outputFormatIDX = i;
                        printf("Video Codec Selected: %s\n",
                                videoCodecs[i].desc);
                        break;
                    }
                }
            break;

            case '~':

                a = checkSupportedParamScript(previewFormatArray, numpreviewFormat, cmd);
                if (a > -1) {
                    params.setPreviewFormat(cmd + 1);
                } else {
                    printf("\nNot supported parameter %s from sensor %d\n\n", cmd + 1, camera_index);
                }

                if ( hardwareActive )
                    camera->setParameters(params.flatten());

                break;

            case '$':

                a = checkSupportedParamScript(pictureFormatArray, numpictureFormat, cmd);
                if (a > -1) {
                    params.setPictureFormat(cmd + 1);
                } else {
                    printf("\nNot supported parameter %s from sensor %d\n\n", cmd + 1, camera_index);
                }

                queueEmpty = true;
                if ( bufferSourceOutputThread.get() ) {
                    if ( 0 < bufferSourceOutputThread->hasBuffer() ) {
                        queueEmpty = false;
                    }
                }
                if ( hardwareActive && queueEmpty ) {
                    camera->setParameters(params.flatten());
                }

                break;
            case '-':
                for(i = 0; i < length_audio_Codecs; i++)
                {
                    if( strcmp((cmd + 1), audioCodecs[i].desc) == 0)
                    {
                        audioCodecIDX = i;
                        printf("Selected Audio: %s\n", audioCodecs[i].desc);
                        break;
                    }
                }
                break;

            case 'A':
                camera_index=atoi(cmd+1);
                camera_index %= numCamera;

                printf("%s selected.\n", cameras[camera_index]);
                firstTime = true;

                if ( hardwareActive ) {
                    stopPreview();
                    closeCamera();
                    openCamera();
                } else {
                    closeCamera();
                    openCamera();
                }
                break;

            case 'a':
                char * temp_str;

                temp_str = strtok(cmd+1,"!");
                printf("Latitude %s \n",temp_str);
                params.set(params.KEY_GPS_LATITUDE, temp_str);
                temp_str=strtok(NULL,"!");
                printf("Longitude %s \n",temp_str);
                params.set(params.KEY_GPS_LONGITUDE, temp_str);
                temp_str=strtok(NULL,"!");
                printf("Altitude %s \n",temp_str);
                params.set(params.KEY_GPS_ALTITUDE, temp_str);

                if ( hardwareActive )
                    camera->setParameters(params.flatten());
                break;

            case 'l':
                a = checkSupportedParamScriptResol(Vcapture_Array, numVcaptureSize, cmd, &resol_index);
                if (a > -1) {
                    VcaptureSizeIDX = resol_index;
                } else {
                    printf("\nNot supported parameter %s from sensor %d\n\n", cmd + 1, camera_index);
                }
                break;

            case 'L':
                if(stereoMode)
                {
                    a = checkSupportedParamScriptLayout(stereoLayout, numLay, cmd, &stereoLayoutIDX);
                    if (a > -1) {
                        params.set(KEY_S3D_PRV_FRAME_LAYOUT, cmd + 1);
                    } else {
                        printf("\nNot supported parameter %s from sensor %d\n\n", cmd + 1, camera_index);
                    }


                    getSizeParametersFromCapabilities();
                    if (hardwareActive && previewRunning) {
                        stopPreview();
                        camera->setParameters(params.flatten());
                        startPreview();
                    } else if (hardwareActive) {
                        camera->setParameters(params.flatten());
                    }
                }
                break;


            case '.':
                if(stereoMode)
                {
                    a = checkSupportedParamScriptLayout(stereoCapLayout, numCLay, cmd, &stereoCapLayoutIDX);
                    if (a > -1) {
                        params.set(KEY_S3D_CAP_FRAME_LAYOUT_VALUES, cmd + 1);
                    } else {
                        printf("\nNot supported parameter %s from sensor %d\n\n", cmd + 1, camera_index);
                    }


                    getSizeParametersFromCapabilities();
                    if (hardwareActive && previewRunning) {
                        stopPreview();
                        camera->setParameters(params.flatten());
                        startPreview();
                    } else if (hardwareActive) {
                        camera->setParameters(params.flatten());
                    }
                }
                break;

            case ']':
                for(i = 0; i < length_V_bitRate; i++)
                {
                    if( strcmp((cmd + 1), VbitRate[i].desc) == 0)
                    {
                        VbitRateIDX = i;
                        printf("Video Bit Rate: %s\n", VbitRate[i].desc);
                        break;
                    }
                }
                break;
            case ':':

                a = checkSupportedParamScriptResol(thumbnail_Array, numthumbnailSize, cmd, &resol_index);
                if (a > -1) {
                    params.set(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH, thumbnail_Array[resol_index]->width);
                    params.set(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT,thumbnail_Array[resol_index]->height);
                    thumbSizeIDX = resol_index;
                } else {
                    printf("\nNot supported parameter %s from sensor %d\n\n", cmd + 1, camera_index);
                }

                if ( hardwareActive ) {
                    camera->setParameters(params.flatten());
                }


                break;

            case '\'':
                thumbQuality = atoi(cmd + 1);

                params.set(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY, thumbQuality);
                if ( hardwareActive )
                    camera->setParameters(params.flatten());
                break;

            case '*':
                if ( hardwareActive )
                    camera->startRecording();
                break;

            case 't':
                params.setPreviewFormat((cmd + 1));
                if ( hardwareActive )
                    camera->setParameters(params.flatten());
                break;

            case 'o':
                jpegQuality = atoi(cmd + 1);
                params.set(CameraParameters::KEY_JPEG_QUALITY, jpegQuality);

                if ( hardwareActive )
                    camera->setParameters(params.flatten());

                break;


            case '&':
                printf("Enabling Preview Callback");
                dump_preview = 1;
                camera->setPreviewCallbackFlags(CAMERA_FRAME_CALLBACK_FLAG_ENABLE_MASK);
                break;


            case 'k':
                ippIDX_old = atoi(cmd + 1);
                params.set(KEY_IPP, atoi(cmd + 1));
                if ( hardwareActive )
                    camera->setParameters(params.flatten());

                requestBufferSourceReset();

                break;

            case 'K':
                params.set(KEY_GBCE, (cmd+1));
                if ( hardwareActive )
                    camera->setParameters(params.flatten());

                break;

            case 'F':
                if ( hardwareActive ) {
                    camera->sendCommand(CAMERA_CMD_START_FACE_DETECTION, 0, 0);
                    faceDetectToggle = true;
                }

                break;

            case 'I':
                params.set(KEY_AF_TIMEOUT, (cmd + 1));

                if ( hardwareActive )
                    camera->setParameters(params.flatten());

                break;

            case 'T':

                if ( hardwareActive ) {
                    camera->sendCommand(CAMERA_CMD_STOP_FACE_DETECTION, 0, 0);
                    faceDetectToggle = false;
                }

                break;

            case 'O':
                params.set(KEY_GLBCE, (cmd+1));
                if ( hardwareActive )
                    camera->setParameters(params.flatten());

                break;

            case 'u':
                // HQ should always be in ldc-nsf
                // if not HQ, then return the ipp to its previous state
                if ( !strcmp((cmd + 1), "high-quality") ) {
                    ippIDX_old = ippIDX;
                    ippIDX = 3;
                    params.set(KEY_IPP, ipp_mode[ippIDX]);
                    params.set(CameraParameters::KEY_RECORDING_HINT, CameraParameters::FALSE);
                    previewRotation = 0;
                    params.set(KEY_SENSOR_ORIENTATION, previewRotation);
                } else if ( !strcmp((cmd + 1), "video-mode") ) {
                    params.set(CameraParameters::KEY_RECORDING_HINT, CameraParameters::TRUE);
                    camera->getCameraInfo(camera_index, &cameraInfo);
                    previewRotation = ((360-cameraInfo.orientation)%360);
                    if (previewRotation >= 0 || previewRotation <=360) {
                        params.set(KEY_SENSOR_ORIENTATION, previewRotation);
                    }
                    printf("previewRotation: %d\n", previewRotation);
                } else {
                    ippIDX = ippIDX_old;
                    params.set(CameraParameters::KEY_RECORDING_HINT, CameraParameters::FALSE);
                    previewRotation = 0;
                    params.set(KEY_SENSOR_ORIENTATION, previewRotation);
                }
                a = checkSupportedParamScript(modevalues, nummodevalues, cmd);
                if (a > -1) {
                    params.set(KEY_MODE, (cmd + 1));
                } else {
                    printf("\nNot supported parameter %s from sensor %d\n\n", cmd + 1, camera_index);
                }

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

                params.set(KEY_TEMP_BRACKETING, (cmd + 1));

                if ( hardwareActive )
                    camera->setParameters(params.flatten());

                break;

            case 'H':

                setDefaultExpGainPreset(shotParams, atoi(cmd + 1));
                break;


            case 'n':

            switch (*(cmd + 1)) {
                case 0:
                shotConfigFlush = false;
                    break;
                case 1:
                shotConfigFlush = true;
                    break;
                default:
                printf ("Mangling flush shot config command: \"%s\"\n", (cmd + 1));
                    break;
            }

            updateShotConfigFlushParam();

            break;

            case '?':

                setExpGainPreset(shotParams, cmd + 1, true, PARAM_EXP_BRACKET_PARAM_NONE, shotConfigFlush);

                break;

            case 'W':

                tempBracketRange = atoi(cmd + 1);
                tempBracketRange %= TEMP_BRACKETING_MAX_RANGE;
                if ( 0 == tempBracketRange ) {
                    tempBracketRange = 1;
                }

                params.set(KEY_TEMP_BRACKETING_NEG, tempBracketRange);
                params.set(KEY_TEMP_BRACKETING_POS, tempBracketRange);

                if ( hardwareActive )
                    camera->setParameters(params.flatten());

            break;

            case '#':

                params.set(KEY_TI_BURST, atoi(cmd + 1));
                burstCount = atoi(cmd + 1);

                if ( hardwareActive )
                    camera->setParameters(params.flatten());

                break;

            case 'J':

                a = checkSupportedParamScript(flash, numflash, cmd);
                if (a > -1) {
                    params.set(CameraParameters::KEY_FLASH_MODE, (cmd + 1));
                } else {
                    printf("\nNot supported parameter %s from sensor %d\n\n", cmd + 1, camera_index);
                }

                if ( hardwareActive )
                    camera->setParameters(params.flatten());

                break;

            case 'w':

                a = checkSupportedParamScript(scene, numscene, cmd);
                if (a > -1) {
                    params.set(params.KEY_SCENE_MODE, (cmd + 1));
                } else {
                    printf("\nNot supported parameter %s from sensor %d\n\n", cmd + 1, camera_index);
                }
                if ( hardwareActive )
                    camera->setParameters(params.flatten());

                break;

            case 'B' :
                if(strcmp(vnfstr, "true") == 0) {
                    if (strcmp(cmd + 1, "1") == 0) {
                        trySetVideoNoiseFilter(true);
                    }
                    else if (strcmp(cmd + 1, "0") == 0){
                        trySetVideoNoiseFilter(false);
                    }
                } else {
                    trySetVideoNoiseFilter(false);
                    printf("\n VNF is not supported \n\n");
                }

                if ( hardwareActive ) {
                    camera->setParameters(params.flatten());
                }
                break;


            case 'C' :

                if (strcmp(vstabstr, "true") == 0) {
                    if (strcmp(cmd + 1, "1") == 0) {
                        trySetVideoStabilization(true);
                    } else if (strcmp(cmd + 1, "0") == 0) {
                        trySetVideoStabilization(false);
                    } else {
                        printf("\nNot supported parameter %s from sensor %d\n\n", cmd + 1, camera_index);
                    }
                } else {
                    printf("\nNot supported parameter vstab from sensor %d\n\n", camera_index);
                }

                if ( hardwareActive ) {
                    camera->setParameters(params.flatten());
                }
                break;

            case 'D':
                if ( hardwareActive )
                    camera->stopRecording();
                break;

            case 'E':
                if(hardwareActive)
                    params.unflatten(camera->getParameters());
                printSupportedParams();
                break;

            case 'i':
                iso_mode = atoi(cmd + 1);
                if (iso_mode < numisoMode) {
                    params.set(KEY_ISO, isoMode[iso_mode]);
                } else {
                    printf("\nNot supported parameter %s for iso mode from sensor %d\n\n", cmd + 1, camera_index);
                }

                if ( hardwareActive )
                    camera->setParameters(params.flatten());

                break;

            case 'h':
                sharpness = atoi(cmd + 1);
                params.set(KEY_SHARPNESS, sharpness);

                if ( hardwareActive )
                    camera->setParameters(params.flatten());

                break;

            case '@':
                if ( hardwareActive ) {

                    closeCamera();

                    if ( 0 >= openCamera() ) {
                        printf( "Reconnected to CameraService \n");
                    }
                }

                break;

            case 'c':
                contrast = atoi(cmd + 1);
                params.set(KEY_CONTRAST, contrast);

                if ( hardwareActive ) {
                    camera->setParameters(params.flatten());
                }

                break;

            case 'z':
                zoomtoggle = false;

                if(strcmp(zoomstr, "true") == 0) {
                    for(i = 0; i < length_Zoom; i++) {
                        if( strcmp((cmd + 1), zoom[i].zoom_description) == 0) {
                            zoomIDX = i;
                            zoomtoggle = true;
                            break;
                        }
                    }

                    if (!zoomtoggle) {
                        printf("\nNot supported parameter %s from sensor %d\n\n", cmd + 1, camera_index);
                    }


                    params.set(CameraParameters::KEY_ZOOM, zoom[zoomIDX].idx);

                    if ( hardwareActive ) {
                        camera->setParameters(params.flatten());
                    }
                }

            case 'Z':
                smoothzoomtoggle = false;

                if(strcmp(smoothzoomstr, "true") == 0) {
                    for(i = 0; i < length_Zoom; i++) {
                        if( strcmp((cmd + 1), zoom[i].zoom_description) == 0) {
                            zoomIDX = i;
                            smoothzoomtoggle = true;
                            break;
                        }
                    }

                    if (!smoothzoomtoggle) {
                        printf("\nNot supported parameter %s from sensor %d\n\n", cmd + 1, camera_index);
                    }

                    if ( hardwareActive ) {
                        camera->sendCommand(CAMERA_CMD_START_SMOOTH_ZOOM, zoom[zoomIDX].idx, 0);
                    }
                }
                break;

            case 'j':

                a = checkSupportedParamScript(exposureMode, numExposureMode, cmd);
                if (a > -1) {
                    params.set(KEY_EXPOSURE, (cmd + 1));
                } else {
                    printf("\nNot supported parameter %s from sensor %d\n\n", cmd + 1, camera_index);
                }

                if ( hardwareActive )
                    camera->setParameters(params.flatten());

                break;

            case 'b':
                brightness = atoi(cmd + 1);
                params.set(KEY_BRIGHTNESS, brightness);

                if ( hardwareActive )
                    camera->setParameters(params.flatten());

                break;

            case 's':
                saturation = atoi(cmd + 1);
                params.set(KEY_SATURATION, saturation);

                if ( hardwareActive )
                    camera->setParameters(params.flatten());

                break;

            case 'e':
                a = checkSupportedParamScript(effectss, numEffects, cmd);
                if (a > -1) {
                    params.set(params.KEY_EFFECT, (cmd + 1));
                } else {
                    printf("\nNot supported parameter %s from sensor %d\n\n", cmd + 1, camera_index);
                }

                if ( hardwareActive )
                    camera->setParameters(params.flatten());

                break;

            case 'r':
                if (strcmp((cmd + 1), "MAX_FRAMERATE") == 0) {
                    frameRConst = 0;
                    for (int i=0; i<constCnt; i++) {
                        if (constFramerate[frameRConst] < constFramerate[i]) {
                            frameRConst = i;
                        }
                    }
                    if (0 < constFramerate[frameRConst]) {
                        params.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, fps_const_str[frameRConst]);
                        frameRateIDX = frameRConst;
                        printf("Framerate set: %d fps\n", constFramerate[frameRConst]);
                    } else {
                        printf("\nFramerate is 0!\n");
                    }
                } else {
                    a = checkSupportedParamScriptfpsConst(constFramerate, constCnt, cmd, &frameRConst);
                    if (a > -1) {
                        params.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, fps_const_str[frameRConst]);
                        frameRateIDX = frameRConst;
                    } else {
                        printf("\nNot supported parameter %s from sensor %d\n\n", cmd + 1, camera_index);
                    }
                }
                if ( hardwareActive && previewRunning ) {
                    camera->stopPreview();
                    camera->setParameters(params.flatten());
                    camera->startPreview();
                } else if ( hardwareActive ) {
                    camera->setParameters(params.flatten());
                }
                break;

            case 'R':
                a = checkSupportedParamScriptfpsRange(rangeDescription, rangeCnt, cmd, &frameRRange);
                if (a > -1) {
                    params.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, fps_range_str[frameRRange]);
                    fpsRangeIdx = frameRRange;
                } else {
                    printf("\nNot supported parameter %s from sensor %d\n\n", cmd + 1, camera_index);
                }
                break;

            case 'x':
                a = checkSupportedParamScript(antiband, numAntibanding, cmd);
                if (a > -1) {
                params.set(params.KEY_ANTIBANDING, (cmd + 1));
                } else {
                    printf("\nNot supported parameter %s from sensor %d\n\n", cmd + 1, camera_index);
                }

                if ( hardwareActive )
                    camera->setParameters(params.flatten());
                break;

            case 'g':
                a = checkSupportedParamScript(focus, numfocus, cmd);
                if (a > -1) {
                    params.set(params.KEY_FOCUS_MODE, (cmd + 1));
                } else {
                    printf("\nNot supported parameter %s from sensor %d\n\n", cmd + 1, camera_index);
                }

                if ( hardwareActive )
                    camera->setParameters(params.flatten());
                break;

            case 'G':

                params.set(CameraParameters::KEY_FOCUS_AREAS, (cmd + 1));

                if ( hardwareActive )
                    camera->setParameters(params.flatten());

                break;

            case 'y':

                params.set(CameraParameters::KEY_METERING_AREAS, (cmd + 1));

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
                const char *format = params.getPictureFormat();

                if((0 == strcmp(modevalues[capture_mode], "video-mode")) &&
                   (0 != strcmp(videosnapshotstr, "true"))) {
                    printf("Video Snapshot is not supported\n");
                } else if ( hardwareActive ) {
                    if((NULL != format) && isRawPixelFormat(format)) {
                        createBufferOutputSource();
                        if (bufferSourceOutputThread.get()) {
                            bufferSourceOutputThread->setBuffer(shotParams);
                            bufferSourceOutputThread->setStreamCapture(streamCapture, expBracketIdx);
                        }
                    } else if(strcmp(modevalues[capture_mode], "video-mode") == 0) {
                        msgType = CAMERA_MSG_COMPRESSED_IMAGE |
                                  CAMERA_MSG_RAW_IMAGE;
#ifdef OMAP_ENHANCEMENT_BURST_CAPTURE
                        msgType |= CAMERA_MSG_RAW_BURST;
#endif
                    } else {
                        msgType = CAMERA_MSG_POSTVIEW_FRAME |
                                  CAMERA_MSG_RAW_IMAGE_NOTIFY |
                                  CAMERA_MSG_COMPRESSED_IMAGE |
                                  CAMERA_MSG_SHUTTER;
#ifdef OMAP_ENHANCEMENT_BURST_CAPTURE
                        msgType |= CAMERA_MSG_RAW_BURST;
#endif
                    }

                    gettimeofday(&picture_start, 0);
                    ret = camera->setParameters(params.flatten());
                    if ( ret != NO_ERROR ) {
                        printf("Error returned while setting parameters");
                        break;
                    }
                    ret = camera->takePictureWithParameters(msgType, shotParams.flatten());
                    if ( ret != NO_ERROR ) {
                        printf("Error returned while taking a picture");
                        break;
                    }
                }
                break;
            }

            case 'S':
            {
                if (streamCapture) {
                    streamCapture = false;
                    expBracketIdx = BRACKETING_IDX_DEFAULT;
                    setDefaultExpGainPreset(shotParams, expBracketIdx);
                    // Stop streaming
                    if (bufferSourceOutputThread.get()) {
                        bufferSourceOutputThread->setStreamCapture(streamCapture, expBracketIdx);
                    }
                } else {
                    streamCapture = true;
                    expBracketIdx = BRACKETING_IDX_STREAM;
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
                        bufferSourceInput->setInput(info, params.getPictureFormat(), reprocParams);
                        if (hardwareActive) camera->reprocess(msgType, String8());
                    }
                }
                break;
            }

            case 'd':
                dly = atoi(cmd + 1);
                sleep(dly);
                break;

            case 'q':
                dump_mem_status();
                stopPreview();

                if ( recordingMode ) {
                    stopRecording();
                    closeRecorder();

                    recordingMode = false;
                }
                goto exit;

            case '\n':
                printf("Iteration: %d \n", iteration);
                iteration++;
                break;

            case '{':
                if ( atoi(cmd + 1) > 0 )
                    params.set(KEY_S3D2D_PREVIEW_MODE, "on");
                else
                    params.set(KEY_S3D2D_PREVIEW_MODE, "off");
                if ( hardwareActive )
                    camera->setParameters(params.flatten());
                break;

            case 'M':
                params.set(KEY_MEASUREMENT, (cmd + 1));
                if ( hardwareActive )
                    camera->setParameters(params.flatten());
                break;
            case 'm':
            {
                params.set(KEY_METERING_MODE, (cmd + 1));
                if ( hardwareActive )
                {
                    camera->setParameters(params.flatten());
                }
                break;
            }
            case '<':
            {
                char coord_str[8];
                latitude += degree_by_step;
                if (latitude > 90.0)
                {
                    latitude -= 180.0;
                }
                snprintf(coord_str, 7, "%.7lf", latitude);
                params.set(params.KEY_GPS_LATITUDE, coord_str);
                if ( hardwareActive )
                {
                    camera->setParameters(params.flatten());
                }
                break;
            }

            case '=':
            {
                char coord_str[8];
                longitude += degree_by_step;
                if (longitude > 180.0)
                {
                    longitude -= 360.0;
                }
                snprintf(coord_str, 7, "%.7lf", longitude);
                params.set(params.KEY_GPS_LONGITUDE, coord_str);
                if ( hardwareActive )
                {
                    camera->setParameters(params.flatten());
                }
                break;
            }

            case '>':
            {
                char coord_str[8];
                altitude += 12345.67890123456789;
                if (altitude > 100000.0)
                {
                    altitude -= 200000.0;
                }
                snprintf(coord_str, 7, "%.7lf", altitude);
                params.set(params.KEY_GPS_ALTITUDE, coord_str);
                if ( hardwareActive )
                {
                    camera->setParameters(params.flatten());
                }
                break;
            }

            case 'X':
            {
                char rem_str[384];
                printf("Deleting images from %s \n", images_dir_path);
                if (!sprintf(rem_str, "rm %s/*.jpg", images_dir_path)) {
                    printf("Sprintf Error");
                }
                if (system(rem_str)) {
                    printf("Images were not deleted\n");
                }
                break;
            }

            case '_':
            {
                AutoConvergenceModeIDX = atoi(cmd + 1);
                if ( AutoConvergenceModeIDX < 0 || AutoConvergenceModeIDX > 4 )
                    AutoConvergenceModeIDX = 0;
                params.set(KEY_AUTOCONVERGENCE, autoconvergencemode[AutoConvergenceModeIDX]);
                if (AutoConvergenceModeIDX != 4) {
                    params.set(KEY_MANUAL_CONVERGENCE, manualConv);
                }
                if (hardwareActive) {
                    camera->setParameters(params.flatten());
                }
                break;
            }

            case '^':
                if (strcmp(autoconvergencemode[AutoConvergenceModeIDX], "manual") == 0) {
                    manC = atoi(cmd + 1);
                    if(manC >= manualConvMin  &&  manC <= manualConvMax)
                    {
                        params.set(KEY_MANUAL_CONVERGENCE, manC);
                    }
                    else if(manC < manualConvMin)
                    {
                        printf(" wrong parameter for manual convergence \n");
                        params.set(KEY_MANUAL_CONVERGENCE, manualConvMin);
                    }
                    else
                    {
                        printf(" wrong parameter for manual convergence \n");
                        params.set(KEY_MANUAL_CONVERGENCE, manualConvMax);
                    }
                    if ( hardwareActive )
                        camera->setParameters(params.flatten());
                }
                break;


            case 'Q':
                if ( strcmp (exposureMode[exposure_mode], "manual") == 0) {
                    manE = atoi(cmd + 1);
                    if(manE >= manualExpMin  &&  manE <= manualExpMax)
                    {
                        params.set(KEY_MANUAL_EXPOSURE, manE);
                        params.set(KEY_MANUAL_EXPOSURE_RIGHT, manE);
                    }
                    else if(manE < manualExpMin)
                    {
                        printf(" wrong parameter for manual exposure \n");
                        params.set(KEY_MANUAL_EXPOSURE, manualExpMin);
                        params.set(KEY_MANUAL_EXPOSURE_RIGHT, manualExpMin);
                    }
                    else
                    {
                        printf(" wrong parameter for manual exposure \n");
                        params.set(KEY_MANUAL_EXPOSURE, manualExpMax);
                        params.set(KEY_MANUAL_EXPOSURE_RIGHT, manualExpMax);
                    }

                    if ( hardwareActive )
                    camera->setParameters(params.flatten());
                }
                break;

            case ',':
                if ( strcmp (exposureMode[exposure_mode], "manual") == 0) {
                    manG = atoi(cmd + 1);
                    if(manG >= manualGainMin  &&  manG <= manualGainMax)
                    {
                        params.set(KEY_MANUAL_GAIN_ISO, manG);
                        params.set(KEY_MANUAL_GAIN_ISO_RIGHT, manG);
                    }
                    else if(manG < manualGainMin)
                    {
                        printf(" wrong parameter for manual gain \n");
                        params.set(KEY_MANUAL_GAIN_ISO, manualGainMin);
                        params.set(KEY_MANUAL_GAIN_ISO_RIGHT, manualGainMin);
                    }
                    else
                    {
                        printf(" wrong parameter for manual gain \n");
                        params.set(KEY_MANUAL_GAIN_ISO, manualGainMax);
                        params.set(KEY_MANUAL_GAIN_ISO_RIGHT, manualGainMax);
                    }

                    if ( hardwareActive )
                    camera->setParameters(params.flatten());
                }
                break;

            default:
                printf("Unrecognized command!\n");
                break;
        }

        cmd = strtok_r(NULL, DELIMITER, &ctx);
    }

exit:
    if (stopScript == true)
      {
        return -1;
      }
    else
      {
        return 0;
      }
}


int checkSupportedParamScript(char **array, int size, char *param) {
    for (int i=0; i<size; i++) {
        if (strcmp((param + 1), array[i]) == 0) {
            return 0;
        }
    }
    return -1;
}

int checkSupportedParamScriptLayout(char **array, int size, char *param, int *index) {
    for (int i=0; i<size; i++) {
        if (strcmp((param + 1), array[i]) == 0) {
            *index = i;
            return 0;
        }
    }
    return -1;
}

int checkSupportedParamScriptResol(param_Array **array, int size, char *param, int *num) {
    for (int i=0; i<size; i++) {
        if (strcmp((param + 1), array[i]->name) == 0) {
            *num = i;
            return 0;
        }
    }
    return -1;
}

int checkSupportedParamScriptResol(param_Array **array, int size,
                                   int width, int height, int *num) {
    for (int i=0; i<size; i++) {
        if ((width == array[i]->width) && (height == array[i]->height)) {
            *num = i;
            return 0;
        }
    }
    return -1;
}

int checkSupportedParamScriptfpsConst(int *array, int size, char *param, int *num) {
    for (int i=0; i<size; i++) {
        if (atoi(param + 1) == array[i]) {
            *num = i;
            return 0;
        }
    }
    return -1;
}

int checkSupportedParamScriptfpsRange(char **array, int size, char *param, int *num) {
    for (int i=0; i<size; i++) {
        if (strcmp(param + 1, array[i]) == 0) {
            *num = i;
            return 0;
        }
    }
    return -1;
}

char * get_cycle_cmd(const char *aSrc) {
    unsigned ind = 0;
    char *cycle_cmd = new char[256];

    while ((*aSrc != '+') && (*aSrc != '\0')) {
        cycle_cmd[ind++] = *aSrc++;
    }
    cycle_cmd[ind] = '\0';

    return cycle_cmd;
}

status_t dump_mem_status() {
  system(MEDIASERVER_DUMP);
  return system(MEMORY_DUMP);
}

char *load_script(const char *config) {
    FILE *infile;
    size_t fileSize;
    char *script;
    size_t nRead = 0;

    infile = fopen(config, "r");

    strcpy(script_name,config);

    printf("\n SCRIPT : <%s> is currently being executed \n", script_name);

    printf("\n DIRECTORY CREATED FOR TEST RESULT IMAGES IN MMC CARD : %s \n", output_dir_path);

    if( (NULL == infile)){
        printf("Error while opening script file %s!\n", config);
        return NULL;
    }

    fseek(infile, 0, SEEK_END);
    fileSize = ftell(infile);
    fseek(infile, 0, SEEK_SET);

    script = (char *) malloc(fileSize + 1);

    if ( NULL == script ) {
        printf("Unable to allocate buffer for the script\n");

        return NULL;
    }

    memset(script, 0, fileSize + 1);

    if ((nRead = fread(script, 1, fileSize, infile)) != fileSize) {
        printf("Error while reading script file!\n");

        free(script);
        fclose(infile);
        return NULL;
    }

    fclose(infile);

    return script;
}

int start_logging(int flags, int &pid) {
    int status = 0;

    if (flags == 0) {
        pid = -1;
        return 0;
    }

    pid = fork();
    if (pid == 0)
    {
        char *command_list[] = {"sh", "-c", NULL, NULL};
        char log_cmd[1024];
        // child process to run logging

        // set group id of this process to itself
        // we will use this group id to kill the
        // application logging
        setpgid(getpid(), getpid());

        /* Start logcat */
        if (flags & LOGGING_LOGCAT) {
            if (!sprintf(log_cmd,"logcat > %s/log.txt &", output_dir_path)) {
                printf(" Sprintf Error");
            }
        }

        /* Start Syslink Trace */
        if (flags & LOGGING_SYSLINK) {
            if (!sprintf(log_cmd,"%s /system/bin/syslink_trace_daemon.out -l %s/syslink_trace.txt -f &", log_cmd, output_dir_path)) {
                printf(" Sprintf Error");
            }
        }

        command_list[2] = (char *)log_cmd;
        execvp("/system/bin/sh", command_list);
    } if(pid < 0)
    {
        printf("failed to fork logcat\n");
        return -1;
    }

    //wait for logging to start
    if(waitpid(pid, &status, 0) != pid)
    {
        printf("waitpid failed in log fork\n");
        return -1;
    }else
        printf("logging started... status=%d\n", status);

    return 0;
}

int stop_logging(int flags, int &pid)
{
    if (pid > 0) {
        if (killpg(pid, SIGKILL)) {
            printf("Exit command failed");
            return -1;
        } else {
            printf("\nlogging for script %s is complete\n", script_name);

            if (flags & LOGGING_LOGCAT) {
                printf("   logcat saved @ location: %s\n", output_dir_path);
            }

            if (flags & LOGGING_SYSLINK) {
                printf("   syslink_trace is saved @ location: %s\n\n", output_dir_path);
            }
        }
    }
    return 0;
}

int execute_error_script(char *script) {
    char *cmd, *ctx;
    char id;
    status_t stat = NO_ERROR;

    LOG_FUNCTION_NAME;

    cmd = strtok_r((char *) script, DELIMITER, &ctx);

    while ( NULL != cmd ) {
        id = cmd[0];

        switch (id) {

            case '0': {
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
                //camera = Camera::connect();

                if ( NULL == camera.get() ) {
                    printf("Unable to connect to CameraService\n");
                    return -1;
                }

                break;
            }

            case '3': {
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
                goto exit;

                break;
            }

            default: {
                printf("Unrecognized command!\n");

                break;
            }
        }

        cmd = strtok_r(NULL, DELIMITER, &ctx);
    }

exit:

    return 0;
}



