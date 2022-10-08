/*
 * Copyright (c) 2011 Intel Corporation. All Rights Reserved.
 * Copyright (c) Imagination Technologies Limited, UK
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Elaine Wang <elaine.wang@intel.com>
 *    Zeng Li <zeng.li@intel.com>
 *    Edward Lin <edward.lin@intel.com>
 *
 */
#ifndef _TNG_HOSTCODE_H_
#define _TNG_HOSTCODE_H_

#include "img_types.h"
#include "hwdefs/coreflags.h"
#include "psb_drv_video.h"
#include "psb_surface.h"
#include "tng_cmdbuf.h"
#include "tng_hostdefs.h"
#include "tng_hostheader.h"
#include "tng_jpegES.h"
#include "tng_slotorder.h"

#define tng__max(a, b) ((a)> (b)) ? (a) : (b)
#define tng__min(a, b) ((a) < (b)) ? (a) : (b)

#define F_MASK(basename)  (MASK_##basename)
#define F_SHIFT(basename) (SHIFT_##basename)
#define F_ENCODE(val,basename)  (((val)<<(F_SHIFT(basename)))&(F_MASK(basename)))
#define F_EXTRACT(val,basename) (((val)&(F_MASK(basename)))>>(F_SHIFT(basename)))
#define F_INSERT(word,val,basename) (((word)&~(F_MASK(basename))) | (F_ENCODE((val),basename)))

#define COMM_CMD_DATA_BUF_NUM   (20)
#define TOPAZHP_MAX_HIGHCMD_DATASIZE    256
#define COMM_CMD_DATA_BUF_SIZE  (TOPAZHP_MAX_HIGHCMD_DATASIZE)
#define COMM_WB_DATA_BUF_SIZE          (64)

#define COMM_CMD_CODED_BUF_NUM     (4)
#define COMM_CMD_FRAME_BUF_NUM     (16)
#define COMM_CMD_PICMGMT_BUF_NUM (4)

/**************** command buffer count ****************/    
typedef struct context_ENC_cmdbuf_s {
    unsigned int ui32LowCmdCount;           //!< count of low-priority commands sent to TOPAZ
    unsigned int ui32HighCmdCount;          //!< count of high-priority commands sent to TOPAZ
    unsigned int ui32HighWBReceived;        //!< count of high-priority commands received from TOPAZ
    unsigned int ui32LastSync;                    //!< Last sync value sent
} context_ENC_cmdbuf;

