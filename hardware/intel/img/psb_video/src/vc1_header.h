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
 *    Waldo Bastian <waldo.bastian@intel.com>
 *
 */

#ifndef _VC1_HEADER_H_
#define _VC1_HEADER_H_

#include "psb_def.h"
#include "tng_vld_dec.h"

#include <img_types.h>


/* Picture header parameters */
typedef struct _vc1PicHeader_ {
    /* TTMBF signals whether transform type coding is enabled at the frame or macroblock level. */
    IMG_UINT8   TTMBF;                                                                  /* PICTURE_LAYER::TTMBF - 1 bit */
    /* TTFRM signals the transform type used to transform the 8x8 pixel error signal in predicted blocks. */
    IMG_UINT8   TTFRM;                                                                  /* PICTURE_LAYER::TTFRM - 2 bits */
    /*
        BFRACTION signals a fraction that may take on a limited set of fractional values between 0 and 1,
        denoting the relative temporal position of the B frame within the interval formed by its anchors.
    */
    IMG_UINT8   BFRACTION;                                                              /* PICTURE_LAYER::BFRACTION - 2 bits */
    /*
        CONDOVER affects overlap smoothing in advanced profile.
    */
    IMG_UINT8   CONDOVER;                                                               /* PICTURE_LAYER::CONDOVER - 2 bits */

    /*
        TRANSACFRM shall provide the index that selects the coding set used to decode the
        Transform AC coefficients for the Cb, Cr blocks.
    */
    IMG_UINT8   TRANSACFRM;                                             /* PICTURE_LAYER::TRANSACFRM - 2 bits */
    /*
        TRANSACFRM2 shall provide the index that selects the coding set used to decode the
        Transform AC coefficients for the Y blocks.
    */
    IMG_UINT8   TRANSACFRM2;                                    /* PICTURE_LAYER::TRANSACFRM2 - 2 bits */
    /*
        MVMODE syntax element shall signal one of four motion vector coding modes,
        or the intensity compensation mode.
    */
    IMG_UINT8   MVMODE;                                                 /* PICTURE_LAYER::MVMODE - 2 bits */
    IMG_UINT8   MVMODE2;                                                /* PICTURE_LAYER::MVMODE2 - 2 bits */

    /* These are needed just for finding out what VLC tables are used in the current picture */
    IMG_UINT8   MV4SWITCH;                                              /* PICTURE_LAYER::MV4SWITCH - 1 bit */
    IMG_UINT8   CBPTAB;                                                 /* PICTURE_LAYER::CBPTAB - 2 bits */
    IMG_UINT8   ICBPTAB;                                                /* PICTURE_LAYER::ICBPTAB - 3 bits */
    IMG_UINT8   MVTAB;                                                  /* PICTURE_LAYER::MVTAB - 2 bits */
    IMG_UINT8   IMVTAB;                                                 /* PICTURE_LAYER::IMVTAB - 3 bits */
    IMG_UINT8   MV4BPTAB;                                               /* PICTURE_LAYER::4MVBPTAB - 2 bits */
    IMG_UINT8   MV2BPTAB;                                               /* PICTURE_LAYER::2MVBPTAB - 2 bits */
    IMG_UINT8   MBMODETAB;                                              /* PICTURE_LAYER::MBMODETAB - 3 bits */
    IMG_UINT8   TRANSDCTAB;                                             /* PICTURE_LAYER::TRANSDCTAB - 1 bits */

    /* PQINDEX is used in VLD and the hardware actually only needs to know if it is greater than 8 or not. */
    IMG_UINT8   PQINDEX;                                                /* PICTURE_LAYER::PQINDEX - 5 bits */
    /*
        HALFQP syntax element allows the picture quantizer to be expressed in half step increments.
        If HALFQP == 1, then the picture quantizer step size shall be equal to PQUANT + 1/2. If
        HALFQP == 0, then the picture quantizer step size shall be equal to PQUANT.
    */
    IMG_UINT8   HALFQP;                                                 /* PICTURE_LAYER::HALFQP - 1 bit */

    IMG_UINT8   bNonUniformQuantizer;
    IMG_UINT8   VOPDQUANT_Present;
    IMG_UINT8   bDQUANT_InFrame;        // Indicates whether quantisation can be specified at a MB level
    /* If DQUANTFRM == 0, then the current picture shall only be quantized with PQUANT. */
    IMG_UINT8   DQUANTFRM;                                              /* VOPDQUANT::DQUANTFRM - 1 bit */
    /* DQPROFILE specifies where it is allowable to change quantization step sizes within the current picture. */
    IMG_UINT8   DQPROFILE;                                              /* VOPDQUANT::DQPROFILE - 2 bits */
    /*
        DQBILEVEL determines the number of possible quantization step sizes which can be
        used by each macroblock in the frame.
    */
    IMG_UINT8   DQBILEVEL;                                              /* VOPDQUANT::DQBILEVEL - 1 bit */
    /* DQDBEDGE specifies which two edges will be quantized with ALTPQUANT when DQPROFILE == 'Double Edge'. */
    IMG_UINT8   DQDBEDGE;                                               /* VOPDQUANT::DQDBEDGE - 2 bits */
    /* DQSBEDGE specifies which edge will be quantized with ALTPQUANT when DQPROFILE == 'Single Edge'. */
    IMG_UINT8   DQSBEDGE;                                               /* VOPDQUANT::DQSBEDGE - 2 bits */
    IMG_UINT8   ALTPQUANT;                                              /* VOPDQUANT::ALTPQUANT - 5 bits */

    /* REFDIST defines the number of frames between the current frame and the reference frame. */
    IMG_UINT8   REFDIST;                                                /* PICTURE_LAYER::REFDIST - 1 bit */
    /*
        If NUMREF == 0, then the current interlace P field picture shall reference one field. If
        NUMREF == 1, then the current interlace P field picture shall reference the two temporally
        closest (in display order) I or P field pictures.
    */
    IMG_UINT8   NUMREF;                                                 /* PICTURE_LAYER::NUMREF - 1 bit */
    /*
        If REFFIELD == 1, then the second most temporally recent interlace I or P field picture
        shall be used as reference.
    */
    IMG_UINT8   REFFIELD;                                               /* PICTURE_LAYER::REFFIELD - 1 bit */

    /* MVRANGE specifies the motion vector range. */
    IMG_UINT8   MVRANGE;                                                /* PICTURE_LAYER::MVRANGE - 2 bits */
    /*
        DMVRANGE indicates if the extended range for motion vector differential is used in the
        vertical, horizontal, both or none of the components of the motion vector.
    */
    IMG_UINT8   DMVRANGE;                                               /* PICTURE_LAYER::DMVRANGE - 2 bits */

    /* BitplanePresent indicates which bitplanes are present in the picture header */
    IMG_UINT8   BP_PRESENT;
    /* RawCodingFlag signals whether the bitplanes are coded in raw mode (bit set to 1) or not (set to 0) */
    IMG_UINT8   RAWCODINGFLAG;

} vc1PicHeader;

