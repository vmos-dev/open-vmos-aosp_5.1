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
 *    Li Zeng <li.zeng@intel.com>
 *
 */

#include "pnw_VC1.h"
#include "psb_def.h"
#include "psb_drv_debug.h"
#include "pnw_rotate.h"

#include "vc1_header.h"
#include "vc1_defs.h"

#include "hwdefs/reg_io2.h"
#include "hwdefs/msvdx_offsets.h"
#include "hwdefs/msvdx_cmds_io2.h"
#include "hwdefs/msvdx_vec_reg_io2.h"
#include "hwdefs/msvdx_vec_vc1_reg_io2.h"
#include "hwdefs/msvdx_rendec_vc1_reg_io2.h"
#include "hwdefs/dxva_fw_ctrl.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define VC1_Header_Parser_HW

#define GET_SURFACE_INFO_is_defined(psb_surface) ((int) (psb_surface->extra_info[0]))
#define SET_SURFACE_INFO_is_defined(psb_surface, val) psb_surface->extra_info[0] = (uint32_t) val;
#define GET_SURFACE_INFO_picture_structure(psb_surface) (psb_surface->extra_info[1])
#define SET_SURFACE_INFO_picture_structure(psb_surface, val) psb_surface->extra_info[1] = val;
#define GET_SURFACE_INFO_picture_coding_type(psb_surface) ((int) (psb_surface->extra_info[2]))
#define SET_SURFACE_INFO_picture_coding_type(psb_surface, val) psb_surface->extra_info[2] = (uint32_t) val;

#define SLICEDATA_BUFFER_TYPE(type) ((type==VASliceDataBufferType)?"VASliceDataBufferType":"VAProtectedSliceDataBufferType")

#define PIXELS_TO_MB(x)    ((x + 15) / 16)

#define PRELOAD_BUFFER_SIZE        (4*1024)
#define AUXMSB_BUFFER_SIZE         (1024*1024)

#define HW_SUPPORTED_MAX_PICTURE_WIDTH_VC1   1920
#define HW_SUPPORTED_MAX_PICTURE_HEIGHT_VC1  1088

typedef struct {
    IMG_UINT32              ui32ContextId;
    IMG_UINT32              ui32SliceParams;
    IMG_UINT32              ui32MacroblockNumber;
} VC1PRELOAD;

#define FWPARSER_VC1PRELOAD_SIZE (0x60)

/*!
******************************************************************************
 @LUTs   VLC table selection Look-up Tables
******************************************************************************/

typedef enum {
    VC1_VLC_Code_3x2_2x3_tiles              = 0x0,
    VC1_VLC_FourMV_Pattern_0,
    VC1_VLC_FourMV_Pattern_1,
    VC1_VLC_FourMV_Pattern_2,
    VC1_VLC_FourMV_Pattern_3,
    VC1_VLC_High_Mot_Chroma_DC_Diff_VLC,
    VC1_VLC_High_Mot_Inter_VLC,
    VC1_VLC_High_Mot_Intra_VLC,
    VC1_VLC_High_Mot_Luminance_DC_Diff_VLC,
    VC1_VLC_High_Rate_Inter_VLC,
    VC1_VLC_High_Rate_Intra_VLC,
    VC1_VLC_High_Rate_SUBBLKPAT,
    VC1_VLC_High_Rate_TTBLK,
    VC1_VLC_High_Rate_TTMB,
    VC1_VLC_I_Picture_CBPCY_VLC,
    VC1_VLC_Interlace_2_MVP_Pattern_0,
    VC1_VLC_Interlace_2_MVP_Pattern_1,
    VC1_VLC_Interlace_2_MVP_Pattern_2,
    VC1_VLC_Interlace_2_MVP_Pattern_3,
    VC1_VLC_Interlace_4MV_MB_0,
    VC1_VLC_Interlace_4MV_MB_1,
    VC1_VLC_Interlace_4MV_MB_2,
    VC1_VLC_Interlace_4MV_MB_3,
    VC1_VLC_Interlace_Non_4MV_MB_0,
    VC1_VLC_Interlace_Non_4MV_MB_1,
    VC1_VLC_Interlace_Non_4MV_MB_2,
    VC1_VLC_Interlace_Non_4MV_MB_3,
    VC1_VLC_Interlaced_CBPCY_0,
    VC1_VLC_Interlaced_CBPCY_1,
    VC1_VLC_Interlaced_CBPCY_2,
    VC1_VLC_Interlaced_CBPCY_3,
    VC1_VLC_Interlaced_CBPCY_4,
    VC1_VLC_Interlaced_CBPCY_5,
    VC1_VLC_Interlaced_CBPCY_6,
    VC1_VLC_Interlaced_CBPCY_7,
    VC1_VLC_Low_Mot_Chroma_DC_Diff_VLC,
    VC1_VLC_Low_Mot_Inter_VLC,
    VC1_VLC_Low_Mot_Intra_VLC,
    VC1_VLC_Low_Mot_Luminance_DC_Diff_VLC,
    VC1_VLC_Low_Rate_SUBBLKPAT,
    VC1_VLC_Low_Rate_TTBLK,
    VC1_VLC_Low_Rate_TTMB,
    VC1_VLC_Medium_Rate_SUBBLKPAT,
    VC1_VLC_Medium_Rate_TTBLK,
    VC1_VLC_Medium_Rate_TTMB,
    VC1_VLC_Mid_Rate_Inter_VLC,
    VC1_VLC_Mid_Rate_Intra_VLC,
    VC1_VLC_Mixed_MV_MB_0,
    VC1_VLC_Mixed_MV_MB_1,
    VC1_VLC_Mixed_MV_MB_2,
    VC1_VLC_Mixed_MV_MB_3,
    VC1_VLC_Mixed_MV_MB_4,
    VC1_VLC_Mixed_MV_MB_5,
    VC1_VLC_Mixed_MV_MB_6,
    VC1_VLC_Mixed_MV_MB_7,
    VC1_VLC_Mot_Vector_Diff_VLC_0,
    VC1_VLC_Mot_Vector_Diff_VLC_1,
    VC1_VLC_Mot_Vector_Diff_VLC_2,
    VC1_VLC_Mot_Vector_Diff_VLC_3,
    VC1_VLC_One_Field_Ref_Ilace_MV_0,
    VC1_VLC_One_Field_Ref_Ilace_MV_1,
    VC1_VLC_One_Field_Ref_Ilace_MV_2,
    VC1_VLC_One_Field_Ref_Ilace_MV_3,
    VC1_VLC_One_MV_MB_0,
    VC1_VLC_One_MV_MB_1,
    VC1_VLC_One_MV_MB_2,
    VC1_VLC_One_MV_MB_3,
    VC1_VLC_One_MV_MB_4,
    VC1_VLC_One_MV_MB_5,
    VC1_VLC_One_MV_MB_6,
    VC1_VLC_One_MV_MB_7,
    VC1_VLC_P_Picture_CBPCY_VLC_0,
    VC1_VLC_P_Picture_CBPCY_VLC_1,
    VC1_VLC_P_Picture_CBPCY_VLC_2,
    VC1_VLC_P_Picture_CBPCY_VLC_3,
    VC1_VLC_Two_Field_Ref_Ilace_MV_0,
    VC1_VLC_Two_Field_Ref_Ilace_MV_1,
    VC1_VLC_Two_Field_Ref_Ilace_MV_2,
    VC1_VLC_Two_Field_Ref_Ilace_MV_3,
    VC1_VLC_Two_Field_Ref_Ilace_MV_4,
    VC1_VLC_Two_Field_Ref_Ilace_MV_5,
    VC1_VLC_Two_Field_Ref_Ilace_MV_6,
    VC1_VLC_Two_Field_Ref_Ilace_MV_7,

} VC1_eVLCTables;

static IMG_UINT8 MBMODETableFLDI[][2] = {
    {VC1_VLC_One_MV_MB_0, VC1_VLC_Mixed_MV_MB_0},
    {VC1_VLC_One_MV_MB_1, VC1_VLC_Mixed_MV_MB_1},
    {VC1_VLC_One_MV_MB_2, VC1_VLC_Mixed_MV_MB_2},
    {VC1_VLC_One_MV_MB_3, VC1_VLC_Mixed_MV_MB_3},
    {VC1_VLC_One_MV_MB_4, VC1_VLC_Mixed_MV_MB_4},
    {VC1_VLC_One_MV_MB_5, VC1_VLC_Mixed_MV_MB_5},
    {VC1_VLC_One_MV_MB_6, VC1_VLC_Mixed_MV_MB_6},
    {VC1_VLC_One_MV_MB_7, VC1_VLC_Mixed_MV_MB_7},
};

static IMG_UINT8 MBMODETableFRMI[][2] = {
    {VC1_VLC_Interlace_4MV_MB_0, VC1_VLC_Interlace_Non_4MV_MB_0},
    {VC1_VLC_Interlace_4MV_MB_1, VC1_VLC_Interlace_Non_4MV_MB_1},
    {VC1_VLC_Interlace_4MV_MB_2, VC1_VLC_Interlace_Non_4MV_MB_2},
    {VC1_VLC_Interlace_4MV_MB_3, VC1_VLC_Interlace_Non_4MV_MB_3},
};

static IMG_UINT8 CBPCYTableProg[] = {
    VC1_VLC_P_Picture_CBPCY_VLC_0,
    VC1_VLC_P_Picture_CBPCY_VLC_1,
    VC1_VLC_P_Picture_CBPCY_VLC_2,
    VC1_VLC_P_Picture_CBPCY_VLC_3,
};

static IMG_UINT8 CBPCYTableInterlaced[] = {
    VC1_VLC_Interlaced_CBPCY_0,
    VC1_VLC_Interlaced_CBPCY_1,
    VC1_VLC_Interlaced_CBPCY_2,
    VC1_VLC_Interlaced_CBPCY_3,
    VC1_VLC_Interlaced_CBPCY_4,
    VC1_VLC_Interlaced_CBPCY_5,
    VC1_VLC_Interlaced_CBPCY_6,
    VC1_VLC_Interlaced_CBPCY_7,
};

static IMG_UINT8 FourMVTable[] = {
    VC1_VLC_FourMV_Pattern_0,
    VC1_VLC_FourMV_Pattern_1,
    VC1_VLC_FourMV_Pattern_2,
    VC1_VLC_FourMV_Pattern_3,
};

static IMG_UINT8 Interlace2MVTable[] = {
    VC1_VLC_Interlace_2_MVP_Pattern_0,
    VC1_VLC_Interlace_2_MVP_Pattern_1,
    VC1_VLC_Interlace_2_MVP_Pattern_2,
    VC1_VLC_Interlace_2_MVP_Pattern_3,
};

static IMG_UINT8 ProgressiveMVTable[] = {
    VC1_VLC_Mot_Vector_Diff_VLC_0,
    VC1_VLC_Mot_Vector_Diff_VLC_1,
    VC1_VLC_Mot_Vector_Diff_VLC_2,
    VC1_VLC_Mot_Vector_Diff_VLC_3,
};

static IMG_UINT8 Interlaced1RefMVTable[] = {
    VC1_VLC_One_Field_Ref_Ilace_MV_0,
    VC1_VLC_One_Field_Ref_Ilace_MV_1,
    VC1_VLC_One_Field_Ref_Ilace_MV_2,
    VC1_VLC_One_Field_Ref_Ilace_MV_3,
};

static IMG_UINT8 MVTable2RefIlace[] = {
    VC1_VLC_Two_Field_Ref_Ilace_MV_0,
    VC1_VLC_Two_Field_Ref_Ilace_MV_1,
    VC1_VLC_Two_Field_Ref_Ilace_MV_2,
    VC1_VLC_Two_Field_Ref_Ilace_MV_3,
    VC1_VLC_Two_Field_Ref_Ilace_MV_4,
    VC1_VLC_Two_Field_Ref_Ilace_MV_5,
    VC1_VLC_Two_Field_Ref_Ilace_MV_6,
    VC1_VLC_Two_Field_Ref_Ilace_MV_7,
};

static IMG_UINT8 IntraTablePQIndexLT9[] = {
    VC1_VLC_High_Rate_Intra_VLC,
    VC1_VLC_High_Mot_Intra_VLC,
    VC1_VLC_Mid_Rate_Intra_VLC,
};

static IMG_UINT8 InterTablePQIndexLT9[] = {
    VC1_VLC_High_Rate_Inter_VLC,
    VC1_VLC_High_Mot_Inter_VLC,
    VC1_VLC_Mid_Rate_Inter_VLC,
};

static IMG_UINT8 IntraTablePQIndexGT8[] = {
    VC1_VLC_Low_Mot_Intra_VLC,
    VC1_VLC_High_Mot_Intra_VLC,
    VC1_VLC_Mid_Rate_Intra_VLC,
};

static IMG_UINT8 InterTablePQIndexGT8[] = {
    VC1_VLC_Low_Mot_Inter_VLC,
    VC1_VLC_High_Mot_Inter_VLC,
    VC1_VLC_Mid_Rate_Inter_VLC,
};

/* TODO: Make tables const, don't polute namespace */
extern IMG_UINT16        gaui16vc1VlcTableData[];
extern const IMG_UINT16    gui16vc1VlcTableSize;
extern IMG_UINT16        gaui16vc1VlcIndexData[VLC_INDEX_TABLE_SIZE][3];
extern const IMG_UINT8    gui8vc1VlcIndexSize;


static IMG_UINT16       gaui16Inverse[] = {256, 128, 85, 64, 51, 43, 37, 32};    /* figure 66 */
static IMG_BOOL         gDMVRANGE_ExtHorizontal_RemapTable[] = {0, 1, 0, 1};
static IMG_BOOL         gDMVRANGE_ExtVertical_RemapTable[] = {0, 0, 1, 1};
static IMG_BYTE         gBFRACTION_DenRemapTable[] = {2, 3, 3, 4, 4, 5, 5, 5, 5, 6, 6, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 255, 255};
static IMG_BYTE         gBFRACTION_NumRemapTable[] = {1, 1, 2, 1, 3, 1, 2, 3, 4, 1, 5, 1, 2, 3, 4, 5, 6, 1, 3, 5, 7, 255, 255};


#define INIT_CONTEXT_VC1    context_VC1_p ctx = (context_VC1_p) obj_context->format_data;

#define SURFACE(id)    ((object_surface_p) object_heap_lookup( &ctx->obj_context->driver_data->surface_heap, id ))


static void pnw_VC1_QueryConfigAttributes(
    VAProfile profile,
    VAEntrypoint entrypoint,
    VAConfigAttrib *attrib_list,
    int num_attribs)
{
    int i;
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "pnw_VC1_QueryConfigAttributes\n");

    for (i = 0; i < num_attribs; i++) {
        switch (attrib_list[i].type) {
        case VAConfigAttribMaxPictureWidth:
            if ((entrypoint == VAEntrypointVLD) &&
                (profile == VAProfileVC1Advanced))
                attrib_list[i].value = HW_SUPPORTED_MAX_PICTURE_WIDTH_VC1;
            else
                attrib_list[i].value = VA_ATTRIB_NOT_SUPPORTED;
            break;
        case VAConfigAttribMaxPictureHeight:
            if ((entrypoint == VAEntrypointVLD) &&
                (profile == VAProfileVC1Advanced))
                attrib_list[i].value = HW_SUPPORTED_MAX_PICTURE_HEIGHT_VC1;
            else
                attrib_list[i].value = VA_ATTRIB_NOT_SUPPORTED;
            break;
        default:
            break;
        }
    }

}

static VAStatus pnw_VC1_ValidateConfig(
    object_config_p obj_config)
{
    int i;
    /* Check all attributes */
    for (i = 0; i < obj_config->attrib_count; i++) {
        switch (obj_config->attrib_list[i].type) {
        case VAConfigAttribRTFormat:
            /* Ignore */
            break;

        default:
            return VA_STATUS_ERROR_ATTR_NOT_SUPPORTED;
        }
    }

    return VA_STATUS_SUCCESS;
}


static void psb__VC1_pack_vlc_tables(uint16_t *vlc_packed_data,
                                     uint16_t *gaui16vc1VlcTableData,
                                     int gui16vc1VlcTableSize)
{
    int i, j;
    /************************************************************************************/
    /* Pack the VLC tables into 32-bit format ready for DMA into 15-bit wide RAM.        */
    /************************************************************************************/
    for (i = 0; i < gui16vc1VlcTableSize; i++) {
        j = i * 3;
        vlc_packed_data[i] = 0;
        /* opcode 14:12 *//* width 11:9 *//* symbol 8:0 */
        vlc_packed_data[i] = ((gaui16vc1VlcTableData[j + 0]) << 12) |
                             ((gaui16vc1VlcTableData[j + 1]) << 9)  |
                             (gaui16vc1VlcTableData[j + 2]);
    }
}

static void psb__VC1_pack_index_table_info(uint32_t *packed_index_table,
        uint16_t index_data[VLC_INDEX_TABLE_SIZE][3])
{
    uint32_t  start = 0, end = 0, length = 0, opcode = 0, width = 0;
    int i;

    for (i = 0; i < VLC_INDEX_TABLE_SIZE; i++) {
        start = index_data[i][2];
        if (i < (VLC_INDEX_TABLE_SIZE - 1)) { //Make sure we don't exceed the bound
            end = index_data[i+1][2];
        } else {
            end = start + 500;
        }
        length = end - start;
        width = index_data[i][1];
        opcode = index_data[i][0];

        drv_debug_msg(VIDEO_DEBUG_GENERAL, "packed_index_table[%02d]->start = %08x length = %08x (%d)\n", i, start * 2, length * 2, length * 2);

        packed_index_table[i] = opcode;
        packed_index_table[i] <<= 3;
        packed_index_table[i] |= width;
        packed_index_table[i] <<= 9;
        packed_index_table[i] |= length;
        packed_index_table[i] <<= 16;
        packed_index_table[i] |= start;
    }
}

static VAStatus psb__VC1_check_legal_picture(object_context_p obj_context, object_config_p obj_config)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    CHECK_CONTEXT(obj_context);

    CHECK_CONFIG(obj_config);

    /* MSVDX decode capability for VC-1:
     *     SP@ML
     *     MP@HL
     *     AP@L3
     *
     * Refer to Table 253 (Limitations of profiles and levels) of SMPTE-421M
     */
    switch (obj_config->profile) {
    case VAProfileVC1Simple:
        if ((obj_context->picture_width <= 0) || (obj_context->picture_width > 352)
            || (obj_context->picture_height <= 0) || (obj_context->picture_height > 288)) {
            vaStatus = VA_STATUS_ERROR_RESOLUTION_NOT_SUPPORTED;
        }
        break;

    case VAProfileVC1Main:
        if ((obj_context->picture_width <= 0) || (obj_context->picture_width > 1920)
            || (obj_context->picture_height <= 0) || (obj_context->picture_height > 1088)) {
            vaStatus = VA_STATUS_ERROR_RESOLUTION_NOT_SUPPORTED;
        }
        break;

    case VAProfileVC1Advanced:
        if ((obj_context->picture_width <= 0) || (obj_context->picture_width > 2048)
            || (obj_context->picture_height <= 0) || (obj_context->picture_height > 2048)) {
            vaStatus = VA_STATUS_ERROR_RESOLUTION_NOT_SUPPORTED;
        }
        break;

    default:
        vaStatus = VA_STATUS_ERROR_UNSUPPORTED_PROFILE;
        break;
    }

    return vaStatus;
}

static void pnw_VC1_DestroyContext(object_context_p obj_context);
static void psb__VC1_process_slice_data(context_DEC_p dec_ctx, VASliceParameterBufferBase *vld_slice_param);
static void psb__VC1_end_slice(context_DEC_p dec_ctx);
static void psb__VC1_begin_slice(context_DEC_p dec_ctx, VASliceParameterBufferBase *vld_slice_param);
static VAStatus pnw_VC1_process_buffer(context_DEC_p dec_ctx, object_buffer_p buffer);

