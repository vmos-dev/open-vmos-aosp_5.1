/* Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
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

#ifndef __QCAMERA_INTF_H__
#define __QCAMERA_INTF_H__

#include <media/msmb_isp.h>
#include "cam_types.h"

#define CAM_PRIV_IOCTL_BASE (V4L2_CID_PRIVATE_BASE + 14)
typedef enum {
    /* session based parameters */
    CAM_PRIV_PARM = CAM_PRIV_IOCTL_BASE,
    /* session based action: do auto focus. TRIGGER_AF in HAL3 request */
    CAM_PRIV_DO_AUTO_FOCUS,
    /* session based action: cancel auto focus. TRIGGER_AF in HAL3 request. */
    CAM_PRIV_CANCEL_AUTO_FOCUS,
    /* session based action: prepare for snapshot. TRIGGER_AE_PREPARE in HAL3*/
    CAM_PRIV_PREPARE_SNAPSHOT,
    /* sync stream info. Used for configure_streams */
    CAM_PRIV_STREAM_INFO_SYNC,
    /* stream based parameters*/
    CAM_PRIV_STREAM_PARM,
    /* start ZSL snapshot.*/
    CAM_PRIV_START_ZSL_SNAPSHOT,
    /* stop ZSL snapshot.*/
    CAM_PRIV_STOP_ZSL_SNAPSHOT,
} cam_private_ioctl_enum_t;

