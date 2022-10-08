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
 *    Edward Lin <edward.lin@intel.com>
 *
 */
#ifndef _TNG_PICMGMT_H_
#define _TNG_PICMGMT_H_

#include "img_types.h"
#include "tng_hostcode.h"


#define SHIFT_MTX_MSG_PICMGMT_SUBTYPE                      (0)
#define MASK_MTX_MSG_PICMGMT_SUBTYPE                      (0xff << SHIFT_MTX_MSG_PICMGMT_SUBTYPE)
#define SHIFT_MTX_MSG_PICMGMT_DATA                             (8)
#define MASK_MTX_MSG_PICMGMT_DATA                             (0xffffff << SHIFT_MTX_MSG_PICMGMT_DATA)

#define SHIFT_MTX_MSG_RC_UPDATE_QP                             (0)
#define MASK_MTX_MSG_RC_UPDATE_QP                             (0x3f << SHIFT_MTX_MSG_RC_UPDATE_QP)
#define SHIFT_MTX_MSG_RC_UPDATE_BITRATE                    (6)
#define MASK_MTX_MSG_RC_UPDATE_BITRATE                    (0x03ffffff << SHIFT_MTX_MSG_RC_UPDATE_BITRATE)

#define SHIFT_MTX_MSG_PROVIDE_REF_BUFFER_USE         (0)
#define MASK_MTX_MSG_PROVIDE_REF_BUFFER_USE         (0xff << SHIFT_MTX_MSG_PROVIDE_REF_BUFFER_USE)
#define SHIFT_MTX_MSG_PROVIDE_REF_BUFFER_SLOT       (8)
#define MASK_MTX_MSG_PROVIDE_REF_BUFFER_SLOT       (0xff << SHIFT_MTX_MSG_PROVIDE_REF_BUFFER_SLOT)
#define SHIFT_MTX_MSG_PROVIDE_REF_BUFFER_LT            (16)
#define MASK_MTX_MSG_PROVIDE_REF_BUFFER_LT            (0xff << SHIFT_MTX_MSG_PROVIDE_REF_BUFFER_LT)

#define SHIFT_MTX_MSG_PROVIDE_CODED_BUFFER_SLOT  (0)
#define MASK_MTX_MSG_PROVIDE_CODED_BUFFER_SLOT  (0x0f << SHIFT_MTX_MSG_PROVIDE_CODED_BUFFER_SLOT)
#define SHIFT_MTX_MSG_PROVIDE_CODED_BUFFER_SIZE   (4)
#define MASK_MTX_MSG_PROVIDE_CODED_BUFFER_SIZE   (0x0fffffff << SHIFT_MTX_MSG_PROVIDE_CODED_BUFFER_SLOT)


typedef enum _pic_mgmt_type_
{
    IMG_PICMGMT_REF_TYPE=0,
    IMG_PICMGMT_GOP_STRUCT,
    IMG_PICMGMT_SKIP_FRAME,
    IMG_PICMGMT_EOS,
    IMG_PICMGMT_FLUSH,
    IMG_PICMGMT_QUANT,
} IMG_PICMGMT_TYPE;

/*!
 ***********************************************************************************
 *
 * Description        : PIC_MGMT - SetNextRefType parameters
 *
 ************************************************************************************/
typedef struct tag_IMG_PICMGMT_REF_DATA {
    IMG_FRAME_TYPE  eFrameType;                     //!< Type of the next reference frame (IDR, I, P)
} IMG_PICMGMT_REF_DATA;

/*!
 ***********************************************************************************
 *
 * Description        : PIC_MGMT - IMG_V_SetGopStructure parameters
 *
 ************************************************************************************/
typedef struct tag_IMG_PICMGMT_GOP_DATA {
    IMG_UINT8  ui8BFramePeriod;    //!< B-period
    IMG_UINT32 ui32IFramePeriod;   //!< I-period
} IMG_PICMGMT_GOP_DATA;

/*!
 ***********************************************************************************
 *
 * Description        : PIC_MGMT - IMG_V_SkipFrame parameters
 *
 ************************************************************************************/
typedef struct tag_IMG_PICMGMT_SKIP_DATA {
    IMG_BOOL8   b8Process;          //!< Process skipped frame (to update MV) ?
} IMG_PICMGMT_SKIP_DATA;

/*!
 ***********************************************************************************
 *
 * Description        : PIC_MGMT - IMG_V_EndOfStream parameters
 *
 ************************************************************************************/
typedef struct tag_IMG_PICMGMT_EOS_DATA {
    IMG_UINT32  ui32FrameCount; //!< Number of frames in the stream (incl. skipped)
} IMG_PICMGMT_EOS_DATA;

/*!
 ***********************************************************************************
 *
 * Description        : PIC_MGMT - IMG_V_RCUpdate parameters
 *
 ************************************************************************************/
typedef struct tag_IMG_PICMGMT_RC_UPDATE_DATA {
    IMG_UINT32  ui32BitsPerFrame;         //!< Number of bits in a frame
    IMG_UINT8   ui8VCMIFrameQP;        //!< VCM I frame QP
} IMG_PICMGMT_RC_UPDATE_DATA;

