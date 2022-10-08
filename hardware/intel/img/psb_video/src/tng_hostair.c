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

#include <unistd.h>
#include <stdio.h>
#include <memory.h>
#include "psb_drv_video.h"
#include "psb_drv_debug.h"
#include "tng_hostdefs.h"
#include "tng_hostcode.h"
#include "tng_hostair.h"

/***********************************************************************************
 * Function Name     : functions of pi8AIR_Table table
 ************************************************************************************/
VAStatus tng_air_buf_create(context_ENC_p ctx)
{
    IMG_UINT32 ui32MbNum = (ctx->ui16PictureHeight * ctx->ui16Width) >> 8;
    ctx->sAirInfo.pi8AIR_Table = (IMG_INT8 *)malloc(ui32MbNum);
    drv_debug_msg(VIDEO_DEBUG_GENERAL,"%s: ui32MbNum = %d\n", __FUNCTION__, ui32MbNum);
    if (!ctx->sAirInfo.pi8AIR_Table) {
        drv_debug_msg(VIDEO_DEBUG_ERROR,
            "\nERROR: Error allocating Adaptive Intra Refresh table of Application context (APP_SetVideoParams)");
        return VA_STATUS_ERROR_ALLOCATION_FAILED;
    }
    memset(ctx->sAirInfo.pi8AIR_Table, 0, ui32MbNum);
    return VA_STATUS_SUCCESS;
}

static void tng_air_buf_clear(context_ENC_p ctx)
{
#if 0
    IMG_UINT32 ui32MbNum = (ctx->ui16PictureHeight * ctx->ui16Width) >> 8;
    drv_debug_msg(VIDEO_DEBUG_ERROR,"%s: ui32MbNum = %d, ctx->sAirInfo.pi8AIR_Table = 0x%08x\n", __FUNCTION__, ui32MbNum, ctx->sAirInfo.pi8AIR_Table);
    memset(ctx->sAirInfo.pi8AIR_Table, 0, ui32MbNum);    
    drv_debug_msg(VIDEO_DEBUG_ERROR,"%s: ui32MbNum = %d, ctx->sAirInfo.pi8AIR_Table = 0x%08x\n", __FUNCTION__, ui32MbNum, ctx->sAirInfo.pi8AIR_Table);
#endif
    tng_cmdbuf_insert_command(ctx->obj_context, ctx->ui32StreamID,
        MTX_CMDID_SW_AIR_BUF_CLEAR, 0, 0, 0);
    return ;
}

void tng_air_buf_free(context_ENC_p ctx)
{
    if (ctx->sAirInfo.pi8AIR_Table != NULL)
        free(ctx->sAirInfo.pi8AIR_Table);
    return ;
}

/***********************************************************************************
 * Function Name     : functions for input control
 ************************************************************************************/
static IMG_UINT16 tng__rand(context_ENC_p ctx) 
{
    IMG_UINT16 ui16ret = 0;
    ctx->ui32pseudo_rand_seed =  (IMG_UINT32) ((ctx->ui32pseudo_rand_seed * 1103515245 + 12345) & 0xffffffff); //Using mask, just in case
    ui16ret = (IMG_UINT16)(ctx->ui32pseudo_rand_seed / 65536) % 32768; 
    return ui16ret;
}