/* capability struct definition for HAL 1*/
typedef struct{
    cam_hal_version_t version;

    cam_position_t position;                                /* sensor position: front, back */

    uint16_t isWnrSupported;
    /* supported iso modes */
    uint8_t supported_iso_modes_cnt;
    cam_iso_mode_type supported_iso_modes[CAM_ISO_MODE_MAX];

    /* supported flash modes */
    uint8_t supported_flash_modes_cnt;
    cam_flash_mode_t supported_flash_modes[CAM_FLASH_MODE_MAX];

    uint8_t zoom_ratio_tbl_cnt;                             /* table size for zoom ratios */
    int zoom_ratio_tbl[MAX_ZOOMS_CNT];                      /* zoom ratios table */

    int qcom_supported_feature_mask;      /* mask of qcom specific features supported:
                                           * such as CAM_QCOM_FEATURE_SUPPORTED_FACE_DETECTION*/

    /* supported effect modes */
    uint8_t supported_effects_cnt;
    cam_effect_mode_type supported_effects[CAM_EFFECT_MODE_MAX];

    /* supported scene modes */
    uint8_t supported_scene_modes_cnt;
    cam_scene_mode_type supported_scene_modes[CAM_SCENE_MODE_MAX];

    /* supported auto exposure modes */
    uint8_t supported_aec_modes_cnt;
    cam_auto_exposure_mode_type supported_aec_modes[CAM_AEC_MODE_MAX];

    uint8_t fps_ranges_tbl_cnt;                             /* fps ranges table size */
    cam_fps_range_t fps_ranges_tbl[MAX_SIZES_CNT];          /* fps ranges table */

    /* supported antibanding modes */
    uint8_t supported_antibandings_cnt;
    cam_antibanding_mode_type supported_antibandings[CAM_ANTIBANDING_MODE_MAX];

    /* supported white balance modes */
    uint8_t supported_white_balances_cnt;
    cam_wb_mode_type supported_white_balances[CAM_WB_MODE_MAX];

    /* supported focus modes */
    uint8_t supported_focus_modes_cnt;
    cam_focus_mode_type supported_focus_modes[CAM_FOCUS_MODE_MAX];

    int exposure_compensation_min;       /* min value of exposure compensation index */
    int exposure_compensation_max;       /* max value of exposure compensation index */
    int exposure_compensation_default;   /* default value of exposure compensation index */
    float exposure_compensation_step;
    cam_rational_type_t exp_compensation_step;    /* exposure compensation step value */

    uint8_t video_stablization_supported; /* flag id video stablization is supported */

    uint8_t picture_sizes_tbl_cnt;                          /* picture sizes table size */
    cam_dimension_t picture_sizes_tbl[MAX_SIZES_CNT];       /* picture sizes table */
    /* The minimum frame duration that is supported for each
     * resolution in availableProcessedSizes. Should correspond
     * to the frame duration when only that processed stream
     * is active, with all processing set to FAST */
    int64_t picture_min_duration[MAX_SIZES_CNT];

    /* capabilities specific to HAL 1 */

    int modes_supported;                                    /* mask of modes supported: 2D, 3D */
    uint32_t sensor_mount_angle;                            /* sensor mount angle */

    float focal_length;                                     /* focal length */
    float hor_view_angle;                                   /* horizontal view angle */
    float ver_view_angle;                                   /* vertical view angle */

    uint8_t preview_sizes_tbl_cnt;                          /* preview sizes table size */
    cam_dimension_t preview_sizes_tbl[MAX_SIZES_CNT];       /* preiew sizes table */

    uint8_t video_sizes_tbl_cnt;                            /* video sizes table size */
    cam_dimension_t video_sizes_tbl[MAX_SIZES_CNT];         /* video sizes table */


    uint8_t livesnapshot_sizes_tbl_cnt;                     /* livesnapshot sizes table size */
    cam_dimension_t livesnapshot_sizes_tbl[MAX_SIZES_CNT];  /* livesnapshot sizes table */

    uint8_t hfr_tbl_cnt;                                    /* table size for HFR */
    cam_hfr_info_t hfr_tbl[CAM_HFR_MODE_MAX];               /* HFR table */

    /* supported preview formats */
    uint8_t supported_preview_fmt_cnt;
    cam_format_t supported_preview_fmts[CAM_FORMAT_MAX];

    /* supported picture formats */
    uint8_t supported_picture_fmt_cnt;
    cam_format_t supported_picture_fmts[CAM_FORMAT_MAX];

    /* dimension and supported output format of raw dump from camif */
    uint8_t supported_raw_dim_cnt;
    cam_dimension_t raw_dim[MAX_SIZES_CNT];
    uint8_t supported_raw_fmt_cnt;
    cam_format_t supported_raw_fmts[CAM_FORMAT_MAX];
    /* The minimum frame duration that is supported for above
       raw resolution */
    int64_t raw_min_duration[MAX_SIZES_CNT];

    /* supported focus algorithms */
    uint8_t supported_focus_algos_cnt;
    cam_focus_algorithm_type supported_focus_algos[CAM_FOCUS_ALGO_MAX];


    uint8_t auto_wb_lock_supported;       /* flag if auto white balance lock is supported */
    uint8_t zoom_supported;               /* flag if zoom is supported */
    uint8_t smooth_zoom_supported;        /* flag if smooth zoom is supported */
    uint8_t auto_exposure_lock_supported; /* flag if auto exposure lock is supported */
    uint8_t video_snapshot_supported;     /* flag if video snapshot is supported */

    uint8_t max_num_roi;                  /* max number of roi can be detected */
    uint8_t max_num_focus_areas;          /* max num of focus areas */
    uint8_t max_num_metering_areas;       /* max num opf metering areas */
    uint8_t max_zoom_step;                /* max zoom step value */

    /* QCOM specific control */
    cam_control_range_t brightness_ctrl;  /* brightness */
    cam_control_range_t sharpness_ctrl;   /* sharpness */
    cam_control_range_t contrast_ctrl;    /* contrast */
    cam_control_range_t saturation_ctrl;  /* saturation */
    cam_control_range_t sce_ctrl;         /* skintone enhancement factor */

    cam_padding_info_t padding_info;      /* padding information from PP */
    int8_t min_num_hdr_bufs;              /* minimum number of buffers needed for HDR by imaging module */
    int8_t min_num_pp_bufs;               /* minimum number of buffers needed by postproc module */
    uint32_t min_required_pp_mask;        /* min required pp feature masks for ZSL.
                                           * depends on hardware limitation, i.e. for 8974,
                                           * sharpness is required for all ZSL snapshot frames */

    /* capabilities specific to HAL 3 */

    float min_focus_distance;
    float hyper_focal_distance;

    float focal_lengths[CAM_FOCAL_LENGTHS_MAX];
    uint8_t focal_lengths_count;

    /* Needs to be regular f number instead of APEX */
    float apertures[CAM_APERTURES_MAX];
    uint8_t apertures_count;

    float filter_densities[CAM_FILTER_DENSITIES_MAX];
    uint8_t filter_densities_count;

    uint8_t optical_stab_modes[CAM_OPT_STAB_MAX];
    uint8_t optical_stab_modes_count;

    cam_dimension_t lens_shading_map_size;
    float lens_shading_map[3 * CAM_MAX_MAP_WIDTH *
              CAM_MAX_MAP_HEIGHT];

    cam_dimension_t geo_correction_map_size;
    float geo_correction_map[2 * 3 * CAM_MAX_MAP_WIDTH *
              CAM_MAX_MAP_HEIGHT];

    float lens_position[3];

    /* nano seconds */
    int64_t exposure_time_range[2];

    /* nano seconds */
    int64_t max_frame_duration;

    cam_color_filter_arrangement_t color_arrangement;
    uint8_t num_color_channels;

    float sensor_physical_size[2];

    /* Dimensions of full pixel array, possibly including
       black calibration pixels */
    cam_dimension_t pixel_array_size;
    /* Area of raw data which corresponds to only active
       pixels; smaller or equal to pixelArraySize. */
    cam_rect_t active_array_size;

    /* Maximum raw value output by sensor */
    int32_t white_level;

    /* A fixed black level offset for each of the Bayer
       mosaic channels */
    int32_t black_level_pattern[4];

    /* Time taken before flash can fire again in nano secs */
    int64_t flash_charge_duration;

    /* flash firing power */
    uint8_t supported_flash_firing_level_cnt;
    cam_format_t supported_firing_levels[CAM_FLASH_FIRING_LEVEL_MAX];

    /* Flash Firing Time */
    int64_t flash_firing_time;

    /* Flash Ciolor Temperature */
    uint8_t flash_color_temp;

    /* Flash max Energy */
    uint8_t flash_max_energy;

    /* Maximum number of supported points in the tonemap
       curve */
    int32_t max_tone_map_curve_points;

    /* supported formats */
    uint8_t supported_scalar_format_cnt;
    cam_format_t supported_scalar_fmts[CAM_FORMAT_MAX];

    uint32_t max_face_detection_count;

    /* Number of histogram buckets supported */
    int32_t histogram_size;
    /* Maximum value possible for a histogram bucket */
    int32_t max_histogram_count;

    cam_dimension_t sharpness_map_size;

    /* Maximum value possible for a sharpness map region */
    int32_t max_sharpness_map_value;

    cam_scene_mode_overrides_t scene_mode_overrides[CAM_SCENE_MODE_MAX];

    /*Autoexposure modes for camera 3 api*/
    uint8_t supported_ae_modes_cnt;
    cam_ae_mode_type supported_ae_modes[CAM_AE_MODE_MAX];


    cam_sensitivity_range_t sensitivity_range;
    int32_t max_analog_sensitivity;

    uint8_t flash_available;

    cam_rational_type_t base_gain_factor;    /* sensor base gain factor */

    uint8_t focus_dist_calibrated;

    uint8_t supported_test_pattern_modes_cnt;
    cam_test_pattern_mode_t supported_test_pattern_modes[MAX_TEST_PATTERN_CNT];

    int64_t jpeg_stall_durations[MAX_SIZES_CNT];
    int64_t raw16_stall_durations[MAX_SIZES_CNT];
    cam_illuminant_t reference_illuminant1;
    cam_illuminant_t reference_illuminant2;
    cam_rational_type_t forward_matrix1[3][3];
    cam_rational_type_t forward_matrix2[3][3];
    cam_rational_type_t color_transform1[3][3];
    cam_rational_type_t color_transform2[3][3];
    cam_rational_type_t calibration_transform1[3][3];
    cam_rational_type_t calibration_transform2[3][3];

    cam_opaque_raw_format_t opaque_raw_fmt;
} cam_capability_t;