/*!
 ***********************************************************************************
 *
 * Description        : PIC_MGMT - IMG_V_FlushStream parameters
 *
 ************************************************************************************/
typedef struct tag_IMG_PICMGMT_FLUSH_STREAM_DATA
{
    IMG_UINT32  ui32FlushAtFrame;       //!< Frame Idx to flush the encoder
} IMG_PICMGMT_FLUSH_STREAM_DATA;

/*!
 ***********************************************************************************
 *
 * Description        : PIC_MGMT - IMG_V_SetCustomScalingValues parameters
 *
 ************************************************************************************/
typedef struct tag_IMG_PICMGMT_CUSTOM_QUANT_DATA
{
    IMG_UINT32  ui32Values;             //!< Address of custom quantization values
    IMG_UINT32  ui32Regs4x4Sp;      //!< Address of custom quantization register values for 4x4 Sp
    IMG_UINT32  ui32Regs8x8Sp;      //!< Address of custom quantization register values for 8x8 Sp
    IMG_UINT32  ui32Regs4x4Q;       //!< Address of custom quantization register values for 4x4 Q
    IMG_UINT32  ui32Regs8x8Q;       //!< Address of custom quantization register values for 8x8 Q
} IMG_PICMGMT_CUSTOM_QUANT_DATA;

/*!
***********************************************************************************
@Description        : PROVIDE_BUFFER - Details of the source picture buffer
************************************************************************************/
typedef struct tag_IMG_SOURCE_BUFFER_PARAMS {
    IMG_UINT32      ui32PhysAddrYPlane_Field0;      //!< Source pic phys addr (Y plane, Field 0)
    IMG_UINT32      ui32PhysAddrUPlane_Field0;      //!< Source pic phys addr (U plane, Field 0)
    IMG_UINT32      ui32PhysAddrVPlane_Field0;      //!< Source pic phys addr (V plane, Field 0)
    IMG_UINT32      ui32PhysAddrYPlane_Field1;      //!< Source pic phys addr (Y plane, Field 1)
    IMG_UINT32      ui32PhysAddrUPlane_Field1;      //!< Source pic phys addr (U plane, Field 1)
    IMG_UINT32      ui32PhysAddrVPlane_Field1;      //!< Source pic phys addr (V plane, Field 1)
    IMG_UINT32      ui32HostContext;                //!< Host context value
    IMG_UINT8       ui8DisplayOrderNum;             //!< Number of frames in the stream (incl. skipped)
    IMG_UINT8       ui8SlotNum;                     //!< Number of frames in the stream (incl. skipped)
    IMG_UINT8       uiReserved1;
    IMG_UINT8       uiReserved2;
} IMG_SOURCE_BUFFER_PARAMS;


/*!
***********************************************************************************
@Description        : PROVIDE_BUFFER - Details of the reference picture buffer
************************************************************************************/
typedef struct tag_IMG_BUFFER_REF_DATA {
    IMG_INT8    i8HeaderSlotNum;                        //!< Slot from which to read the slice header (-1 if none)
    IMG_BOOL8   b8LongTerm;                                     //!< Flag identifying the reference as long-term
} IMG_BUFFER_REF_DATA;

/*!
***********************************************************************************
@Description        : PROVIDE_BUFFER - Details of the coded buffer
************************************************************************************/
typedef struct tag_IMG_BUFFER_CODED_DATA {
    IMG_UINT32  ui32Size;           //!< Size of coded buffer in bytes
    IMG_UINT8   ui8SlotNum;             //!< Slot in firmware that this coded buffer should occupy
} IMG_BUFFER_CODED_DATA;

/*!
 * *****************************************************************************
 * @details    PROVIDE_REF_BUFFER - Purpose of the reference buffer
 * @brief      Purpose of the reference buffer
 *****************************************************************************/
typedef enum _buffer_type_
{
	IMG_BUFFER_REF0 = 0,
	IMG_BUFFER_REF1,
	IMG_BUFFER_RECON,
} IMG_REF_BUFFER_TYPE;


/***********************************************************************************
@PICMGMT - functions
************************************************************************************/
VAStatus tng_picmgmt_update(context_ENC_p ctx, IMG_PICMGMT_TYPE eType, unsigned int ref);

/***********************************************************************************
@PROVIDE_BUFFER - functions
************************************************************************************/

IMG_UINT32 tng_send_codedbuf(context_ENC_p ctx, IMG_UINT32 ui32SlotIndex);
IMG_UINT32 tng_send_source_frame(context_ENC_p ctx, IMG_UINT32 ui32SlotIndex, IMG_UINT32 ui32DisplayOrder);
IMG_UINT32 tng_send_rec_frames(context_ENC_p ctx, IMG_INT8 i8HeaderSlotNum, IMG_BOOL bLongTerm);
IMG_UINT32 tng_send_ref_frames(context_ENC_p ctx, IMG_UINT32 ui32RefIndex, IMG_BOOL bLongTerm);

#endif //_TNG_PICMGMT_H_
