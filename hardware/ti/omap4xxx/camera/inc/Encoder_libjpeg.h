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
* @file Encoder_libjpeg.h
*
* This defines API for camerahal to encode YUV using libjpeg
*
*/

#ifndef ANDROID_CAMERA_HARDWARE_ENCODER_LIBJPEG_H
#define ANDROID_CAMERA_HARDWARE_ENCODER_LIBJPEG_H

#include <utils/threads.h>
#include <utils/RefBase.h>

extern "C" {
#include "jhead.h"
}

#define CANCEL_TIMEOUT 3000000 // 3 seconds

namespace android {
/**
 * libjpeg encoder class - uses libjpeg to encode yuv
 */

#define MAX_EXIF_TAGS_SUPPORTED 30
typedef void (*encoder_libjpeg_callback_t) (void* main_jpeg,
                                            void* thumb_jpeg,
                                            CameraFrame::FrameType type,
                                            void* cookie1,
                                            void* cookie2,
                                            void* cookie3,
                                            bool canceled);

// these have to match strings defined in external/jhead/exif.c
static const char TAG_MODEL[] = "Model";
static const char TAG_MAKE[] = "Make";
static const char TAG_FOCALLENGTH[] = "FocalLength";
static const char TAG_DATETIME[] = "DateTime";
static const char TAG_IMAGE_WIDTH[] = "ImageWidth";
static const char TAG_IMAGE_LENGTH[] = "ImageLength";
static const char TAG_GPS_LAT[] = "GPSLatitude";
static const char TAG_GPS_LAT_REF[] = "GPSLatitudeRef";
static const char TAG_GPS_LONG[] = "GPSLongitude";
static const char TAG_GPS_LONG_REF[] = "GPSLongitudeRef";
static const char TAG_GPS_ALT[] = "GPSAltitude";
static const char TAG_GPS_ALT_REF[] = "GPSAltitudeRef";
static const char TAG_GPS_MAP_DATUM[] = "GPSMapDatum";
static const char TAG_GPS_PROCESSING_METHOD[] = "GPSProcessingMethod";
static const char TAG_GPS_VERSION_ID[] = "GPSVersionID";
static const char TAG_GPS_TIMESTAMP[] = "GPSTimeStamp";
static const char TAG_GPS_DATESTAMP[] = "GPSDateStamp";
static const char TAG_ORIENTATION[] = "Orientation";
static const char TAG_FLASH[] = "Flash";
static const char TAG_DIGITALZOOMRATIO[] = "DigitalZoomRatio";
static const char TAG_EXPOSURETIME[] = "ExposureTime";
static const char TAG_APERTURE[] = "ApertureValue";
static const char TAG_ISO_EQUIVALENT[] = "ISOSpeedRatings";
static const char TAG_WHITEBALANCE[] = "WhiteBalance";
static const char TAG_LIGHT_SOURCE[] = "LightSource";
static const char TAG_METERING_MODE[] = "MeteringMode";
static const char TAG_EXPOSURE_PROGRAM[] = "ExposureProgram";
static const char TAG_COLOR_SPACE[] = "ColorSpace";
static const char TAG_CPRS_BITS_PER_PIXEL[] = "CompressedBitsPerPixel";
static const char TAG_FNUMBER[] = "FNumber";
static const char TAG_SHUTTERSPEED[] = "ShutterSpeedValue";
static const char TAG_SENSING_METHOD[] = "SensingMethod";
static const char TAG_CUSTOM_RENDERED[] = "CustomRendered";

class ExifElementsTable {
    public:
        ExifElementsTable() :
           gps_tag_count(0), exif_tag_count(0), position(0),
           jpeg_opened(false), has_datetime_tag(false) { }
        ~ExifElementsTable();

        status_t insertElement(const char* tag, const char* value);
        void insertExifToJpeg(unsigned char* jpeg, size_t jpeg_size);
        status_t insertExifThumbnailImage(const char*, int);
        void saveJpeg(unsigned char* picture, size_t jpeg_size);
        static const char* degreesToExifOrientation(unsigned int);
        static void stringToRational(const char*, unsigned int*, unsigned int*);
        static bool isAsciiTag(const char* tag);
    private:
        ExifElement_t table[MAX_EXIF_TAGS_SUPPORTED];
        unsigned int gps_tag_count;
        unsigned int exif_tag_count;
        unsigned int position;
        bool jpeg_opened;
        bool has_datetime_tag;
};

class Encoder_libjpeg : public Thread {
    /* public member types and variables */
    public:
        struct params {
            uint8_t* src;
            int src_size;
            uint8_t* dst;
            int dst_size;
            int quality;
            int in_width;
            int in_height;
            int out_width;
            int out_height;
            int right_crop;
            int start_offset;
            const char* format;
            size_t jpeg_size;
         };
    /* public member functions */
    public:
        Encoder_libjpeg(params* main_jpeg,
                        params* tn_jpeg,
                        encoder_libjpeg_callback_t cb,
                        CameraFrame::FrameType type,
                        void* cookie1,
                        void* cookie2,
                        void* cookie3)
            : Thread(false), mMainInput(main_jpeg), mThumbnailInput(tn_jpeg), mCb(cb),
              mCancelEncoding(false), mCookie1(cookie1), mCookie2(cookie2), mCookie3(cookie3),
              mType(type), mThumb(NULL) {
            this->incStrong(this);
            mCancelSem.Create(0);
        }

        ~Encoder_libjpeg() {
            CAMHAL_LOGVB("~Encoder_libjpeg(%p)", this);
        }

        virtual bool threadLoop() {
            size_t size = 0;
            sp<Encoder_libjpeg> tn = NULL;
            if (mThumbnailInput) {
                // start thread to encode thumbnail
                mThumb = new Encoder_libjpeg(mThumbnailInput, NULL, NULL, mType, NULL, NULL, NULL);
                mThumb->run();
            }

            // encode our main image
            size = encode(mMainInput);

            // signal cancel semaphore incase somebody is waiting
            mCancelSem.Signal();

            // check if it is main jpeg thread
            if(mThumb.get()) {
                // wait until tn jpeg thread exits.
                mThumb->join();
                mThumb.clear();
                mThumb = NULL;
            }

            if(mCb) {
                mCb(mMainInput, mThumbnailInput, mType, mCookie1, mCookie2, mCookie3, mCancelEncoding);
            }

            // encoder thread runs, self-destructs, and then exits
            this->decStrong(this);
            return false;
        }

        void cancel() {
           mCancelEncoding = true;
           if (mThumb.get()) {
               mThumb->cancel();
               mCancelSem.WaitTimeout(CANCEL_TIMEOUT);
           }
        }

        void getCookies(void **cookie1, void **cookie2, void **cookie3) {
            if (cookie1) *cookie1 = mCookie1;
            if (cookie2) *cookie2 = mCookie2;
            if (cookie3) *cookie3 = mCookie3;
        }

    private:
        params* mMainInput;
        params* mThumbnailInput;
        encoder_libjpeg_callback_t mCb;
        bool mCancelEncoding;
        void* mCookie1;
        void* mCookie2;
        void* mCookie3;
        CameraFrame::FrameType mType;
        sp<Encoder_libjpeg> mThumb;
        Semaphore mCancelSem;

        size_t encode(params*);
};

}

#endif