typedef enum {
    CAM_STREAM_PARAM_TYPE_DO_REPROCESS = CAM_INTF_PARM_DO_REPROCESS,
    CAM_STREAM_PARAM_TYPE_SET_BUNDLE_INFO = CAM_INTF_PARM_SET_BUNDLE,
    CAM_STREAM_PARAM_TYPE_MAX
} cam_stream_param_type_e;

typedef struct {
    uint8_t buf_index;            /* buf index to the source frame buffer that needs reprocess,
                                    (assume buffer is already mapped)*/
    uint32_t frame_idx;           /* frame id of source frame to be reprocessed */
    int32_t ret_val;              /* return value from reprocess. Could have different meanings.
                                     i.e., faceID in the case of face registration. */
    uint8_t meta_present;         /* if there is meta data associated with this reprocess frame */

    uint32_t meta_stream_handle;  /* meta data stream ID. only valid if meta_present != 0 */
    uint8_t meta_buf_index;       /* buf index to meta data buffer. only valid if meta_present != 0 */

    /* opaque metadata required for reprocessing */
    char private_data[MAX_METADATA_PAYLOAD_SIZE];

    cam_rect_t crop_rect;
} cam_reprocess_param;

typedef struct {
    cam_stream_param_type_e type;
    union {
        cam_reprocess_param reprocess;  /* do reprocess */
        cam_bundle_config_t bundleInfo; /* set bundle info*/
    };
} cam_stream_parm_buffer_t;