static VAStatus pnw_VC1_CreateContext(
    object_context_p obj_context,
    object_config_p obj_config)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    context_VC1_p ctx;
    /* Validate flag */
    /* Validate picture dimensions */
    vaStatus = psb__VC1_check_legal_picture(obj_context, obj_config);
    if (VA_STATUS_SUCCESS != vaStatus) {
        drv_debug_msg(VIDEO_DEBUG_ERROR, "Warning: got invalid picture, but still let go\n");
        /* DEBUG_FAILURE;
        return vaStatus; */
        vaStatus = VA_STATUS_SUCCESS;
    }

    ctx = (context_VC1_p) malloc(sizeof(struct context_VC1_s));
    CHECK_ALLOCATION(ctx);

    memset(ctx, 0, sizeof(struct context_VC1_s));
    obj_context->format_data = (void*) ctx;
    ctx->obj_context = obj_context;
    ctx->pic_params = NULL;

    ctx->dec_ctx.begin_slice = psb__VC1_begin_slice;
    ctx->dec_ctx.process_slice = psb__VC1_process_slice_data;
    ctx->dec_ctx.end_slice = psb__VC1_end_slice;
    ctx->dec_ctx.process_buffer = pnw_VC1_process_buffer;

    switch (obj_config->profile) {
    case VAProfileVC1Simple:
        ctx->profile = WMF_PROFILE_SIMPLE;
        break;

    case VAProfileVC1Main:
        ctx->profile = WMF_PROFILE_MAIN;
        break;

    case VAProfileVC1Advanced:
        ctx->profile = WMF_PROFILE_ADVANCED;
        break;

    default:
        ASSERT(0 == 1);
        vaStatus = VA_STATUS_ERROR_UNKNOWN;
    }

    // TODO

    if (vaStatus == VA_STATUS_SUCCESS) {
        vaStatus = psb_buffer_create(obj_context->driver_data,
                                     PRELOAD_BUFFER_SIZE,
                                     psb_bt_vpu_only,
                                     &ctx->preload_buffer);
        DEBUG_FAILURE;
    }
    ctx->dec_ctx.preload_buffer = &ctx->preload_buffer;

    if (vaStatus == VA_STATUS_SUCCESS) {
        unsigned char *preload;
        if (0 ==  psb_buffer_map(&ctx->preload_buffer, &preload)) {
            memset(preload, 0, PRELOAD_BUFFER_SIZE);
            psb_buffer_unmap(&ctx->preload_buffer);
        } else {
            vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
            DEBUG_FAILURE;
        }
    }

    if (vaStatus == VA_STATUS_SUCCESS) {
        vaStatus = psb_buffer_create(obj_context->driver_data,
                                     AUXMSB_BUFFER_SIZE,
                                     psb_bt_vpu_only,
                                     &ctx->aux_msb_buffer);
        DEBUG_FAILURE;
    }

    if (vaStatus == VA_STATUS_SUCCESS) {
        vaStatus = psb_buffer_create(obj_context->driver_data,
                                     512*1024,
                                     psb_bt_vpu_only,
                                     &ctx->aux_line_buffer);
        DEBUG_FAILURE;
    }

    if (vaStatus == VA_STATUS_SUCCESS) {
#ifdef VC1_Header_Parser_HW
            vaStatus = psb_buffer_create(obj_context->driver_data,
                                         0xa000 * 3,  //0x8800
                                         psb_bt_vpu_only,
                                         &ctx->bitplane_hw_buffer);
            DEBUG_FAILURE;
#else
            vaStatus = psb_buffer_create(obj_context->driver_data,
                                         0x8000,
                                         psb_bt_vpu_only,
                                         &ctx->bitplane_hw_buffer);
            DEBUG_FAILURE;
#endif

    }

    if (vaStatus == VA_STATUS_SUCCESS) {
        vaStatus = psb_buffer_create(obj_context->driver_data,
                                     (gui16vc1VlcTableSize * sizeof(IMG_UINT16) + 0xfff) & ~0xfff,
                                     psb_bt_cpu_vpu,
                                     &ctx->vlc_packed_table);
        DEBUG_FAILURE;
    }
    if (vaStatus == VA_STATUS_SUCCESS) {
        uint16_t *vlc_packed_data_address;
        if (0 ==  psb_buffer_map(&ctx->vlc_packed_table, (unsigned char **)&vlc_packed_data_address)) {
            psb__VC1_pack_vlc_tables(vlc_packed_data_address, gaui16vc1VlcTableData, gui16vc1VlcTableSize);
            psb_buffer_unmap(&ctx->vlc_packed_table);
            psb__VC1_pack_index_table_info(ctx->vlc_packed_index_table, gaui16vc1VlcIndexData);
        } else {
            vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
            DEBUG_FAILURE;
        }
    }

    if (vaStatus == VA_STATUS_SUCCESS) {
        vaStatus = vld_dec_CreateContext(&ctx->dec_ctx, obj_context);
        DEBUG_FAILURE;
    }

    if (vaStatus != VA_STATUS_SUCCESS) {
        pnw_VC1_DestroyContext(obj_context);
    }

    return vaStatus;
}

static void pnw_VC1_DestroyContext(
    object_context_p obj_context)
{
    INIT_CONTEXT_VC1
    int i;

    vld_dec_DestroyContext(&ctx->dec_ctx);

    psb_buffer_destroy(&ctx->vlc_packed_table);
    psb_buffer_destroy(&ctx->aux_msb_buffer);
    psb_buffer_destroy(&ctx->aux_line_buffer);
    psb_buffer_destroy(&ctx->preload_buffer);
    psb_buffer_destroy(&ctx->bitplane_hw_buffer);

    if (ctx->pic_params) {
        free(ctx->pic_params);
        ctx->pic_params = NULL;
    }

    free(obj_context->format_data);
    obj_context->format_data = NULL;
}

static uint32_t psb__vc1_get_izz_scan_index(context_VC1_p ctx)
{
    if (ctx->pic_params->picture_fields.bits.frame_coding_mode == VC1_FCM_FRMI) {
        // P_PICTURE_ADV_FRAME_INTERLACE
        return 3;
    }
    if (PIC_TYPE_IS_INTRA(ctx->pic_params->picture_fields.bits.picture_type)) {
        // I-picture tables
        return 4;
    } else {
        /* Assume P frame */
        if ((ctx->profile == WMF_PROFILE_SIMPLE) ||
            (ctx->profile == WMF_PROFILE_MAIN)) {
            // P-picture Simple/Main tables
            return 0;
        } else { /* Advanced profile */
            if (ctx->pic_params->picture_fields.bits.frame_coding_mode == VC1_FCM_P) {
                // P-picture Advanced Progressive tables
                return 1;
            } else { /* Interlaced Field */
                // P-picture Advanced Field Interlaced tables
                return 2;
            }
        }
    }
}


//#define psb__trace_message(...)

#define P(x)    psb__trace_message("PARAMS: " #x "\t= %d\n", p->x)
static void psb__VC1_trace_pic_params(VAPictureParameterBufferVC1 *p)
{
#define P0(x)   psb__trace_message("PARAMS: " #x "\t= %d\n", p->sequence_fields.bits.x)
    P0(interlace);
    P0(syncmarker);
    P0(overlap);

    P(coded_width);
    P(coded_height);

#define P2(x)   psb__trace_message("PARAMS: " #x "\t= %d\n", p->picture_fields.bits.x)
    /* picture_fields */
    P2(picture_type);
    P2(frame_coding_mode);
    P2(top_field_first);
    P2(is_first_field);
    P2(intensity_compensation);

#define P1(x)   psb__trace_message("PARAMS: " #x "\t= %d\n", p->entrypoint_fields.bits.x)
    P1(closed_entry);
    P1(broken_link);
    P1(loopfilter);

    P(conditional_overlap_flag);
    P(fast_uvmc_flag);

#define P3(x)   psb__trace_message("PARAMS: " #x "\t= %d\n", p->range_mapping_fields.bits.x)
    /* range_mapping_fields */
    P3(luma_flag);
    P3(luma);
    P3(chroma_flag);
    P3(chroma);

    P(b_picture_fraction);
    P(cbp_table);
    P(mb_mode_table);
    P(range_reduction_frame);
    P(rounding_control);
    P(post_processing);
    P(picture_resolution_index);
    P(luma_scale);
    P(luma_shift);

    P(raw_coding.value);
    P(bitplane_present.value);

#define P4(x)   psb__trace_message("PARAMS: " #x "\t= %d\n", p->reference_fields.bits.x)
    P4(reference_distance_flag);
    P4(reference_distance);
    P4(num_reference_pictures);
    P4(reference_field_pic_indicator);

#define P5(x)   psb__trace_message("PARAMS: " #x "\t= %d\n", p->mv_fields.bits.x)
    P5(mv_mode);
    P5(mv_mode2);

    P5(mv_table);
    P5(two_mv_block_pattern_table);
    P5(four_mv_switch);
    P5(four_mv_block_pattern_table);
    P5(extended_mv_flag);
    P5(extended_mv_range);
    P5(extended_dmv_flag);
    P5(extended_dmv_range);

#define P6(x)   psb__trace_message("PARAMS: " #x "\t= %d\n", p->pic_quantizer_fields.bits.x)

    P6(dquant);
    P6(quantizer);
    P6(half_qp);
    P6(pic_quantizer_scale);
    P6(pic_quantizer_type);
    P6(dq_frame);
    P6(dq_profile);
    P6(dq_sb_edge);
    P6(dq_db_edge);
    P6(dq_binary_level);
    P6(alt_pic_quantizer);

#define P7(x)   psb__trace_message("PARAMS: " #x "\t= %d\n", p->transform_fields.bits.x)

    P7(variable_sized_transform_flag);
    P7(mb_level_transform_type_flag);
    P7(frame_level_transform_type);
    P7(transform_ac_codingset_idx1);
    P7(transform_ac_codingset_idx2);
    P7(intra_transform_dc_table);
}