//APP_FillSliceMap
IMG_UINT32 tng_fill_slice_map(context_ENC_p ctx, IMG_INT32 i32SlotNum, IMG_UINT32 ui32StreamIndex)
{
    context_ENC_mem *ps_mem = &(ctx->ctx_mem[ui32StreamIndex]);
    unsigned char *pvBuffer;
    IMG_UINT8 ui8SlicesPerPicture;
    IMG_UINT8 ui8HalfWaySlice;
    IMG_UINT32 ui32HalfwayBU;

    ui8SlicesPerPicture = ctx->ui8SlicesPerPicture;
    ui32HalfwayBU = 0;
    ui8HalfWaySlice=ui8SlicesPerPicture/2;
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: slot num = %d, aso = %d\n", __FUNCTION__, i32SlotNum, ctx->bArbitrarySO);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: stream id = %d, addr = 0x%x\n", __FUNCTION__, ui32StreamIndex, ps_mem->bufs_slice_map.virtual_addr);

    psb_buffer_map(&(ps_mem->bufs_slice_map), &(ps_mem->bufs_slice_map.virtual_addr));
    if (ps_mem->bufs_slice_map.virtual_addr == NULL) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s error: mapping slice map\n", __FUNCTION__);
        goto out1;
    }

    pvBuffer = (unsigned char*)(ps_mem->bufs_slice_map.virtual_addr + (i32SlotNum * ctx->ctx_mem_size.slice_map));
    if (pvBuffer == NULL) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "%s: pvBuffer == NULL\n", __FUNCTION__);
        goto out1;
    }

    if (ctx->bArbitrarySO) {
        IMG_UINT8 ui8Index;
        IMG_UINT8 ui32FirstBUInSlice;
        IMG_UINT8 ui8SizeInKicks;
        IMG_UINT8 ui8TotalBUs;
        IMG_UINT8 aui8SliceNumbers[MAX_SLICESPERPIC];

	memset(aui8SliceNumbers, 0, MAX_SLICESPERPIC);

        ui8SlicesPerPicture = tng__rand(ctx) % ctx->ui8SlicesPerPicture + 1;
        // Fill slice map
        // Fill number of slices
        * pvBuffer = ui8SlicesPerPicture;
        pvBuffer++;

        for (ui8Index = 0; ui8Index < ui8SlicesPerPicture; ui8Index++)
            aui8SliceNumbers[ui8Index] = ui8Index;

	// randomise slice numbers
        for (ui8Index = 0; ui8Index < 20; ui8Index++) {
            IMG_UINT8 ui8FirstCandidate;
            IMG_UINT8 ui8SecondCandidate;
            IMG_UINT8 ui8Temp;

            ui8FirstCandidate = tng__rand(ctx) % ui8SlicesPerPicture;
            ui8SecondCandidate = tng__rand(ctx) % ui8SlicesPerPicture;

            ui8Temp = aui8SliceNumbers[ui8FirstCandidate];
            aui8SliceNumbers[ui8FirstCandidate] = aui8SliceNumbers[ui8SecondCandidate];
            aui8SliceNumbers[ui8SecondCandidate] = ui8Temp;
        }

        ui8TotalBUs = (ctx->ui16PictureHeight / 16) * (ctx->ui16Width / 16) / ctx->sRCParams.ui32BUSize;

        ui32FirstBUInSlice = 0;

        for (ui8Index = 0; ui8Index < ui8SlicesPerPicture - 1; ui8Index++) {
            IMG_UINT32 ui32BUsCalc;
            if (ui8Index==ui8HalfWaySlice) ui32HalfwayBU=ui32FirstBUInSlice;

            ui32BUsCalc=(ui8TotalBUs - 1 * (ui8SlicesPerPicture - ui8Index));
            if(ui32BUsCalc)
                ui8SizeInKicks = tng__rand(ctx) %ui32BUsCalc  + 1;
            else
                ui8SizeInKicks = 1;
            ui8TotalBUs -= ui8SizeInKicks;

            // slice number
            * pvBuffer = aui8SliceNumbers[ui8Index];
            pvBuffer++;

            // SizeInKicks BU
            * pvBuffer = ui8SizeInKicks;
            pvBuffer++;
            ui32FirstBUInSlice += (IMG_UINT32) ui8SizeInKicks;
        }
        ui8SizeInKicks = ui8TotalBUs;
        // slice number
        * pvBuffer = aui8SliceNumbers[ui8SlicesPerPicture - 1];
        pvBuffer++;

        // last BU
        * pvBuffer = ui8SizeInKicks;
        pvBuffer++;
    } else {
        // Fill standard Slice Map (non arbitrary)
        IMG_UINT8 ui8Index;
        IMG_UINT8 ui8SliceNumber;
        IMG_UINT8 ui32FirstBUInSlice;
        IMG_UINT8 ui8SizeInKicks;
        IMG_UINT32 ui32SliceHeight;

        // Fill number of slices
        * pvBuffer = ui8SlicesPerPicture;
        pvBuffer++;


        ui32SliceHeight = (ctx->ui16PictureHeight / ctx->ui8SlicesPerPicture) & ~15;

        ui32FirstBUInSlice = 0;
        ui8SliceNumber = 0;
        for (ui8Index = 0; ui8Index < ui8SlicesPerPicture - 1; ui8Index++) {
            if (ui8Index==ui8HalfWaySlice) ui32HalfwayBU=ui32FirstBUInSlice;
            ui8SizeInKicks = ((ui32SliceHeight / 16)*(ctx->ui16Width/16))/ctx->sRCParams.ui32BUSize;

            // slice number
            * pvBuffer = ui8SliceNumber;
            pvBuffer++;
            // SizeInKicks BU
            * pvBuffer = ui8SizeInKicks;
            pvBuffer++;

            ui8SliceNumber++;
            ui32FirstBUInSlice += (IMG_UINT32) ui8SizeInKicks;
        }
        ui32SliceHeight = ctx->ui16PictureHeight - ui32SliceHeight * (ctx->ui8SlicesPerPicture - 1);
        if (ui8Index==ui8HalfWaySlice) ui32HalfwayBU=ui32FirstBUInSlice;
        ui8SizeInKicks = ((ui32SliceHeight / 16)*(ctx->ui16Width/16))/ctx->sRCParams.ui32BUSize;

        // slice number
        * pvBuffer = ui8SliceNumber;    pvBuffer++;
        // last BU
        * pvBuffer = ui8SizeInKicks;    pvBuffer++;
    }

out1:
    psb_buffer_unmap(&(ps_mem->bufs_slice_map));
    ctx->ui32HalfWayBU[i32SlotNum] = ui32HalfwayBU;
    drv_debug_msg(VIDEO_DEBUG_GENERAL,"%s: ui32HalfWayBU = %d\n", __FUNCTION__, ctx->ui32HalfWayBU[i32SlotNum]);
    return ui32HalfwayBU;
}

//IMG_V_GetInpCtrlBuf
static VAStatus tng__map_inp_ctrl_buf( 
    context_ENC_p ctx,
    IMG_UINT8   ui8SlotNumber,
    IMG_UINT8 **ppsInpCtrlBuf)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS; 
    context_ENC_mem* ps_mem = &(ctx->ctx_mem[0]);
    context_ENC_mem_size *ps_mem_size = &(ctx->ctx_mem_size);
    if (ppsInpCtrlBuf == NULL) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL,"%s: ppsInpCtrlBuf == NULL\n", __FUNCTION__);
        return VA_STATUS_ERROR_INVALID_PARAMETER;
    }

    *ppsInpCtrlBuf = NULL; // Not enabled

    // if enabled, return the input-control buffer corresponding to this slot
    if (ctx->bEnableInpCtrl) {
        vaStatus = psb_buffer_map(&(ps_mem->bufs_mb_ctrl_in_params), ppsInpCtrlBuf);
        if (vaStatus == VA_STATUS_SUCCESS)
            *ppsInpCtrlBuf += ui8SlotNumber * ps_mem_size->mb_ctrl_in_params;
        else
            psb_buffer_unmap(&(ps_mem->bufs_mb_ctrl_in_params));
    }

    return vaStatus;
}