/* stream info */
typedef struct {
    /* stream ID from server */
    uint32_t stream_svr_id;

    /* stream type */
    cam_stream_type_t stream_type;

    /* image format */
    cam_format_t fmt;

    /* image dimension */
    cam_dimension_t dim;

    /* buffer plane information, will be calc based on stream_type, fmt,
       dim, and padding_info(from stream config). Info including:
       offset_x, offset_y, stride, scanline, plane offset */
    cam_stream_buf_plane_info_t buf_planes;

    /* number of stream bufs will be allocated */
    uint8_t num_bufs;

    /* streaming type */
    cam_streaming_mode_t streaming_mode;
    /* num of frames needs to be generated.
     * only valid when streaming_mode = CAM_STREAMING_MODE_BURST */
    uint8_t num_of_burst;

    /* stream specific pp config */
    cam_pp_feature_config_t pp_config;

    /* this section is valid if offline reprocess type stream */
    cam_stream_reproc_config_t reprocess_config;

    cam_stream_parm_buffer_t parm_buf;    /* stream based parameters */
} cam_stream_info_t;

/*****************************************************************************
 *                 Code for Domain Socket Based Parameters                   *
 ****************************************************************************/

#define POINTER_OF(PARAM_ID,TABLE_PTR)    \
        (&(TABLE_PTR->entry[PARAM_ID].data))

#define SET_PARM_VALID_BIT(PARAM_ID,TABLE_PTR,VALID_BIT) \
        (TABLE_PTR->entry[PARAM_ID].valid=VALID_BIT)

#define IS_PARM_VALID(PARAM_ID,TABLE_PTR) \
        (TABLE_PTR->entry[PARAM_ID].valid)

#define GET_FIRST_PARAM_ID(TABLE_PTR)     \
        (TABLE_PTR->first_flagged_entry)

#define SET_FIRST_PARAM_ID(TABLE_PTR,PARAM_ID)     \
        TABLE_PTR->first_flagged_entry=PARAM_ID

#define GET_NEXT_PARAM_ID(CURRENT_PARAM_ID,TABLE_PTR)    \
        (TABLE_PTR->entry[CURRENT_PARAM_ID].next_flagged_entry)

#define SET_NEXT_PARAM_ID(CURRENT_PARAM_ID,TABLE_PTR,NEXT_PARAM_ID)    \
        TABLE_PTR->entry[CURRENT_PARAM_ID].next_flagged_entry=NEXT_PARAM_ID;

#define INCLUDE(PARAM_ID,DATATYPE,COUNT)  \
        DATATYPE member_variable_##PARAM_ID[ COUNT ]