static VAStatus psb__VC1_process_picture_param(context_VC1_p ctx, object_buffer_p obj_buffer)
{
    VAStatus vaStatus;
    VAPictureParameterBufferVC1 *pic_params;
    IMG_UINT8   ui8LumaScale1 = 0, ui8LumaShift1 = 0, ui8LumaScale2 = 0, ui8LumaShift2 = 0;
    object_surface_p obj_surface = ctx->obj_context->current_render_target;

    ASSERT(obj_buffer->type == VAPictureParameterBufferType);
    ASSERT(obj_buffer->num_elements == 1);
    ASSERT(obj_buffer->size == sizeof(VAPictureParameterBufferVC1));

    CHECK_INVALID_PARAM((obj_buffer->num_elements != 1) ||
        (obj_buffer->size != sizeof(VAPictureParameterBufferVC1)));

    /* Transfer ownership of VAPictureParameterBufferVC1 data */
    pic_params = (VAPictureParameterBufferVC1 *) obj_buffer->buffer_data;
    if (ctx->pic_params) {
        free(ctx->pic_params);
    }
    ctx->pic_params = pic_params;
    obj_buffer->buffer_data = NULL;
    obj_buffer->size = 0;

    if (psb_video_trace_fp && (psb_video_trace_level & VABUF_TRACE))
        psb__VC1_trace_pic_params(pic_params);

    if (pic_params->pic_quantizer_fields.bits.quantizer == 0) {
        /* Non uniform quantizer indicates PQINDEX > 8 */
        ctx->pqindex_gt8 = (pic_params->pic_quantizer_fields.bits.pic_quantizer_type == 0);
    } else {
        /* PQUANT (pic_quantizer_scale) == PQINDEX */
        ctx->pqindex_gt8 = (pic_params->pic_quantizer_fields.bits.pic_quantizer_scale > 8);
    }

    /*
     * We decode to ctx->decoded_surface This is inloop target
     * the out of loop decoded picture is stored in ctx->obj_context->current_render_target
     */
    if (pic_params->inloop_decoded_picture == VA_INVALID_SURFACE) {
        /* No out of loop deblocking */
        ctx->decoded_surface = ctx->obj_context->current_render_target;
    } else {
        ctx->decoded_surface = SURFACE(pic_params->inloop_decoded_picture);
        CHECK_SURFACE(ctx->decoded_surface);
    }

    //SET_SURFACE_INFO_picture_coding_type(ctx->decoded_surface->psb_surface, pic_params->picture_fields.bits.frame_coding_mode);
    SET_SURFACE_INFO_picture_coding_type(ctx->obj_context->current_render_target->psb_surface, pic_params->picture_fields.bits.frame_coding_mode);
    ctx->forward_ref_fcm =  pic_params->picture_fields.bits.frame_coding_mode;
    ctx->backward_ref_fcm =  pic_params->picture_fields.bits.frame_coding_mode;

    /* Lookup surfaces for backward/forward references */
    ctx->forward_ref_surface = NULL;
    ctx->backward_ref_surface = NULL;
    if (pic_params->forward_reference_picture != VA_INVALID_SURFACE) {
        ctx->forward_ref_surface = SURFACE(pic_params->forward_reference_picture);
    }
    if (pic_params->backward_reference_picture != VA_INVALID_SURFACE) {
        ctx->backward_ref_surface = SURFACE(pic_params->backward_reference_picture);
    }

    if (ctx->forward_ref_surface)
        ctx->forward_ref_fcm = GET_SURFACE_INFO_picture_coding_type(ctx->forward_ref_surface->psb_surface);

    if (ctx->backward_ref_surface)
        ctx->backward_ref_fcm = GET_SURFACE_INFO_picture_coding_type(ctx->backward_ref_surface->psb_surface);

#if 0
    if (NULL == ctx->forward_ref_surface) {
        /* for mmu fault protection */
        ctx->forward_ref_surface = ctx->decoded_surface;
    }
    if (NULL == ctx->backward_ref_surface) {
        /* for mmu fault protection */
        ctx->backward_ref_surface = ctx->decoded_surface;
    }
#endif

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Target ref = %08x ID = %08x\n",  ctx->obj_context->current_render_target->psb_surface,  ctx->obj_context->current_render_target->surface_id);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Decoded ref = %08x ID = %08x\n", ctx->decoded_surface->psb_surface, pic_params->inloop_decoded_picture);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Forward ref = %08x ID = %08x\n", ctx->forward_ref_surface ? ctx->forward_ref_surface->psb_surface : 0, pic_params->forward_reference_picture);
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "Backwrd ref = %08x ID = %08x\n", ctx->backward_ref_surface ? ctx->backward_ref_surface->psb_surface : 0, pic_params->backward_reference_picture);

    // NOTE: coded_width and coded_height do not have to be an exact multiple of MBs

    ctx->display_picture_width = pic_params->coded_width;
    ctx->display_picture_height = pic_params->coded_height;
    ctx->picture_width_mb = PIXELS_TO_MB(ctx->display_picture_width);
    ctx->picture_height_mb = PIXELS_TO_MB(ctx->display_picture_height);
    ctx->coded_picture_width = ctx->picture_width_mb * 16;
    ctx->coded_picture_height = ctx->picture_height_mb * 16;
    if ((WMF_PROFILE_ADVANCED == ctx->profile) && (VC1_FCM_FLDI == pic_params->picture_fields.bits.frame_coding_mode)) {
        ctx->picture_height_mb /= 2;
        ctx->coded_picture_height = ctx->picture_height_mb * 16 * 2;
    }

    if (obj_surface->share_info) {
        obj_surface->share_info->coded_width = ctx->coded_picture_width;
        obj_surface->share_info->coded_height = ctx->coded_picture_height;
    }

    ctx->size_mb = ctx->picture_width_mb * ctx->picture_height_mb;

    uint32_t colocated_size = ((ctx->size_mb + 1) * 2 + 128) * VC1_MB_PARAM_STRIDE;
    //uint32_t colocated_size = (ctx->size_mb + 1) * 2 * VC1_MB_PARAM_STRIDE + 0x2000;

    vaStatus = vld_dec_allocate_colocated_buffer(&ctx->dec_ctx, ctx->decoded_surface, colocated_size);
    vaStatus = vld_dec_allocate_colocated_buffer(&ctx->dec_ctx, ctx->obj_context->current_render_target, colocated_size);

    CHECK_VASTATUS();

    /* TODO: Store top_field_first and frame_coding_mode (or even all of pic_params) for the current frame
     * so that it can be referenced when the same frame is used as reference frame
    */

    if (ctx->profile != WMF_PROFILE_ADVANCED) {
        /* Simple and Main profiles always use progressive pictures*/
        pic_params->picture_fields.bits.frame_coding_mode = VC1_FCM_P;
    }

    if ((pic_params->picture_fields.bits.frame_coding_mode == VC1_FCM_P) || (pic_params->picture_fields.bits.frame_coding_mode == VC1_FCM_FRMI)) {
        pic_params->picture_fields.bits.top_field_first = 1;
    }

    ctx->bitplane_present = 0;
    switch (pic_params->picture_fields.bits.picture_type) {
    case WMF_PTYPE_I:
    case WMF_PTYPE_BI:
        ctx->bitplane_present |= (pic_params->bitplane_present.flags.bp_overflags && !pic_params->raw_coding.flags.overflags) ? 0x04 : 0;
        ctx->bitplane_present |= (pic_params->bitplane_present.flags.bp_ac_pred && !pic_params->raw_coding.flags.ac_pred) ? 0x02 : 0;
        ctx->bitplane_present |= (pic_params->bitplane_present.flags.bp_field_tx && !pic_params->raw_coding.flags.field_tx) ? 0x01 : 0;
        break;

    case WMF_PTYPE_P:
        ctx->bitplane_present |= (pic_params->bitplane_present.flags.bp_mv_type_mb && !pic_params->raw_coding.flags.mv_type_mb) ? 0x04 : 0;
        ctx->bitplane_present |= (pic_params->bitplane_present.flags.bp_skip_mb && !pic_params->raw_coding.flags.skip_mb) ? 0x02 : 0;
        break;

    case WMF_PTYPE_B: /* B picture */
        ctx->bitplane_present |= (pic_params->bitplane_present.flags.bp_forward_mb && !pic_params->raw_coding.flags.forward_mb) ? 0x04 : 0;
        ctx->bitplane_present |= (pic_params->bitplane_present.flags.bp_skip_mb && !pic_params->raw_coding.flags.skip_mb) ? 0x02 : 0;
        ctx->bitplane_present |= (pic_params->bitplane_present.flags.bp_direct_mb && !pic_params->raw_coding.flags.direct_mb) ? 0x01 : 0;
        break;

    default:
        break;
    }
    drv_debug_msg(VIDEO_DEBUG_GENERAL, "bitplane_present_flag = %02x raw_coding_flag = %02x bitplane_present = %02x\n",
                             pic_params->bitplane_present.value, pic_params->raw_coding.value, ctx->bitplane_present);

    if (pic_params->reference_fields.bits.reference_distance_flag == 0) {
        pic_params->reference_fields.bits.reference_distance = 0;
    }

    /* conditional_overlap_flag is not always defined, but MSVDX expects it to be set in those cases anyway */
    if (ctx->profile == WMF_PROFILE_ADVANCED) {
        if (pic_params->sequence_fields.bits.overlap == FALSE) {
            ctx->condover = 0; /* No overlap smoothing */
        } else if (pic_params->pic_quantizer_fields.bits.pic_quantizer_scale < 9) {
            ctx->condover = pic_params->conditional_overlap_flag;
        } else {
            ctx->condover = 2;
        }
    } else {
        if ((pic_params->picture_fields.bits.picture_type == WMF_PTYPE_B) || (pic_params->sequence_fields.bits.overlap == FALSE) || (pic_params->pic_quantizer_fields.bits.pic_quantizer_scale < 9)) {
            ctx->condover = 0; /* No overlap smoothing */
        } else {
            ctx->condover = 2;
        }
    }

    /************************** Calculate the IZZ scan index ****************************/
    ctx->scan_index = psb__vc1_get_izz_scan_index(ctx);
    /************************************************************************************/

    /**************************** Calculate MVMODE and MVMODE2 **************************/
    ctx->mv_mode = pic_params->mv_fields.bits.mv_mode;
    if (ctx->mv_mode == WMF_MVMODE_INTENSITY_COMPENSATION) {
        ctx->mv_mode = pic_params->mv_fields.bits.mv_mode2;
    }

    /* Neither MVMODE nor MVMODE2 are signaled at the picture level for interlaced frame pictures,
       but MVMODE can be determine for P pictures depending on the value of MV4SWITCH, and for B
       pictures it is by default 1MV mode. */
    if ((pic_params->picture_fields.bits.frame_coding_mode == VC1_FCM_FRMI) && PIC_TYPE_IS_INTER(pic_params->picture_fields.bits.picture_type)) {
        if ((pic_params->picture_fields.bits.picture_type == WMF_PTYPE_P) && (pic_params->mv_fields.bits.four_mv_switch == 1)) {
            ctx->mv_mode = WMF_MVMODE_MIXED_MV;
            pic_params->mv_fields.bits.mv_mode = WMF_MVMODE_MIXED_MV;
        } else {
            ctx->mv_mode = WMF_MVMODE_1MV;
            pic_params->mv_fields.bits.mv_mode = WMF_MVMODE_1MV;
        }
    }
    /************************************************************************************/


    /******************************** Calculate HALFPEL *********************************/
    if ((ctx->mv_mode == WMF_MVMODE_1MV) || (ctx->mv_mode == WMF_MVMODE_MIXED_MV)) {
        ctx->half_pel = 0;
    } else {
        ctx->half_pel = 1;
    }
    /************************************************************************************/

    /* TODO: Are we using the correct size for this ? */
    ctx->pull_back_x = COMPUTE_PULLBACK(pic_params->coded_width);
    ctx->pull_back_y = COMPUTE_PULLBACK(pic_params->coded_height);

    if (pic_params->mv_fields.bits.extended_dmv_flag == 1) {
        ctx->extend_x = gDMVRANGE_ExtHorizontal_RemapTable[pic_params->mv_fields.bits.extended_dmv_range];
        ctx->extend_y = gDMVRANGE_ExtVertical_RemapTable[pic_params->mv_fields.bits.extended_dmv_range];
    } else {
        ctx->extend_x = IMG_FALSE;
        ctx->extend_y = IMG_FALSE;
    }

    /* B interlaced field picture and ...?? */
    ctx->ui32ScaleFactor = 0;
    ctx->i8FwrdRefFrmDist = 0;
    ctx->i8BckwrdRefFrmDist = 0;
    if (pic_params->picture_fields.bits.picture_type == WMF_PTYPE_B) {
        IMG_UINT32 ui32BFractionDen;
        IMG_UINT32 ui32BFractionNum;

        IMG_UINT32 ui32FrameReciprocal;

        if (pic_params->b_picture_fraction > (sizeof(gBFRACTION_DenRemapTable) / sizeof(IMG_BYTE) - 1))
            pic_params->b_picture_fraction = sizeof(gBFRACTION_DenRemapTable) / sizeof(IMG_BYTE) - 1;

        ui32BFractionDen = gBFRACTION_DenRemapTable[pic_params->b_picture_fraction];
        ui32BFractionNum = gBFRACTION_NumRemapTable[pic_params->b_picture_fraction];

        if (ui32BFractionDen > (sizeof(gaui16Inverse) / sizeof(IMG_UINT16)))
            ui32BFractionDen = sizeof(gaui16Inverse) / sizeof(IMG_UINT16);

        if (ui32BFractionDen == 0) {
            drv_debug_msg(VIDEO_DEBUG_ERROR, "Invalid ui32BFractionDen value %d\n", ui32BFractionDen);
            ui32BFractionDen = 1;
        }

        ui32FrameReciprocal = gaui16Inverse[ui32BFractionDen - 1];
        ctx->ui32ScaleFactor    = ui32BFractionNum * ui32FrameReciprocal;

        if (pic_params->picture_fields.bits.frame_coding_mode == VC1_FCM_FLDI) {
            ctx->i8FwrdRefFrmDist   = (IMG_INT8)((ctx->ui32ScaleFactor * pic_params->reference_fields.bits.reference_distance) >> 8);     /* 10.4.6.2 */
            ctx->i8BckwrdRefFrmDist = pic_params->reference_fields.bits.reference_distance - ctx->i8FwrdRefFrmDist - 1;

            if (ctx->i8BckwrdRefFrmDist < 0) {
                ctx->i8BckwrdRefFrmDist = 0;
            }
        }
    }

    /* Compute the mode config parameter */
    /*
       MODE_CONFIG[1:0] =
        VC-1 intensity compensation flag, derived from MVMODE = Intensity compensation, and INTCOMPFIELD
        00 – No intensity compensation
        01 – Intensity compensation for top field
        10 – Intensity compensation for bottom field
        11 – Intensity compensation for the frame

       MODE_CONFIG[3:2] =
        VC-1 reference range scaling, derived from RANGERED, RANGEREDFRM for current frame and reference frame.
        00 – No scaling
        01 – Scale down
        10 – Scale up
        11 – No scaling
     */

    /****************************** INTENSITY COMPENSATION ******************************/
    /* For each NEW reference frame, rotate IC history */
    if (PIC_TYPE_IS_REF(pic_params->picture_fields.bits.picture_type) &&
        pic_params->picture_fields.bits.is_first_field &&
        (pic_params->picture_fields.bits.frame_coding_mode == VC1_FCM_FLDI)) {
        /*
           This is the first field picture of a new frame, so move the IC params for both field
           pictures of the last frame (from position [1][0] for the first field and position [1][1] for
           the second field to positions [0][0] and [0][1] respectevely).
        */
        memcpy(&ctx->sICparams[0][0], &ctx->sICparams[1][0], sizeof(IC_PARAM));
        memcpy(&ctx->sICparams[0][1], &ctx->sICparams[1][1], sizeof(IC_PARAM));

        memset(&ctx->sICparams[1][0], 0, sizeof(IC_PARAM));
        memset(&ctx->sICparams[1][1], 0, sizeof(IC_PARAM));
    }

    if (pic_params->picture_fields.bits.picture_type == WMF_PTYPE_P) {
        ctx->ui8CurrLumaScale1 = 0;
        ctx->ui8CurrLumaShift1 = 0;
        ctx->ui8CurrLumaScale2 = 0;
        ctx->ui8CurrLumaShift2 = 0;

        if (pic_params->picture_fields.bits.frame_coding_mode != VC1_FCM_FRMI) {
            if (pic_params->picture_fields.bits.intensity_compensation) {
                /* Intensity compensation of reference */
                if (pic_params->picture_fields.bits.frame_coding_mode != VC1_FCM_FLDI) { // progressive picture
                    ctx->mode_config = 0x3;

                    ui8LumaScale1 = pic_params->luma_scale & 0x3F;
                    ui8LumaShift1 = pic_params->luma_shift & 0x3F;

                    if (ui8LumaScale1 != 0 || ui8LumaShift1 != 0) {
                        ctx->ui8CurrLumaScale1 = ui8LumaScale1;
                        ctx->ui8CurrLumaShift1 = ui8LumaShift1;
                    }
                } else { // field interlaced picture
                    // top field
                    ui8LumaScale1 = pic_params->luma_scale & 0x3F;
                    ui8LumaShift1 = pic_params->luma_shift & 0x3F;

                    // bottom field
                    ui8LumaScale2 = ui8LumaScale1; /* TODO: How to keep track of top/bottom field intensity comp? */
                    ui8LumaShift2 = ui8LumaShift1; /* TODO: How to keep track of top/bottom field intensity comp? */

                    /* Check what fields undergo intensity compensation */
                    ctx->ui8IntCompField = 0;
                    if (ui8LumaScale1 != 0 || ui8LumaShift1 != 0) {
                        ctx->ui8IntCompField = 1;
                    }
                    if (ui8LumaScale2 != 0 || ui8LumaShift2 != 0) {
                        ctx->ui8IntCompField |= 2;
                    }

                    switch (ctx->ui8IntCompField) {
                    case 0: /* none */
                        ctx->mode_config = 0x0;
                        break;

                    case 1: /* top */
                        ctx->mode_config = 0x1;

                        // IC parameters for top field
                        ctx->ui8CurrLumaScale1 = ui8LumaScale1;
                        ctx->ui8CurrLumaShift1 = ui8LumaShift1;
                        break;

                    case 2: /* bottom */
                        ctx->mode_config = 0x2;

                        // IC parameters for bottom field
                        ctx->ui8CurrLumaScale2 = ui8LumaScale2;
                        ctx->ui8CurrLumaShift2 = ui8LumaShift2;
                        break;

                    case 3: /* both */
                        ctx->mode_config = 0x3;

                        // IC parameters for top field
                        ctx->ui8CurrLumaScale1 = ui8LumaScale1;
                        ctx->ui8CurrLumaShift1 = ui8LumaShift1;

                        // IC parameters for bottom field
                        ctx->ui8CurrLumaScale2 = ui8LumaScale2;
                        ctx->ui8CurrLumaShift2 = ui8LumaShift2;
                        break;
                    }
                }
            } else {
                ctx->mode_config = 0;
            }
        } else { // interlaced frame P picture
            if (pic_params->picture_fields.bits.intensity_compensation) { /* iINSO */
                ctx->mode_config = 0x3;   // intensity compensate whole frame
            } else {
                ctx->mode_config = 0;
            }
        }
    } else if (PIC_TYPE_IS_INTRA(pic_params->picture_fields.bits.picture_type)) {
        ctx->mode_config = 0;
    }

    /*
        10.3.8 Intensity Compensation:
        If intensity compensation is performed on a reference field, then after decoding the field,
        the post-compensated pixel values shall be retained and shall be used when decoding the next
        field. If the next field indicates that the field that was intensity compensated by the
        previous field is to have intensity compensation performed again then the post-compensated
        field shall be used. Therefore, when a reference field has intensity compensation performed
        twice, the result of the first intensity compensation operation shall be used as input
        for the second intensity compensation.
    */
    /*
        Don't forget point 9.1.1.4 in VC1 Spec:

        If the current frame, coded as two interlace field pictures, contains at least one P or B
        field, and if this P or B field uses one or both field in another frame as a reference, where
        the reference frame was also coded as a interlace field pictue, then the TFF of the current
        frame and reference frame shall be the same.
    */
    if ((pic_params->picture_fields.bits.picture_type == WMF_PTYPE_P) && (pic_params->picture_fields.bits.frame_coding_mode == VC1_FCM_FLDI)) {
        if (pic_params->picture_fields.bits.top_field_first) { // top field first
            if (!pic_params->picture_fields.bits.is_first_field) { // this is the second field picture (and bottom)
                if (ctx->ui8IntCompField & 0x1) {
                    /* The second and bottom field picture of the current frame
                       intensity compensates the top field of the current frame. */
                    ctx->sICparams[1][0].ui8LumaScale1 = ui8LumaScale1;
                    ctx->sICparams[1][0].ui8LumaShift1 = ui8LumaShift1;
                    ctx->sICparams[1][0].ui8IC1 = 1;
                }
                if (ctx->ui8IntCompField & 0x2) {
                    /* The second and bottom field picture of the current frame
                       intensity compensates the bottom field of the previous frame. */
                    ctx->sICparams[0][1].ui8LumaScale2 = ui8LumaScale2;
                    ctx->sICparams[0][1].ui8LumaShift2 = ui8LumaShift2;
                    ctx->sICparams[0][1].ui8IC2 = 2;
                }
            } else { // first field picture (and top)
                if (ctx->ui8IntCompField & 0x1) {
                    /* The first and top field picture of the current frame
                       intensity compensates the top field of the previous frame. */
                    ctx->sICparams[0][0].ui8LumaScale2 = ui8LumaScale1;
                    ctx->sICparams[0][0].ui8LumaShift2 = ui8LumaShift1;
                    ctx->sICparams[0][0].ui8IC2 = 1;
                }
                if (ctx->ui8IntCompField & 0x2) {
                    /* The first and top field picture of the current frame
                       intensity compensates the bottom field of the previous frame. */
                    ctx->sICparams[0][1].ui8LumaScale1 = ui8LumaScale2;
                    ctx->sICparams[0][1].ui8LumaShift1 = ui8LumaShift2;
                    ctx->sICparams[0][1].ui8IC1 = 2;
                }
            }
        } else { // bottom field first
            if (!pic_params->picture_fields.bits.is_first_field) { // this is the second field picture (and top)
                if (ctx->ui8IntCompField & 0x2) {
                    /* The second and top field picture of the current frame
                       intensity compensates the bottom field of the current frame. */
                    ctx->sICparams[1][1].ui8LumaScale1 = ui8LumaScale2;
                    ctx->sICparams[1][1].ui8LumaShift1 = ui8LumaShift2;
                    ctx->sICparams[1][1].ui8IC1 = 2;
                }
                if (ctx->ui8IntCompField & 0x1) {
                    /* The second and top field picture of the current frame
                       intensity compensates the top field of the previous frame. */
                    ctx->sICparams[0][0].ui8LumaScale2 = ui8LumaScale1;
                    ctx->sICparams[0][0].ui8LumaShift2 = ui8LumaShift1;
                    ctx->sICparams[0][0].ui8IC2 = 1;
                }
            } else { // first field picture (and bottom)
                if (ctx->ui8IntCompField & 0x1) {
                    /* The first and bottom field picture of the current frame
                       intensity compensates the top field of the previous frame. */
                    ctx->sICparams[0][0].ui8LumaScale1 = ui8LumaScale1;
                    ctx->sICparams[0][0].ui8LumaShift1 = ui8LumaShift1;
                    ctx->sICparams[0][0].ui8IC1 = 1;
                }
                if (ctx->ui8IntCompField & 0x2) {
                    /* The first and bottom field picture of the current frame
                       intensity compensates the bottom field of the previous frame. */
                    ctx->sICparams[0][1].ui8LumaScale2 = ui8LumaScale2;
                    ctx->sICparams[0][1].ui8LumaShift2 = ui8LumaShift2;
                    ctx->sICparams[0][1].ui8IC2 = 2;
                }
            }
        }
    }
    /************************************************************************************/

    /********************************* RANGE REDUCTION **********************************/
    /* Determine the difference between the range reduction of current and reference picture */
    if (ctx->profile == WMF_PROFILE_MAIN) {
        /* Range Reduction is only enabled for Main Profile */
        /* The RANGEREDFRM values from the reference pictures are;
            psVLDContext->bRef0RangeRed
            psVLDContext->bRef1RangeRed */

        switch (pic_params->picture_fields.bits.picture_type) {
        case WMF_PTYPE_I:
        case WMF_PTYPE_BI:
            /* no reference picture scaling */
            break;

        case WMF_PTYPE_P: /* P picture */
            /*
                8.3.4.11 also need to scale the previously reconstructed anchor frame prior to using it for MC if:
                - RANGEREDFRM == 1 and ref RANGEREDFRM flag is not signalled scale down previous reconstructed frame.
                - RANGEREDFRM == 0 and ref RANGEREDFRM flag is set scale up previous reconstructed frame.
             */
            if (ctx->pic_params->range_reduction_frame && !ctx->bRef0RangeRed) {
                ctx->mode_config |= (0x1 << 2); // scale down previous reconstructed frame
            } else if (!ctx->pic_params->range_reduction_frame && ctx->bRef0RangeRed) {
                ctx->mode_config |= (0x2 << 2); // scale up previous reconstructed frame
            } else { /* neither or both are set */
                ctx->mode_config |= (0x0 << 2); // no scaling of reference
            }
            break;

        case WMF_PTYPE_B: /* B picture */
            /*
               8.4.4.14 RANGEREDFRM shall be the same as for the temporally subsequent anchor frame (display order)
               If this is set then the current decoded frame shall be scalled up similar to P frame.
               Scaling for the temporally (display order) preceeding frame will be applied as for P frames
             */
            if (ctx->bRef0RangeRed && !ctx->bRef1RangeRed) {
                ctx->mode_config |= (0x1 << 2);
            } else if (!ctx->bRef0RangeRed && ctx->bRef1RangeRed) {
                ctx->mode_config |= (0x2 << 2);
            } else { /* neither or both are set */
                ctx->mode_config |= (0x0 << 2); // no scaling of reference
            }
            break;

        default:
            break;
        }
    } else {
        ctx->mode_config |= (0x0 << 2);
    }
    /************************************************************************************/

    /********************************** Slice structure *********************************/
    if (VC1_FCM_FLDI == pic_params->picture_fields.bits.frame_coding_mode) {
        if ((pic_params->picture_fields.bits.top_field_first && pic_params->picture_fields.bits.is_first_field) ||
            (!pic_params->picture_fields.bits.top_field_first && !pic_params->picture_fields.bits.is_first_field)) {
            // Top field
            ctx->slice_field_type = 0;
            ctx->bottom_field = 0;
        } else {
            // Bottom field
            ctx->slice_field_type = 1;
            ctx->bottom_field = 1;
        }
    } else {
        // progressive or interlaced frame
        ctx->slice_field_type = 2;
        ctx->bottom_field = 1;
    }
    /************************************************************************************/

    /************************* FCM for the reference pictures ***************************/
    ctx->ui8FCM_Ref0Pic = ctx->pic_params->picture_fields.bits.frame_coding_mode;
    ctx->ui8FCM_Ref1Pic = ctx->pic_params->picture_fields.bits.frame_coding_mode;
    if (ctx->obj_context->frame_count == 0)
        ctx->ui8FCM_Ref2Pic = ctx->pic_params->picture_fields.bits.frame_coding_mode;

    if (PIC_TYPE_IS_REF(pic_params->picture_fields.bits.picture_type) ||
        ((pic_params->picture_fields.bits.picture_type == WMF_PTYPE_B) &&       /* The second B field picture in an             */
         (ctx->pic_params->picture_fields.bits.frame_coding_mode == VC1_FCM_FLDI) &&    /* interlaced field coded frame shall   */
         !pic_params->picture_fields.bits.is_first_field)) {            /* reference the first field picture.   */
        if (ctx->pic_params->picture_fields.bits.frame_coding_mode == VC1_FCM_FLDI && !pic_params->picture_fields.bits.is_first_field) {
            /* The current picture is the second field of the frame, then the previous field picture
               is in the same frame. Therefore the FCM of the first field is the same as the FCM of the
            current field and the first field will be reference 0. */
            ctx->ui8FCM_Ref0Pic = ctx->pic_params->picture_fields.bits.frame_coding_mode;
        } else if (ctx->pic_params->picture_fields.bits.frame_coding_mode == VC1_FCM_FLDI && pic_params->picture_fields.bits.is_first_field) {
            /* The current picture is the first field of the frame, then the previous field picture
               is in a different frame and will be reference 1. */
            ctx->ui8FCM_Ref1Pic = ctx->ui8FCM_Ref2Pic;
        } else { // progresive or interlaced frame picture
            ctx->ui8FCM_Ref1Pic = ctx->ui8FCM_Ref2Pic;
        }
    }
    /************************************************************************************/

    /************************* TFF for the reference pictures ***************************/
    if (ctx->obj_context->frame_count == 0) {
        ctx->bTFF_FwRefFrm = pic_params->picture_fields.bits.top_field_first;
        ctx->bTFF_BwRefFrm = pic_params->picture_fields.bits.top_field_first;
    }
    if (PIC_TYPE_IS_REF(pic_params->picture_fields.bits.picture_type) &&
        ((ctx->pic_params->picture_fields.bits.frame_coding_mode != VC1_FCM_FLDI) ||
         pic_params->picture_fields.bits.is_first_field)) {
        ctx->bTFF_FwRefFrm = ctx->bTFF_BwRefFrm;
    }
    /************************************************************************************/

    psb_CheckInterlaceRotate(ctx->obj_context, (void *)ctx->pic_params);

    return VA_STATUS_SUCCESS;
}

static VAStatus psb__VC1_process_bitplane(context_VC1_p ctx, object_buffer_p obj_buffer)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    ASSERT(obj_buffer->type == VABitPlaneBufferType);
    ASSERT(ctx->pic_params);
    /* We need to have data in the bitplane buffer */
    CHECK_INVALID_PARAM((NULL == obj_buffer->psb_buffer) || (0 == obj_buffer->size));

    ctx->bitplane_buffer = obj_buffer->psb_buffer;
    ctx->has_bitplane = TRUE;
    return vaStatus;
}

/*
 * This function extracts the information about a given table from the index of VLC tables.
 */