typedef struct context_ENC_mem_s {
    struct psb_buffer_s bufs_mtx_context;
    struct psb_buffer_s bufs_seq_header; //pSeqHeaderMem //!< Partially coded Sequence header
    struct psb_buffer_s bufs_sub_seq_header; //pSubSetSeqHeaderMem //!< Partially coded Subset sequence header for H264 mvc
    struct psb_buffer_s bufs_sei_header;  //pAUDHeaderMem + pSEIBufferingPeriodHeaderMem + pSEIPictureTimingHeaderMem
    struct psb_buffer_s bufs_pic_template;  //+ apPicHdrTemplateMem[4]
    struct psb_buffer_s bufs_slice_template; //apSliceParamsTemplateMem[NUM_SLICE_TYPES];
    struct psb_buffer_s bufs_lowpower_params;

    struct psb_buffer_s bufs_above_params;  //psAboveParams[TOPAZHP_NUM_PIPES] //!< Picture level parameters (supplied by driver)
    struct psb_buffer_s bufs_recon_pictures;  //apsReconPictures[MAX_PIC_NODES_ARRAYSIZE];// three reference pictures (2 input and 1 output)
    struct psb_buffer_s bufs_colocated;  //apsColocated[MAX_PIC_NODES_ARRAYSIZE];// three colocated vector stores (2 input and 1 output)
    struct psb_buffer_s bufs_mv;  //apsMV[MAX_MV_ARRAYSIZE];          // two colocated vector stores
    struct psb_buffer_s bufs_interview_mv;  //apsInterViewMV[2];      

    struct psb_buffer_s bufs_src_phy_addr;   //apSrcPhysAddr;

    // WEIGHTED PREDICTION
    struct psb_buffer_s bufs_weighted_prediction;  //apsWeightedPredictionMem[MAX_SOURCE_SLOTS_SL];
    struct psb_buffer_s bufs_flat_gop;  //pFlatGopStruct;        //!< Flat MiniGop structure
    struct psb_buffer_s bufs_hierar_gop;  //pHierarGopStruct; //!< Hierarchical MiniGop structure
    
#ifdef LTREFHEADER
    struct psb_buffer_s bufs_lt_ref_header;  //pLtRefHeader[MAX_SOURCE_SLOTS_SL];
#endif

    struct psb_buffer_s bufs_custom_quant;   
    struct psb_buffer_s bufs_slice_map;  //IMG_BUFFER* apsSliceMap[MAX_SOURCE_SLOTS_SL];  //!< Slice map of the source picture
    
    /*  | MVSetingsB0 | MVSetingsB1 | ... | MVSetings Bn |  */
    struct psb_buffer_s bufs_mv_setting_btable;  //pMVSettingsBTable;// three colocated vector stores (2 input and 1 output)
    struct psb_buffer_s bufs_mv_setting_hierar;  //pMVSettingsHierarchical;
    struct psb_buffer_s bufs_recon_buffer;  //psReconBuffer;
    struct psb_buffer_s bufs_patch_recon_buffer;  //psPatchedReconBuffer;
    
    struct psb_buffer_s bufs_first_pass_out_params; //sFirstPassOutParamBuf[MAX_SOURCE_SLOTS_SL]; //!< Output Parameters of the First Pass
#ifndef EXCLUDE_BEST_MP_DECISION_DATA
    struct psb_buffer_s bufs_first_pass_out_best_multipass_param; //sFirstPassOutBestMultipassParamBuf[MAX_SOURCE_SLOTS_SL]; //!< Output Selectable Best MV Parameters of the First Pass
#endif
    struct psb_buffer_s bufs_mb_ctrl_in_params;      //sMBCtrlInParamsBuf[MAX_SOURCE_SLOTS_SL]; //!< Input Parameters to the second pass

    struct psb_buffer_s bufs_ref_frames;

    //defined for dual-stream
    struct psb_buffer_s bufs_lowpower_data;
    struct psb_buffer_s bufs_lowpower_reg;
} context_ENC_mem;

