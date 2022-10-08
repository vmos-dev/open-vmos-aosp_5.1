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
 *     * Neither the name of The Linux Foundation. nor the names of its
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

#ifndef _C2DEXT_H_
#define _C2DEXT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "c2d2.h"

typedef enum {                                  /*!< lens correction type           */
    C2D_LENSCORRECT_AFFINE          = 0,        /*!< affine, default (3x2 matrix)   */
    C2D_LENSCORRECT_PERSPECTIVE     = (1 << 0), /*!< perspective bit  (3x3) matrix  */
    C2D_LENSCORRECT_BILINEAR        = (1 << 1), /*!< bilinear bit                   */
    C2D_LENSCORRECT_ORIGIN_IN_MIDDLE= (1 << 2), /*!< rotation origin for matrix is in the middle */
    C2D_LENSCORRECT_SOURCE_RECT     = (1 << 3), /*!< enables source_rect field. When set, client has to provide C2D_LENSCORRECT_OBJECT.source_rect
                                                     if not set source surface dimensions are used.*/
    C2D_LENSCORRECT_TARGET_RECT     = (1 << 4), /*!< enables target_rect field. When set, client has to provide C2D_LENSCORRECT_OBJECT.target_rect
                                                     if not set target surface dimensions are used. */
} C2D_LENSCORRECT_TYPE;

typedef struct C2D_LENSCORRECT_OBJ_STR {
    uint32   srcId;                     /*!< source surface */
    C2D_RECT blitSize;                  /*!< C2D_RECT for blit size */
    C2D_RECT gridSize;                  /*!< C2D_RECT for grid size */
    int32    offsetX;                   /*!< source x offset */
    int32    offsetY;                   /*!< source y offset */
    uint32   transformType;             /*!< (C2D_LENSCORRECT_AFFINE or C2D_LENSCORRECT_PERSPECTIVE) + C2D_LENSCORRECT_BILINEAR */
    float   *transformMatrices;         /*!<  transformMatrix array, 3x2 or 3x3 depending transformType  */
    C2D_RECT source_rect;               /*!< region of the source surface,   16.16 fp */
    C2D_RECT target_rect;               /*!< position and scaling in target, 16.16 fp */
}C2D_LENSCORRECT_OBJECT;

/* ------------------------------------------------------------------- *//*!
 * \external
 * \brief   Lens correction, affine or perspective
 *
 * \param    uint32 targetSurface
 * \param    C2D_LENSCORRECT_OBJECT sourceObject
 * \return  C2D_STATUS_OK on success
 *//* ------------------------------------------------------------------- */
C2D_API C2D_STATUS c2dLensCorrection(
    uint32                  targetSurface,
    C2D_LENSCORRECT_OBJECT *sourceObject);

/* ------------------------------------------------------------------- *//*!
 * \external
 * \brief   Locks surface and sets surfaces host pointer
 *
 * \param    uint32 surface
 * \param    void** pointer to void pointer where host address is returned
 * \return  C2D_STATUS_OK on success
 *//* ------------------------------------------------------------------- */
C2D_API C2D_STATUS c2dLockSurface(
    uint32   a_surface,
    void   **y_pHostAddress,
    void   **u_pHostAddress,
    void   **v_pHostAddress);

/* ------------------------------------------------------------------- *//*!
 * \external
 * \brief   Unlocks surface
 *
 * \param    uint32 surface
 * \return  C2D_STATUS_OK on success
 *//* ------------------------------------------------------------------- */
C2D_API C2D_STATUS c2dUnlockSurface(
    uint32 a_surface);

/* ------------------------------------------------------------------- *//*!
 * \external
 * \brief   Fills surface information parameters
 *
 * \param   uint32 surface
 * \param   uint32 *width
 * \param   uint32 *height
 * \param   uint32 *format
 * \return  C2D_STATUS_OK on success
 *//* ------------------------------------------------------------------- */
C2D_API C2D_STATUS c2dGetSurfaceInfo(
    uint32  a_surface,
    uint32 *width,
    uint32 *height,
    uint32 *format );

/* ------------------------------------------------------------------- *//*!
 * \external
 * \brief   Fills device information parameters
 *
 * \param    uint32 *count
 * \return  C2D_STATUS_OK on success
 *//* ------------------------------------------------------------------- */
C2D_API C2D_STATUS c2dGetDeviceInfo(
    uint32 *count );


//=============================================================================
// C2D extension for adding shader support
//=============================================================================
typedef enum {                          /*!< C2D sampler type  enum         */
    C2D_SAMPLER_FRAG_SRC    = (1 << 0), /*!< Frag sampler for source surface*/
    C2D_SAMPLER_FRAG_DST    = (1 << 1), /*!< Frag sampler for target surface*/
    C2D_SAMPLER_FRAG_MASK   = (1 << 2)  /*!< Frag sampler for mask surface  */
}C2D_SAMPLER_TYPE;


struct c2d_sampler {                        /*!< C2D sampler structure      */
    char*               sampler_name;       /*!< sampler name               */
    uint32              sampler_name_len;   /*!< sampler name length        */
    C2D_SAMPLER_TYPE    sampler_type;       /*!< sampler type               */
};


struct c2d_uniform {                        /*!< C2D uniform structure      */
    char *              u_name;             /*!< uniform name               */
    int                 u_name_len;         /*!< uniform name length        */
    void*               u_data;             /*!< uniform data buffer        */
    uint32              u_data_len;         /*!< uniform data buffer length */
    uint32              u_item_size;        /*!< per item size in buffer    */
    uint32              u_item_count;       /*!< total item count in buffer */
};


struct c2d_program {                        /*!< C2D shader structure       */
    unsigned char*      program_binary;     /*!< shader binary (VS + FS)    */
    uint32              program_binary_len; /*!< shader binary length       */
    struct c2d_uniform* uniform_list;       /*!< uniform list in the shader */
    uint32              uniform_count;      /*!< uniform count in the shader*/
    struct c2d_sampler* sampler_list;       /*!< sampler list in the shader */
    uint32              sampler_count;      /*!< sampler count in the shader*/
};


/* ------------------------------------------------------------------- *//*!
 * \external
 * \brief   adding a user defined program in C2D that can be used later
 *
 * \param    struct c2d_program *   the program that user wants to Add
 * \return   uint32                 this is the id that will be used later
 *                                  to refer the program
 *
 *//* ------------------------------------------------------------------- */
C2D_API uint32 c2dAddProgram(struct c2d_program* a_program);


/* ------------------------------------------------------------------- *//*!
 * \external
 * \brief   Select a shader to be used
 *
 * \param   pid         this is the program id user want to remove
 * \return  C2D_STATUS  C2D_STATUS_OK on success
 *//* ------------------------------------------------------------------- */
C2D_API C2D_STATUS c2dRemoveProgram(uint32 pid);


/* ------------------------------------------------------------------- *//*!
 * \external
 * \brief   Select a program to be used by C2D.
 *
 * \param   pid             this is the program id user want to use
 * \return  C2D_STATUS      C2D_STATUS__OK on success
 *//* ------------------------------------------------------------------- */
C2D_API C2D_STATUS c2dActivateProgram(uint32 pid);


/* ------------------------------------------------------------------- *//*!
 * \external
 * \brief   Deselect a program to be used by C2D.
 *
 * \param   pid             this is the program id user want to deactivate
 * \return  C2D_STATUS      C2D_STATUS__OK on success
 *//* ------------------------------------------------------------------- */
C2D_API C2D_STATUS c2dDeactivateProgram(uint32 pid);


#ifdef __cplusplus
}
#endif

#endif /* _C2DEXT_H_ */
