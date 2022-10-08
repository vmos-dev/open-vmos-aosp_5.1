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

#include <sys/wait.h>

#include "camera_test.h"

using namespace android;

extern bool stopScript;
extern bool hardwareActive;
extern sp<Camera> camera;
extern CameraParameters params;
extern bool recordingMode;
extern int camera_index;
extern int rotation;
extern const preview_size previewSize [];
extern const Vcapture_size VcaptureSize [];
extern const capture_Size captureSize[];
extern const outformat outputFormat[];
extern const video_Codecs videoCodecs[];
extern const audio_Codecs audioCodecs[];
extern const V_bitRate VbitRate[];
extern const fps_ranges fpsRanges[];
extern const fpsConst_Ranges fpsConstRanges[];
extern const fpsConst_RangesSec fpsConstRangesSec[];
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
extern timeval autofocus_start, picture_start;
extern const char *cameras[];
extern double latitude;
extern double degree_by_step;
extern double longitude;
extern double altitude;
extern char dir_path[80];
extern int AutoConvergenceModeIDX;
extern const char *autoconvergencemode[];
extern const char *manualconvergencevalues[];
extern const int ManualConvergenceDefaultValueIDX;
extern size_t length_cam;
extern char script_name[];
extern int restartCount;
extern bool bLogSysLinkTrace;
extern int bufferStarvationTest;
extern size_t length_previewSize;
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

static const String16 processName("camera_test");