static VAStatus tng__unmap_inp_ctrl_buf( 
    context_ENC_p ctx,
    IMG_UINT8   __maybe_unused ui8SlotNumber,
    IMG_UINT8 **ppsInpCtrlBuf)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS; 
    context_ENC_mem* ps_mem = &(ctx->ctx_mem[0]);

    // if enabled, return the input-control buffer corresponding to this slot
    if (*ppsInpCtrlBuf != NULL) {
        psb_buffer_unmap(&(ps_mem->bufs_mb_ctrl_in_params));
        *ppsInpCtrlBuf = NULL; // Not enabled
    }
    return vaStatus;
}

//APP_FillInpCtrlBuf
#define DEFAULT_INTER_INTRA_SCALE_TBL_IDX   (0)
#define DEFAULT_CODED_SKIPPED_SCALE_TBL_IDX (0)
#define DEFAULT_INPUT_QP                    (0xF)
#define SIZEOF_MB_IN_CTRL_PARAM     (2)

static void tng__fill_inp_ctrl_buf(
    context_ENC_p ctx,
    IMG_UINT8 *pInpCtrlBuf,
    IMG_INT16 i16IntraRefresh,
    IMG_INT8* pi8QP,
    IMG_UINT32 __maybe_unused ui32HalfWayBU)
{
    IMG_PVOID   pvBuffer;
    IMG_UINT32  ui32MBFrameWidth;
    IMG_UINT32  ui32MBPictureHeight;
    IMG_UINT32  ui32MBSliceHeight;
    IMG_UINT16  ui16DefaultParam;
    IMG_UINT16  ui16IntraParam;
    IMG_BOOL	bRefresh=IMG_FALSE;
    IMG_UINT32 ui32CurrentIndex;
    IMG_UINT32 ui32MBx, ui32MBy;
    IMG_UINT16 *pui16MBParam;
    IMG_INT8	i8QPInit;
    IMG_INT8	i8QP;
    IMG_INT8	iMaxQP;

#ifdef BRN_30324
    IMG_UINT32 ui32HalfWayMB=ui32HalfWayBU * ctx->sRCParams.ui32BUSize;
#endif

    if (pi8QP == NULL) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL,"%s: start QP == NULL\n", __FUNCTION__);
        return ;
    }

    drv_debug_msg(VIDEO_DEBUG_GENERAL,"%s: start QP = %d\n", __FUNCTION__, *pi8QP);

    if (i16IntraRefresh > 0) {
        bRefresh=IMG_TRUE;
    }

    iMaxQP = 31;
    if (ctx->eStandard == IMG_STANDARD_H264) {
        iMaxQP = 51;
    }
    if(pi8QP) {
        i8QPInit = * pi8QP;
    } else {
        i8QPInit = DEFAULT_INPUT_QP;
    }
    // get the buffer
    // IMG_C_GetBuffer(psActiveContext->hContext, pInpCtrlBuf, &pvBuffer,IMG_TRUE);
    pvBuffer = (IMG_PVOID) pInpCtrlBuf;

    //fill data
    ui32MBFrameWidth  = (ctx->ui16Width/16);
    ui32MBPictureHeight = (ctx->ui16PictureHeight/16);
    ui32MBSliceHeight = (ui32MBPictureHeight/ctx->ui8SlicesPerPicture);

    pui16MBParam = (IMG_UINT16 *)pvBuffer;
    ui32CurrentIndex=0;

    for(ui32MBy = 0; ui32MBy < (IMG_UINT32)(ctx->ui16PictureHeight / 16); ui32MBy++) {
        for(ui32MBx = 0; ui32MBx < ui32MBFrameWidth; ui32MBx++) {
            IMG_UINT16 ui16MBParam = 0;

#ifdef BRN_30324
            if (ui32HalfWayMB && ui32CurrentIndex == ui32HalfWayMB)
                if (ctx->ui8SlicesPerPicture > 1 && ctx->i32NumPipes > 1) {
                    ui32CurrentIndex=(((ui32CurrentIndex)+31)&(~31));
                }
#endif
            i8QP = i8QPInit + ((tng__rand(ctx)%6)-3);
            i8QP = tng__max(tng__min(i8QP, iMaxQP), ctx->sRCParams.iMinQP);

            ui16DefaultParam = ( i8QP<<10) | (3 <<7) |(3<<4);
            ui16IntraParam =  ( i8QP<<10)	| (0 <<7) |(0<<4);

            ui16MBParam = ui16DefaultParam;
            if (bRefresh) {
                if ((IMG_INT32)ui32CurrentIndex > ctx->i32LastCIRIndex) {
                    ctx->i32LastCIRIndex = ui32CurrentIndex;
                    ui16MBParam = ui16IntraParam;
                    i16IntraRefresh--;
                    if(i16IntraRefresh <= 0)
                        bRefresh = IMG_FALSE;
                }
            }
            pui16MBParam[ui32CurrentIndex++] = ui16MBParam;
        }
    }

    if (bRefresh) {
        ctx->i32LastCIRIndex = -1;
        while (i16IntraRefresh) {
            i8QP = i8QPInit + ((tng__rand(ctx)%6)-3);
            i8QP = tng__max(tng__min(i8QP, iMaxQP), ctx->sRCParams.iMinQP);
            ui16IntraParam = ( i8QP<<10) |(0 <<7) |(0<<4);
            pui16MBParam[++ctx->i32LastCIRIndex] = ui16IntraParam;
            i16IntraRefresh--;
        }
    }

    drv_debug_msg(VIDEO_DEBUG_GENERAL,"%s: end QP = %d\n", __FUNCTION__, *pi8QP);
    //release buffer
    //IMG_C_ReleaseBuffer(psActiveContext->hContext, pInpCtrlBuf,IMG_TRUE);
    return ;
}