static void psb__VC1_extract_table_info(context_VC1_p ctx, sTableData *psInfo, int idx)
{
    IMG_UINT32 tmp;

    if (idx >= VLC_INDEX_TABLE_SIZE)
        idx = VLC_INDEX_TABLE_SIZE - 1;

    tmp = ctx->vlc_packed_index_table[idx];
    psInfo->aui16StartLocation      = (IMG_UINT16)(tmp & 0xffff);
    psInfo->aui16VLCTableLength     = (IMG_UINT16)((tmp >> 16) & 0x1ff);
    psInfo->aui16InitialWidth       = (IMG_UINT16)((tmp >> 25) & 0x7);
    psInfo->aui16InitialOpcode      = (IMG_UINT16)((tmp >> 28) & 0x3);
}

/*
 * This function selects the VLD tables from the picture layer parameters.
 */
static void psb__VC1_write_VLC_tables(context_VC1_p ctx)
{
    VAPictureParameterBufferVC1 *pic_params = ctx->pic_params;
    IMG_UINT16          ui16Table = 0, ui16IntraTable = 0, ui16InterTable = 0, aui16Table[3];
    IMG_UINT32          i, ui32TableNum = 0;

    /* select the required table from the n different types
            A - vc1DEC_I_Picture_CBPCY_VLC            (1)       ¬
            B - vc1DEC_P_Picture_CBPCY_VLC_N          (4)       |
            C - vc1DEC_Interlaced_CBPCY_N             (8)       |
            D - vc1DEC_FourMV_Pattern_N               (4)       |
            E - vc1DEC_INTERLACE_2_MVP_Pattern_N      (4)       |
            F - vc1DEC_Mot_Vector_Diff_VLC_N          (4)       |   MB Layer
            G - vc1DEC_One_Field_Ref_Ilace_MV_N       (4)       |
            H - vc1DEC_Two_Field_Ref_Ilace_MV_N       (8)       |
            I - vc1DEC_Mixed_MV_MB_N                  (8)       |
            J - vc1DEC_One_MV_MB_N                    (8)       |
            K - vc1DEC_INTERLACE_4MV_MB_N             (4)       |
            L - vc1DEC_INTERLACE_Non_4MV_MB_N         (4)       |
            M - vc1DEC_X_Rate_TTMB                    (3)       -
            N - vc1DEC_X_Rate_TTBLK                   (3)       ¬
            O - vc1DEC_X_Rate_SUBBLKPAT               (3)       |
            P - vc1DEC_X_X_Inter_VLC                  (4)       |   Block Layer
            Q - vc1DEC_X_X_Intra_VLC                  (4)       |
            R - vc1DEC_X_Mot_Luminance_DC_Diff_VLC    (2)       |
            S - vc1DEC_X_Mot_Chroma_DC_Diff_VLC       (2)       -

            X - vc1DEC_Code_3x2_2x3_tiles             (1)   NOT USED    */

    /*!
    ***********************************************************************************
    @ Table A,B,C  VLC CBPCY Tables

        [VC1]   7.1.3.1 Coded Block Pattern (CBPCY) (Variable size)[I, P,B]

            CBPCY is a variable-sized syntax element that shall be present in all
            I and BI picture macroblocks, and may be present in P and B picture
            macroblocks. In P and B pictures, CBPCY shall be decoded using
            the VLC table specified by the CBPTAB syntax element as described in
            section 7.1.1.39. The CBP tables for P and B pictures are listed in
            section 11.6.


        [VC1]   9.1.3.2 Coded Block Pattern (CBPCY) (Variable size)[I, P,B]

            Table 102: ICBPTAB code-table

            A  vc1DEC_I_Picture_CBPCY_VLC            (1)
            B  vc1DEC_P_Picture_CBPCY_VLC_N          (4)
            C  vc1DEC_Interlaced_CBPCY_N             (8)

    ***********************************************************************************/

    if ((!pic_params->sequence_fields.bits.interlace) || (pic_params->picture_fields.bits.frame_coding_mode == VC1_FCM_P)) {
        if (PIC_TYPE_IS_INTRA(pic_params->picture_fields.bits.picture_type)) {
            ui16Table = VC1_VLC_I_Picture_CBPCY_VLC;
        } else if (PIC_TYPE_IS_INTER(pic_params->picture_fields.bits.picture_type)) {
            psb__bounds_check(pic_params->cbp_table, 4);
            ui16Table = CBPCYTableProg[pic_params->cbp_table];
        }
    } else { /* Interlaced */
        if (PIC_TYPE_IS_INTRA(pic_params->picture_fields.bits.picture_type)) {
            ui16Table = VC1_VLC_I_Picture_CBPCY_VLC;
        } else {
            psb__bounds_check(pic_params->cbp_table, 8);
            ui16Table = CBPCYTableInterlaced[pic_params->cbp_table];  /* LUT */
        }
    }

    psb__VC1_extract_table_info(ctx, &ctx->sTableInfo[ui32TableNum], ui16Table);
    ui32TableNum++;

    /*!
    ************************************************************
    @ Table D   VLC 4MV Pattern

    [VC1]       Table 104: 4MVBP code-table

            Tables 116-119

            D vc1DEC_FourMV_Pattern_N               (4)
    ************************************************************/
    psb__bounds_check(pic_params->mv_fields.bits.four_mv_block_pattern_table, 4);
    ui16Table = FourMVTable[pic_params->mv_fields.bits.four_mv_block_pattern_table];

    psb__VC1_extract_table_info(ctx, &ctx->sTableInfo[ui32TableNum], ui16Table);
    ui32TableNum++;

    /*!
    ************************************************************************************
    @ Table E  VLC 2MVBP Tables


        Table 103: 2MVBP code-table

        for Tables 120-123

        E vc1DEC_INTERLACE_2_MVP_Pattern_N      (4)
    ***********************************************************************************/
    psb__bounds_check(pic_params->mv_fields.bits.two_mv_block_pattern_table, 4);
    ui16Table = Interlace2MVTable[pic_params->mv_fields.bits.two_mv_block_pattern_table];

    psb__VC1_extract_table_info(ctx, &ctx->sTableInfo[ui32TableNum], ui16Table);
    ui32TableNum++;

    /*!
    ************************************************************************************
    @ Table F,G,H  VLC MV Tables

        [VC1]   MVDATA                                  Variable size   vlclbf  7.1.3.8

        7.1.3.8 Motion Vector Data (MVDATA)(Variable size)[P]

        MVDATA is a variable sized syntax element that may be present in P picture
        macroblocks. This syntax element decodes to the motion vector(s) for the
        macroblock. The table used to decode this syntax element is specified by the
        MVTAB syntax element in the picture layer as specified in section 7.1.1.38.

        F   vc1DEC_Mot_Vector_Diff_VLC_N          (4)

        [VC1]   9.1.1.34        INTERLACE Motion Vector Table (IMVTAB) (2 or 3 bits)

        Table 100:      IMVTAB code-table for P INTERLACE field picture with NUMREF = 0,
                            and for P/B INTERLACE frame pictures

            IMVTAB      Motion Vector Table
            00          1-Reference Table 0
            01          1-Reference Table 1
            10          1-Reference Table 2
            11          1-Reference Table 3

        Table 101:      IMVTAB code-table for P INTERLACE field pictures with NUMREF = 1,
                            and for B INTERLACE field pictures

            IMVTAB      Motion Vector Table
            000         2-Reference Table 0
            001         2-Reference Table 1
            010         2-Reference Table 2
            011         2-Reference Table 3
            100         2-Reference Table 4
            101         2-Reference Table 5
            110         2-Reference Table 6
            111         2-Reference Table 7

        G   vc1DEC_One_Field_Ref_Ilace_MV_N       (4)
        H   vc1DEC_Two_Field_Ref_Ilace_MV_N       (8)

    ***********************************************************************************/
    if ((!pic_params->sequence_fields.bits.interlace) || (pic_params->picture_fields.bits.frame_coding_mode == VC1_FCM_P)) {
        psb__bounds_check(pic_params->mv_fields.bits.mv_table, 4);
        ui16Table = ProgressiveMVTable[pic_params->mv_fields.bits.mv_table];
    } else {
        if (
            (
                PIC_TYPE_IS_INTER(pic_params->picture_fields.bits.picture_type) &&
                (pic_params->picture_fields.bits.frame_coding_mode == VC1_FCM_FRMI)
            )
            ||
            (
                (pic_params->picture_fields.bits.picture_type == WMF_PTYPE_P) &&
                (pic_params->picture_fields.bits.frame_coding_mode == VC1_FCM_FLDI) &&
                (pic_params->reference_fields.bits.num_reference_pictures == 0)
            )
        ) {
            /* One field */
            psb__bounds_check(pic_params->mv_fields.bits.mv_table, 4);
            ui16Table = Interlaced1RefMVTable[pic_params->mv_fields.bits.mv_table];
        } else { /*if (((FCM == VC1_FCM_FLDI) && (NUMREF == 0) && (PTYPE == WMF_PTYPE_P)) || ((PTYPE == WMF_PTYPE_B) && (FCM == VC1_FCM_FLDI))) */
            /* two field */
            psb__bounds_check(pic_params->mv_fields.bits.mv_table, 8);
            ui16Table = MVTable2RefIlace[pic_params->mv_fields.bits.mv_table];   /* LUT */
        }
    }

    psb__VC1_extract_table_info(ctx, &ctx->sTableInfo[ui32TableNum], ui16Table);
    ui32TableNum++;

    /*!
    ************************************************************************************
    @ Table I,J,K,L  VLC MBMODE Tables

        I vc1DEC_Mixed_MV_MB_N                  (8)
        J vc1DEC_One_MV_MB_N                    (8)
        K vc1DEC_INTERLACE_4MV_MB_N             (4)
        L vc1DEC_INTERLACE_Non_4MV_MB_N         (4)
    ***********************************************************************************/
    ui16Table = 0;
    if (pic_params->sequence_fields.bits.interlace && (pic_params->picture_fields.bits.frame_coding_mode > VC1_FCM_P)) {
        if (PIC_TYPE_IS_INTER(pic_params->picture_fields.bits.picture_type)) {
            if (pic_params->picture_fields.bits.frame_coding_mode == VC1_FCM_FLDI) {
                psb__bounds_check(pic_params->mb_mode_table, 8);
                /* 9.1.1.33 use MBMODETAB and MVMODE to select field interlaced tables */
                ui16Table = MBMODETableFLDI[pic_params->mb_mode_table][(pic_params->mv_fields.bits.mv_mode == WMF_MVMODE_MIXED_MV) ? 1 : 0];
            } else if (pic_params->picture_fields.bits.frame_coding_mode == VC1_FCM_FRMI) {
                psb__bounds_check(pic_params->mb_mode_table, 4);
                /* 9.1.1.33 use MBMODETAB and MV4SWITCH to select frame interlaced tables */
                ui16Table = MBMODETableFRMI[pic_params->mb_mode_table][(pic_params->mv_fields.bits.four_mv_switch) ? 0 : 1];
            }
        }
    }

    psb__VC1_extract_table_info(ctx, &ctx->sTableInfo[ui32TableNum], ui16Table);
    ui32TableNum++;

    /*!
    ************************************************************************************
    @ Table M,N,O  VLC PQUANT Tables

    [WMV9]      3.2.2.10        MB-level Transform Type (TTMB)(Variable size)[P,B]
    [WMV9]      3.2.3.15        Block-level Transform Type (TTBLK)(Variable size)[inter]

    [WMV9]      3.2.3.16        Transform sub-block pattern (SUBBLKPAT)(Variable size)[inter]

            M vc1DEC_X_Rate_TTMB                    (3)
            N vc1DEC_X_Rate_TTBLK                   (3)
            O vc1DEC_X_Rate_SUBBLKPAT               (3)

        TTBLK and TTMB P and B Pictures only

    ***********************************************************************************/
    aui16Table[0] = 0;
    aui16Table[1] = 0;
    aui16Table[2] = 0;

    if (pic_params->pic_quantizer_fields.bits.pic_quantizer_scale <= 4) {            /* high rate */
        aui16Table[2]   = VC1_VLC_High_Rate_SUBBLKPAT;
        aui16Table[1]   = VC1_VLC_High_Rate_TTBLK;
        aui16Table[0]   = VC1_VLC_High_Rate_TTMB;
    } else if (pic_params->pic_quantizer_fields.bits.pic_quantizer_scale <= 12) {     /* med rate */
        aui16Table[2]   = VC1_VLC_Medium_Rate_SUBBLKPAT;
        aui16Table[1]   = VC1_VLC_Medium_Rate_TTBLK;
        aui16Table[0]   = VC1_VLC_Medium_Rate_TTMB;
    } else {                                                     /* low rate */
        aui16Table[2]   = VC1_VLC_Low_Rate_SUBBLKPAT;
        aui16Table[1]   = VC1_VLC_Low_Rate_TTBLK;
        aui16Table[0]   = VC1_VLC_Low_Rate_TTMB;
    }

    for (i = ui32TableNum; i < ui32TableNum + 3; i++) {
        psb__VC1_extract_table_info(ctx, &ctx->sTableInfo[i], aui16Table[i-ui32TableNum]);
    }

    ui32TableNum = ui32TableNum + 3;

    {
        /*!
        ***********************************************************************************************
        Inter Coded Blocks

        Table 54: Index/Coding Set Correspondence for PQINDEX <= 7
            Y, Cb and Cr blocks

            Index       Table
            0           High Rate Inter
            1           High Motion Inter
            2           Mid Rate Inter

        Table 55: Index/Coding Set Correspondence for PQINDEX > 7
            Y, Cb and Cr blocks

            Index       Table
            0           Low Motion Inter
            1           High Motion Inter
            2           Mid Rate Inter

        ----------------------------------------------------------------------------------
        Intra Blocks

        8 AC Coeff Coding Sets:
            4 x INTRA, 4 x INTER

            Y use Intra, CrCb use Inter

        Table 38: Coding Set Correspondence for PQINDEX <= 7

            Y blocks                                            Cb and Cr blocks
            Index       Table                                   Index   Table
            0           High Rate Intra                 0               High Rate Inter
            1           High Motion Intra               1               High Motion Inter
            2           Mid Rate Intra                  2               Mid Rate Inter

        Table 39: Coding Set Correspondence for PQINDEX > 7

            Y blocks                                            Cb and Cr blocks
            Index       Table                                   Index   Table
            0           Low Motion Intra                0               Low Motion Inter
            1           High Motion Intra               1               High Motion Inter
            2           Mid Rate Intra                  2               Mid Rate Inter

        The value decoded from the DCTACFRM2 syntax element shall be used
        as the coding set index for Y blocks and the value decoded from the
        DCTACFRM syntax element shall be used as the coding set
        index for Cb and Cr blocks.

            P vc1DEC_X_X_Inter_VLC
            Q vc1DEC_X_X_Intra_VLC


        for I pictures  TRANSACFRM specifies the Inter Coding set
                        TRANSACFRM2 specifies the Intra Coding set

        for P pictures  TRANSACFRM specifies Inter and Intra Coding set


        ***************************************************************************************************/
        IMG_UINT32  ui32IntraCodingSetIndex = PIC_TYPE_IS_INTRA(pic_params->picture_fields.bits.picture_type)
                                              ?  pic_params->transform_fields.bits.transform_ac_codingset_idx2
                                              :  pic_params->transform_fields.bits.transform_ac_codingset_idx1;

        IMG_UINT32  ui32InterCodingSetIndex = pic_params->transform_fields.bits.transform_ac_codingset_idx1;

        /* For PQINDEX < 9 the uniform quantizer should be used, as indicated by PQUANTIZER == 1 */
        if (!ctx->pqindex_gt8) {
            ui16IntraTable = IntraTablePQIndexLT9[ui32IntraCodingSetIndex];
            ui16InterTable = InterTablePQIndexLT9[ui32InterCodingSetIndex];
        } else {
            ui16IntraTable = IntraTablePQIndexGT8[ui32IntraCodingSetIndex];
            ui16InterTable = InterTablePQIndexGT8[ui32InterCodingSetIndex];
        }

        psb__VC1_extract_table_info(ctx, &ctx->sTableInfo[ui32TableNum], ui16IntraTable);
        ui32TableNum++;

        psb__VC1_extract_table_info(ctx, &ctx->sTableInfo[ui32TableNum], ui16InterTable);
        ui32TableNum++;
    }

    /*!
    ************************************************************************************
    @ Table R & S  VLC TRANSDCTAB Tables

                R vc1DEC_X_Mot_Luminance_DC_Diff_VLC    (2)
                S vc1DEC_X_Mot_Chroma_DC_Diff_VLC       (2)

    [VC1]       8.1.1.2 Intra Transform DC Table
            TRANSDCTAB is a one-bit syntax element that shall specify which of two
            tables is used to decode the Transform DC coefficients in intra-coded blocks.
            If TRANSDCTAB = 0, then the low motion table of Section 11.7 shall be used.
            If TRANSDCTAB = 1, then the high motion table of Section 11.7 shall be used.

    [VC1]       8.1.1.2 Intra Transform DC Table
            TRANSDCTAB is a one-bit syntax element that shall specify which of two
            tables is used to decode the Transform DC coefficients in intra-coded blocks.
            If TRANSDCTAB = 0, then the low motion table of Section 11.7 shall be used.
            If TRANSDCTAB = 1, then the high motion table of Section 11.7 shall be used.

    ***********************************************************************************/
    if (pic_params->transform_fields.bits.intra_transform_dc_table == 0) {
        /* low motion */

        /* VC1_VLC_Low_Mot_Luminance_DC_Diff_VLC */
        ui16IntraTable = VC1_VLC_Low_Mot_Luminance_DC_Diff_VLC;

        /* VC1_VLC_Low_Mot_Chroma_DC_Diff_VLC */
        ui16InterTable = VC1_VLC_Low_Mot_Chroma_DC_Diff_VLC;
    } else { /* TRANSDCTAB == 1 */
        /* high motion */
        /* VC1_VLC_High_Mot_Luminance_DC_Diff_VLC */
        ui16IntraTable = VC1_VLC_High_Mot_Luminance_DC_Diff_VLC;

        /* VC1_VLC_High_Mot_Chroma_DC_Diff_VLC */
        ui16InterTable = VC1_VLC_High_Mot_Chroma_DC_Diff_VLC;
    }

    psb__VC1_extract_table_info(ctx, &ctx->sTableInfo[ui32TableNum], ui16IntraTable);
    ui32TableNum++;

    psb__VC1_extract_table_info(ctx, &ctx->sTableInfo[ui32TableNum], ui16InterTable);
    ui32TableNum++;

    /* at the end determine how many tables have been chosen
        this should be constant and equal 12 */
    ctx->ui32NumTables = ui32TableNum;
    ASSERT(ctx->ui32NumTables == 12);
}