/* Sequence header parameters */
typedef struct _vc1SeqHeader_ {
    IMG_UINT8   POSTPROCFLAG;
    IMG_UINT8   PULLDOWN;
    IMG_UINT8   INTERLACE;
    IMG_UINT8   TFCNTRFLAG;
    IMG_UINT8   FINTERPFLAG;
    IMG_UINT8   PSF;
    IMG_UINT8   EXTENDED_DMV;

    IMG_UINT8   PANSCAN_FLAG;
    IMG_UINT8   REFDIST_FLAG;
    IMG_UINT8   LOOPFILTER;
    IMG_UINT8   FASTUVMC;
    IMG_UINT8   EXTENDED_MV;
    IMG_UINT8   DQUANT;
    IMG_UINT8   VSTRANSFORM;

    IMG_UINT8   QUANTIZER;
    IMG_UINT8   MULTIRES;
    IMG_UINT8   SYNCMARKER;
    IMG_UINT8   RANGERED;
    IMG_UINT8   MAXBFRAMES;

    IMG_UINT8   OVERLAP;

    IMG_UINT8   PROFILE;

} vc1SeqHeader;

//! Maximum number of VLC tables for MB/block layer decode
#define MAX_VLC_TABLES  (12)

typedef struct {        //              Information about VLC tables
    IMG_UINT16  aui16StartLocation;             //!<    Byte offset within the packed VLC tables array
    IMG_UINT16  aui16VLCTableLength;    //!<    Length of the table in number of bytes
    IMG_UINT16  aui16RAMLocation;               //!<    Location of the VLC table in the destination buffer
    IMG_UINT16  aui16InitialWidth;
    IMG_UINT16  aui16InitialOpcode;

} sTableData;

typedef struct {        //              Information about the intensity compensation history of previous pictures
    IMG_UINT8   ui8LumaScale1;
    IMG_UINT8   ui8LumaShift1;

    IMG_UINT8   ui8LumaScale2;
    IMG_UINT8   ui8LumaShift2;

    /* Indication of what fields undergo intensity compensation */
    IMG_UINT8   ui8IC1; // 1 - IC top field, 2 - IC bottom field, 3 - IC both fields
    IMG_UINT8   ui8IC2;

} IC_PARAM;