/***********************************************************************************
 * Function Name     : APP_FillInputControl
 * Inputs                   : psContext
 * Description           : Fills input control buffer for a given source picture
 ************************************************************************************/
static void tng__fill_input_control(
    context_ENC_p ctx,
    IMG_UINT8 ui8SlotNum,
    IMG_UINT32 __maybe_unused ui32HalfWayBU)
{
    IMG_UINT8 * pInpCtrlBuf = NULL;
    IMG_INT8 i8InitialQp = ctx->sRCParams.ui32InitialQp;
    // Get pointer to MB Control buffer for current source buffer (if input control is enabled, otherwise buffer is NULL)
    // Please refer to kernel tng_setup_cir_buf()
    /*
    tng__map_inp_ctrl_buf(ctx, ui8SlotNum, &pInpCtrlBuf);
    if (pInpCtrlBuf!= IMG_NULL) {
        tng__fill_inp_ctrl_buf(ctx, pInpCtrlBuf,(IMG_INT16)(ctx->ui16IntraRefresh), &i8InitialQp, ui32HalfWayBU);
    }
    tng__unmap_inp_ctrl_buf(ctx, ui8SlotNum, &pInpCtrlBuf);
    */
    tng_cmdbuf_insert_command(ctx->obj_context, ctx->ui32StreamID,
        MTX_CMDID_SW_FILL_INPUT_CTRL, ui8SlotNum, 0, 0);

    return ;
}

static void tng__send_air_inp_ctrl_buf(context_ENC_p ctx, IMG_INT8 *pInpCtrlBuf)
{
    //IMG_PVOID pvICBuffer;
    IMG_UINT16 ui16IntraParam;
    //IMG_BOOL bRefresh = IMG_FALSE;
    IMG_UINT32 ui32CurrentCnt, ui32SentCnt;
    IMG_UINT32 ui32MBMaxSize;
    IMG_UINT16 *pui16MBParam;
    IMG_UINT32 ui32NewScanPos, ui32Skip;

#ifdef ADAPTIVE_INTRA_REFRESH_DEBUG_OUTPUT
    IMG_CHAR TmpOutputTble[396]; //Debug only
#endif
    ui16IntraParam = (0 << 7) | (0 << 4);

    if (ctx->ui32FrameCount[0] < 1)
        return;
    
    // get the buffer
    pui16MBParam = (IMG_UINT16 *) pInpCtrlBuf;

    //fill data 
    ui32MBMaxSize = (IMG_UINT32)(ctx->ui16PictureHeight / 16) * (IMG_UINT32)(ctx->ui16Width / 16);
    ui32CurrentCnt = 0;
    if (ctx->sAirInfo.i16AIRSkipCnt >= 0)
        ui32Skip = ctx->sAirInfo.i16AIRSkipCnt;
    else
        //ui32Skip=APP_Rand() % psActiveContext->sAirInfo.i32NumAIRSPerFrame; // Pseudorandom skip.
        ui32Skip = (ctx->ui32FrameCount[0] & 0x7) + 1; // Pseudorandom skip.
    
#ifdef ADAPTIVE_INTRA_REFRESH_DEBUG_OUTPUT
        {
            IMG_UINT32 tsp;
            if (fp)
            {
                fp = fopen("SADvals.txt", "a");
            }
            else
            {
                fp = fopen("SADvals.txt", "w");
            }
    
            fprintf(fp, "\n---------------------------------------------------------------------------\n");
            fprintf(fp, "SENDING SADvals  (skip:%i)\n", ui32Skip);
    
            for (tsp = 0; tsp < ui32MBMaxSize; tsp++)
            {
                if (ctx->sAirInfo.pi8AIR_Table[tsp] > 0)
                {
                    TmpOutputTble[tsp] = 'x';
                }
                else
                {
                    TmpOutputTble[tsp] = 'o';
                }
            }
        }
#endif
    
    ui32NewScanPos = (IMG_UINT32) (ctx->sAirInfo.ui16AIRScanPos + ui32Skip) % ui32MBMaxSize;
    ui32CurrentCnt = ui32SentCnt = 0;

    while (ui32CurrentCnt < ui32MBMaxSize &&
        ((ctx->sAirInfo.i32NumAIRSPerFrame == 0) ||
        ui32SentCnt < (IMG_UINT32) ctx->sAirInfo.i32NumAIRSPerFrame)) {
        IMG_UINT16 ui16MBParam;

        if (ctx->sAirInfo.pi8AIR_Table[ui32NewScanPos] >= 0) {
            // Mark the entry as 'touched'
            ctx->sAirInfo.pi8AIR_Table[ui32NewScanPos] = -1 - ctx->sAirInfo.pi8AIR_Table[ui32NewScanPos];
    
            if (ctx->sAirInfo.pi8AIR_Table[ui32NewScanPos] < -1) {
                ui16MBParam = pui16MBParam[ui32NewScanPos] & (0xFF << 10);
                ui16MBParam |= ui16IntraParam;
                pui16MBParam[ui32NewScanPos] = ui16MBParam;
                ctx->sAirInfo.pi8AIR_Table[ui32NewScanPos]++;
#ifdef ADAPTIVE_INTRA_REFRESH_DEBUG_OUTPUT
                TmpOutputTble[ui32NewScanPos]='I';
#endif
                ui32NewScanPos += ui32Skip;
                ui32SentCnt++;
            }
            ui32CurrentCnt++;
        }
    
        ui32NewScanPos++;
        ui32NewScanPos = ui32NewScanPos % ui32MBMaxSize;
        if (ui32NewScanPos == ctx->sAirInfo.ui16AIRScanPos) {
            /* we have looped around */ 
            break;
        }
    }
    
    ctx->sAirInfo.ui16AIRScanPos = ui32NewScanPos;
    
#ifdef ADAPTIVE_INTRA_REFRESH_DEBUG_OUTPUT
        {
            IMG_UINT32 tsp;
            for (tsp = 0; tsp < ui32MBMaxSize; tsp++)
            {
                if (tsp % ((IMG_UINT32)(ctx->ui16Width/16)) == 0)
                {
                    fprintf(fp, "\n%c", TmpOutputTble[tsp]);
                }
                else
                {
                    fprintf(fp, "%c", TmpOutputTble[tsp]);
                }
            }
    
            fprintf(fp, "\n---------------------------------------------------------------------------\n");
            fclose(fp);
        }
#endif

    drv_debug_msg(VIDEO_DEBUG_GENERAL,"%s: end\n", __FUNCTION__);
    return ;
}