int execute_functional_script(char *script) {
    char *cmd, *ctx, *cycle_cmd, *temp_cmd;
    char id;
    unsigned int i;
    int dly;
    int cycleCounter = 1;
    int tLen = 0;
    unsigned int iteration = 0;
    status_t ret = NO_ERROR;
    int frameR = 20;
    int frameRateIndex = 0;

    LOG_FUNCTION_NAME;

    dump_mem_status();

    cmd = strtok_r((char *) script, DELIMITER, &ctx);

    while ( NULL != cmd && (stopScript == false)) {
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
                rotation = atoi(cmd + 1);
                params.set(CameraParameters::KEY_ROTATION, rotation);

                if ( hardwareActive )
                    camera->setParameters(params.flatten());

                break;

            case '4':
            {
                printf("Setting resolution...");
                int width, height;
                for(i = 0; i < length_previewSize ; i++)
                {
                    if( strcmp((cmd + 1), previewSize[i].desc) == 0)
                    {
                        width = previewSize[i].width;
                        height = previewSize[i].height;
                        previewSizeIDX = i;
                        break;
                    }
                }

                if (i == length_previewSize )   //if the resolution is not in the supported ones
                {
                    char *res = NULL;
                    res = strtok(cmd + 1, "x");
                    width = atoi(res);
                    res = strtok(NULL, "x");
                    height = atoi(res);
                }

                if ( NULL != params.get(KEY_STEREO_CAMERA) ) {
                    if ( strcmp(params.get(KEY_STEREO_CAMERA), "true") == 0 ) {
                        height *=2;
                    }
                }

                printf("Resolution: %d x %d\n", width, height);
                params.setPreviewSize(width, height);
                reSizePreview = true;

                if ( hardwareActive && previewRunning ) {
                    camera->stopPreview();
                    camera->setParameters(params.flatten());
                    camera->startPreview();
                } else if ( hardwareActive ) {
                    camera->setParameters(params.flatten());
                }

                break;
            }
            case '5':

                for (i = 0; i < length_capture_Size; i++) {
                    if ( strcmp((cmd + 1), captureSize[i].name) == 0)
                        break;
                }

                if (  i < length_capture_Size ) {
                    params.setPictureSize(captureSize[i].width, captureSize[i].height);
                    captureSizeIDX = i;
                }

                if ( hardwareActive )
                    camera->setParameters(params.flatten());

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
                params.set(params.KEY_WHITE_BALANCE, (cmd + 1));

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
                params.setPreviewFormat(cmd + 1);
                if ( hardwareActive )
                    camera->setParameters(params.flatten());

                break;

            case '$':
                params.setPictureFormat(cmd + 1);
                if ( hardwareActive )
                    camera->setParameters(params.flatten());

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
           //     camera_index %= ARRAY_SIZE(cameras);
                camera_index %= length_cam;
                if ( camera_index == 2)
                    params.set(KEY_STEREO_CAMERA, "true");
                else
                    params.set(KEY_STEREO_CAMERA, "false");

                printf("%s selected.\n", cameras[camera_index]);

                if ( hardwareActive ) {
                    stopPreview();
                    closeCamera();
                    openCamera();
                } else {
                    closeCamera();
                    openCamera();
                }

                if (camera_index == 0) params.setPreviewFrameRate(30);
                  else params.setPreviewFrameRate(27);


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
            case 'L':
                for(i = 0; i < lenght_Vcapture_size; i++)
                {
                    if( strcmp((cmd + 1), VcaptureSize[i].desc) == 0)
                    {
                        VcaptureSizeIDX = i;
                        printf("Video Capture Size: %s\n", VcaptureSize[i].desc);
                        break;
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
                int width, height;
                for(i = 0; i < length_previewSize ; i++)
                {
                    if( strcmp((cmd + 1), previewSize[i].desc) == 0)
                    {
                        width = previewSize[i].width;
                        height = previewSize[i].height;
                        thumbSizeIDX = i;
                        break;
                    }
                }

                if (i == length_previewSize )   //if the resolution is not in the supported ones
                {
                    char *res = NULL;
                    res = strtok(cmd + 1, "x");
                    width = atoi(res);
                    res = strtok(NULL, "x");
                    height = atoi(res);
                }

                params.set(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH, width);
                params.set(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, height);

                if ( hardwareActive )
                    camera->setParameters(params.flatten());

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

                break;

            case 'K':
                params.set(KEY_GBCE, (cmd+1));
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

            case 'O':
                params.set(KEY_GLBCE, (cmd+1));
                if ( hardwareActive )
                    camera->setParameters(params.flatten());

                break;

            case 'u':
                // HQ should always be in ldc-nsf
                // if not HQ, then return the ipp to its previous state
                if( !strcmp(capture[capture_mode], "high-quality") ) {
                    ippIDX_old = ippIDX;
                    ippIDX = 3;
                    params.set(KEY_IPP, ipp_mode[ippIDX]);
                } else {
                    ippIDX = ippIDX_old;
                }

                params.set(KEY_MODE, (cmd + 1));
                if ( hardwareActive )
                    camera->setParameters(params.flatten());

                break;

            case 'U':

                params.set(KEY_TEMP_BRACKETING, (cmd + 1));

                if ( hardwareActive )
                    camera->setParameters(params.flatten());

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

                params.set(KEY_BURST, atoi(cmd + 1));

                if ( hardwareActive )
                    camera->setParameters(params.flatten());

                break;

            case 'J':
                params.set(CameraParameters::KEY_FLASH_MODE, (cmd+1));

                if ( hardwareActive )
                    camera->setParameters(params.flatten());

                break;

            case 'w':
                params.set(params.KEY_SCENE_MODE, (cmd + 1));

                if ( hardwareActive )
                    camera->setParameters(params.flatten());

                break;

            case 'B' :
                params.set(KEY_VNF, (cmd + 1));

                if ( hardwareActive )
                    camera->setParameters(params.flatten());


            case 'C' :
                params.set(KEY_VSTAB, (cmd + 1));

                if ( hardwareActive )
                    camera->setParameters(params.flatten());
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
                params.set(KEY_ISO, iso_mode);

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
            case 'Z':

#if defined(OMAP_ENHANCEMENT) && defined(TARGET_OMAP3)
                params.set(CameraParameters::KEY_ZOOM, atoi(cmd + 1));
#else

                for(i = 0; i < length_Zoom; i++)
                {
                    if( strcmp((cmd + 1), zoom[i].zoom_description) == 0)
                    {
                        zoomIDX = i;
                        break;
                    }
                }

                params.set(CameraParameters::KEY_ZOOM, zoom[zoomIDX].idx);
#endif

                if ( hardwareActive )
                    camera->setParameters(params.flatten());

                break;

            case 'j':
                params.set(KEY_EXPOSURE, (cmd + 1));

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
                params.set(params.KEY_EFFECT, (cmd + 1));

                if ( hardwareActive )
                    camera->setParameters(params.flatten());

                break;

            case 'r':

                frameR = atoi(cmd + 1);


                if (camera_index == 0) {
                    for (i = 0; i < length_fpsConst_Ranges; i++) {
                        if (frameR == fpsConstRanges[i].constFramerate)
                            frameRateIndex = i;

                    }
                } else {
                    for (i = 0; i < length_fpsConst_RangesSec; i++) {
                        if (frameR == fpsConstRangesSec[i].constFramerate)
                            frameRateIndex = i;
                    }
                }


                if (camera_index == 0)
                    params.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, fpsConstRanges[frameRateIndex].range);
                else
                    params.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, fpsConstRangesSec[frameRateIndex].range);


                if ( hardwareActive && previewRunning ) {
                    camera->stopPreview();
                    camera->setParameters(params.flatten());
                    camera->startPreview();
                } else if ( hardwareActive ) {
                    camera->setParameters(params.flatten());
                }

                break;

            case 'R':
                for(i = 0; i < length_fps_ranges; i++)
                {
                    if( strcmp((cmd + 1), fpsRanges[i].rangeDescription) == 0)
                    {
                        fpsRangeIdx = i;
                        printf("Selected Framerate range: %s\n", fpsRanges[i].rangeDescription);
                        if ( hardwareActive ) {
                            params.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, fpsRanges[i].range);
                            params.remove(CameraParameters::KEY_PREVIEW_FRAME_RATE);
                            camera->setParameters(params.flatten());
                        }
                        break;
                    }
                }
                break;

            case 'x':
                params.set(params.KEY_ANTIBANDING, (cmd + 1));

                if ( hardwareActive )
                    camera->setParameters(params.flatten());

                break;

            case 'g':
                params.set(params.KEY_FOCUS_MODE, (cmd + 1));

                if ( hardwareActive )
                    camera->setParameters(params.flatten());

                break;

            case 'G':

                params.set(CameraParameters::KEY_FOCUS_AREAS, (cmd + 1));

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
                    ret = camera->takePicture(CAMERA_MSG_COMPRESSED_IMAGE|CAMERA_MSG_RAW_IMAGE);

                if ( ret != NO_ERROR )
                    printf("Error returned while taking a picture");
                break;

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
                char rem_str[50];
                printf("Deleting images from %s \n", dir_path);
                if(!sprintf(rem_str,"rm %s/*.jpg",dir_path))
                    printf("Sprintf Error");
                if(system(rem_str))
                    printf("Images were not deleted\n");
                break;
            }

            case '_':
            {
                AutoConvergenceModeIDX = atoi(cmd + 1);
                if ( AutoConvergenceModeIDX < 0 || AutoConvergenceModeIDX > 4 )
                    AutoConvergenceModeIDX = 0;
                params.set(KEY_AUTOCONVERGENCE, autoconvergencemode[AutoConvergenceModeIDX]);
                if ( AutoConvergenceModeIDX != 4 )
                    params.set(KEY_MANUALCONVERGENCE_VALUES, manualconvergencevalues[ManualConvergenceDefaultValueIDX]);
                if ( hardwareActive )
                    camera->setParameters(params.flatten());
                break;
            }

            case '^':
            {
                char strtmpval[7];
                if ( strcmp (autoconvergencemode[AutoConvergenceModeIDX], AUTOCONVERGENCE_MODE_MANUAL) == 0) {
                    sprintf(strtmpval,"%d", atoi(cmd + 1));
                    params.set(KEY_MANUALCONVERGENCE_VALUES, strtmpval);
                    if ( hardwareActive )
                        camera->setParameters(params.flatten());
                }
                break;
            }

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