static void psb__VC1_build_VLC_tables(context_VC1_p ctx)
{
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;
    unsigned int i;
    uint16_t RAM_location = 0;
    uint32_t reg_value;

    for (i = 0; i < ctx->ui32NumTables; i++) {
        if (RAM_location & 0x03) {
            /* Align */
            RAM_location += 4 - (RAM_location & 0x03);
        }
        ctx->sTableInfo[i].aui16RAMLocation = RAM_location;

        /* VLC Table */
        /* Write a LLDMA Cmd to transfer VLD Table data */
        psb_cmdbuf_dma_write_cmdbuf(cmdbuf, &ctx->vlc_packed_table,
                                      ctx->sTableInfo[i].aui16StartLocation * sizeof(IMG_UINT16), /* origin */
                                      ctx->sTableInfo[i].aui16VLCTableLength * sizeof(IMG_UINT16), /* size */
                                      RAM_location * sizeof(IMG_UINT32), /* destination */
                                      DMA_TYPE_VLC_TABLE);
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "table[%02d] start_loc = %08x RAM_location = %08x | %08x\n", i, ctx->sTableInfo[i].aui16StartLocation * sizeof(IMG_UINT16), RAM_location, RAM_location * sizeof(IMG_UINT32));
        RAM_location += ctx->sTableInfo[i].aui16VLCTableLength;
    }

    /* Write the vec registers with the index data for each of the tables */
    psb_cmdbuf_reg_start_block(cmdbuf, 0);

    reg_value = 0;
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR0, VLC_TABLE_ADDR0, ctx->sTableInfo[0].aui16RAMLocation);
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR0, VLC_TABLE_ADDR1, ctx->sTableInfo[1].aui16RAMLocation);
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR0), reg_value);

    reg_value = 0;
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR1, VLC_TABLE_ADDR2, ctx->sTableInfo[2].aui16RAMLocation);
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR1, VLC_TABLE_ADDR3, ctx->sTableInfo[3].aui16RAMLocation);
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR1), reg_value);

    reg_value = 0;
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR2, VLC_TABLE_ADDR4, ctx->sTableInfo[4].aui16RAMLocation);
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR2, VLC_TABLE_ADDR5, ctx->sTableInfo[5].aui16RAMLocation);
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR2), reg_value);

    reg_value = 0;
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR3, VLC_TABLE_ADDR6, ctx->sTableInfo[6].aui16RAMLocation);
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR3, VLC_TABLE_ADDR7, ctx->sTableInfo[7].aui16RAMLocation);
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR3), reg_value);

    reg_value = 0;
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR4, VLC_TABLE_ADDR8, ctx->sTableInfo[8].aui16RAMLocation);
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR4, VLC_TABLE_ADDR9, ctx->sTableInfo[9].aui16RAMLocation);
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR4), reg_value);

    reg_value = 0;
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR5, VLC_TABLE_ADDR10, ctx->sTableInfo[10].aui16RAMLocation);
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR5, VLC_TABLE_ADDR11, ctx->sTableInfo[11].aui16RAMLocation);
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC, CR_VEC_VLC_TABLE_ADDR5), reg_value);

    reg_value = 0;
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC, CR_VEC_VLC_TABLE_INITIAL_WIDTH0, VLC_TABLE_INITIAL_WIDTH0, ctx->sTableInfo[0].aui16InitialWidth);
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC, CR_VEC_VLC_TABLE_INITIAL_WIDTH0, VLC_TABLE_INITIAL_WIDTH1, ctx->sTableInfo[1].aui16InitialWidth);
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC, CR_VEC_VLC_TABLE_INITIAL_WIDTH0, VLC_TABLE_INITIAL_WIDTH2, ctx->sTableInfo[2].aui16InitialWidth);
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC, CR_VEC_VLC_TABLE_INITIAL_WIDTH0, VLC_TABLE_INITIAL_WIDTH3, ctx->sTableInfo[3].aui16InitialWidth);
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC, CR_VEC_VLC_TABLE_INITIAL_WIDTH0, VLC_TABLE_INITIAL_WIDTH4, ctx->sTableInfo[4].aui16InitialWidth);
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC, CR_VEC_VLC_TABLE_INITIAL_WIDTH0, VLC_TABLE_INITIAL_WIDTH5, ctx->sTableInfo[5].aui16InitialWidth);
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC, CR_VEC_VLC_TABLE_INITIAL_WIDTH0, VLC_TABLE_INITIAL_WIDTH6, ctx->sTableInfo[6].aui16InitialWidth);
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC, CR_VEC_VLC_TABLE_INITIAL_WIDTH0, VLC_TABLE_INITIAL_WIDTH7, ctx->sTableInfo[7].aui16InitialWidth);
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC, CR_VEC_VLC_TABLE_INITIAL_WIDTH0, VLC_TABLE_INITIAL_WIDTH8, ctx->sTableInfo[8].aui16InitialWidth);
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC, CR_VEC_VLC_TABLE_INITIAL_WIDTH0, VLC_TABLE_INITIAL_WIDTH9, ctx->sTableInfo[9].aui16InitialWidth);
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC, CR_VEC_VLC_TABLE_INITIAL_WIDTH0), reg_value);

    reg_value = 0;
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC, CR_VEC_VLC_TABLE_INITIAL_WIDTH1, VLC_TABLE_INITIAL_WIDTH10, ctx->sTableInfo[10].aui16InitialWidth);
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC, CR_VEC_VLC_TABLE_INITIAL_WIDTH1, VLC_TABLE_INITIAL_WIDTH11, ctx->sTableInfo[11].aui16InitialWidth);
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC, CR_VEC_VLC_TABLE_INITIAL_WIDTH1), reg_value);

    reg_value = 0;
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC, CR_VEC_VLC_TABLE_INITIAL_OPCODE0, VLC_TABLE_INITIAL_OPCODE0, ctx->sTableInfo[0].aui16InitialOpcode);
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC, CR_VEC_VLC_TABLE_INITIAL_OPCODE0, VLC_TABLE_INITIAL_OPCODE1, ctx->sTableInfo[1].aui16InitialOpcode);
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC, CR_VEC_VLC_TABLE_INITIAL_OPCODE0, VLC_TABLE_INITIAL_OPCODE2, ctx->sTableInfo[2].aui16InitialOpcode);
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC, CR_VEC_VLC_TABLE_INITIAL_OPCODE0, VLC_TABLE_INITIAL_OPCODE3, ctx->sTableInfo[3].aui16InitialOpcode);
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC, CR_VEC_VLC_TABLE_INITIAL_OPCODE0, VLC_TABLE_INITIAL_OPCODE4, ctx->sTableInfo[4].aui16InitialOpcode);
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC, CR_VEC_VLC_TABLE_INITIAL_OPCODE0, VLC_TABLE_INITIAL_OPCODE5, ctx->sTableInfo[5].aui16InitialOpcode);
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC, CR_VEC_VLC_TABLE_INITIAL_OPCODE0, VLC_TABLE_INITIAL_OPCODE6, ctx->sTableInfo[6].aui16InitialOpcode);
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC, CR_VEC_VLC_TABLE_INITIAL_OPCODE0, VLC_TABLE_INITIAL_OPCODE7, ctx->sTableInfo[7].aui16InitialOpcode);
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC, CR_VEC_VLC_TABLE_INITIAL_OPCODE0, VLC_TABLE_INITIAL_OPCODE8, ctx->sTableInfo[8].aui16InitialOpcode);
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC, CR_VEC_VLC_TABLE_INITIAL_OPCODE0, VLC_TABLE_INITIAL_OPCODE9, ctx->sTableInfo[9].aui16InitialOpcode);
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC, CR_VEC_VLC_TABLE_INITIAL_OPCODE0, VLC_TABLE_INITIAL_OPCODE10, ctx->sTableInfo[10].aui16InitialOpcode);
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC, CR_VEC_VLC_TABLE_INITIAL_OPCODE0, VLC_TABLE_INITIAL_OPCODE11, ctx->sTableInfo[11].aui16InitialOpcode);
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC, CR_VEC_VLC_TABLE_INITIAL_OPCODE0), reg_value);

    psb_cmdbuf_reg_end_block(cmdbuf);
}

static void psb__VC1_send_rendec_params(context_VC1_p ctx, VASliceParameterBufferVC1 *slice_param)
{
    VAPictureParameterBufferVC1 *pic_params = ctx->pic_params;
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;
    psb_surface_p deblock_surface = ctx->decoded_surface->psb_surface;
    psb_surface_p target_surface = ctx->obj_context->current_render_target->psb_surface;
    uint32_t cmd;
    IMG_UINT32    ui32MBParamMemOffset;
    IMG_UINT8     ui8PrevLumaScale = 0, ui8PrevLumaShift = 0;
    IMG_UINT8     ui8BackLumaScale = 0, ui8BackLumaShift = 0;
    IMG_UINT8     ui8PrevBotLumaShift = 0, ui8PrevBotLumaScale = 0;
    IMG_UINT8     ui8PrevIC = 0, ui8BackIC = 0, ui8PrevBotIC = 0;

    /* Align MB Parameter memory */
    ui32MBParamMemOffset  = ((pic_params->picture_fields.bits.frame_coding_mode == VC1_FCM_FLDI) && (!pic_params->picture_fields.bits.is_first_field)) ?
                            (ctx->size_mb * VC1_MB_PARAM_STRIDE) : 0;
    ui32MBParamMemOffset += 0x00000fff;
    ui32MBParamMemOffset &= 0xfffff000;

    /****************************** INTENSITY COMPENSATION ******************************/
    if (pic_params->picture_fields.bits.frame_coding_mode == VC1_FCM_FLDI) {
        if (pic_params->picture_fields.bits.picture_type == WMF_PTYPE_P) {
            if (pic_params->picture_fields.bits.top_field_first) { // top field first
                if (!pic_params->picture_fields.bits.is_first_field) { // this is the second field picture (and bottom)
                    if (ctx->sICparams[0][1].ui8IC1 == 2) {
                        /* The first and top field picture of the current frame
                           intensity compensates the bottom field of the previous frame. */
                        ui8PrevLumaScale = ctx->sICparams[0][1].ui8LumaScale1;
                        ui8PrevLumaShift = ctx->sICparams[0][1].ui8LumaShift1;
                        ui8PrevIC = 2;
                    }
                } else { // first field picture (and top)
                    if (ctx->sICparams[0][0].ui8IC1 == 1) {
                        /* The second and bottom field picture of the previous frame
                           intensity compensates the top field of the previous frame. */
                        ui8PrevLumaScale = ctx->sICparams[0][0].ui8LumaScale1;
                        ui8PrevLumaShift = ctx->sICparams[0][0].ui8LumaShift1;
                        ui8PrevIC = 1;
                    }
                }
            } else { // botom field first
                if (!pic_params->picture_fields.bits.is_first_field) { // this is the second field picture (and top)
                    if (ctx->sICparams[0][0].ui8IC1 == 1) {
                        /* The first and bottom field picture of the current frame
                           intensity compensates the top field of the previous frame. */
                        ui8PrevLumaScale = ctx->sICparams[0][0].ui8LumaScale1;
                        ui8PrevLumaShift = ctx->sICparams[0][0].ui8LumaShift1;
                        ui8PrevIC = 1;
                    }
                } else { // first field picture (and bottom)
                    if (ctx->sICparams[0][1].ui8IC1 == 2) {
                        /* The second and top field picture of the previous frame
                           intensity compensates the bottom field of the previous frame. */
                        ui8PrevLumaScale = ctx->sICparams[0][1].ui8LumaScale1;
                        ui8PrevLumaShift = ctx->sICparams[0][1].ui8LumaShift1;
                        ui8PrevIC = 2;
                    }
                }
            }
        } else if (pic_params->picture_fields.bits.picture_type == WMF_PTYPE_B) {
            /*
                First frame - second temporally closest reference frame to the B frame
                Second frame - first temporally closest reference frame to the B frame
            */
            if (pic_params->picture_fields.bits.top_field_first) { // top field first
                if (ctx->sICparams[0][0].ui8IC1 == 1) {
                    /* The second and bottom field of the first reference frame intensity
                       compensates the first and top field of the first reference frame. */
                    ui8PrevLumaScale = ctx->sICparams[0][0].ui8LumaScale1;
                    ui8PrevLumaShift = ctx->sICparams[0][0].ui8LumaShift1;
                    ui8PrevIC = 1;
                }
                if (ctx->sICparams[0][0].ui8IC2 == 1) {
                    /* The first and top field of the second reference frame intensity
                       compensates the first and top field of the first reference frame. */
                    ui8BackLumaScale = ctx->sICparams[0][0].ui8LumaScale2;
                    ui8BackLumaShift = ctx->sICparams[0][0].ui8LumaShift2;
                    ui8BackIC = 1;
                }
                if (ctx->sICparams[0][1].ui8IC2 == 2) {
                    /* The first and top field of the second reference frame intensity
                       compensates the second and bottom field of the first reference frame. */
                    ui8PrevBotLumaScale = ctx->sICparams[0][1].ui8LumaScale2;
                    ui8PrevBotLumaShift = ctx->sICparams[0][1].ui8LumaShift2;
                    ui8PrevBotIC = 2;
                }
            } else { // botom field first
                if (ctx->sICparams[0][1].ui8IC1 == 2) {
                    /* The second and top field of the first reference frame intensity
                       compensates the first and bottom field of the first reference frame. */
                    ui8BackLumaScale = ctx->sICparams[0][1].ui8LumaScale1;
                    ui8BackLumaShift = ctx->sICparams[0][1].ui8LumaShift1;
                    ui8BackIC = 2;
                }
                if (ctx->sICparams[0][1].ui8IC2 == 2) {
                    /* The first and bottom field of the second reference frame intensity
                       compensates the first and bottom field of the first reference frame. */
                    ui8PrevBotLumaScale = ctx->sICparams[0][1].ui8LumaScale2;
                    ui8PrevBotLumaShift = ctx->sICparams[0][1].ui8LumaShift2;
                    ui8PrevBotIC = 2;
                }
                if (ctx->sICparams[0][0].ui8IC1 == 1) {
                    /* The first and bottom field of the second reference frame intensity
                       compensates the second and top field of the first reference frame. */
                    ui8PrevLumaScale = ctx->sICparams[0][0].ui8LumaScale1;
                    ui8PrevLumaShift = ctx->sICparams[0][0].ui8LumaShift1;
                    ui8PrevIC = 1;
                }
            }
        }
    }
    /************************************************************************************/

    /* CHUNK: 1 - VC1SEQUENCE00 */
    psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_CMDS, DISPLAY_PICTURE_SIZE));
    *cmdbuf->rendec_chunk_start |= CMD_RENDEC_BLOCK_FLAG_VC1_CMD_PATCH;

    /* VC1SEQUENCE00    Command: Display Picture Size (sequence) */
    cmd = 0;
    /* TODO: Can "display size" and "coded size" be different? */
    REGIO_WRITE_FIELD(cmd, VC1_RENDEC_CMD, VC1SEQUENCE00, PICTURE_HEIGHT, (ctx->display_picture_height - 1)); /* display picture size - 1 */
    REGIO_WRITE_FIELD(cmd, VC1_RENDEC_CMD, VC1SEQUENCE00, PICTURE_WIDTH, (ctx->display_picture_width - 1));
    psb_cmdbuf_rendec_write(cmdbuf, cmd);

    /* VC1SEQUENCE00    Command: Coded Picture Size  (sequence) */
    cmd = 0;
    REGIO_WRITE_FIELD(cmd, VC1_RENDEC_CMD, VC1SEQUENCE00, PICTURE_HEIGHT, (ctx->coded_picture_height - 1)); /* coded picture size - 1 */
    REGIO_WRITE_FIELD(cmd, VC1_RENDEC_CMD, VC1SEQUENCE00, PICTURE_WIDTH, (ctx->coded_picture_width - 1));
    psb_cmdbuf_rendec_write(cmdbuf, cmd);

    /* VC1SEQUENCE01    Command: Operating Mode (sequence) */
    cmd = 0;
    REGIO_WRITE_FIELD(cmd, VC1_RENDEC_CMD, VC1SEQUENCE01, CHROMA_INTERLEAVED,   0); /* 0 = CbCr - MSVDX default */
    REGIO_WRITE_FIELD(cmd, VC1_RENDEC_CMD, VC1SEQUENCE01, ROW_STRIDE,           target_surface->stride_mode);
    REGIO_WRITE_FIELD(cmd, VC1_RENDEC_CMD, VC1SEQUENCE01, CODEC_MODE,           2); /* MODE_VC1 */
    REGIO_WRITE_FIELD(cmd, VC1_RENDEC_CMD, VC1SEQUENCE01, CODEC_PROFILE,        ctx->profile);
    REGIO_WRITE_FIELD(cmd, VC1_RENDEC_CMD, VC1SEQUENCE01, ASYNC_MODE,           0/*((pPicParams->bPicDeblocked & 0x02) ? 0:1)*/); // @TODO: async mode should be synchronous or pre-load for VC1
    REGIO_WRITE_FIELD(cmd, VC1_RENDEC_CMD, VC1SEQUENCE01, CHROMA_FORMAT,        1);
    REGIO_WRITE_FIELD(cmd, VC1_RENDEC_CMD, VC1SEQUENCE01, INTERLACED, ((pic_params->picture_fields.bits.frame_coding_mode & 0x02) >> 1));           /* if progressive, INTERLACE is always 0 */
    REGIO_WRITE_FIELD(cmd, VC1_RENDEC_CMD, VC1SEQUENCE01, VC1_OVERLAP,          pic_params->sequence_fields.bits.overlap);
#ifndef VC1_Header_Parser_HW
        REGIO_WRITE_FIELD(cmd, VC1_RENDEC_CMD, VC1SEQUENCE01, PIC_CONDOVER,         ctx->condover);
        REGIO_WRITE_FIELD(cmd, VC1_RENDEC_CMD, VC1SEQUENCE01, PIC_QUANT,            pic_params->pic_quantizer_fields.bits.pic_quantizer_scale);
#endif
    ctx->obj_context->operating_mode = cmd;
    psb_cmdbuf_rendec_write(cmdbuf, cmd);

    /* LUMA_RECONSTRUCTED_PICTURE_BASE_ADDRESSES                                    */
    psb_cmdbuf_rendec_write_address(cmdbuf, &target_surface->buf, target_surface->buf.buffer_ofs);

    /* CHROMA_RECONSTRUCTED_PICTURE_BASE_ADDRESSES                                  */
    psb_cmdbuf_rendec_write_address(cmdbuf, &target_surface->buf, target_surface->buf.buffer_ofs + target_surface->chroma_offset);

    /* Aux MSB buffer */
    psb_cmdbuf_rendec_write_address(cmdbuf, &ctx->aux_msb_buffer, 0);

    psb_cmdbuf_rendec_end(cmdbuf);

    /* CHUNK: 2 - VC1SLICE00 */
    psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_CMDS, MC_CACHE_CONFIGURATION));

    /* VC1SLICE00           Command: Cache Configuration (picture?) */
    cmd = 0;
    REGIO_WRITE_FIELD(cmd, VC1_RENDEC_CMD, VC1SLICE00, CONFIG_REF_OFFSET,  72);
    REGIO_WRITE_FIELD(cmd, VC1_RENDEC_CMD, VC1SLICE00, CONFIG_ROW_OFFSET,  4);
    psb_cmdbuf_rendec_write(cmdbuf, cmd);

    /* VC1SLICE01           Command: VC1 Intensity Compensation Parameter (picture or slice) */
    cmd = 0;
    REGIO_WRITE_FIELD(cmd, VC1_RENDEC_CMD, VC1SLICE01, VC1_LUMSHIFT2,  ctx->ui8CurrLumaShift2); /* INTERLACE field P pictures */
    REGIO_WRITE_FIELD(cmd, VC1_RENDEC_CMD, VC1SLICE01, VC1_LUMSCALE2,  ctx->ui8CurrLumaScale2); /* INTERLACE field P pictures */
    REGIO_WRITE_FIELD(cmd, VC1_RENDEC_CMD, VC1SLICE01, VC1_LUMSHIFT1,  ctx->ui8CurrLumaShift1);
    REGIO_WRITE_FIELD(cmd, VC1_RENDEC_CMD, VC1SLICE01, VC1_LUMSCALE1,  ctx->ui8CurrLumaScale1);
    psb_cmdbuf_rendec_write(cmdbuf, cmd);

    psb_cmdbuf_rendec_end(cmdbuf);

    vld_dec_setup_alternative_frame(ctx->obj_context);

    if (ctx->pic_params->picture_fields.bits.frame_coding_mode == VC1_FCM_P && CONTEXT_ROTATE(ctx->obj_context))
        deblock_surface = ctx->obj_context->current_render_target->out_loop_surface;

    /* CHUNK: 3 */
    psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_CMDS, VC1_LUMA_RANGE_MAPPING_BASE_ADDRESS));

    /* VC1 Luma Range Mapping Base Address */
    psb_cmdbuf_rendec_write_address(cmdbuf, &deblock_surface->buf, deblock_surface->buf.buffer_ofs);

    /* VC1 Chroma Range Mapping Base Address */
    psb_cmdbuf_rendec_write_address(cmdbuf, &deblock_surface->buf, deblock_surface->chroma_offset + deblock_surface->buf.buffer_ofs);

    /* VC1SLICE03       Range Map Control (current picture) */
    cmd = 0;
    REGIO_WRITE_FIELD(cmd, VC1_RENDEC_CMD, VC1SLICE03, RANGE_MAPUV_FLAG,  pic_params->range_mapping_fields.bits.chroma_flag /*RANGE_MAPUV_FLAG*/);
    REGIO_WRITE_FIELD(cmd, VC1_RENDEC_CMD, VC1SLICE03, RANGE_MAPUV,       pic_params->range_mapping_fields.bits.chroma);
    REGIO_WRITE_FIELD(cmd, VC1_RENDEC_CMD, VC1SLICE03, RANGE_MAPY_FLAG,   pic_params->range_mapping_fields.bits.luma_flag /*RANGE_MAPY_FLAG*/);
    REGIO_WRITE_FIELD(cmd, VC1_RENDEC_CMD, VC1SLICE03, RANGE_MAPY,        pic_params->range_mapping_fields.bits.luma);
    psb_cmdbuf_rendec_write(cmdbuf, cmd);

    /* Store VC1SLICE03 bits in lower bits of Range Mapping Base Address */
    /* VC1 Luma Range Mapping Base Address */
    RELOC(*ctx->dec_ctx.p_range_mapping_base0, /*cmd + */deblock_surface->buf.buffer_ofs, &deblock_surface->buf);
    RELOC(*ctx->dec_ctx.p_range_mapping_base1, deblock_surface->buf.buffer_ofs + deblock_surface->chroma_offset, &deblock_surface->buf);

    *ctx->dec_ctx.cmd_params |= cmd;
    /* VC1 Intensity Compensation Backward/Previous     */
    /*
            3.3.10 VC1 Intensity Compensation Backward/Previous:
            The parameters applied in VC1 Intensity Compensation Parameters are the Intensity Compensation
            applied to forward prediction. In the case of Interlaced P field pictures, the second field can
            be Intensity Compensated relative to the first P field picture. If this is done, when decoding
            B pictures the first field backward MV reference to P picture needs to be Intensity Compensated
            with VC1_LUMSCALE_BACK and VC1_LUMSHIFT_BACK. (The command should contain the Intensity
            Compensation parameters that were used for opposite parity field when decoding 2nd P field picture).

            The parameters will only be used if VC1_BACK_INT_COMP in Slice Params command indicates
            Backward Intensity Compensation is used.
    */
    cmd = 0;
    REGIO_WRITE_FIELD(cmd, VC1_RENDEC_CMD, VC1SLICE04, VC1_LUMSHIFT_PREV,  ui8PrevLumaShift);
    REGIO_WRITE_FIELD(cmd, VC1_RENDEC_CMD, VC1SLICE04, VC1_LUMSCALE_PREV,  ui8PrevLumaScale);
    REGIO_WRITE_FIELD(cmd, VC1_RENDEC_CMD, VC1SLICE04, VC1_LUMSHIFT_BACK,  ui8BackLumaShift);
    REGIO_WRITE_FIELD(cmd, VC1_RENDEC_CMD, VC1SLICE04, VC1_LUMSCALE_BACK,  ui8BackLumaScale);
    psb_cmdbuf_rendec_write(cmdbuf, cmd);