#define VLC_INDEX_TABLE_SIZE    83

struct context_VC1_s {
    struct context_DEC_s dec_ctx;
    object_context_p obj_context; /* back reference */

    uint32_t profile;

    /* Picture parameters */
    VAPictureParameterBufferVC1 *pic_params;
    object_surface_p forward_ref_surface;
    object_surface_p backward_ref_surface;
    object_surface_p decoded_surface;

    uint32_t display_picture_width;     /* in pixels */
    uint32_t display_picture_height;    /* in pixels */
    uint32_t coded_picture_width;       /* in pixels */
    uint32_t coded_picture_height;      /* in pixels */

    uint32_t picture_width_mb;          /* in macroblocks */
    uint32_t picture_height_mb;         /* in macroblocks */
    uint32_t size_mb;                   /* in macroblocks */

    IMG_BOOL                            is_first_slice;

    uint32_t scan_index;
    uint32_t slice_field_type;
    uint8_t                             bitplane_present; /* Required bitplanes */
    IMG_BOOL                            has_bitplane; /* Whether a bitplane was passed to the driver */
    IMG_BOOL                            bottom_field;
    IMG_BOOL                            half_pel;
    IMG_BOOL                            extend_x;
    IMG_BOOL                            extend_y;
    uint8_t                             mode_config;
    uint32_t                            pull_back_x;
    uint32_t                            pull_back_y;
    uint8_t                             condover; /* Adjusted local version of conditional_overlap_flag */
    uint8_t                             mv_mode; /* Adjusted local version of mv_mode / mv_mode2 */
    IMG_BOOL                            pqindex_gt8; /* Flag to indicate PQINDEX > 8 */

    /* TODO:
         LOOPFILTER
    */



    /*
        CurrPic is the picture currently being decoded.
        Ref0Pic is the most recent forward reference picture (only used when decoding interlaced field pictures and
                        the reference picture is in the same frame as the current picture).
        Ref1Pic can either be the most recent forward reference picture (when decoding interlaced frame or
                        progressive pictures) or the 2nd most recent reference picture (when decoding interlaced field pictures).
        Ref2Pic is the most recent backward reference picture (only used when decoding B pictures).
    */
    IMG_UINT8                           ui8FCM_Ref0Pic; /* Used */
    IMG_UINT8                           ui8FCM_Ref1Pic; /* Used */
    IMG_UINT8                           ui8FCM_Ref2Pic; /* Used */

    IMG_BOOL                            bTFF_FwRefFrm;  /* Used */
    IMG_BOOL                            bTFF_BwRefFrm;  /* Used */
    IMG_BOOL                            bRef0RangeRed;                                          /* RangeRed flag                                                                */
    IMG_BOOL                            bRef1RangeRed;

    IMG_UINT8                           ui8IntCompField; /* Used, replace with local var? */
    IMG_UINT8                           ui8MVmode; /* TODO: Fix, differs slightly from pic_params->mv_mode! */
    IMG_INT8                            i8FwrdRefFrmDist; /* Used */
    IMG_INT8                            i8BckwrdRefFrmDist; /* Used */
    IMG_UINT32                          ui32ScaleFactor; /* Used */

    /* IC parameters in current picture */
    IMG_UINT8                           ui8CurrLumaScale1;
    IMG_UINT8                           ui8CurrLumaShift1;
    IMG_UINT8                           ui8CurrLumaScale2;
    IMG_UINT8                           ui8CurrLumaShift2;
    /* Information about the intensity compensation history of previous pictures */
    IC_PARAM                            sICparams[2][2];


    /* VLC table information */
    IMG_UINT32                          ui32NumTables;                  /* VLC table accumulator */
    sTableData                          sTableInfo[MAX_VLC_TABLES];     /* structure of VLC table information */

    /* Split buffers */
    int split_buffer_pending;

    /* List of VASliceParameterBuffers */
    object_buffer_p *slice_param_list;
    int slice_param_list_size;
    int slice_param_list_idx;

    /* VLC packed data */
    struct psb_buffer_s vlc_packed_table;
    uint32_t vlc_packed_index_table[VLC_INDEX_TABLE_SIZE];

    /* Preload buffer */
    struct psb_buffer_s preload_buffer;

    /* CoLocated buffers */
    struct psb_buffer_s *colocated_buffers;
    int colocated_buffers_size;
    int colocated_buffers_idx;

    /* Aux MSB buffer */
    struct psb_buffer_s aux_msb_buffer;
    struct psb_buffer_s aux_line_buffer;
    psb_buffer_p bitplane_buffer;
    struct psb_buffer_s bitplane_hw_buffer; /* For hw parase */