typedef struct context_ENC_mem_size_s {
    IMG_UINT32 mtx_context;
    IMG_UINT32 seq_header;       //pSeqHeaderMem //!< Partially coded Sequence header
                                                 //+ pSubSetSeqHeaderMem //!< Partially coded Subset sequence header for H264 mvc
    IMG_UINT32 sei_header;        //pAUDHeaderMem + pSEIBufferingPeriodHeaderMem + pSEIPictureTimingHeaderMem
    IMG_UINT32 pic_template;      //+ apPicHdrTemplateMem[4]
    IMG_UINT32 slice_template;    //apSliceParamsTemplateMem[NUM_SLICE_TYPES];
    IMG_UINT32 writeback;

    IMG_UINT32 above_params;  //psAboveParams[TOPAZHP_NUM_PIPES] //!< Picture level parameters (supplied by driver)
    IMG_UINT32 recon_pictures;  //apsReconPictures[MAX_PIC_NODES_ARRAYSIZE];// three reference pictures (2 input and 1 output)
    IMG_UINT32 colocated;          //apsColocated[MAX_PIC_NODES_ARRAYSIZE];// three colocated vector stores (2 input and 1 output)
    IMG_UINT32 mv;                   //apsMV[MAX_MV_ARRAYSIZE];          // two colocated vector stores
    IMG_UINT32 interview_mv;    //apsInterViewMV[2];      

    IMG_UINT32 src_phy_addr;   //apSrcPhysAddr;

    // WEIGHTED PREDICTION
    IMG_UINT32 weighted_prediction;  //apsWeightedPredictionMem[MAX_SOURCE_SLOTS_SL];
    IMG_UINT32 flat_gop;  //pFlatGopStruct;        //!< Flat MiniGop structure
    IMG_UINT32 hierar_gop;  //pHierarGopStruct; //!< Hierarchical MiniGop structure
    
#ifdef LTREFHEADER
    IMG_UINT32 lt_ref_header;  //pLtRefHeader[MAX_SOURCE_SLOTS_SL];
#endif

    IMG_UINT32 custom_quant;   
    IMG_UINT32 slice_map;  //IMG_BUFFER* apsSliceMap[MAX_SOURCE_SLOTS_SL];  //!< Slice map of the source picture
    
    /*  | MVSetingsB0 | MVSetingsB1 | ... | MVSetings Bn |  */
    IMG_UINT32 mv_setting_btable;  //pMVSettingsBTable;// three colocated vector stores (2 input and 1 output)
    IMG_UINT32 mv_setting_hierar;  //pMVSettingsHierarchical;
    IMG_UINT32 recon_buffer;  //psReconBuffer;
    IMG_UINT32 patch_recon_buffer;  //psPatchedReconBuffer;
    IMG_UINT32 first_pass_out_params; //!< Output Parameters of the First Pass
#ifndef EXCLUDE_BEST_MP_DECISION_DATA
    IMG_UINT32 first_pass_out_best_multipass_param;//!< Output Selectable Best MV Parameters of the First Pass
#endif
    IMG_UINT32 mb_ctrl_in_params;

    //defined for dual-stream
    IMG_UINT32 lowpower_params;
    IMG_UINT32 lowpower_data;
/****************************************/
    IMG_UINT32 coded_buf;

} context_ENC_mem_size;

/**************** surface buffer infor ****************/
typedef struct context_ENC_frame_buf_s {
    object_surface_p  src_surface;
    object_surface_p  rec_surface;
#ifdef _TNG_FRAMES_
    object_surface_p  ref_surface;
    object_surface_p  ref_surface1;
#else
    object_surface_p  ref_surface[16];
#endif
    object_buffer_p   coded_buf;

    IMG_UINT8         ui8SourceSlotReserved;
    IMG_UINT8         ui8CodedSlotReserved;

    // save previous settings
    object_surface_p    previous_src_surface;
    object_surface_p    previous_ref_surface;
    object_surface_p    previous_dest_surface; /* reconstructed surface */
    object_buffer_p     previous_coded_buf;
    object_buffer_p     pprevious_coded_buf;

    IMG_UINT16  ui16BufferStride;  //!< input buffer stride
    IMG_UINT16  ui16BufferHeight;  //!< input buffer width
} context_ENC_frame_buf;

typedef struct CARC_PARAMS_s
{
    IMG_BOOL    bCARC;
    IMG_INT32   i32CARCBaseline;
    IMG_UINT32  ui32CARCThreshold;
    IMG_UINT32  ui32CARCCutoff;
    IMG_UINT32  ui32CARCNegRange;
    IMG_UINT32  ui32CARCNegScale;
    IMG_UINT32  ui32CARCPosRange;
    IMG_UINT32  ui32CARCPosScale;
    IMG_UINT32  ui32CARCShift;
} CARC_PARAMS;

typedef  struct _LOWPOWER_PARAMS {
    IMG_UINT32 host_context;
    IMG_UINT32 codec;
    IMG_UINT32 data_saving_buf_handle;    /*TTM buffer handle*/
    IMG_UINT32 data_saving_buf_size;        /*MTX RAM size - Firmware Size. Maximum value is 64K*/
    IMG_UINT32 reg_saving_buf_handle;      /*TTM buffer handle. One page is enough.*/
    IMG_UINT32 reg_saving_buf_size;
} LOWPOWER_PARAMS;