#if 0
    /* VC1 Intensity Compensation Previous Bottom */
    if (ui8PrevBotIC) {
        /*
            The VDMC dynamically applies intensity compensation when generating reference predicted data
            for P/B fields/frames. In the case of Interlaced B field pictures, both the top field and
            bottom field could be Intensity Compensated twice (if all previous P field pictures applied
            separate top and bottom Intensity Compensation). If this is the case, the VC1 previous field
            defined in 3.3.10 should apply to top field, whilst the parameters defined in this register
            apply to the bottom field. The VC1_PREV_BOT_INT_COMP field of Slice Params command indicates
            if the fields in this register are used.
        */
        cmd = 0;
        REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, VC1_INTENSITY_COMPENSATION_, VC1_LUMSHIFT_PREV_BOT, ui8PrevBotLumaShift);
        REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, VC1_INTENSITY_COMPENSATION_, VC1_LUMSCALE_PREV_BOT, ui8PrevBotLumaScale);
        pcmdBuffer[i++] = REGISTER_OFFSET(MSVDX_CMDS, VC1_INTENSITY_COMPENSATION_);
        pcmdBuffer[i++] = cmd;
    }
#endif
    psb_cmdbuf_rendec_end(cmdbuf);

    /*
        Reference Picture Base Addresses

        The reference picture pointers always include the current picture at first location (0) and
        the oldest reference in the next location (1). For B pictures the subsequent reference
        frame (display order) is 2.
    */
    if ((pic_params->picture_fields.bits.picture_type != WMF_PTYPE_I) && (pic_params->picture_fields.bits.picture_type != WMF_PTYPE_BI)) {
        /* CHUNK: 4 */
        psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_CMDS, REFERENCE_PICTURE_BASE_ADDRESSES));

        /********************** CURRENT PICTURE **********************/
        psb_cmdbuf_rendec_write_address(cmdbuf, &target_surface->buf, target_surface->buf.buffer_ofs);
        psb_cmdbuf_rendec_write_address(cmdbuf, &target_surface->buf, target_surface->buf.buffer_ofs + target_surface->chroma_offset);

        /*************** FORWARD REFERENCE *****************/
        if (ctx->forward_ref_surface) {
            /*
                In VC1, if a P field picture references both top field and bottom field, but the two fields
                are stored in different frame stores, then the most recently decoded field will use reference
                index 0, and the other field will use reference index 1.

                Progressive P pictures use always reference index 1.
            */
            psb_cmdbuf_rendec_write_address(cmdbuf, &ctx->forward_ref_surface->psb_surface->buf, ctx->forward_ref_surface->psb_surface->buf.buffer_ofs);
            psb_cmdbuf_rendec_write_address(cmdbuf, &ctx->forward_ref_surface->psb_surface->buf, ctx->forward_ref_surface->psb_surface->\
                                            buf.buffer_ofs + ctx->forward_ref_surface->psb_surface->chroma_offset);
            (ctx->forward_ref_surface->psb_surface->buf).unfence_flag = 1;
        }

        /*************** BACKWARD REFERENCE *****************/
        if (ctx->backward_ref_surface) {
            psb_cmdbuf_rendec_write_address(cmdbuf, &ctx->backward_ref_surface->psb_surface->buf, ctx->backward_ref_surface->psb_surface->buf.buffer_ofs);
            psb_cmdbuf_rendec_write_address(cmdbuf, &ctx->backward_ref_surface->psb_surface->buf, ctx->backward_ref_surface->psb_surface\
                                            ->buf.buffer_ofs + ctx->backward_ref_surface->psb_surface->chroma_offset);
            (ctx->backward_ref_surface->psb_surface->buf).unfence_flag = 1;
        }

        /*** fixed crc error for vc1 ***/
        target_surface->buf.unfence_flag = 0;

        psb_cmdbuf_rendec_end(cmdbuf);
    }

    /* CHUNK: 5 - VC1SLICE02 */
    psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_CMDS, SLICE_PARAMS));
    *cmdbuf->rendec_chunk_start |= CMD_RENDEC_BLOCK_FLAG_VC1_SP_PATCH;

    /* VC1SLICE02           Command: Slice Params (picture or slice) */
    cmd = 0;

    //REGIO_WRITE_FIELD(cmd, MSVDX_CMDS, SLICE_PARAMS, VC1_PREV_BOT_INT_COMP,  ui8PrevBotIC);
    REGIO_WRITE_FIELD(cmd, VC1_RENDEC_CMD, VC1SLICE02, VC1_PREV_INT_COMP,  ui8PrevIC);
    REGIO_WRITE_FIELD(cmd, VC1_RENDEC_CMD, VC1SLICE02, VC1_BACK_INT_COMP,  ui8BackIC);
    REGIO_WRITE_FIELD(cmd, VC1_RENDEC_CMD, VC1SLICE02, RND_CTRL_BIT,       pic_params->rounding_control);
    REGIO_WRITE_FIELD(cmd, VC1_RENDEC_CMD, VC1SLICE02, MODE_CONFIG,        ctx->mode_config);
#ifndef VC1_Header_Parser_HW
        REGIO_WRITE_FIELD(cmd, VC1_RENDEC_CMD, VC1SLICE02, SUBPEL_FILTER_MODE, ((ctx->mv_mode == WMF_MVMODE_1MV_HALF_PEL_BILINEAR) && !(pic_params->picture_fields.bits.frame_coding_mode == VC1_FCM_FRMI)) ? 0 : 1);
        REGIO_WRITE_FIELD(cmd, VC1_RENDEC_CMD, VC1SLICE02, SLICE_CODE_TYPE, (pic_params->picture_fields.bits.picture_type == WMF_PTYPE_BI) ? 0 : (pic_params->picture_fields.bits.picture_type & 0x3));    /* BI is sent as I */
#endif
    REGIO_WRITE_FIELD(cmd, VC1_RENDEC_CMD, VC1SLICE02, VC1_FASTUVMC,       pic_params->fast_uvmc_flag);
    REGIO_WRITE_FIELD(cmd, VC1_RENDEC_CMD, VC1SLICE02, VC1_LOOPFILTER,     pic_params->entrypoint_fields.bits.loopfilter);
    REGIO_WRITE_FIELD(cmd, VC1_RENDEC_CMD, VC1SLICE02, SLICE_FIELD_TYPE,   ctx->slice_field_type);
    psb_cmdbuf_rendec_write(cmdbuf, cmd);

    psb_cmdbuf_rendec_end(cmdbuf);
#ifdef VC1_Header_Parser_HW
        REGIO_WRITE_FIELD(cmd, VC1_RENDEC_CMD, VC1SLICE02, SLICE_CODE_TYPE, (pic_params->picture_fields.bits.picture_type == WMF_PTYPE_BI) ? 0 : (pic_params->picture_fields.bits.picture_type & 0x3));
#endif

    *ctx->dec_ctx.p_slice_params = cmd;

    /* ------------------------------- Back-End Registers --------------------------------- */

    /* CHUNK: 6 (Back-end registers) */
    psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_VEC, VC1_CR_VEC_VC1_BE_SPS0));

    /* CR_VEC_VC1_BE_SPS0 */
    cmd = 0;
    REGIO_WRITE_FIELD(cmd, MSVDX_VEC_VC1, CR_VEC_VC1_BE_SPS0, VC1_BE_EXTENDED_DMV,   pic_params->mv_fields.bits.extended_dmv_flag);
    REGIO_WRITE_FIELD(cmd, MSVDX_VEC_VC1, CR_VEC_VC1_BE_SPS0, VC1_BE_EXTENDED_MV,    pic_params->mv_fields.bits.extended_mv_flag);
    REGIO_WRITE_FIELD(cmd, MSVDX_VEC_VC1, CR_VEC_VC1_BE_SPS0, VC1_BE_FASTUVMC,       pic_params->fast_uvmc_flag);
    REGIO_WRITE_FIELD(cmd, MSVDX_VEC_VC1, CR_VEC_VC1_BE_SPS0, VC1_BE_INTERLACE,      pic_params->sequence_fields.bits.interlace);
    //REGIO_WRITE_FIELD(cmd, MSVDX_VEC_VC1, CR_VEC_VC1_BE_SPS0, VC1_BE_PROFILE,      ctx->profile);
    psb_cmdbuf_rendec_write(cmdbuf, cmd);

    /* CR_VEC_VC1_BE_SPS1 */
    cmd = 0;
    REGIO_WRITE_FIELD(cmd, MSVDX_VEC_VC1, CR_VEC_VC1_BE_SPS1, VC1_BE_PIC_HEIGHT_IN_MBS_LESS1, ctx->picture_height_mb - 1);
    psb_cmdbuf_rendec_write(cmdbuf, cmd);

    /* CR_VEC_VC1_BE_SPS2 */
    cmd = 0;
    REGIO_WRITE_FIELD(cmd, MSVDX_VEC_VC1, CR_VEC_VC1_BE_SPS2, VC1_BE_PIC_WIDTH_IN_MBS_LESS1, ctx->picture_width_mb - 1);
    psb_cmdbuf_rendec_write(cmdbuf, cmd);

    psb_cmdbuf_rendec_end(cmdbuf);

    /* CHUNK: 6b (Back-end registers) */
    psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_VEC, VC1_CR_VEC_VC1_BE_PPS2));
    *cmdbuf->rendec_chunk_start |= CMD_RENDEC_BLOCK_FLAG_VC1_BE_PATCH;

    /* CR_VEC_VC1_BE_PPS2 */
    cmd = 0;
    //REGIO_WRITE_FIELD(cmd, MSVDX_VEC_VC1, CR_VEC_VC1_BE_PPS2, VC1_BE_FCM_REF2, ctx->ui8FCM_Ref2Pic);
    //REGIO_WRITE_FIELD(cmd, MSVDX_VEC_VC1, CR_VEC_VC1_BE_PPS2, VC1_BE_FCM_REF1, ctx->ui8FCM_Ref1Pic);
    //REGIO_WRITE_FIELD(cmd, MSVDX_VEC_VC1, CR_VEC_VC1_BE_PPS2, VC1_BE_FCM_REF0, ctx->ui8FCM_Ref0Pic);
    REGIO_WRITE_FIELD(cmd, MSVDX_VEC_VC1, CR_VEC_VC1_BE_PPS2, VC1_BE_FCM_REF2, ctx->backward_ref_fcm);
    REGIO_WRITE_FIELD(cmd, MSVDX_VEC_VC1, CR_VEC_VC1_BE_PPS2, VC1_BE_FCM_REF1, ctx->forward_ref_fcm);
    //REGIO_WRITE_FIELD(cmd, MSVDX_VEC_VC1, CR_VEC_VC1_BE_PPS2, VC1_BE_FCM_REF0, GET_SURFACE_INFO_picture_coding_type(ctx->decoded_surface->psb_surface));
    REGIO_WRITE_FIELD(cmd, MSVDX_VEC_VC1, CR_VEC_VC1_BE_PPS2, VC1_BE_FCM_REF0, GET_SURFACE_INFO_picture_coding_type(ctx->obj_context->current_render_target->psb_surface));
    REGIO_WRITE_FIELD(cmd, MSVDX_VEC_VC1, CR_VEC_VC1_BE_PPS2, VC1_BE_COLLOCATED_SKIPPED, 0); // @TODO: Really need this?
    psb_cmdbuf_rendec_write(cmdbuf, cmd);

    /* CR_VEC_VC1_BE_PPS0 */
    cmd = 0;
#ifndef VC1_Header_Parser_HW
        REGIO_WRITE_FIELD(cmd, MSVDX_VEC_VC1, CR_VEC_VC1_BE_PPS0, VC1_BE_IQ_OVERLAP, ((pic_params->picture_fields.bits.picture_type == WMF_PTYPE_B) || (ctx->condover == 0)) ? 0 : 1);
        REGIO_WRITE_FIELD(cmd, MSVDX_VEC_VC1, CR_VEC_VC1_BE_PPS0, VC1_BE_UNIFORM_QUANTIZER, pic_params->pic_quantizer_fields.bits.pic_quantizer_type);
        REGIO_WRITE_FIELD(cmd, MSVDX_VEC_VC1, CR_VEC_VC1_BE_PPS0, VC1_BE_HALFQP,            pic_params->pic_quantizer_fields.bits.half_qp);
        REGIO_WRITE_FIELD(cmd, MSVDX_VEC_VC1, CR_VEC_VC1_BE_PPS0, VC1_BE_BFRACTION,         pic_params->b_picture_fraction);
#endif
    REGIO_WRITE_FIELD(cmd, MSVDX_VEC_VC1, CR_VEC_VC1_BE_PPS0, VC1_BE_TFF_FWD,           ctx->bTFF_FwRefFrm);
    REGIO_WRITE_FIELD(cmd, MSVDX_VEC_VC1, CR_VEC_VC1_BE_PPS0, VC1_BE_TFF_BWD,           ctx->bTFF_BwRefFrm);
    REGIO_WRITE_FIELD(cmd, MSVDX_VEC_VC1, CR_VEC_VC1_BE_PPS0, VC1_BE_TFF,               pic_params->picture_fields.bits.top_field_first);
    REGIO_WRITE_FIELD(cmd, MSVDX_VEC_VC1, CR_VEC_VC1_BE_PPS0, VC1_BE_SECOND_FIELD,      !pic_params->picture_fields.bits.is_first_field);
    REGIO_WRITE_FIELD(cmd, MSVDX_VEC_VC1, CR_VEC_VC1_BE_PPS0, VC1_BE_FCM,               pic_params->picture_fields.bits.frame_coding_mode);
    REGIO_WRITE_FIELD(cmd, MSVDX_VEC_VC1, CR_VEC_VC1_BE_PPS0, VC1_BE_RNDCTRL,           pic_params->rounding_control);
    psb_cmdbuf_rendec_write(cmdbuf, cmd);

    /* CR_VEC_VC1_BE_PPS1 */
    cmd = 0;
#ifndef VC1_Header_Parser_HW
        REGIO_WRITE_FIELD(cmd, MSVDX_VEC_VC1, CR_VEC_VC1_BE_PPS1, VC1_BE_EXTEND_Y,       ctx->extend_y);
        REGIO_WRITE_FIELD(cmd, MSVDX_VEC_VC1, CR_VEC_VC1_BE_PPS1, VC1_BE_EXTEND_X,       ctx->extend_x);
        REGIO_WRITE_FIELD(cmd, MSVDX_VEC_VC1, CR_VEC_VC1_BE_PPS1, VC1_BE_QUANTIZER, (pic_params->pic_quantizer_fields.bits.pic_quantizer_type ? 0x03 /* uniform */ : 0x02 /* non-uniform */));
        REGIO_WRITE_FIELD(cmd, MSVDX_VEC_VC1, CR_VEC_VC1_BE_PPS1, VC1_BE_PQUANT,         pic_params->pic_quantizer_fields.bits.pic_quantizer_scale);
        REGIO_WRITE_FIELD(cmd, MSVDX_VEC_VC1, CR_VEC_VC1_BE_PPS1, VC1_BE_MVMODE,         pic_params->mv_fields.bits.mv_mode);
        REGIO_WRITE_FIELD(cmd, MSVDX_VEC_VC1, CR_VEC_VC1_BE_PPS1, VC1_BE_MVMODE2,        pic_params->mv_fields.bits.mv_mode2);
        REGIO_WRITE_FIELD(cmd, MSVDX_VEC_VC1, CR_VEC_VC1_BE_PPS1, VC1_BE_PTYPE,          pic_params->picture_fields.bits.picture_type);
#endif
    psb_cmdbuf_rendec_write(cmdbuf, cmd);

    /* CR_VEC_VC1_BE_MVD0 */
    cmd = 0;
#ifndef VC1_Header_Parser_HW
        REGIO_WRITE_FIELD(cmd, MSVDX_VEC_VC1, CR_VEC_VC1_BE_MVD0, VC1_BE_BRPD,  ctx->i8BckwrdRefFrmDist);     /* 10.4.6.2 */
        REGIO_WRITE_FIELD(cmd, MSVDX_VEC_VC1, CR_VEC_VC1_BE_MVD0, VC1_BE_FRPD,  ctx->i8FwrdRefFrmDist);
#endif
    psb_cmdbuf_rendec_write(cmdbuf, cmd);

    /* CR_VEC_VC1_BE_MVD1 */
    cmd = 0;
#ifndef VC1_Header_Parser_HW
        REGIO_WRITE_FIELD(cmd, MSVDX_VEC_VC1, CR_VEC_VC1_BE_MVD1, VC1_BE_SCALEFACTOR, ctx->ui32ScaleFactor);  /* figure 66 */
