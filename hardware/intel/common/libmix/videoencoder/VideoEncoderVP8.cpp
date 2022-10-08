/*
* Copyright (c) 2009-2011 Intel Corporation.  All rights reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include <string.h>
#include <stdlib.h>
#include "VideoEncoderLog.h"
#include "VideoEncoderVP8.h"
#include <va/va_tpi.h>
#include <va/va_enc_vp8.h>

VideoEncoderVP8::VideoEncoderVP8()
    :VideoEncoderBase() {

        mVideoParamsVP8.profile = 0;
        mVideoParamsVP8.error_resilient = 0;
        mVideoParamsVP8.num_token_partitions = 4;
        mVideoParamsVP8.kf_auto = 0;
        mVideoParamsVP8.kf_min_dist = 128;
        mVideoParamsVP8.kf_max_dist = 128;
        mVideoParamsVP8.min_qp = 0;
        mVideoParamsVP8.max_qp = 63;
        mVideoParamsVP8.init_qp = 26;
        mVideoParamsVP8.rc_undershoot = 100;
        mVideoParamsVP8.rc_overshoot = 100;
        mVideoParamsVP8.hrd_buf_size = 1000;
        mVideoParamsVP8.hrd_buf_initial_fullness = 500;
        mVideoParamsVP8.hrd_buf_optimal_fullness = 600;
        mVideoParamsVP8.max_frame_size_ratio = 0;

        mVideoConfigVP8.force_kf = 0;
        mVideoConfigVP8.refresh_entropy_probs = 0;
        mVideoConfigVP8.value = 0;
        mVideoConfigVP8.sharpness_level = 2;

        mVideoConfigVP8ReferenceFrame.no_ref_last = 0;
        mVideoConfigVP8ReferenceFrame.no_ref_gf = 0;
        mVideoConfigVP8ReferenceFrame.no_ref_arf = 0;
        mVideoConfigVP8ReferenceFrame.refresh_last = 1;
        mVideoConfigVP8ReferenceFrame.refresh_golden_frame = 1;
        mVideoConfigVP8ReferenceFrame.refresh_alternate_frame = 1;

        mComParams.profile = VAProfileVP8Version0_3;
}

VideoEncoderVP8::~VideoEncoderVP8() {
}

Encode_Status VideoEncoderVP8::start() {

    Encode_Status ret = ENCODE_SUCCESS;
    LOG_V( "Begin\n");

    ret = VideoEncoderBase::start ();
    CHECK_ENCODE_STATUS_RETURN("VideoEncoderBase::start");

    if (mComParams.rcMode == VA_RC_VCM) {
        mRenderBitRate = false;
    }

    LOG_V( "end\n");
    return ret;
}


Encode_Status VideoEncoderVP8::renderSequenceParams() {
    Encode_Status ret = ENCODE_SUCCESS;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VAEncSequenceParameterBufferVP8 vp8SeqParam = VAEncSequenceParameterBufferVP8();

    LOG_V( "Begin\n");

    vp8SeqParam.frame_width = mComParams.resolution.width;
    vp8SeqParam.frame_height = mComParams.resolution.height;
    vp8SeqParam.error_resilient = mVideoParamsVP8.error_resilient;
    vp8SeqParam.kf_auto = mVideoParamsVP8.kf_auto;
    vp8SeqParam.kf_min_dist = mVideoParamsVP8.kf_min_dist;
    vp8SeqParam.kf_max_dist = mVideoParamsVP8.kf_max_dist;
    vp8SeqParam.bits_per_second = mComParams.rcParams.bitRate;
    memcpy(vp8SeqParam.reference_frames, mAutoRefSurfaces, sizeof(mAutoRefSurfaces) * mAutoReferenceSurfaceNum);

    vaStatus = vaCreateBuffer(
            mVADisplay, mVAContext,
            VAEncSequenceParameterBufferType,
            sizeof(vp8SeqParam),
            1, &vp8SeqParam,
            &mSeqParamBuf);
    CHECK_VA_STATUS_RETURN("vaCreateBuffer");

    vaStatus = vaRenderPicture(mVADisplay, mVAContext, &mSeqParamBuf, 1);
    CHECK_VA_STATUS_RETURN("vaRenderPicture");

    LOG_V( "End\n");
	return ret;
}

Encode_Status VideoEncoderVP8::renderPictureParams(EncodeTask *task) {
    Encode_Status ret = ENCODE_SUCCESS;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VAEncPictureParameterBufferVP8 vp8PicParam = VAEncPictureParameterBufferVP8();
    LOG_V( "Begin\n");

    vp8PicParam.coded_buf = task->coded_buffer;
    vp8PicParam.pic_flags.value = 0;
    vp8PicParam.ref_flags.bits.force_kf = mVideoConfigVP8.force_kf; //0;
    if(!vp8PicParam.ref_flags.bits.force_kf) {
        vp8PicParam.ref_flags.bits.no_ref_last = mVideoConfigVP8ReferenceFrame.no_ref_last;
        vp8PicParam.ref_flags.bits.no_ref_arf = mVideoConfigVP8ReferenceFrame.no_ref_arf;
        vp8PicParam.ref_flags.bits.no_ref_gf = mVideoConfigVP8ReferenceFrame.no_ref_gf;
    }
    vp8PicParam.pic_flags.bits.refresh_entropy_probs = 0;
    vp8PicParam.sharpness_level = 2;
    vp8PicParam.pic_flags.bits.num_token_partitions = 2;
    vp8PicParam.pic_flags.bits.refresh_last = mVideoConfigVP8ReferenceFrame.refresh_last;
    vp8PicParam.pic_flags.bits.refresh_golden_frame = mVideoConfigVP8ReferenceFrame.refresh_golden_frame;
    vp8PicParam.pic_flags.bits.refresh_alternate_frame = mVideoConfigVP8ReferenceFrame.refresh_alternate_frame;

    vaStatus = vaCreateBuffer(
            mVADisplay, mVAContext,
            VAEncPictureParameterBufferType,
            sizeof(vp8PicParam),
            1, &vp8PicParam,
            &mPicParamBuf);
    CHECK_VA_STATUS_RETURN("vaCreateBuffer");

    vaStatus = vaRenderPicture(mVADisplay, mVAContext, &mPicParamBuf, 1);
    CHECK_VA_STATUS_RETURN("vaRenderPicture");

    LOG_V( "End\n");
    return ret;
}

Encode_Status VideoEncoderVP8::renderRCParams(uint32_t layer_id, bool total_bitrate)
{
    VABufferID rc_param_buf;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VAEncMiscParameterBuffer *misc_param;
    VAEncMiscParameterRateControl *misc_rate_ctrl;

    vaStatus = vaCreateBuffer(mVADisplay, mVAContext,
                              VAEncMiscParameterBufferType,
                              sizeof(VAEncMiscParameterBuffer) + sizeof(VAEncMiscParameterRateControl),
                              1,NULL,&rc_param_buf);
    CHECK_VA_STATUS_RETURN("vaCreateBuffer");

    vaMapBuffer(mVADisplay, rc_param_buf,(void **)&misc_param);

    misc_param->type = VAEncMiscParameterTypeRateControl;
    misc_rate_ctrl = (VAEncMiscParameterRateControl *)misc_param->data;
    memset(misc_rate_ctrl, 0, sizeof(*misc_rate_ctrl));

    if(total_bitrate)
        misc_rate_ctrl->bits_per_second = mComParams.rcParams.bitRate;
    else
    {
        misc_rate_ctrl->rc_flags.bits.temporal_id = layer_id;
        if(mTemporalLayerBitrateFramerate[layer_id].bitRate != 0)
             misc_rate_ctrl->bits_per_second = mTemporalLayerBitrateFramerate[layer_id].bitRate;
    }

    misc_rate_ctrl->target_percentage = 100;
    misc_rate_ctrl->window_size = 1000;
    misc_rate_ctrl->initial_qp = mVideoParamsVP8.init_qp;
    misc_rate_ctrl->min_qp = mVideoParamsVP8.min_qp;
    misc_rate_ctrl->basic_unit_size = 0;
    misc_rate_ctrl->max_qp = mVideoParamsVP8.max_qp;

    vaUnmapBuffer(mVADisplay, rc_param_buf);

    vaStatus = vaRenderPicture(mVADisplay,mVAContext, &rc_param_buf, 1);
    CHECK_VA_STATUS_RETURN("vaRenderPicture");;
    return 0;
}

Encode_Status VideoEncoderVP8::renderFrameRateParams(uint32_t layer_id, bool total_framerate)
{
    VABufferID framerate_param_buf;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VAEncMiscParameterBuffer *misc_param;
    VAEncMiscParameterFrameRate * misc_framerate;
    uint32_t frameRateNum = mComParams.frameRate.frameRateNum;
    uint32_t frameRateDenom = mComParams.frameRate.frameRateDenom;

    vaStatus = vaCreateBuffer(mVADisplay, mVAContext,
                              VAEncMiscParameterBufferType,
                              sizeof(VAEncMiscParameterBuffer) + sizeof(VAEncMiscParameterFrameRate),
                              1,NULL,&framerate_param_buf);
    CHECK_VA_STATUS_RETURN("vaCreateBuffer");

    vaMapBuffer(mVADisplay, framerate_param_buf,(void **)&misc_param);
    misc_param->type = VAEncMiscParameterTypeFrameRate;
    misc_framerate = (VAEncMiscParameterFrameRate *)misc_param->data;
    memset(misc_framerate, 0, sizeof(*misc_framerate));

    if(total_framerate)
        misc_framerate->framerate = (unsigned int) (frameRateNum + frameRateDenom /2) / frameRateDenom;
    else
    {
        misc_framerate->framerate_flags.bits.temporal_id = layer_id;
        if(mTemporalLayerBitrateFramerate[layer_id].frameRate != 0)
            misc_framerate->framerate = mTemporalLayerBitrateFramerate[layer_id].frameRate;
    }

    vaUnmapBuffer(mVADisplay, framerate_param_buf);

    vaStatus = vaRenderPicture(mVADisplay,mVAContext, &framerate_param_buf, 1);
    CHECK_VA_STATUS_RETURN("vaRenderPicture");;

    return 0;
}

Encode_Status VideoEncoderVP8::renderHRDParams(void)
{
    VABufferID hrd_param_buf;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VAEncMiscParameterBuffer *misc_param;
    VAEncMiscParameterHRD * misc_hrd;
    vaStatus = vaCreateBuffer(mVADisplay, mVAContext,
                              VAEncMiscParameterBufferType,
                              sizeof(VAEncMiscParameterBuffer) + sizeof(VAEncMiscParameterHRD),
                              1,NULL,&hrd_param_buf);
    CHECK_VA_STATUS_RETURN("vaCreateBuffer");

    vaMapBuffer(mVADisplay, hrd_param_buf,(void **)&misc_param);
    misc_param->type = VAEncMiscParameterTypeHRD;
    misc_hrd = (VAEncMiscParameterHRD *)misc_param->data;
    memset(misc_hrd, 0, sizeof(*misc_hrd));
    misc_hrd->buffer_size = 1000;
    misc_hrd->initial_buffer_fullness = 500;
    misc_hrd->optimal_buffer_fullness = 600;
    vaUnmapBuffer(mVADisplay, hrd_param_buf);

    vaStatus = vaRenderPicture(mVADisplay,mVAContext, &hrd_param_buf, 1);
    CHECK_VA_STATUS_RETURN("vaRenderPicture");;

    return 0;
}

Encode_Status VideoEncoderVP8::renderMaxFrameSizeParams(void)
{
    VABufferID max_frame_size_param_buf;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VAEncMiscParameterBuffer *misc_param;
    VAEncMiscParameterBufferMaxFrameSize * misc_maxframesize;
    unsigned int frameRateNum = mComParams.frameRate.frameRateNum;
    unsigned int frameRateDenom = mComParams.frameRate.frameRateDenom;
    unsigned int frameRate = (unsigned int)(frameRateNum + frameRateDenom /2);
    unsigned int bitRate = mComParams.rcParams.bitRate;

    vaStatus = vaCreateBuffer(mVADisplay, mVAContext,
                              VAEncMiscParameterBufferType,
                              sizeof(VAEncMiscParameterBuffer) + sizeof(VAEncMiscParameterHRD),
                              1,NULL,&max_frame_size_param_buf);
    CHECK_VA_STATUS_RETURN("vaCreateBuffer");

    vaMapBuffer(mVADisplay, max_frame_size_param_buf,(void **)&misc_param);
    misc_param->type = VAEncMiscParameterTypeMaxFrameSize;
    misc_maxframesize = (VAEncMiscParameterBufferMaxFrameSize *)misc_param->data;
    memset(misc_maxframesize, 0, sizeof(*misc_maxframesize));
    misc_maxframesize->max_frame_size = (unsigned int)((bitRate/frameRate) * mVideoParamsVP8.max_frame_size_ratio);
    vaUnmapBuffer(mVADisplay, max_frame_size_param_buf);

    vaStatus = vaRenderPicture(mVADisplay,mVAContext, &max_frame_size_param_buf, 1);
    CHECK_VA_STATUS_RETURN("vaRenderPicture");;

    return 0;
}

Encode_Status VideoEncoderVP8::renderLayerStructureParam(void)
{
    VABufferID layer_struc_buf;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VAEncMiscParameterBuffer *misc_param;
    VAEncMiscParameterTemporalLayerStructure *misc_layer_struc;
    uint32_t i;

    vaStatus = vaCreateBuffer(mVADisplay, mVAContext,
		               VAEncMiscParameterBufferType,
			       sizeof(VAEncMiscParameterBuffer) + sizeof(VAEncMiscParameterTemporalLayerStructure),
			       1, NULL, &layer_struc_buf);

    CHECK_VA_STATUS_RETURN("vaCreateBuffer");
    vaMapBuffer(mVADisplay, layer_struc_buf, (void **)&misc_param);
    misc_param->type = VAEncMiscParameterTypeTemporalLayerStructure;
    misc_layer_struc = (VAEncMiscParameterTemporalLayerStructure *)misc_param->data;
    memset(misc_layer_struc, 0, sizeof(*misc_layer_struc));

    misc_layer_struc->number_of_layers = mComParams.numberOfLayer;
    misc_layer_struc->periodicity = mComParams.nPeriodicity;
    LOGE("renderLayerStructureParam misc_layer_struc->number_of_layers is %d",misc_layer_struc->number_of_layers);

    for(i=0;i<mComParams.nPeriodicity;i++)
    {
        misc_layer_struc->layer_id[i] = mComParams.nLayerID[i];
    }

    vaUnmapBuffer(mVADisplay, layer_struc_buf);

    vaStatus = vaRenderPicture(mVADisplay, mVAContext, &layer_struc_buf, 1);
    CHECK_VA_STATUS_RETURN("vaRenderPicture");;

    return 0;
}


Encode_Status VideoEncoderVP8::sendEncodeCommand(EncodeTask *task) {

    Encode_Status ret = ENCODE_SUCCESS;
    uint32_t i;

    if (mFrameNum == 0) {
        ret = renderSequenceParams();
        ret = renderFrameRateParams(0,true);
        ret = renderRCParams(0,true);
        ret = renderHRDParams();
        ret = renderMaxFrameSizeParams();
        if(mRenderMultiTemporal)
        {
            ret = renderLayerStructureParam();
            mRenderMultiTemporal = false;

        }

        if(mComParams.numberOfLayer > 1)
            for(i=0;i<mComParams.numberOfLayer;i++)
            {
                ret = renderFrameRateParams(i, false);
                ret = renderRCParams(i, false);
            }

        CHECK_ENCODE_STATUS_RETURN("renderSequenceParams");
    }

    if (mRenderBitRate){
        ret = renderRCParams(0,true);
        CHECK_ENCODE_STATUS_RETURN("renderRCParams");

        mRenderBitRate = false;
    }

    if (mRenderFrameRate) {
        ret = renderFrameRateParams(0,true);
        CHECK_ENCODE_STATUS_RETURN("renderFrameRateParams");

        mRenderFrameRate = false;
    }

    if (mRenderMaxFrameSize) {
        ret = renderMaxFrameSizeParams();
        CHECK_ENCODE_STATUS_RETURN("renderMaxFrameSizeParams");

        mRenderMaxFrameSize = false;
    }

    ret = renderPictureParams(task);
    CHECK_ENCODE_STATUS_RETURN("renderPictureParams");

    if(mForceKFrame) {
        mVideoConfigVP8.force_kf = 0;//rest it as default value
        mForceKFrame = false;
    }

    LOG_V( "End\n");
    return ret;
}


Encode_Status VideoEncoderVP8::derivedSetParams(VideoParamConfigSet *videoEncParams) {

	CHECK_NULL_RETURN_IFFAIL(videoEncParams);
	VideoParamsVP8 *encParamsVP8 = reinterpret_cast <VideoParamsVP8*> (videoEncParams);

	if (encParamsVP8->size != sizeof(VideoParamsVP8)) {
		return ENCODE_INVALID_PARAMS;
	}

	mVideoParamsVP8 = *encParamsVP8;
	return ENCODE_SUCCESS;
}

Encode_Status VideoEncoderVP8::derivedGetParams(VideoParamConfigSet *videoEncParams) {

	CHECK_NULL_RETURN_IFFAIL(videoEncParams);
	VideoParamsVP8 *encParamsVP8 = reinterpret_cast <VideoParamsVP8*> (videoEncParams);

	if (encParamsVP8->size != sizeof(VideoParamsVP8)) {
	       return ENCODE_INVALID_PARAMS;
        }

        *encParamsVP8 = mVideoParamsVP8;
        return ENCODE_SUCCESS;
}

Encode_Status VideoEncoderVP8::derivedGetConfig(VideoParamConfigSet *videoEncConfig) {

		int layer_id;
        CHECK_NULL_RETURN_IFFAIL(videoEncConfig);

        switch (videoEncConfig->type)
        {
                case VideoConfigTypeVP8:{
                        VideoConfigVP8 *encConfigVP8 =
                                reinterpret_cast<VideoConfigVP8*> (videoEncConfig);

                        if (encConfigVP8->size != sizeof(VideoConfigVP8)) {
                                return ENCODE_INVALID_PARAMS;
                        }

                        *encConfigVP8 = mVideoConfigVP8;
                }
                break;

                case VideoConfigTypeVP8ReferenceFrame:{

                        VideoConfigVP8ReferenceFrame *encConfigVP8ReferenceFrame =
                                reinterpret_cast<VideoConfigVP8ReferenceFrame*> (videoEncConfig);

                        if (encConfigVP8ReferenceFrame->size != sizeof(VideoConfigVP8ReferenceFrame)) {
                                return ENCODE_INVALID_PARAMS;
                        }

                        *encConfigVP8ReferenceFrame = mVideoConfigVP8ReferenceFrame;

                }
                break;

                case VideoConfigTypeVP8MaxFrameSizeRatio :{

                        VideoConfigVP8MaxFrameSizeRatio *encConfigVP8MaxFrameSizeRatio =
                                reinterpret_cast<VideoConfigVP8MaxFrameSizeRatio*> (videoEncConfig);

                        if (encConfigVP8MaxFrameSizeRatio->size != sizeof(VideoConfigVP8MaxFrameSizeRatio)) {
                                return ENCODE_INVALID_PARAMS;
                        }

                        encConfigVP8MaxFrameSizeRatio->max_frame_size_ratio = mVideoParamsVP8.max_frame_size_ratio;
                }
                break;

                default: {
                   LOG_E ("Invalid Config Type");
                   break;
                }
       }

       return ENCODE_SUCCESS;
}

Encode_Status VideoEncoderVP8::derivedSetConfig(VideoParamConfigSet *videoEncConfig) {

        int layer_id;
        CHECK_NULL_RETURN_IFFAIL(videoEncConfig);

        switch (videoEncConfig->type)
        {
                case VideoConfigTypeVP8:{
                        VideoConfigVP8 *encConfigVP8 =
                                reinterpret_cast<VideoConfigVP8*> (videoEncConfig);

                        if (encConfigVP8->size != sizeof(VideoConfigVP8)) {
                                return ENCODE_INVALID_PARAMS;
                        }

                        mVideoConfigVP8 = *encConfigVP8;
                }
                break;

                case VideoConfigTypeVP8ReferenceFrame:{
                        VideoConfigVP8ReferenceFrame *encConfigVP8ReferenceFrame =
                                reinterpret_cast<VideoConfigVP8ReferenceFrame*> (videoEncConfig);

                        if (encConfigVP8ReferenceFrame->size != sizeof(VideoConfigVP8ReferenceFrame)) {
                                return ENCODE_INVALID_PARAMS;
                        }

                        mVideoConfigVP8ReferenceFrame = *encConfigVP8ReferenceFrame;

                }
                break;

                case VideoConfigTypeVP8MaxFrameSizeRatio:{
                        VideoConfigVP8MaxFrameSizeRatio *encConfigVP8MaxFrameSizeRatio =
                                reinterpret_cast<VideoConfigVP8MaxFrameSizeRatio*> (videoEncConfig);

                        if (encConfigVP8MaxFrameSizeRatio->size != sizeof(VideoConfigVP8MaxFrameSizeRatio)) {
                                return ENCODE_INVALID_PARAMS;
                        }

                        mVideoParamsVP8.max_frame_size_ratio = encConfigVP8MaxFrameSizeRatio->max_frame_size_ratio;
                        mRenderMaxFrameSize = true;
		}
                break;

                case VideoConfigTypeIDRRequest:{
                        VideoParamConfigSet *encConfigVP8KFrameRequest =
                                reinterpret_cast<VideoParamConfigSet*> (videoEncConfig);

                        mVideoConfigVP8.force_kf = 1;
                        mForceKFrame = true;
                 }
                 break;

                default: {
            LOG_E ("Invalid Config Type");
            break;
                }
        }
        return ENCODE_SUCCESS;
}