typedef struct _H264_PICMGMT_UP_PARAMS {
    IMG_INT8 updated;
    IMG_INT8 ref_type;
    IMG_INT8 gop_struct;
    IMG_INT8 skip_frame;

    IMG_INT8 eos;
    IMG_INT8 rc_updated;
    IMG_INT8 flush;
    IMG_INT8 quant;
} H264_PICMGMT_UP_PARAMS;

/*! 
 *    \ADAPTIVE_INTRA_REFRESH_INFO_TYPE
 *    \brief Structure for parameters requierd for Adaptive intra refresh.
 */
typedef struct
{
    IMG_INT8   *pi8AIR_Table;
    IMG_INT32   i32NumAIRSPerFrame;
    IMG_INT16   i16AIRSkipCnt;
    IMG_UINT16  ui16AIRScanPos;
    IMG_INT32   i32SAD_Threshold;
} ADAPTIVE_INTRA_REFRESH_INFO_TYPE;


struct context_ENC_s {
    object_context_p obj_context; /* back reference */
    context_ENC_mem_size ctx_mem_size;
    context_ENC_frame_buf ctx_frame_buf;
    context_ENC_mem ctx_mem[2];
    context_ENC_cmdbuf ctx_cmdbuf[2];
    IMG_UINT32  ui32FrameCount[2];

    struct psb_buffer_s bufs_writeback;

    IMG_FRAME_TYPE eFrameType;
    IMG_FRAME_TYPE ePreFrameType;
    
    IMG_CODEC eCodec;
    CARC_PARAMS  sCARCParams;
    IMG_ENC_CAPS sCapsParams;
    IMG_ENCODE_FEATURES sEncFeatures;
    H264_CROP_PARAMS sCropParams;
    H264_VUI_PARAMS sVuiParams;
    FRAME_ORDER_INFO sFrameOrderInfo;
    // Adaptive Intra Refresh Control structure
    ADAPTIVE_INTRA_REFRESH_INFO_TYPE sAirInfo;

    IMG_UINT32  ui32RawFrameCount;
    IMG_UINT32  ui32HalfWayBU[NUM_SLICE_TYPES];
    IMG_UINT32  ui32LastPicture;

    IMG_UINT32  ui32CoreRev;
    IMG_UINT32  ui32StreamID;
    IMG_UINT32  ui32FCode;
    IMG_UINT32  ui32BasicUnit;
    IMG_UINT8   ui8ProfileIdc;
    IMG_UINT8   ui8LevelIdc;
    IMG_UINT8   ui8FieldCount;
    IMG_UINT8   ui8VPWeightedImplicitBiPred;
    IMG_UINT8   ui8MaxNumRefFrames;
    IMG_UINT8   i8CQPOffset;
    IMG_INT     iFineYSearchSize;
 
    IMG_UINT8   aui8CustomQuantParams4x4[6][16];
    IMG_UINT8   aui8CustomQuantParams8x8[2][64];
    IMG_UINT32  ui32CustomQuantMask;

    IMG_BOOL    bInsertPicHeader;
    IMG_UINT32  ui32PpsScalingCnt;

    /**************** FIXME: unknown ****************/
    IMG_UINT    uiCbrBufferTenths;           //TOPAZHP_DEFAULT_uiCbrBufferTenths

    /**************** IMG_VIDEO_PARAMS ****************/
    IMG_BOOL16 bVPAdaptiveRoundingDisable;
    IMG_INT16  ui16UseCustomScalingLists;
    IMG_UINT8  ui8RefSpacing;
    IMG_BOOL   bUseDefaultScalingList;
    IMG_BOOL   bEnableLossless;
    IMG_BOOL   bLossless8x8Prefilter;
    IMG_BOOL   bEnableCumulativeBiases;