#endif
    psb_cmdbuf_rendec_write(cmdbuf, cmd);

    /* CR_VEC_VC1_BE_MVD2 */
    cmd = 0;
    REGIO_WRITE_FIELD(cmd, MSVDX_VEC_VC1, CR_VEC_VC1_BE_MVD2, VC1_BE_PULLBACK_X, ctx->pull_back_x);
    psb_cmdbuf_rendec_write(cmdbuf, cmd);

    /* CR_VEC_VC1_BE_MVD3 */
    cmd = 0;
    REGIO_WRITE_FIELD(cmd, MSVDX_VEC_VC1, CR_VEC_VC1_BE_MVD3, VC1_BE_PULLBACK_Y, ctx->pull_back_y);
    psb_cmdbuf_rendec_write(cmdbuf, cmd);

    /* CR_VEC_VC1_BE_MVD4 */
    cmd = 0;
    REGIO_WRITE_FIELD(cmd, MSVDX_VEC_VC1, CR_VEC_VC1_BE_MVD4, VC1_BE_FIRST_MB_IN_SLICE_Y, slice_param->slice_vertical_position);
    psb_cmdbuf_rendec_write(cmdbuf, cmd);

    /* CR_VEC_VC1_BE_MVD5 */
    cmd = 0;
#ifndef VC1_Header_Parser_HW
        REGIO_WRITE_FIELD(cmd, MSVDX_VEC_VC1, CR_VEC_VC1_BE_MVD5, VC1_BE_REFDIST,               pic_params->reference_fields.bits.reference_distance);
        REGIO_WRITE_FIELD(cmd, MSVDX_VEC_VC1, CR_VEC_VC1_BE_MVD5, VC1_BE_NUMREF,                pic_params->reference_fields.bits.num_reference_pictures);
        REGIO_WRITE_FIELD(cmd, MSVDX_VEC_VC1, CR_VEC_VC1_BE_MVD5, VC1_BE_REFFIELD,              pic_params->reference_fields.bits.reference_field_pic_indicator);
        REGIO_WRITE_FIELD(cmd, MSVDX_VEC_VC1, CR_VEC_VC1_BE_MVD5, VC1_BE_MVRANGE,               pic_params->mv_fields.bits.extended_mv_range);
        REGIO_WRITE_FIELD(cmd, MSVDX_VEC_VC1, CR_VEC_VC1_BE_MVD5, VC1_BE_HALFPEL_FLAG,  ctx->half_pel);
#endif
    //REGIO_WRITE_FIELD(cmd, MSVDX_VEC_VC1, CR_VEC_VC1_BE_MVD5, VC1_BE_FRAME_CODING_MODE,       pic_params->picture_fields.bits.frame_coding_mode);
    REGIO_WRITE_FIELD(cmd, MSVDX_VEC_VC1, CR_VEC_VC1_BE_MVD5, VC1_BE_BOTTOM_FIELD_FLAG, ctx->bottom_field);
    REGIO_WRITE_FIELD(cmd, MSVDX_VEC_VC1, CR_VEC_VC1_BE_MVD5, VC1_BE_ADVANCED_PROFILE, (ctx->profile == WMF_PROFILE_ADVANCED) ? 1 : 0);
    REGIO_WRITE_FIELD(cmd, MSVDX_VEC_VC1, CR_VEC_VC1_BE_MVD5, VC1_BE_SCAN_INDEX,        ctx->scan_index);
    psb_cmdbuf_rendec_write(cmdbuf, cmd);

    psb_cmdbuf_rendec_end(cmdbuf);

    /* CHUNK: 6c (Back-end registers) */
    psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_VEC, VC1_CR_VEC_VC1_BE_PARAM_BASE_ADDR));

    drv_debug_msg(VIDEO_DEBUG_GENERAL, "pnw_VC1: picture_type = %d\n", pic_params->picture_fields.bits.picture_type);

    if (PIC_TYPE_IS_INTRA(pic_params->picture_fields.bits.picture_type) || (pic_params->picture_fields.bits.picture_type == WMF_PTYPE_P)) {
        psb_buffer_p colocated_target_buffer = vld_dec_lookup_colocated_buffer(&ctx->dec_ctx, target_surface);
        ASSERT(colocated_target_buffer);
        if (colocated_target_buffer) {
            psb_cmdbuf_rendec_write_address(cmdbuf, colocated_target_buffer, ui32MBParamMemOffset);
        } else {
            /* This is an error */
            psb_cmdbuf_rendec_write(cmdbuf, 0);
        }
    } else if (pic_params->picture_fields.bits.picture_type == WMF_PTYPE_B) {
        ASSERT(ctx->forward_ref_surface);
        psb_buffer_p colocated_forward_ref_buffer = ctx->forward_ref_surface ? vld_dec_lookup_colocated_buffer(&ctx->dec_ctx, ctx->forward_ref_surface->psb_surface) : 0;
        ASSERT(colocated_forward_ref_buffer);
        if (colocated_forward_ref_buffer) {
            psb_cmdbuf_rendec_write_address(cmdbuf, colocated_forward_ref_buffer, ui32MBParamMemOffset);
        } else {
            /* This is an error */
            psb_cmdbuf_rendec_write(cmdbuf, 0);
        }
    }
    psb_cmdbuf_rendec_end(cmdbuf);

    if (!PIC_TYPE_IS_INTRA(pic_params->picture_fields.bits.picture_type)) {
        /* CHUNK: 6d (Back-end registers) */
        psb_cmdbuf_rendec_start(cmdbuf, RENDEC_REGISTER_OFFSET(MSVDX_VEC, VC1_CR_VEC_VC1_BE_COLPARAM_BASE_ADDR));

        if (pic_params->picture_fields.bits.picture_type == WMF_PTYPE_P) {
            /* CR_VEC_VC1_BE_COLPARAM_BASE_ADDR */
            ASSERT(ctx->forward_ref_surface);
            psb_buffer_p colocated_forward_ref_buffer = ctx->forward_ref_surface ? vld_dec_lookup_colocated_buffer(&ctx->dec_ctx, ctx->forward_ref_surface->psb_surface) : NULL;
            ASSERT(colocated_forward_ref_buffer);
            if (colocated_forward_ref_buffer) {
                psb_cmdbuf_rendec_write_address(cmdbuf, colocated_forward_ref_buffer, ui32MBParamMemOffset);
            } else {
                /* This is an error */
                psb_cmdbuf_rendec_write(cmdbuf, 0);
            }
        } else if (pic_params->picture_fields.bits.picture_type == WMF_PTYPE_B) {
            /* CR_VEC_VC1_BE_COLPARAM_BASE_ADDR */
            ASSERT(ctx->backward_ref_surface);
            psb_buffer_p colocated_backward_ref_buffer;

            if (NULL == ctx->backward_ref_surface) {
                drv_debug_msg(VIDEO_DEBUG_ERROR, "%s L%d Invalid backward_ref_surface handle\n", __FUNCTION__, __LINE__);
                return;
            }

            colocated_backward_ref_buffer = ctx->backward_ref_surface->psb_surface ? vld_dec_lookup_colocated_buffer(&ctx->dec_ctx, ctx->backward_ref_surface->psb_surface) : NULL;
            ASSERT(colocated_backward_ref_buffer);
            if (colocated_backward_ref_buffer) {
                psb_cmdbuf_rendec_write_address(cmdbuf, colocated_backward_ref_buffer, ui32MBParamMemOffset);
            } else {
                /* This is an error */
                psb_cmdbuf_rendec_write(cmdbuf, 0);
            }
        }

        psb_cmdbuf_rendec_end(cmdbuf);
    }

    /* psb_cmdbuf_rendec_end_block( cmdbuf ); */
}


static void psb__VC1_load_sequence_registers(context_VC1_p ctx)
{
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;
    uint32_t reg_value;

    psb_cmdbuf_reg_start_block(cmdbuf, 0);

    /* FE_CONTROL */
    reg_value = 0;
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC, CR_VEC_ENTDEC_FE_CONTROL, ENTDEC_FE_PROFILE, ctx->profile);
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC, CR_VEC_ENTDEC_FE_CONTROL, ENTDEC_FE_MODE, 2);  /* 2 - VC1 */
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC, CR_VEC_ENTDEC_FE_CONTROL), reg_value);

    /* FE_SPS0 */
    reg_value = 0;
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VC1, CR_VEC_VC1_FE_SPS0, VC1_FE_SYNCMARKER, ctx->pic_params->sequence_fields.bits.syncmarker);
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VC1, CR_VEC_VC1_FE_SPS0, VC1_FE_VSTRANSFORM, ctx->pic_params->transform_fields.bits.variable_sized_transform_flag);
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VC1, CR_VEC_VC1_FE_SPS0, VC1_FE_INTERLACE, ctx->pic_params->sequence_fields.bits.interlace);
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC_VC1, CR_VEC_VC1_FE_SPS0), reg_value);

    psb_cmdbuf_reg_end_block(cmdbuf);

}

static void psb__VC1_load_picture_registers(context_VC1_p ctx, VASliceParameterBufferVC1 *slice_param)
{
    VAPictureParameterBufferVC1 *pic_params = ctx->pic_params;
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;
    uint32_t reg_value;
    int bEnableMVDLite = FALSE;

    psb_cmdbuf_rendec_start(cmdbuf, REG_MSVDX_VEC_OFFSET + MSVDX_VEC_CR_VEC_ENTDEC_BE_CONTROL_OFFSET);
    reg_value = 0;
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC, CR_VEC_ENTDEC_BE_CONTROL, ENTDEC_BE_PROFILE, ctx->profile);
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC, CR_VEC_ENTDEC_BE_CONTROL, ENTDEC_BE_MODE, 2);  /* 2 - VC1 */
    psb_cmdbuf_rendec_write(cmdbuf, reg_value);
    psb_cmdbuf_rendec_end(cmdbuf);

#ifdef VC1_Header_Parser_HW
    psb_cmdbuf_reg_start_block(cmdbuf, CMD_REGVALPAIR_FLAG_VC1PATCH);
#else
    psb_cmdbuf_reg_start_block(cmdbuf, 0);
#endif

    /* Enable MVD lite for Progressive or FLDI P */
    if (
        (
            (pic_params->sequence_fields.bits.interlace && (pic_params->picture_fields.bits.frame_coding_mode == VC1_FCM_FLDI)) ||
            (!pic_params->sequence_fields.bits.interlace) ||
            (pic_params->sequence_fields.bits.interlace && (pic_params->picture_fields.bits.frame_coding_mode == VC1_FCM_P))
        ) &&
        (pic_params->picture_fields.bits.picture_type == WMF_PTYPE_P)
    ) {
        bEnableMVDLite = TRUE;
    }

    /* FE_PPS0 */
    reg_value = 0;
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VC1, CR_VEC_VC1_FE_PPS0, VC1_FE_PIC_WIDTH_IN_MBS_LESS1,  ctx->picture_width_mb - 1);
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VC1, CR_VEC_VC1_FE_PPS0, VC1_FE_PIC_HEIGHT_IN_MBS_LESS1, ctx->picture_height_mb - 1);
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VC1, CR_VEC_VC1_FE_PPS0, VC1_FE_FIRST_MB_IN_SLICE_Y,     slice_param->slice_vertical_position);
#ifndef VC1_Header_Parser_HW
        REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VC1, CR_VEC_VC1_FE_PPS0, VC1_FE_PTYPE,                   pic_params->picture_fields.bits.picture_type);
#endif
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VC1, CR_VEC_VC1_FE_PPS0, VC1_FE_FCM,                     pic_params->picture_fields.bits.frame_coding_mode);
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC_VC1, CR_VEC_VC1_FE_PPS0), reg_value);

    /* FE_PPS1 */
    reg_value = 0;
#ifndef VC1_Header_Parser_HW
        REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VC1, CR_VEC_VC1_FE_PPS1, VC1_FE_BP_FORMAT,     IMG_FALSE); // interleaved format
        REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VC1, CR_VEC_VC1_FE_PPS1, VC1_FE_BP_PRESENT,      ctx->bitplane_present);
        REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VC1, CR_VEC_VC1_FE_PPS1, VC1_FE_RAWCODINGFLAG, (pic_params->raw_coding.value & 0x7F)); // 7-bits
        REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VC1, CR_VEC_VC1_FE_PPS1, VC1_FE_MVMODE,      pic_params->mv_fields.bits.mv_mode);
        REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VC1, CR_VEC_VC1_FE_PPS1, VC1_FE_MVMODE2,     pic_params->mv_fields.bits.mv_mode2);
        REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VC1, CR_VEC_VC1_FE_PPS1, VC1_FE_TTMBF,       pic_params->transform_fields.bits.mb_level_transform_type_flag);
        REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VC1, CR_VEC_VC1_FE_PPS1, VC1_FE_TTFRM,       pic_params->transform_fields.bits.frame_level_transform_type);
        REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VC1, CR_VEC_VC1_FE_PPS1, VC1_FE_BFRACTION,   pic_params->b_picture_fraction);
        REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VC1, CR_VEC_VC1_FE_PPS1, VC1_FE_CONDOVER,    ctx->condover);
        REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VC1, CR_VEC_VC1_FE_PPS1, VC1_FE_EXTEND_X,    ctx->extend_x);
        REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VC1, CR_VEC_VC1_FE_PPS1, VC1_FE_EXTEND_Y,    ctx->extend_y);
#endif
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC_VC1, CR_VEC_VC1_FE_PPS1), reg_value);

    /* FE_PPS2 */
    reg_value = 0;
#ifndef VC1_Header_Parser_HW
        REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VC1, CR_VEC_VC1_FE_PPS2, VC1_FE_DQXBEDGE, (pic_params->pic_quantizer_fields.bits.dq_profile == 1) ? pic_params->pic_quantizer_fields.bits.dq_db_edge : pic_params->pic_quantizer_fields.bits.dq_sb_edge);
        REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VC1, CR_VEC_VC1_FE_PPS2, VC1_FE_DQUANT,            pic_params->pic_quantizer_fields.bits.dquant);
        REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VC1, CR_VEC_VC1_FE_PPS2, VC1_FE_PQUANT,            pic_params->pic_quantizer_fields.bits.pic_quantizer_scale);
        REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VC1, CR_VEC_VC1_FE_PPS2, VC1_FE_HALFQP,            pic_params->pic_quantizer_fields.bits.half_qp);
        if (((ctx->profile == WMF_PROFILE_ADVANCED) && (pic_params->pic_quantizer_fields.bits.dquant != 0))
            || ((ctx->profile != WMF_PROFILE_ADVANCED) && ((pic_params->picture_fields.bits.picture_type == WMF_PTYPE_B) || (pic_params->picture_fields.bits.picture_type == WMF_PTYPE_P))) && (pic_params->pic_quantizer_fields.bits.dquant != 0)) {
            REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VC1, CR_VEC_VC1_FE_PPS2, VC1_FE_VOPDQUANT_PRESENT, 1);
        } else {
            REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VC1, CR_VEC_VC1_FE_PPS2, VC1_FE_VOPDQUANT_PRESENT, 0);
        }
        REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VC1, CR_VEC_VC1_FE_PPS2, VC1_FE_DQUANTFRM,         pic_params->pic_quantizer_fields.bits.dq_frame);
        {
            IMG_BOOL DQUANT_INFRAME = (pic_params->pic_quantizer_fields.bits.dquant == 2) ||
                                      ((pic_params->pic_quantizer_fields.bits.dquant == 1) && pic_params->pic_quantizer_fields.bits.dq_frame) ||
                                      ((pic_params->pic_quantizer_fields.bits.dquant == 3) && pic_params->pic_quantizer_fields.bits.dq_frame);
            REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VC1, CR_VEC_VC1_FE_PPS2, VC1_FE_DQUANT_INFRAME, DQUANT_INFRAME);
        }
        REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VC1, CR_VEC_VC1_FE_PPS2, VC1_FE_ALTPQUANT,         pic_params->pic_quantizer_fields.bits.alt_pic_quantizer);
        REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VC1, CR_VEC_VC1_FE_PPS2, VC1_FE_DQPROFILE,         pic_params->pic_quantizer_fields.bits.dq_profile);
        REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VC1, CR_VEC_VC1_FE_PPS2, VC1_FE_DQBILEVEL,         pic_params->pic_quantizer_fields.bits.dq_binary_level);
        REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VC1, CR_VEC_VC1_FE_PPS2, VC1_FE_PQINDEX_GT8,       ctx->pqindex_gt8);
        REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VC1, CR_VEC_VC1_FE_PPS2, VC1_FE_TRANSACFRM,        pic_params->transform_fields.bits.transform_ac_codingset_idx1);
        REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VC1, CR_VEC_VC1_FE_PPS2, VC1_FE_TRANSACFRM2,       pic_params->transform_fields.bits.transform_ac_codingset_idx2);
#endif
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VC1, CR_VEC_VC1_FE_PPS2, VC1_FE_DQUANT, pic_params->pic_quantizer_fields.bits.dquant);
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC_VC1, CR_VEC_VC1_FE_PPS2), reg_value);

    /* MVD_LITE0 */
    reg_value = 0;
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VC1, CR_VEC_VC1_FE_MVD_LITE0, VC1_FE_MVD_LITE_ENABLE,   bEnableMVDLite);
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VC1, CR_VEC_VC1_FE_MVD_LITE0, VC1_FE_PULLBACK_X,        ctx->pull_back_x);
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VC1, CR_VEC_VC1_FE_MVD_LITE0, VC1_FE_PULLBACK_Y,        ctx->pull_back_y);
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC_VC1, CR_VEC_VC1_FE_MVD_LITE0), reg_value);

    /* MVD_LITE1 */
    reg_value = 0;
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VC1, CR_VEC_VC1_FE_MVD_LITE1, VC1_FE_TFF,              pic_params->picture_fields.bits.top_field_first);
#ifndef VC1_Header_Parser_HW
        REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VC1, CR_VEC_VC1_FE_MVD_LITE1, VC1_FE_REFDIST,          pic_params->reference_fields.bits.reference_distance);
        REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VC1, CR_VEC_VC1_FE_MVD_LITE1, VC1_FE_NUMREF,           pic_params->reference_fields.bits.num_reference_pictures);
        REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VC1, CR_VEC_VC1_FE_MVD_LITE1, VC1_FE_REFFIELD,         pic_params->reference_fields.bits.reference_field_pic_indicator);
        REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VC1, CR_VEC_VC1_FE_MVD_LITE1, VC1_FE_MVRANGE,          pic_params->mv_fields.bits.extended_mv_range);
        REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VC1, CR_VEC_VC1_FE_MVD_LITE1, VC1_FE_HALFPEL_FLAG,     ctx->half_pel);
        //REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VC1, CR_VEC_VC1_FE_MVD_LITE1, VC1_FE_FRAME_CODING_MODE,    pic_params->picture_fields.bits.frame_coding_mode);
#endif
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VC1, CR_VEC_VC1_FE_MVD_LITE1, VC1_FE_BOTTOM_FIELD_FLAG,      ctx->bottom_field);
    REGIO_WRITE_FIELD(reg_value, MSVDX_VEC_VC1, CR_VEC_VC1_FE_MVD_LITE1, VC1_FE_ADVANCED_PROFILE, (ctx->profile == WMF_PROFILE_ADVANCED) ? 1 : 0);
    psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC_VC1, CR_VEC_VC1_FE_MVD_LITE1), reg_value);

    psb_cmdbuf_reg_end_block(cmdbuf);
}

static void psb__VC1_setup_bitplane(context_VC1_p ctx)
{
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;

    psb_cmdbuf_reg_start_block(cmdbuf, 0);

#ifdef VC1_Header_Parser_HW
        psb_cmdbuf_reg_set_address(cmdbuf, REGISTER_OFFSET(MSVDX_VEC_VC1, CR_VEC_VC1_FE_BITPLANES_BASE_ADDR0),
                                   &ctx->bitplane_hw_buffer, 0);
        psb_cmdbuf_reg_set_address(cmdbuf, REGISTER_OFFSET(MSVDX_VEC_VC1, CR_VEC_VC1_FE_BITPLANES_BASE_ADDR1),
                                   &ctx->bitplane_hw_buffer, 0xa000);
        psb_cmdbuf_reg_set_address(cmdbuf, REGISTER_OFFSET(MSVDX_VEC_VC1, CR_VEC_VC1_FE_BITPLANES_BASE_ADDR2),
                                   &ctx->bitplane_hw_buffer, 0xa000 * 2);
#else
        if (ctx->bitplane_present)
            psb_cmdbuf_reg_set_address(cmdbuf, REGISTER_OFFSET(MSVDX_VEC_VC1, CR_VEC_VC1_FE_BITPLANES_BASE_ADDR0),
                                       ctx->bitplane_buffer, 0);
        else
            psb_cmdbuf_reg_set(cmdbuf, REGISTER_OFFSET(MSVDX_VEC_VC1, CR_VEC_VC1_FE_BITPLANES_BASE_ADDR0), 0);
#endif
    psb_cmdbuf_reg_end_block(cmdbuf);
}