char *load_script(char *config) {
    FILE *infile;
    size_t fileSize;
    char *script;
    size_t nRead = 0;
    char dir_name[40];
    size_t count;
    char rCount [5];

    count = 0;

    infile = fopen(config, "r");

    strcpy(script_name,config);

    // remove just the '.txt' part of the config
    while((config[count] != '.') && (count < sizeof(dir_name)/sizeof(dir_name[0])))
        count++;

    printf("\n SCRIPT : <%s> is currently being executed \n",script_name);
    if(strncpy(dir_name,config,count) == NULL)
        printf("Strcpy error");

    dir_name[count]=NULL;

    if(strcat(dir_path,dir_name) == NULL)
        printf("Strcat error");

    if(restartCount)
    {
      sprintf(rCount,"_%d",restartCount);
      if(strcat(dir_path, rCount) == NULL)
        printf("Strcat error RestartCount");
    }

    printf("\n COMPLETE FOLDER PATH : %s \n",dir_path);
    if(mkdir(dir_path,0777) == -1) {
        printf("\n Directory %s was not created \n",dir_path);
    } else {
        printf("\n Directory %s was created \n",dir_path);
    }
    printf("\n DIRECTORY CREATED FOR TEST RESULT IMAGES IN MMC CARD : %s \n",dir_name);

    if( (NULL == infile)){
        printf("Error while opening script file %s!\n", config);
        return NULL;
    }

    fseek(infile, 0, SEEK_END);
    fileSize = ftell(infile);
    fseek(infile, 0, SEEK_SET);

    script = (char *) malloc(fileSize);

    if ( NULL == script ) {
        printf("Unable to allocate buffer for the script\n");

        return NULL;
    }

    if ((nRead = fread(script, 1, fileSize, infile)) != fileSize) {
        printf("Error while reading script file!\n");

        free(script);
        fclose(infile);
        return NULL;
    }

    fclose(infile);

    return script;
}