// Adaptive Intra Refresh (AIR) - send the AIR values to the next bufferk
// APP_UpdateAdaptiveIntraRefresh_Send
static void tng__update_air_send(context_ENC_p ctx, IMG_UINT8 ui8SlotNum)
{
    IMG_UINT8 *pInpCtrlBuf = NULL;
    drv_debug_msg(VIDEO_DEBUG_GENERAL,"%s: start\n", __FUNCTION__);
    // Get pointer to MB Control buffer for current source buffer (if input control is enabled, otherwise buffer is NULL)
#if 0
    tng__map_inp_ctrl_buf(ctx, ui8SlotNum, &pInpCtrlBuf);
    if(pInpCtrlBuf!= IMG_NULL) {
        tng__send_air_inp_ctrl_buf(ctx, (IMG_INT8 *)pInpCtrlBuf);
    }
    tng__unmap_inp_ctrl_buf(ctx, ui8SlotNum, &pInpCtrlBuf);
    drv_debug_msg(VIDEO_DEBUG_GENERAL,"%s: end\n", __FUNCTION__);
#endif
    tng_cmdbuf_insert_command(ctx->obj_context, ctx->ui32StreamID,
        MTX_CMDID_SW_UPDATE_AIR_SEND, ui8SlotNum, 0, 0);
    return ;
}

/***********************************************************************************
 * Function Name     : functions for output control
 ************************************************************************************/
//IMG_V_GetFirstPassOutBuf
VAStatus tng__map_first_pass_out_buf( 
    context_ENC_p ctx,
    IMG_UINT8   __maybe_unused ui8SlotNumber,
    IMG_UINT8 **ppsFirstPassOutBuf)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS; 
    context_ENC_mem* ps_mem = &(ctx->ctx_mem[0]);

    if (ppsFirstPassOutBuf == NULL) {
        drv_debug_msg(VIDEO_DEBUG_GENERAL,"%s: ppsFirstPassOutBuf == NULL\n", __FUNCTION__);
        return VA_STATUS_ERROR_INVALID_PARAMETER;
    }

    *ppsFirstPassOutBuf = NULL; // Not enabled

    // if enabled, return the input-control buffer corresponding to this slot
    if (ctx->bEnableInpCtrl) {
         vaStatus = psb_buffer_map(&(ps_mem->bufs_first_pass_out_params), ppsFirstPassOutBuf);
        if (vaStatus != VA_STATUS_SUCCESS)
            psb_buffer_unmap(&(ps_mem->bufs_first_pass_out_params));
    }

    return vaStatus;
}

VAStatus tng__unmap_first_pass_out_buf( 
    context_ENC_p ctx,
    IMG_UINT8 __maybe_unused ui8SlotNumber,
    IMG_UINT8 **ppsFirstPassOutBuf)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS; 
    context_ENC_mem* ps_mem = &(ctx->ctx_mem[0]);

    // if enabled, return the input-control buffer corresponding to this slot
    if (*ppsFirstPassOutBuf != NULL) {
        psb_buffer_unmap(&(ps_mem->bufs_first_pass_out_params));
        *ppsFirstPassOutBuf = NULL; // Not enabled
    }

    return vaStatus;
}

//IMG_V_GetBestMBDecisionOutBuf
VAStatus tng__map_best_mb_decision_out_buf( 
    context_ENC_p ctx,
    IMG_UINT8   __maybe_unused ui8SlotNumber,
    IMG_UINT8  **ppsBestMBDecisionOutBuf)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS; 
    context_ENC_mem* ps_mem = &(ctx->ctx_mem[0]);

    // if enabled, return the input-control buffer corresponding to this slot
    if (ctx->bEnableInpCtrl)
        vaStatus = psb_buffer_map(&(ps_mem->bufs_first_pass_out_best_multipass_param), ppsBestMBDecisionOutBuf);
    else
        *ppsBestMBDecisionOutBuf = NULL; // Not enabled

    return vaStatus;
}

VAStatus tng__unmap_best_mb_decision_out_buf( 
    context_ENC_p ctx,
    IMG_UINT8   __maybe_unused ui8SlotNumber,
    IMG_UINT8  **ppsBestMBDecisionOutBuf)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS; 
    context_ENC_mem* ps_mem = &(ctx->ctx_mem[0]);

    // if enabled, return the input-control buffer corresponding to this slot
    if (*ppsBestMBDecisionOutBuf != NULL) {
        psb_buffer_unmap(&(ps_mem->bufs_first_pass_out_best_multipass_param));
        *ppsBestMBDecisionOutBuf = NULL; // Not enabled
     }

    return vaStatus;
}