    /*!
     ***********************************************************************************
     * Description        : Video encode context
     ************************************************************************************/
    /* stream level params */
    IMG_STANDARD  eStandard;                //!< Video standard
    IMG_UINT16  ui16SourceWidth;             //!< source frame width
    IMG_UINT16  ui16SourceHeight;            //!< source frame height
    IMG_UINT16  ui16Width;             //!< target output width
    IMG_UINT16  ui16FrameHeight;  //!< target output height
    IMG_UINT16  ui16PictureHeight;     //!< target output height
    IMG_UINT16  ui16BufferStride;              //!< input buffer stride
    IMG_UINT16  ui16BufferHeight;             //!< input buffer width
    IMG_UINT8   ui8FrameRate;

    IMG_UINT32  ui32DebugCRCs;
    IMG_FORMAT  eFormat;            //!< Pixel format of the source surface

    /* Numbers of array elements that will be allocated */
    IMG_INT32   i32PicNodes;
    IMG_INT32   i32MVStores;
    IMG_INT32   i32CodedBuffers;

    /* slice control parameters */

    /* per core params */
    PIC_PARAMS  sPicParams;  //!< Picture level parameters (supplied by driver)
    IMG_BOOL    bWeightedPrediction;
    IMG_UINT8   ui8WeightedBiPred;
    IMG_UINT8   ui8CustomQuantSlot;

    /* these values set at picture level & written in at slice */
    IMG_UINT32  ui32IPEControl;         //!< common bits IPE control register for entire picture 
    IMG_UINT32  ui32PredCombControl;    //!< common bits of Predictor-combiner control register for entire picture
    IMG_BOOL    bCabacEnabled;          //!< FLAG to enable Cabac mode
    IMG_UINT32  ui32CabacBinLimit;      //!< Min Bin Limit after which the Topaz hardware would encode MB as IPCM
    IMG_UINT32  ui32CabacBinFlex;       //!< Max Flex-Limit, the Topaz-HW will encode MB as IPCM after (BinLimit+BinFlex)

    IMG_UINT32  ui32FirstPicFlags;
    IMG_UINT32  ui32NonFirstPicFlags;

    IMG_BOOL    bIsInterlaced;
    IMG_BOOL    bIsInterleaved;
    IMG_BOOL    bTopFieldFirst;
    IMG_BOOL    bArbitrarySO;
    IMG_UINT32  ui32NextSlice;
    IMG_UINT8   ui8SlicesPerPicture;
    IMG_UINT8   ui8DeblockIDC;
    //  We want to keep track of the basic unit size, as it is needed in deciding the number of macroblocks in a kick
    IMG_UINT32  ui32KickSize;
    IMG_UINT32  ui32VopTimeResolution;
    IMG_UINT32  ui32IdrPeriod;
    IMG_UINT32  ui32IntraCnt;
    IMG_UINT32  ui32IntraCntSave;
    IMG_BOOL    bMultiReferenceP;
    IMG_BOOL    bSpatialDirect;
    IMG_UINT8   ui8MPEG2IntraDCPrecision; // Only used in MPEG2, 2 bit field (0 = 8 bit, 1 = 9 bit, 2 = 10 bit and 3=11 bit precision). Set to zero for other encode standards.

    IMG_MV_SETTINGS sMVSettingsIdr;
    IMG_MV_SETTINGS sMVSettingsNonB[MAX_BFRAMES + 1];

    /*  | MVSetingsB0 | MVSetingsB1 | ... | MVSetings Bn |  */
    IMG_BOOL    b_is_mv_setting_hierar;

    // Source slots
    IMG_FRAME  *apsSourceSlotBuff[MAX_SOURCE_SLOTS_SL]; // Source slots
    IMG_UINT32 aui32SourceSlotPOC[MAX_SOURCE_SLOTS_SL]; // POCs of frames in slots
    IMG_UINT32 ui32pseudo_rand_seed;
    IMG_UINT8  ui8SlotsInUse;                           // Number of source slots used
    IMG_UINT8  ui8SlotsCoded;                          // Number of coded slots used

    IMG_BOOL   bSrcAllocInternally;                        // True for internal source frame allocation