static void psb__VC1_Send_Parse_Header_Cmd(context_VC1_p ctx, IMG_BOOL new_pic)
{
    PARSE_HEADER_CMD*       pParseHeaderCMD;
    VAPictureParameterBufferVC1 *pic_params = ctx->pic_params;
    psb_cmdbuf_p cmdbuf = ctx->obj_context->cmdbuf;

    //pParseHeaderCMD                                  = (PARSE_HEADER_CMD*)mCtrlAlloc.AllocateSpace(sizeof(PARSE_HEADER_CMD));
    pParseHeaderCMD = (PARSE_HEADER_CMD*)cmdbuf->cmd_idx;
    cmdbuf->cmd_idx += sizeof(PARSE_HEADER_CMD) / sizeof(uint32_t);

    pParseHeaderCMD->ui32Cmd                 = CMD_PARSE_HEADER;
    if (!new_pic) {
        pParseHeaderCMD->ui32Cmd        |= CMD_PARSE_HEADER_NEWSLICE;
    }

//        pParseHeaderCMD->ui32SeqHdrData  = (sVC1HeaderParser.sSeqHdr.EXTENDED_DMV&0x1)  << VC1_SEQHDR_EXTENDED_DMV;
    pParseHeaderCMD->ui32SeqHdrData  = (pic_params->mv_fields.bits.extended_dmv_flag) << VC1_SEQHDR_EXTENDED_DMV;

//        pParseHeaderCMD->ui32SeqHdrData |= (sVC1HeaderParser.sSeqHdr.PSF&0x1)                   << VC1_SEQHDR_PSF;
    pParseHeaderCMD->ui32SeqHdrData |= (pic_params->sequence_fields.bits.psf)               << VC1_SEQHDR_PSF;

//        pParseHeaderCMD->ui32SeqHdrData |= (sVC1HeaderParser.sSeqHdr.FINTERPFLAG&0x1)   << VC1_SEQHDR_FINTERPFLAG;
    pParseHeaderCMD->ui32SeqHdrData |= (pic_params->sequence_fields.bits.finterpflag) << VC1_SEQHDR_FINTERPFLAG;

//        pParseHeaderCMD->ui32SeqHdrData |= (sVC1HeaderParser.sSeqHdr.TFCNTRFLAG&0x1)    << VC1_SEQHDR_TFCNTRFLAG;
    pParseHeaderCMD->ui32SeqHdrData |= (pic_params->sequence_fields.bits.tfcntrflag) << VC1_SEQHDR_TFCNTRFLAG;;

//        pParseHeaderCMD->ui32SeqHdrData |= (sVC1HeaderParser.sSeqHdr.INTERLACE&0x1)             << VC1_SEQHDR_INTERLACE;
    pParseHeaderCMD->ui32SeqHdrData |= (pic_params->sequence_fields.bits.interlace)         << VC1_SEQHDR_INTERLACE;

//        pParseHeaderCMD->ui32SeqHdrData |= (sVC1HeaderParser.sSeqHdr.PULLDOWN&0x1)              << VC1_SEQHDR_PULLDOWN;
    pParseHeaderCMD->ui32SeqHdrData |= (pic_params->sequence_fields.bits.pulldown)          << VC1_SEQHDR_PULLDOWN;

//        pParseHeaderCMD->ui32SeqHdrData |= (sVC1HeaderParser.sSeqHdr.POSTPROCFLAG&0x1)  << VC1_SEQHDR_POSTPROCFLAG;
    pParseHeaderCMD->ui32SeqHdrData |= (pic_params->post_processing & 0x1)          << VC1_SEQHDR_POSTPROCFLAG;

//        pParseHeaderCMD->ui32SeqHdrData |= (sVC1HeaderParser.sSeqHdr.VSTRANSFORM&0x1)   << VC1_SEQHDR_VSTRANSFORM;
    pParseHeaderCMD->ui32SeqHdrData |= (pic_params->transform_fields.bits.variable_sized_transform_flag) << VC1_SEQHDR_VSTRANSFORM;

//        pParseHeaderCMD->ui32SeqHdrData |= (rser.sSeqHdr.DQUANT&0x3)                << VC1_SEQHDR_DQUANT;
    pParseHeaderCMD->ui32SeqHdrData |= (pic_params->pic_quantizer_fields.bits.dquant) << VC1_SEQHDR_DQUANT;

//        pParseHeaderCMD->ui32SeqHdrData |= (sVC1HeaderParser.sSeqHdr.EXTENDED_MV&0x1)   << VC1_SEQHDR_EXTENDED_MV;
    pParseHeaderCMD->ui32SeqHdrData |= (pic_params->mv_fields.bits.extended_mv_flag) << VC1_SEQHDR_EXTENDED_MV;

//        pParseHeaderCMD->ui32SeqHdrData |= (sVC1HeaderParser.sSeqHdr.FASTUVMC&0x1)              << VC1_SEQHDR_FASTUVMC;
    pParseHeaderCMD->ui32SeqHdrData |= (pic_params->fast_uvmc_flag & 0x1)                       << VC1_SEQHDR_FASTUVMC;

//        pParseHeaderCMD->ui32SeqHdrData |= (sVC1HeaderParser.sSeqHdr.LOOPFILTER&0x1)    << VC1_SEQHDR_LOOPFILTER;
    pParseHeaderCMD->ui32SeqHdrData |= (pic_params->entrypoint_fields.bits.loopfilter) << VC1_SEQHDR_LOOPFILTER;

//        pParseHeaderCMD->ui32SeqHdrData |= (sVC1HeaderParser.sSeqHdr.REFDIST_FLAG&0x1)  << VC1_SEQHDR_REFDIST_FLAG;
    pParseHeaderCMD->ui32SeqHdrData |= (pic_params->reference_fields.bits.reference_distance_flag) << VC1_SEQHDR_REFDIST_FLAG;

//        pParseHeaderCMD->ui32SeqHdrData |= (sVC1HeaderParser.sSeqHdr.PANSCAN_FLAG&0x1)  << VC1_SEQHDR_PANSCAN_FLAG;
    pParseHeaderCMD->ui32SeqHdrData |= (pic_params->entrypoint_fields.bits.panscan_flag) << VC1_SEQHDR_PANSCAN_FLAG;

//        pParseHeaderCMD->ui32SeqHdrData |= (sVC1HeaderParser.sSeqHdr.MAXBFRAMES&0x7)    << VC1_SEQHDR_MAXBFRAMES;
    pParseHeaderCMD->ui32SeqHdrData |= (pic_params->sequence_fields.bits.max_b_frames) << VC1_SEQHDR_MAXBFRAMES;

//        pParseHeaderCMD->ui32SeqHdrData |= (sVC1HeaderParser.sSeqHdr.RANGERED&0x1)              << VC1_SEQHDR_RANGERED;
    pParseHeaderCMD->ui32SeqHdrData |= (pic_params->sequence_fields.bits.rangered) << VC1_SEQHDR_RANGERED;

//        pParseHeaderCMD->ui32SeqHdrData |= (sVC1HeaderParser.sSeqHdr.SYNCMARKER&0x1)    << VC1_SEQHDR_SYNCMARKER;
    pParseHeaderCMD->ui32SeqHdrData |= (pic_params->sequence_fields.bits.syncmarker) << VC1_SEQHDR_SYNCMARKER;

//        pParseHeaderCMD->ui32SeqHdrData |= (sVC1HeaderParser.sSeqHdr.MULTIRES&0x1)              << VC1_SEQHDR_MULTIRES;
    pParseHeaderCMD->ui32SeqHdrData |= (pic_params->sequence_fields.bits.multires) << VC1_SEQHDR_MULTIRES;

//        pParseHeaderCMD->ui32SeqHdrData |= (sVC1HeaderParser.sSeqHdr.QUANTIZER&0x3)             << VC1_SEQHDR_QUANTIZER;
    pParseHeaderCMD->ui32SeqHdrData |= (pic_params->pic_quantizer_fields.bits.quantizer) << VC1_SEQHDR_QUANTIZER;

//        pParseHeaderCMD->ui32SeqHdrData |= (sVC1HeaderParser.sSeqHdr.OVERLAP&0x1)               << VC1_SEQHDR_OVERLAP;
    pParseHeaderCMD->ui32SeqHdrData |= (pic_params->sequence_fields.bits.overlap) << VC1_SEQHDR_OVERLAP;

//        pParseHeaderCMD->ui32SeqHdrData |= (sVC1HeaderParser.sSeqHdr.PROFILE&0x3)               << VC1_SEQHDR_PROFILE;
    pParseHeaderCMD->ui32SeqHdrData |= (ctx->profile) << VC1_SEQHDR_PROFILE;

//        pParseHeaderCMD->ui32SeqHdrData |= (msPicParam.bSecondField&0x1)                                << VC1_SEQHDR_SECONDFIELD;
    pParseHeaderCMD->ui32SeqHdrData |= (!pic_params->picture_fields.bits.is_first_field) << VC1_SEQHDR_SECONDFIELD;

//        pParseHeaderCMD->ui32SeqHdrData |= (mpDestFrame->FrameCodingMode()&0x3)                 << VC1_SEQHDR_FCM_CURRPIC;
    pParseHeaderCMD->ui32SeqHdrData |= (pic_params->picture_fields.bits.frame_coding_mode & 0x3) << VC1_SEQHDR_FCM_CURRPIC;

//        pParseHeaderCMD->ui32SeqHdrData |= (mui8PicType&0x3)                                              << VC1_SEQHDR_PICTYPE;
    pParseHeaderCMD->ui32SeqHdrData |= (pic_params->picture_fields.bits.picture_type & 0x3) << VC1_SEQHDR_PICTYPE;

//        pParseHeaderCMD->ui32SeqHdrData |= ((msPicParam.bBidirectionalAveragingMode>>4)&0x1) << VC1_SEQHDR_ICFLAG;
    pParseHeaderCMD->ui32SeqHdrData |= (pic_params->picture_fields.bits.intensity_compensation) <<  VC1_SEQHDR_ICFLAG;

    pParseHeaderCMD->ui32PicDimensions              = ctx->picture_width_mb;
    pParseHeaderCMD->ui32PicDimensions         |= (ctx->picture_height_mb << 16);

//        pParseHeaderCMD->ui32BitplaneAddr[0]    = (psBitplaneHWBuffer[0]->GetTopDeviceMemAlloc())->GetDeviceVirtAddress();
//        pParseHeaderCMD->ui32BitplaneAddr[1]    = (psBitplaneHWBuffer[1]->GetTopDeviceMemAlloc())->GetDeviceVirtAddress();
//        pParseHeaderCMD->ui32BitplaneAddr[2]    = (psBitplaneHWBuffer[2]->GetTopDeviceMemAlloc())->GetDeviceVirtAddress();
    RELOC(pParseHeaderCMD->ui32BitplaneAddr[0], ctx->bitplane_hw_buffer.buffer_ofs, &ctx->bitplane_hw_buffer);
    RELOC(pParseHeaderCMD->ui32BitplaneAddr[1], ctx->bitplane_hw_buffer.buffer_ofs + 0xa000, &ctx->bitplane_hw_buffer);
    RELOC(pParseHeaderCMD->ui32BitplaneAddr[2], ctx->bitplane_hw_buffer.buffer_ofs + 0xa000 * 2, &ctx->bitplane_hw_buffer);

//        pParseHeaderCMD->ui32VLCTableAddr       =       psVlcPackedTableData->GetTopDeviceMemAlloc()->GetDeviceVirtAddress();
    RELOC(pParseHeaderCMD->ui32VLCTableAddr, ctx->vlc_packed_table.buffer_ofs, &ctx->vlc_packed_table);
    /*
        pParseHeaderCMD->ui32ICParamData[0]      = ((msPicParam.wBitstreamFcodes >> 8) & 0xFF);
        pParseHeaderCMD->ui32ICParamData[0] |= ((msPicParam.wBitstreamPCEelements >> 8) & 0xFF) << 8;
        if( mpForwardRefFrame->TopFieldFirst() )
                pParseHeaderCMD->ui32ICParamData[0] |= (1 << 16);
    */
    pParseHeaderCMD->ui32ICParamData[0]      = ((pic_params->luma_scale >> 8) & 0xFF);
    pParseHeaderCMD->ui32ICParamData[0] |= ((pic_params->luma_shift >> 8) & 0xFF) << 8;
    if (ctx->bTFF_FwRefFrm)
        pParseHeaderCMD->ui32ICParamData[0] |= (1 << 16);
    /*
        pParseHeaderCMD->ui32ICParamData[1]      = (msPicParam.wBitstreamFcodes & 0xFF);
        pParseHeaderCMD->ui32ICParamData[1]     |= (msPicParam.wBitstreamPCEelements & 0xFF) << 8;
        if( mpDestFrame->TopFieldFirst() )
                pParseHeaderCMD->ui32ICParamData[1] |= (1 << 16);
    */
    pParseHeaderCMD->ui32ICParamData[1]      = (pic_params->luma_scale & 0xFF);
    pParseHeaderCMD->ui32ICParamData[1]     |= (pic_params->luma_shift & 0xFF) << 8;
    if (pic_params->picture_fields.bits.top_field_first)
        pParseHeaderCMD->ui32ICParamData[1] |= (1 << 16);

    pParseHeaderCMD->ui32ICParamData[0] = 0x00010000;
    pParseHeaderCMD->ui32ICParamData[1] = 0x00010020;
}

static void psb__VC1_begin_slice(context_DEC_p dec_ctx, VASliceParameterBufferBase *vld_slice_param)
{
    VASliceParameterBufferVC1 *slice_param = (VASliceParameterBufferVC1 *) vld_slice_param;
    context_VC1_p ctx = (context_VC1_p)dec_ctx;

    dec_ctx->bits_offset = slice_param->macroblock_offset;
    dec_ctx->SR_flags = (ctx->profile == WMF_PROFILE_ADVANCED) ? (CMD_ENABLE_RBDU_EXTRACTION | CMD_SR_VERIFY_STARTCODE) : 0;
}

static void psb__VC1_process_slice_data(context_DEC_p dec_ctx, VASliceParameterBufferBase *vld_slice_param)
{
    VASliceParameterBufferVC1 *slice_param = (VASliceParameterBufferVC1 *) vld_slice_param;
    context_VC1_p ctx = (context_VC1_p)dec_ctx;

    psb__VC1_load_sequence_registers(ctx);

#ifndef VC1_Header_Parser_HW
        psb__VC1_write_VLC_tables(ctx);
        psb__VC1_build_VLC_tables(ctx);
#else
        psb__VC1_Send_Parse_Header_Cmd(ctx, ctx->is_first_slice);
#endif

    psb__VC1_load_picture_registers(ctx, slice_param);
    psb__VC1_setup_bitplane(ctx);
    psb__VC1_send_rendec_params(ctx, slice_param);
}

static void psb__VC1_end_slice(context_DEC_p dec_ctx)
{
    context_VC1_p ctx = (context_VC1_p)dec_ctx;

    ctx->obj_context->first_mb = 0;
    if (ctx->is_first_slice) {
        ctx->obj_context->flags |= FW_VA_RENDER_IS_FIRST_SLICE;
    }
    //if (ctx->bitplane_present)
    {
        ctx->obj_context->flags |= FW_VA_RENDER_VC1_BITPLANE_PRESENT;
    }
    ctx->obj_context->last_mb = ((ctx->picture_height_mb - 1) << 8) | (ctx->picture_width_mb - 1);

    *(ctx->dec_ctx.slice_first_pic_last) = (ctx->obj_context->first_mb << 16) | (ctx->obj_context->last_mb);
    if (psb_video_trace_fp && (psb_video_trace_level & AUXBUF_TRACE)) {
        psb__debug_schedule_hexdump("Preload buffer", &ctx->preload_buffer, 0, PRELOAD_BUFFER_SIZE);
        psb__debug_schedule_hexdump("AUXMSB buffer", &ctx->aux_msb_buffer, 0, 0x8000 /* AUXMSB_BUFFER_SIZE */);
        psb__debug_schedule_hexdump("VLC Table", &ctx->vlc_packed_table, 0, gui16vc1VlcTableSize * sizeof(IMG_UINT16));
    }

    ctx->is_first_slice = FALSE; /* Reset */
}

static VAStatus pnw_VC1_BeginPicture(
    object_context_p obj_context)
{
    INIT_CONTEXT_VC1

    if (ctx->pic_params) {
        free(ctx->pic_params);
        ctx->pic_params = NULL;
    }
    ctx->is_first_slice = TRUE;

    return VA_STATUS_SUCCESS;
}

static VAStatus pnw_VC1_process_buffer(
    context_DEC_p dec_ctx,
    object_buffer_p buffer)
{
    context_VC1_p ctx = (context_VC1_p)dec_ctx;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    object_buffer_p obj_buffer = buffer;

    switch (obj_buffer->type) {
    case VAPictureParameterBufferType:
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "pnw_VC1_RenderPicture got VAPictureParameterBuffer\n");
        vaStatus = psb__VC1_process_picture_param(ctx, obj_buffer);
        DEBUG_FAILURE;
        break;

    case VABitPlaneBufferType:
        drv_debug_msg(VIDEO_DEBUG_GENERAL, "pnw_VC1_RenderPicture got VABitPlaneBuffer\n");
        vaStatus = psb__VC1_process_bitplane(ctx, obj_buffer);
        DEBUG_FAILURE;
        break;

    default:
        vaStatus = VA_STATUS_ERROR_UNKNOWN;
        DEBUG_FAILURE;
    }

    return vaStatus;
}

static VAStatus pnw_VC1_EndPicture(
    object_context_p obj_context)
{
    INIT_CONTEXT_VC1

    if (psb_context_flush_cmdbuf(ctx->obj_context)) {
        return VA_STATUS_ERROR_UNKNOWN;
    }

    ASSERT(ctx->pic_params);
    if (!ctx->pic_params) {
        return VA_STATUS_ERROR_UNKNOWN;
    }

    /********* Keep some picture parameters of the previously decoded picture ***********/
    if (PIC_TYPE_IS_REF(ctx->pic_params->picture_fields.bits.picture_type)) { // I or P
        /* Assume that the picture that we just decoded (the picture previous to the one that
           is about to be decoded) is the backward reference picture for a B picture. */
        /* TODO: Make this more robust */
        ctx->ui8FCM_Ref2Pic = ctx->pic_params->picture_fields.bits.frame_coding_mode;

        /* For interlaced field pictures only */
        if ((ctx->pic_params->picture_fields.bits.frame_coding_mode != VC1_FCM_FLDI) || !ctx->pic_params->picture_fields.bits.is_first_field) {
            ctx->bTFF_BwRefFrm = ctx->pic_params->picture_fields.bits.top_field_first;
        }
    }

    ctx->bRef1RangeRed = ctx->bRef0RangeRed;
    if (PIC_TYPE_IS_REF(ctx->pic_params->picture_fields.bits.picture_type)) {
        ctx->bRef0RangeRed = ctx->pic_params->range_reduction_frame;
    }
    /***********************************************************************************/

    free(ctx->pic_params);
    ctx->pic_params = NULL;

    return VA_STATUS_SUCCESS;
}

struct format_vtable_s pnw_VC1_vtable = {
queryConfigAttributes:
    pnw_VC1_QueryConfigAttributes,
validateConfig:
    pnw_VC1_ValidateConfig,
createContext:
    pnw_VC1_CreateContext,
destroyContext:
    pnw_VC1_DestroyContext,
beginPicture:
    pnw_VC1_BeginPicture,
renderPicture:
    vld_dec_RenderPicture,
endPicture:
    pnw_VC1_EndPicture
};