// Calculate Adaptive Intra Refresh (AIR)
static void tng__calc_air_inp_ctrl_buf(context_ENC_p ctx, IMG_UINT8 *pFirstPassOutBuf, IMG_UINT8 *pBestMBDecisionCtrlBuf)
{
    IMG_UINT8 *pSADPointer;
    IMG_UINT8 *pvSADBuffer;
    IMG_UINT8 ui8IsAlreadyIntra;
    IMG_UINT32 ui32MBFrameWidth;
    IMG_UINT32 ui32MBPictureHeight;
    IMG_UINT16 ui16IntraParam;
    IMG_UINT32 ui32MBx, ui32MBy;
    IMG_UINT32 ui32SADParam;
    IMG_UINT32 ui32tSAD_Threshold, ui32tSAD_ThresholdLo, ui32tSAD_ThresholdHi;
    IMG_UINT32 ui32MaxMBs, ui32NumMBsOverThreshold, ui32NumMBsOverLo, ui32NumMBsOverHi;
    IMG_BEST_MULTIPASS_MB_PARAMS *psBestMB_Params;
    IMG_FIRST_STAGE_MB_PARAMS *psFirstMB_Params;

    ui16IntraParam = (0 << 7) | (0 << 4);
    ui32NumMBsOverThreshold = ui32NumMBsOverLo = ui32NumMBsOverHi = 0;
    //drv_debug_msg(VIDEO_DEBUG_GENERAL,"%s: start\n", __FUNCTION__);

//    if (psActiveContext->ui32EncodeSent < (IMG_UINT32)psActiveContext->ui8MaxSourceSlots + 1)
//    if (ctx->ui32FrameCount[0] < (IMG_UINT32)(ctx->ui8SlotsInUse + 1))
//        return;
    drv_debug_msg(VIDEO_DEBUG_GENERAL,"%s: start\n", __FUNCTION__);
    //fill data 
    ui32MBFrameWidth  = (ctx->ui16Width/16);
    ui32MBPictureHeight = (ctx->ui16PictureHeight/16);
	
    // get the SAD results buffer (either IPE0 and IPE1 results or, preferably, the more accurate Best Multipass SAD results)
    if (pBestMBDecisionCtrlBuf) {
        pvSADBuffer = pBestMBDecisionCtrlBuf;
        drv_debug_msg(VIDEO_DEBUG_GENERAL,"AIR active: Using Best Multipass SAD values ");

//#ifdef  MULTIPASS_MV_PLACEMENT_ISSUE_FIXED
	if ((ctx->ui8EnableSelStatsFlags & ESF_MP_BEST_MOTION_VECTOR_STATS))
//#endif
	{
            // The actual Param structures (which contain SADs) are located after the Multipass Motion Vector entries
	    pvSADBuffer += (ui32MBPictureHeight * (ui32MBFrameWidth) * sizeof(IMG_BEST_MULTIPASS_MB_PARAMS_IPMV));
        }
    } else {
        pvSADBuffer = pFirstPassOutBuf;
        drv_debug_msg(VIDEO_DEBUG_GENERAL,"AIR active: Using IPE SAD values ");
    }

    if (ctx->sAirInfo.i32NumAIRSPerFrame == 0)
        ui32MaxMBs = ui32MBFrameWidth * ui32MBPictureHeight; // Default to ALL MB's in frame
    else if (ctx->sAirInfo.i32NumAIRSPerFrame < 0)
        ctx->sAirInfo.i32NumAIRSPerFrame = ui32MaxMBs = ((ui32MBFrameWidth * ui32MBPictureHeight) + 99) / 100; // Default to 1% of MB's in frame (min 1)
    else
        ui32MaxMBs = ctx->sAirInfo.i32NumAIRSPerFrame;

    pSADPointer = (IMG_UINT8 *)pvSADBuffer;
	
    if (ctx->sAirInfo.i32SAD_Threshold >= 0)
        ui32tSAD_Threshold = (IMG_UINT16)ctx->sAirInfo.i32SAD_Threshold;
    else {
        // Running auto adjust threshold adjust mode
        if (ctx->sAirInfo.i32SAD_Threshold == -1) {
            // This will occur only the first time
            if (pBestMBDecisionCtrlBuf) {
                psBestMB_Params=(IMG_BEST_MULTIPASS_MB_PARAMS *) pSADPointer; // Auto seed the threshold with the first value
                ui32SADParam = psBestMB_Params->ui32SAD_Inter_MBInfo & IMG_BEST_MULTIPASS_SAD_MASK;
            } else {
                psFirstMB_Params=(IMG_FIRST_STAGE_MB_PARAMS *) pSADPointer; // Auto seed the threshold with the first value
                ui32SADParam = (IMG_UINT32) psFirstMB_Params->ui16Ipe0Sad;
            }
            ctx->sAirInfo.i32SAD_Threshold = -1 - ui32SADParam; // Negative numbers indicate auto-adjusting threshold
        }
        ui32tSAD_Threshold = (IMG_UINT32) - (ctx->sAirInfo.i32SAD_Threshold + 1);
    }

    ui32tSAD_ThresholdLo = ui32tSAD_Threshold / 2;
    ui32tSAD_ThresholdHi = ui32tSAD_Threshold + ui32tSAD_ThresholdLo;

    drv_debug_msg(VIDEO_DEBUG_GENERAL,"Th:%u, MaxMbs:%u, Skp:%i\n", (unsigned int)ui32tSAD_Threshold, (unsigned int)ui32MaxMBs, ctx->sAirInfo.i16AIRSkipCnt);

#ifdef ADAPTIVE_INTRA_REFRESH_DEBUG_OUTPUT
	if (fp)
		fp=fopen("SADvals.txt","a");
	else
		fp=fopen("SADvals.txt","w");

	if (ctx->sAirInfo.i32SAD_Threshold>=0)
		if (ctx->sAirInfo.i32NumAIRSPerFrame>0)
			fprintf(fp, "S_SADThreshold: %i	MaxMBs: %i\n", ui32tSAD_Threshold, ui32MaxMBs);
		else
			fprintf(fp, "S_SADThreshold: %i	MaxMBs: NA\n", ui32tSAD_Threshold, ui32MaxMBs);
	else
		fprintf(fp, "V_SADThreshold: %i	MaxMBs: %i\n", ui32tSAD_Threshold, ui32MaxMBs);

	if (pBestMBDecisionCtrlBuf)
		fprintf(fp, "Using Best Multipass SAD values\n");
	else
		fprintf(fp, "Using Motion Search Data IPE SAD values\n");
#endif

    // This loop could be optimised to a single counter if necessary, retaining for clarity
    for (ui32MBy = 0; ui32MBy < ui32MBPictureHeight; ui32MBy++) {
        for( ui32MBx=0; ui32MBx<ui32MBFrameWidth; ui32MBx++) {
#ifdef ADAPTIVE_INTRA_REFRESH_DEBUG_OUTPUT	
            IMG_CHAR cMarked;
            cMarked='_';
#endif
            // Turn all negative table values to positive (reset 'touched' state of a block that may have been set in APP_SendAIRInpCtrlBuf())
	    if (ctx->sAirInfo.pi8AIR_Table[ui32MBy *  ui32MBFrameWidth + ui32MBx] < 0)
                ctx->sAirInfo.pi8AIR_Table[ui32MBy *  ui32MBFrameWidth + ui32MBx] = -1 - ctx->sAirInfo.pi8AIR_Table[ui32MBy *  ui32MBFrameWidth + ui32MBx];

            // This will read the SAD value from the buffer (either IPE0 SAD or the superior Best multipass parameter structure SAD value)
            if (pBestMBDecisionCtrlBuf) {
                psBestMB_Params = (IMG_BEST_MULTIPASS_MB_PARAMS *) pSADPointer;
                ui32SADParam = psBestMB_Params->ui32SAD_Inter_MBInfo & IMG_BEST_MULTIPASS_SAD_MASK;

                if ((psBestMB_Params->ui32SAD_Intra_MBInfo & IMG_BEST_MULTIPASS_MB_TYPE_MASK) >> IMG_BEST_MULTIPASS_MB_TYPE_SHIFT == 1)
                    ui8IsAlreadyIntra = 1;
                else
                    ui8IsAlreadyIntra = 0;

                pSADPointer=(IMG_UINT8 *) &(psBestMB_Params[1]);
            } else {
                psFirstMB_Params=(IMG_FIRST_STAGE_MB_PARAMS *) pSADPointer;
                ui32SADParam = (IMG_UINT32) psFirstMB_Params->ui16Ipe0Sad;
                ui32SADParam += (IMG_UINT32) psFirstMB_Params->ui16Ipe1Sad;
                ui32SADParam /= 2;
                ui8IsAlreadyIntra = 0; // We don't have the information to determine this
                pSADPointer=(IMG_UINT8 *) &(psFirstMB_Params[1]);
            }

            if (ui32SADParam >= ui32tSAD_ThresholdLo) {
                ui32NumMBsOverLo++;

                if (ui32SADParam >= ui32tSAD_Threshold) {
#ifdef ADAPTIVE_INTRA_REFRESH_DEBUG_OUTPUT
                    cMarked='i';
#endif	

                    // if (!ui8IsAlreadyIntra) // Don't mark this block if it's just been encoded as an Intra block anyway
                    // (results seem better without this condition anyway)
                    {
                        ctx->sAirInfo.pi8AIR_Table[ui32MBy *  ui32MBFrameWidth + ui32MBx]++;
#ifdef ADAPTIVE_INTRA_REFRESH_DEBUG_OUTPUT	
                        cMarked='I';
#endif	
                    }
                    ui32NumMBsOverThreshold++;
                    if (ui32SADParam >= ui32tSAD_ThresholdHi)
                        ui32NumMBsOverHi++;
                }
            }

#ifdef ADAPTIVE_INTRA_REFRESH_DEBUG_OUTPUT	
            fprintf(fp,"%4x[%i]%c,	",ui32SADParam, ctx->sAirInfo.pi8AIR_Table[ui32MBy * ui32MBFrameWidth + ui32MBx], cMarked);
#endif
        }
        pSADPointer=(IMG_UINT8 *) ALIGN_64(((IMG_UINT32) pSADPointer));
#ifdef ADAPTIVE_INTRA_REFRESH_DEBUG_OUTPUT
	fprintf(fp,"\n");
#endif
    }

    // Test and process running adaptive threshold case
    if (ctx->sAirInfo.i32SAD_Threshold < 0) {
        // Adjust our threshold (to indicate it's auto-adjustable store it as a negative value minus 1)
        if (ui32NumMBsOverLo <= ui32MaxMBs)
            ctx->sAirInfo.i32SAD_Threshold = (IMG_INT32) - ((IMG_INT32)ui32tSAD_ThresholdLo) - 1;
        else
            if (ui32NumMBsOverHi >= ui32MaxMBs)
                ctx->sAirInfo.i32SAD_Threshold = (IMG_INT32) - ((IMG_INT32)ui32tSAD_ThresholdHi) - 1;
            else {
                if (ui32MaxMBs < ui32NumMBsOverThreshold) {
                    ctx->sAirInfo.i32SAD_Threshold = ((IMG_INT32)ui32tSAD_ThresholdHi - (IMG_INT32)ui32tSAD_Threshold);
                    ctx->sAirInfo.i32SAD_Threshold *= ((IMG_INT32)ui32MaxMBs - (IMG_INT32)ui32NumMBsOverThreshold);
                    ctx->sAirInfo.i32SAD_Threshold /= ((IMG_INT32)ui32NumMBsOverHi - (IMG_INT32)ui32NumMBsOverThreshold);
                    ctx->sAirInfo.i32SAD_Threshold += ui32tSAD_Threshold;
                } else {
                    ctx->sAirInfo.i32SAD_Threshold = ((IMG_INT32)ui32tSAD_Threshold - (IMG_INT32)ui32tSAD_ThresholdLo);
                    ctx->sAirInfo.i32SAD_Threshold *= ((IMG_INT32)ui32MaxMBs - (IMG_INT32)ui32NumMBsOverLo);
                    ctx->sAirInfo.i32SAD_Threshold /= ((IMG_INT32)ui32NumMBsOverThreshold - (IMG_INT32)ui32NumMBsOverLo);
                    ctx->sAirInfo.i32SAD_Threshold += ui32tSAD_ThresholdLo;
                }
                ctx->sAirInfo.i32SAD_Threshold = -ctx->sAirInfo.i32SAD_Threshold - 1;
            }

#ifdef ADAPTIVE_INTRA_REFRESH_DEBUG_OUTPUT
            fprintf(fp,"THRESHOLDS ADJUSTMENT\nThrLo:%i	ThrMid:%i	ThrHi:%i\nMBsLo:%i	MBsMid:%i	MBsHi:%i\n",ui32tSAD_ThresholdLo, ui32tSAD_Threshold, ui32tSAD_ThresholdHi, ui32NumMBsOverLo, ui32NumMBsOverThreshold, ui32NumMBsOverHi);
            fprintf(fp,"Target No. MB's:%i\nThreshold adjusted to: %i\n",ui32MaxMBs, -(ctx->sAirInfo.i32SAD_Threshold));
#endif
    }

#ifdef ADAPTIVE_INTRA_REFRESH_DEBUG_OUTPUT
    fprintf(fp,"\n MBs tagged:%i\n", ui32NumMBsOverThreshold);
    fclose(fp);
#endif
    drv_debug_msg(VIDEO_DEBUG_GENERAL,"%s: end\n", __FUNCTION__);
    return;
}