    // Coded slots
    //IMG_CODED_BUFFER *  apsCodedSlotBuff[MAX_CODED_BUFFERS];        // Coded slots
    IMG_BOOL  bCodedAllocInternally;                      // True for internal coded frame allocation
    //IMG_CODED_BUFFER *  apsInternalCoded[MAX_CODED_BUFFERS];        // Frames placed in slots when using internal coded frame allocation


    IMG_UINT32  ui32FlushAtFrame;
    IMG_UINT32  ui32FlushedAtFrame;
    IMG_UINT32  ui32EncodeSent;
    IMG_UINT32  ui32EncodeRequested;
    IMG_UINT32  ui32FramesEncoded;
    IMG_BOOL    bEncoderIdle;       // Indicates that the encoder is waiting for data, Set to true at start of encode
    IMG_BOOL    bAutoEncode;
    IMG_BOOL    bSliceLevel;
    IMG_BOOL    bAborted;

    IMG_UINT32  ui32ReconPOC;
    IMG_UINT32  ui32NextRecon;
    IMG_UINT32  ui32BuffersStatusReg;

    IMG_RC_PARAMS   sRCParams;
    IMG_BIAS_TABLES sBiasTables;
    IMG_BIAS_PARAMS sBiasParams;

    IMG_UINT8  ui8H263SourceFormat;

    IMG_BOOL   bOverlapping;
    IMG_BOOL   bInsertSeqHeader;

    IMG_UINT32 ui32EncodePicProcessing;
    IMG_UINT8  ui8ExtraWBRetrieved;

    IMG_UINT8   ui8EnableSelStatsFlags;   //!< Flags to enable selective first-pass statistics gathering by the hardware. Bit 1 - First Stage Motion Search Data, Bit 2 - Best Multipass MB Decision Data, Bit 3 - Best Multipass Motion Vectors. (First stage Table 2 motion vectors are always switched on)

    IMG_BOOL   bEnableInpCtrl;  //!< Enable Macro-block input control
    IMG_BOOL   bEnableAIR;      //!< Enable Adaptive Intra Refresh
    IMG_BOOL   bEnableCIR;	//!< Enable Cyclic Intra Refresh
    IMG_INT32  i32NumAIRMBs;    //!< n = Max number of AIR MBs per frame, 0 = _ALL_ MBs over threshold will be marked as AIR Intras, -1 = Auto 10%
    IMG_INT32  i32AIRThreshold; //!< n = SAD Threshold above which a MB is a AIR MB candidate,  -1 = Auto adjusting threshold
    IMG_INT16  i16AIRSkipCnt;   //?!< n = Number of MBs to skip in AIR Table between frames, -1 = Random (0 - NumAIRMbs) skip between frames in AIR table
    // INPUT CONTROL
    IMG_UINT16 ui16IntraRefresh;
    IMG_INT32  i32LastCIRIndex;

    IMG_BOOL   bEnableHostBias; 
    IMG_BOOL   bEnableHostQP;

    IMG_BOOL   bCustomScaling; 
    IMG_BOOL   bPpsScaling; 
    IMG_BOOL   bH2648x8Transform;
    IMG_BOOL   bH264IntraConstrained;
    IMG_UINT32 ui32VertMVLimit;
    IMG_BOOL16 bLimitNumVectors;
//    IMG_BOOL16 bDisableBitStuffing;
    IMG_UINT8  ui8CodedSkippedIndex;
    IMG_UINT8  ui8InterIntraIndex;
    // SEI_INSERTION
    IMG_BOOL   bInsertHRDParams;
    //
    IMG_UINT32 uChunksPerMb;
    IMG_UINT32 uMaxChunks;
    IMG_UINT32 uPriorityChunks;

    IMG_UINT8  ui8SourceSlotReserved;
    IMG_UINT8  ui8CodedSlotReserved;
    IMG_UINT8  ui8SliceReceivedInFrame;

    /* Low latency stuff */
    IMG_UINT8  ui8ActiveCodedBuffer;
    IMG_UINT8  ui8BasePipe;
    IMG_UINT8  ui8PipesToUse;
    IMG_UINT32 ui32ActiveBufferBytesCoded;
    IMG_UINT32 ui32AcriveBufferPreviousBytes;