    uint32_t *p_range_mapping_base; /* pointer to ui32RangeMappingBase in CMD_HEADER_VC1 */
    uint32_t *p_range_mapping_base1;
    uint32_t *p_slice_params; /* pointer to ui32SliceParams in CMD_HEADER_VC1 */
    uint32_t *slice_first_pic_last;
    uint32_t *alt_output_flags;

    uint32_t forward_ref_fcm;
    uint32_t backward_ref_fcm;
};

typedef struct context_VC1_s *context_VC1_p;

/* VC1 Sequence Header Data Bit Positions Within 32 bit Command Word */
#define VC1_SEQHDR_EXTENDED_DMV         0
#define VC1_SEQHDR_PSF                          1
#define VC1_SEQHDR_SECONDFIELD          2
#define VC1_SEQHDR_FINTERPFLAG          3
#define VC1_SEQHDR_TFCNTRFLAG           4
#define VC1_SEQHDR_INTERLACE            5
#define VC1_SEQHDR_PULLDOWN                     6
#define VC1_SEQHDR_POSTPROCFLAG         7
#define VC1_SEQHDR_VSTRANSFORM          8
#define VC1_SEQHDR_DQUANT                       9
#define VC1_SEQHDR_EXTENDED_MV          11
#define VC1_SEQHDR_FASTUVMC                     12
#define VC1_SEQHDR_LOOPFILTER           13
#define VC1_SEQHDR_REFDIST_FLAG         14
#define VC1_SEQHDR_PANSCAN_FLAG         15
#define VC1_SEQHDR_MAXBFRAMES           16
#define VC1_SEQHDR_RANGERED                     19
#define VC1_SEQHDR_SYNCMARKER           20
#define VC1_SEQHDR_MULTIRES                     21
#define VC1_SEQHDR_QUANTIZER            22
#define VC1_SEQHDR_OVERLAP                      24
#define VC1_SEQHDR_PROFILE                      25
#define VC1_SEQHDR_PICTYPE                      27
#define VC1_SEQHDR_ICFLAG                       29
#define VC1_SEQHDR_FCM_CURRPIC          30


#if 0
/****************************************************************************************/
/*                                                                      Local Context                                                                           */
/****************************************************************************************/

typedef struct {
    VA_CONTEXT                                  sVAContext;  /* Must be the first Item */

    const char                                          *pszHandleType;

    VA_PictureParameters                        sPicParamVC1;
    vc1PicHeader                                        sPicHdr;
    vc1SeqHeader                                        sSeqHdr;

    VA_SliceInfo                                        *pSliceCtrl;
    PVRSRV_CLIENT_MEM_INFO                      *psAuxMSBBuffer;
    PVRSRV_CLIENT_MEM_INFO                      *hVlcPackedTableData;
    PVRSRV_CLIENT_MEM_INFO                      *psCoLocatedData;
    PVRSRV_CLIENT_MEM_INFO                      *psPreloadBuffer;

#if (!VC1_INTERLEAVED_BITPLANE || VC1_BITPLANE_HARDWARE)
    PVRSRV_CLIENT_MEM_INFO                      *hBitplaneData[3];
    IMG_UINT8                                           *pui8BitplaneData[3];
#else
    PVRSRV_CLIENT_MEM_INFO                      *hBitplaneData;
    IMG_UINT8                                           *pui8BitplaneData;
#endif
#ifdef VC1_LLDMA
    CMD_BUFFER                                          sCmdBuff;
#else
    PVRSRV_CLIENT_MEM_INFO          *psRenderCommandBuffer;
#endif
    PVRSRV_CLIENT_MEM_INFO                      *psStatusBuffer;
    IMG_UINT32                                          *pui32CmdBuffer;
    IMG_UINT32                                          *pui32CmdBufferBase;
    IMG_UINT32                                          ui32CoLocatedSlot;


    /* Status report */
    IMG_BOOL                            bBetweenBeginAndEnd;
    IMG_UINT32                          ui32StatusWof;
    IMG_UINT32                          ui32StatusRof;
    IMG_UINT32                          ui32DevPhysStatusAddr;

    SGX_DDSURFDATA*                     psDecodeSurfData;
    SGX_DDSURFDATA*                     psDeblockSurfData;
    SGX_DDSURFDATA*                     psForwardRefSurfData;
    SGX_DDSURFDATA*                     psBackwardRefSurfData;


} VC1VLDContext;
#endif

/*
******************************************************************************
 Extern global variables
******************************************************************************/

extern vc1PicHeader             *psPicHdr;
extern vc1SeqHeader             *psSeqHdr;

#endif /* _VC1_HEADER_H_ */