// Adaptive Intra Refresh (AIR) - Calculate the new AIR values based upon Motion Search feedback
// APP_UpdateAdaptiveIntraRefresh_Calc
static void tng_update_air_calc(context_ENC_p ctx, IMG_UINT8 ui8SlotNum)
{
    IMG_UINT8  *pFirstPassOutBuf = NULL;
    IMG_UINT8  *pBestMBDecisionCtrlBuf = NULL;
#if 0
    // Get pointer to MB Control buffer for current source buffer (if input control is enabled, otherwise buffer is NULL)
    tng__map_first_pass_out_buf(ctx, ui8SlotNum, &pFirstPassOutBuf);
    tng__map_best_mb_decision_out_buf(ctx, ui8SlotNum, &pBestMBDecisionCtrlBuf);

    if(pFirstPassOutBuf || pBestMBDecisionCtrlBuf)
	tng__calc_air_inp_ctrl_buf (ctx, pFirstPassOutBuf, pBestMBDecisionCtrlBuf);

    tng__unmap_first_pass_out_buf(ctx, ui8SlotNum, &pFirstPassOutBuf);
    tng__unmap_best_mb_decision_out_buf(ctx, ui8SlotNum, &pBestMBDecisionCtrlBuf);
#endif
    tng_cmdbuf_insert_command(ctx->obj_context, ctx->ui32StreamID,
        MTX_CMDID_SW_UPDATE_AIR_CALC, ui8SlotNum, 0, 0);
}