    IMG_UINT8  ui8HighestStorageNumber;

    IMG_BOOL   bEnableMVC;         //!< True if MVC is enabled. False by default
    IMG_UINT16 ui16MVCViewIdx;     //!< View Idx of this MVC view
    IMG_BOOL   bHighLatency;
    IMG_UINT32 uMBspS;
    IMG_BOOL   bSkipDuplicateVectors;
    IMG_BOOL   bNoOffscreenMv;
    IMG_BOOL   idr_force_flag;

    IMG_BOOL   bNoSequenceHeaders;
    IMG_BOOL   bUseFirmwareALLRC; //!< Defines if aLL RC firmware to be loaded


    //JPEG encode buffer sizes
    uint32_t jpeg_pic_params_size;
    uint32_t jpeg_header_mem_size;
    uint32_t jpeg_header_interface_mem_size;

    //JPEG encode context data
    TOPAZHP_JPEG_ENCODER_CONTEXT *jpeg_ctx;

    /* Save actual H263 width/height */
    IMG_UINT16 h263_actual_width;
    IMG_UINT16 h263_actual_height;

    uint32_t buffer_size;
    uint32_t initial_buffer_fullness;

    /* qp/maxqp/minqp/bitrate/intra_period */
    uint32_t rc_update_flag;
    IMG_UINT16 max_qp;
};

typedef struct context_ENC_s *context_ENC_p;

#define SURFACE_INFO_SKIP_FLAG_SETTLED 0X80000000
#define GET_SURFACE_INFO_skipped_flag(psb_surface) ((int) (psb_surface->extra_info[5]))
#define SET_SURFACE_INFO_skipped_flag(psb_surface, value) psb_surface->extra_info[5] = (SURFACE_INFO_SKIP_FLAG_SETTLED | value)
#define CLEAR_SURFACE_INFO_skipped_flag(psb_surface) psb_surface->extra_info[5] = 0

VAStatus tng_CreateContext(object_context_p obj_context,
                           object_config_p obj_config,
                           unsigned char is_JPEG);

//VAStatus tng_InitContext(context_ENC_p ctx);

void tng_DestroyContext(
    object_context_p obj_context,
    unsigned char is_JPEG);

VAStatus tng_BeginPicture(context_ENC_p ctx);
VAStatus tng_EndPicture(context_ENC_p ctx);

void tng_setup_slice_params(
    context_ENC_p  ctx, IMG_UINT16 YSliceStartPos,
    IMG_UINT16 SliceHeight, IMG_BOOL IsIntra,
    IMG_BOOL  VectorsValid, int bySliceQP);

VAStatus tng__send_encode_slice_params(
    context_ENC_p ctx,
    IMG_BOOL IsIntra,
    IMG_UINT16 CurrentRow,
    IMG_UINT8  DeblockIDC,
    IMG_UINT32 FrameNum,
    IMG_UINT16 SliceHeight,
    IMG_UINT16 CurrentSlice);

VAStatus tng_RenderPictureParameter(context_ENC_p ctx);
void tng__setup_enc_profile_features(context_ENC_p ctx, IMG_UINT32 ui32EncProfile);
VAStatus tng__patch_hw_profile(context_ENC_p ctx);
void tng_reset_encoder_params(context_ENC_p ctx);
unsigned int tng__get_ipe_control(IMG_CODEC  eEncodingFormat);
void tng__UpdateRCBitsTransmitted(context_ENC_p ctx);
void tng__trace_in_params(IMG_MTX_VIDEO_CONTEXT* psMtxEncCtx);
void tng__trace_mtx_context(IMG_MTX_VIDEO_CONTEXT* psMtxEncCtx);
VAStatus tng__alloc_init_buffer(
    psb_driver_data_p driver_data,
    unsigned int size,
    psb_buffer_type_t type,
    psb_buffer_p buf);

#endif  //_TNG_HOSTCODE_H_