typedef union {
/**************************************************************************************
 *  ID from (cam_intf_metadata_type_t)                DATATYPE                     COUNT
 **************************************************************************************/
    /* common between HAL1 and HAL3 */
    INCLUDE(CAM_INTF_PARM_HAL_VERSION,              	int32_t,                     1);
    INCLUDE(CAM_INTF_META_STREAM_INFO,              	cam_stream_size_info_t,      1);
    INCLUDE(CAM_INTF_META_STREAM_ID,                	cam_stream_ID_t,             1);
    INCLUDE(CAM_INTF_META_HISTOGRAM,                    cam_hist_stats_t,            1);
    INCLUDE(CAM_INTF_META_FACE_DETECTION,               cam_face_detection_data_t,   1);
    INCLUDE(CAM_INTF_META_AUTOFOCUS_DATA,               cam_auto_focus_data_t,       1);
    INCLUDE(CAM_INTF_META_CROP_DATA,                    cam_crop_data_t,             1);

    /* Specific to HAl1 */
    INCLUDE(CAM_INTF_META_PREP_SNAPSHOT_DONE,           int32_t,                     1);
    INCLUDE(CAM_INTF_META_GOOD_FRAME_IDX_RANGE,         cam_frame_idx_range_t,       1);
    INCLUDE(CAM_INTF_PARM_ANTIBANDING,                  int8_t,                      1);
    /* Specific to HAL3 */
    INCLUDE(CAM_INTF_META_FRAME_NUMBER_VALID,           int32_t,                     1);
    INCLUDE(CAM_INTF_META_URGENT_FRAME_NUMBER_VALID,    int32_t,                     1);
    INCLUDE(CAM_INTF_META_FRAME_DROPPED,                cam_frame_dropped_t,         1);
    INCLUDE(CAM_INTF_META_PENDING_REQUESTS,             uint32_t,                    1);
    INCLUDE(CAM_INTF_META_FRAME_NUMBER,                 uint32_t,                    1);
    INCLUDE(CAM_INTF_META_URGENT_FRAME_NUMBER,          uint32_t,                    1);
    INCLUDE(CAM_INTF_META_COLOR_CORRECT_MODE,           uint8_t,                     1);
    INCLUDE(CAM_INTF_META_AWB_REGIONS,                  cam_area_t,                  5);
    INCLUDE(CAM_INTF_META_FRAMES_STALLED,               uint8_t,                     1);
    /* HAL1 only control */
    INCLUDE(CAM_INTF_PARM_SHARPNESS,                	int32_t,                     1);
    INCLUDE(CAM_INTF_PARM_CONTRAST,                 	int32_t,                     1);
    INCLUDE(CAM_INTF_PARM_SATURATION,              	int32_t,                     1);
    INCLUDE(CAM_INTF_PARM_BRIGHTNESS,               	int32_t,                     1);
    INCLUDE(CAM_INTF_PARM_ISO,                      	int32_t,                     1);
    INCLUDE(CAM_INTF_PARM_ZOOM,                     	int32_t,                     1);
    INCLUDE(CAM_INTF_PARM_ROLLOFF,                  	int32_t,                     1);
    INCLUDE(CAM_INTF_PARM_MODE,                     	int32_t,                     1);
    INCLUDE(CAM_INTF_PARM_AEC_ALGO_TYPE,            	int32_t,                     1);
    INCLUDE(CAM_INTF_PARM_FOCUS_ALGO_TYPE,          	int32_t,                     1);
    INCLUDE(CAM_INTF_PARM_AEC_ROI,                  	cam_set_aec_roi_t,           1);
    INCLUDE(CAM_INTF_PARM_AF_ROI,                   	cam_roi_info_t,              1);
    INCLUDE(CAM_INTF_PARM_SCE_FACTOR,               	int32_t,                     1);
    INCLUDE(CAM_INTF_PARM_FD,                       	cam_fd_set_parm_t,           1);
    INCLUDE(CAM_INTF_PARM_MCE,                      	int32_t,                     1);
    INCLUDE(CAM_INTF_PARM_HFR,                      	int32_t,                     1);
    INCLUDE(CAM_INTF_PARM_WAVELET_DENOISE,          	cam_denoise_param_t,         1);
    INCLUDE(CAM_INTF_PARM_HISTOGRAM,                	int32_t,                     1);
    INCLUDE(CAM_INTF_PARM_ASD_ENABLE,               	int32_t,                     1);
    INCLUDE(CAM_INTF_PARM_RECORDING_HINT,           	int32_t,                     1);
    INCLUDE(CAM_INTF_PARM_HDR,                      	cam_exp_bracketing_t,        1);
    INCLUDE(CAM_INTF_PARM_FRAMESKIP,                	int32_t,                     1);
    INCLUDE(CAM_INTF_PARM_ZSL_MODE,                 	int32_t,                     1);
    INCLUDE(CAM_INTF_PARM_HDR_NEED_1X,              	int32_t,                     1);
    INCLUDE(CAM_INTF_PARM_LOCK_CAF,                 	int32_t,                     1);
    INCLUDE(CAM_INTF_PARM_VIDEO_HDR,                	int32_t,                     1);

    /* HAL3 external control */
    INCLUDE(CAM_INTF_PARM_BESTSHOT_MODE,                uint8_t,                     1);
    INCLUDE(CAM_INTF_META_PRECAPTURE_TRIGGER,           uint8_t,                     1);
    INCLUDE(CAM_INTF_META_AF_TRIGGER_NOTICE,            uint8_t,                     1);
    INCLUDE(CAM_INTF_PARM_REDEYE_REDUCTION,             int32_t,                     1);
    INCLUDE(CAM_INTF_PARM_EV,                       	int32_t,                     1);
    INCLUDE(CAM_INTF_PARM_EV_STEP,                  	cam_rational_type_t,         1);
    INCLUDE(CAM_INTF_PARM_AEC_LOCK,                 	uint8_t,                     1);
    INCLUDE(CAM_INTF_PARM_FPS_RANGE,                	cam_fps_range_t,             1);
    INCLUDE(CAM_INTF_PARM_AWB_LOCK,                 	uint8_t,                     1);
    INCLUDE(CAM_INTF_PARM_EFFECT,                   	int32_t,                     1);
    INCLUDE(CAM_INTF_META_AEC_PRECAPTURE_TRIGGER,   	cam_trigger_t,               1);
    INCLUDE(CAM_INTF_META_AF_TRIGGER,               	cam_trigger_t,               1);
    INCLUDE(CAM_INTF_META_DEMOSAIC,                 	int32_t,                     1);
    INCLUDE(CAM_INTF_PARM_LED_MODE,                 	int32_t,                     1);
    INCLUDE(CAM_INTF_META_NOISE_REDUCTION_STRENGTH, 	int32_t,                     1);
    INCLUDE(CAM_INTF_META_SHADING_STRENGTH,         	uint8_t,                     1);
    INCLUDE(CAM_INTF_META_TONEMAP_MODE,             	uint8_t,                     1);
    INCLUDE(CAM_INTF_META_TONEMAP_CURVES,           	cam_rgb_tonemap_curves,      1);
    INCLUDE(CAM_INTF_META_CAPTURE_INTENT,           	uint8_t,                     1);
    INCLUDE(CAM_INTF_META_LENS_SHADING_MAP_MODE,    	uint8_t,                     1);
    INCLUDE(CAM_INTF_PARM_DIS_ENABLE,               	int32_t,                     1);
    /* HAL3 external metadata */
    INCLUDE(CAM_INTF_META_BLACK_LEVEL_LOCK,             uint8_t,                     1);
    INCLUDE(CAM_INTF_META_COLOR_CORRECT_TRANSFORM,      cam_color_correct_matrix_t,  1);
    INCLUDE(CAM_INTF_META_COLOR_CORRECT_GAINS,          cam_color_correct_gains_t,   1);
    INCLUDE(CAM_INTF_META_PRED_COLOR_CORRECT_TRANSFORM, cam_color_correct_matrix_t,  1);
    INCLUDE(CAM_INTF_META_PRED_COLOR_CORRECT_GAINS,     cam_color_correct_gains_t,   1);
    INCLUDE(CAM_INTF_META_AEC_MODE,                     uint8_t,                     1);
    INCLUDE(CAM_INTF_META_AEC_ROI,                      cam_area_t,                  5);
    INCLUDE(CAM_INTF_META_AEC_STATE,                    uint8_t,                     1);
    INCLUDE(CAM_INTF_PARM_FOCUS_MODE,                   uint8_t,                     1);
    INCLUDE(CAM_INTF_META_AF_ROI,                       cam_area_t,                  5);
    INCLUDE(CAM_INTF_META_AF_STATE,                     uint8_t,                     1);
    INCLUDE(CAM_INTF_PARM_WHITE_BALANCE,                int32_t,                     1);
    INCLUDE(CAM_INTF_META_AWB_STATE,                    uint8_t,                     1);
    INCLUDE(CAM_INTF_META_MODE,                         uint8_t,                     1);
    INCLUDE(CAM_INTF_META_EDGE_MODE,                    cam_edge_application_t,      1);
    INCLUDE(CAM_INTF_META_FLASH_POWER,                  uint8_t,                     1);
    INCLUDE(CAM_INTF_META_FLASH_FIRING_TIME,            int64_t,                     1);
    INCLUDE(CAM_INTF_META_FLASH_MODE,                   uint8_t,                     1);
    INCLUDE(CAM_INTF_META_FLASH_STATE,                  int32_t,                     1);
    INCLUDE(CAM_INTF_META_HOTPIXEL_MODE,                uint8_t,                     1);
    INCLUDE(CAM_INTF_META_JPEG_GPS_COORDINATES,         double,                      3);
    INCLUDE(CAM_INTF_META_JPEG_GPS_PROC_METHODS,        uint8_t,                     GPS_PROCESSING_METHOD_SIZE);
    INCLUDE(CAM_INTF_META_JPEG_GPS_TIMESTAMP,           int64_t,                     1);
    INCLUDE(CAM_INTF_META_JPEG_ORIENTATION,             int32_t,                     1);
    INCLUDE(CAM_INTF_META_JPEG_QUALITY,                 uint8_t,                     1);
    INCLUDE(CAM_INTF_META_JPEG_THUMB_QUALITY,           uint8_t,                     1);
    INCLUDE(CAM_INTF_META_JPEG_THUMB_SIZE,              cam_dimension_t,             1);
    INCLUDE(CAM_INTF_META_LENS_APERTURE,                float,                       1);
    INCLUDE(CAM_INTF_META_LENS_FILTERDENSITY,           float,                       1);
    INCLUDE(CAM_INTF_META_LENS_FOCAL_LENGTH,            float,                       1);
    INCLUDE(CAM_INTF_META_LENS_FOCUS_DISTANCE,          float,                       1);
    INCLUDE(CAM_INTF_META_LENS_FOCUS_RANGE,             float,                       2);
    INCLUDE(CAM_INTF_META_LENS_STATE,                   uint8_t,                     1);
    INCLUDE(CAM_INTF_META_LENS_OPT_STAB_MODE,           uint8_t,                     1);
    INCLUDE(CAM_INTF_META_NOISE_REDUCTION_MODE,         uint8_t,                     1);
    INCLUDE(CAM_INTF_META_SCALER_CROP_REGION,           cam_crop_region_t,           1);
    INCLUDE(CAM_INTF_META_SENSOR_EXPOSURE_TIME,         int64_t,                     1);
    INCLUDE(CAM_INTF_META_SENSOR_FRAME_DURATION,        int64_t,                     1);
    INCLUDE(CAM_INTF_META_SENSOR_SENSITIVITY,           int32_t,                     1);
    INCLUDE(CAM_INTF_META_SENSOR_TIMESTAMP,             struct timeval,              1);
    INCLUDE(CAM_INTF_META_SENSOR_ROLLING_SHUTTER_SKEW,  int64_t,                     1);
    INCLUDE(CAM_INTF_META_SHADING_MODE,                 uint8_t,                     1);
    INCLUDE(CAM_INTF_META_STATS_FACEDETECT_MODE,        uint8_t,                     1);
    INCLUDE(CAM_INTF_META_SCENE_FLICKER,                uint8_t,                     1);
    INCLUDE(CAM_INTF_META_STATS_HISTOGRAM_MODE,         uint8_t,                     1);
    INCLUDE(CAM_INTF_META_STATS_SHARPNESS_MAP_MODE,     uint8_t,                     1);
    INCLUDE(CAM_INTF_META_STATS_SHARPNESS_MAP,          cam_sharpness_map_t,         3);
    INCLUDE(CAM_INTF_META_LENS_SHADING_MAP,             cam_lens_shading_map_t,      1);
    /* HAL internal metadata */
    INCLUDE(CAM_INTF_META_AEC_INFO,                     cam_3a_params_t,             1);
    INCLUDE(CAM_INTF_META_TEST_PATTERN_DATA,            cam_test_pattern_data_t,     1);
    INCLUDE(CAM_INTF_META_OTP_WB_GRGB,                  float,                       1);
    INCLUDE(CAM_INTF_META_PROFILE_TONE_CURVE,           cam_profile_tone_curve,      1);
    INCLUDE(CAM_INTF_META_NEUTRAL_COL_POINT,            cam_neutral_col_point_t,     1);
    INCLUDE(CAM_INTF_META_PRIVATE_DATA,                 char,                        MAX_METADATA_PAYLOAD_SIZE);
} metadata_type_t;

/****************************DO NOT MODIFY BELOW THIS LINE!!!!*********************/

typedef struct {
    metadata_type_t data;
    uint8_t valid;
    uint8_t next_flagged_entry;
} metadata_entry_type_t;

typedef struct {
    uint8_t first_flagged_entry;
    metadata_entry_type_t entry[CAM_INTF_PARM_MAX];
    /*Tuning Data */
    uint8_t is_tuning_params_valid;
    tuning_params_t tuning_params;
} metadata_buffer_t;

typedef metadata_buffer_t parm_buffer_t;
typedef metadata_type_t parm_type_t;

#endif /* __QCAMERA_INTF_H__ */