/***********************************************************************************
 * Function Name     : 
 * Inputs                   : 
 * Description           : 
 ************************************************************************************/
void tng_air_set_input_control(context_ENC_p ctx, IMG_UINT8 __maybe_unused ui8StreamID)
{
    IMG_UINT8 ui8SlotIndex = ctx->ui8SlotsCoded;
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: slot index = %d\n", __FUNCTION__, ctx->ui8SlotsCoded);
    //IMG_UINT32 ui32HalfWayBU;
    //ui32HalfWayBU = tng_fill_slice_map(ctx, ui8SlotIndex, ui8StreamID);

    ////////////////////////////// INPUT CONTROL
    // Add input control stuff here
    tng__fill_input_control(ctx, ui8SlotIndex, ctx->ui32HalfWayBU[ui8SlotIndex]);

    // Adaptive Intra Refresh (AIR) - send the AIR values to the next buffer
    if (ctx->bEnableAIR)
        tng__update_air_send(ctx, ui8SlotIndex);
}


/***********************************************************************************
 * Function Name     : 
 * Inputs                   : 
 * Description           : 
 ************************************************************************************/
void tng_air_set_output_control(context_ENC_p ctx, IMG_UINT8 __maybe_unused ui8StreamID)
{
    IMG_UINT8 ui8SlotIndex = ctx->ui8SlotsCoded;

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "%s: slot index = %d\n", __FUNCTION__, ctx->ui8SlotsCoded);

    if ((ctx->eFrameType == IMG_INTRA_IDR) ||
        (ctx->eFrameType == IMG_INTRA_FRAME))
        tng_air_buf_clear(ctx);
    else
        tng_update_air_calc(ctx, ui8SlotIndex);

    return;
}