int start_logging(char *config, int &pid) {
    char dir_name[40];
    size_t count = 0;
    int status = 0;

    // remove just the '.txt' part of the config
    while((config[count] != '.') && (count < sizeof(dir_name)/sizeof(dir_name[0])))
        count++;

    if(strncpy(dir_name,config,count) == NULL)
        printf("Strcpy error");

    dir_name[count]=NULL;

    pid = fork();
    if (pid == 0)
    {
        char *command_list[] = {"sh", "-c", NULL, NULL};
        char log_cmd[120];
        // child process to run logging

        // set group id of this process to itself
        // we will use this group id to kill the
        // application logging
        setpgid(getpid(), getpid());

        /* Start logcat */
        if(!sprintf(log_cmd,"logcat > /sdcard/%s/log.txt &",dir_name))
            printf(" Sprintf Error");

        /* Start Syslink Trace */
        if(bLogSysLinkTrace) {
            if(!sprintf(log_cmd,"%s /system/bin/syslink_trace_daemon.out -l /sdcard/%s/syslink_trace.txt -f &",log_cmd, dir_name))
                printf(" Sprintf Error");
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

int stop_logging(int &pid)
{
    if(pid > 0)
    {
        if(killpg(pid, SIGKILL))
        {
            printf("Exit command failed");
            return -1;
        } else {
            printf("\nlogging for script %s is complete\n   logcat saved @ location: %s\n",script_name,dir_path);
            if (bLogSysLinkTrace)
                printf("   syslink_trace is saved @ location: %s\n\n",dir_path);
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



