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
 */


/*!
******************************************************************************
 @file   : mem_io.h

 @brief         Memory structure access macros.

 @date          12/09/05

 <b>Description:</b>\n

 This file contains a set of memory access macros for accessing packed memory
 structures.

 <b>Platform:</b>\n
 Platform Independent

 @Version       1.0

******************************************************************************/

/*
******************************************************************************
 Modifications :-

 $Log: mem_io.h $

  --- Revision Logs Removed ---

  --- Revision Logs Removed ---

  --- Revision Logs Removed ---

  --- Revision Logs Removed ---

  --- Revision Logs Removed ---

  --- Revision Logs Removed ---

  --- Revision Logs Removed ---

  --- Revision Logs Removed ---

  --- Revision Logs Removed ---

  --- Revision Logs Removed ---


*****************************************************************************/

#if !defined (__MEM_IO_H__)
#define __MEM_IO_H__

#if (__cplusplus)
extern "C" {
#endif

#include "img_types.h"

#ifdef DOXYGEN_WILL_SEE_THIS
    /*!
    ******************************************************************************

     @Function  MEMIO_READ_FIELD

     @Description

     This macro is used to extract a field from a packed memory based structure.

     @Input             vpMem:          A pointer to the memory structure.

     @Input             field:          The name of the field to be extracted.

     @Return    IMG_UINT32:     The value of the field - right aligned.

    ******************************************************************************/
    IMG_UINT32 MEMIO_READ_FIELD(IMG_VOID *      vpMem, field);

    /*!
    ******************************************************************************

     @Function  MEMIO_READ_TABLE_FIELD

     @Description

     This macro is used to extract the value of a field in a table in a packed
     memory based structure.

     @Input             vpMem:          A pointer to the memory structure.

     @Input             field:          The name of the field to be extracted.

     @Input             ui32TabIndex:           The table index of the field to be extracted.

     @Return    IMG_UINT32:     The value of the field - right aligned.

    ******************************************************************************/
    IMG_UINT32 MEMIO_READ_TABLE_FIELD(IMG_VOID *        vpMem, field, IMG_UINT32 ui32TabIndex);

    /*!
    ******************************************************************************

     @Function  MEMIO_READ_REPEATED_FIELD

     @Description

     This macro is used to extract the value of a repeated field in a packed
     memory based structure.

     @Input             vpMem:          A pointer to the memory structure.

     @Input             field:          The name of the field to be extracted.

     @Input             ui32RepIndex:           The repeat index of the field to be extracted.

     @Return    IMG_UINT32:     The value of the field - right aligned.

    ******************************************************************************/
    IMG_UINT32 MEMIO_READ_REPEATED_FIELD(IMG_VOID *     vpMem, field, IMG_UINT32 ui32RepIndex);

    /*!
    ******************************************************************************

     @Function  MEMIO_READ_TABLE_REPEATED_FIELD

     @Description

     This macro is used to extract the value of a repeated field in a table
     in a packed memory based structure.

     @Input             vpMem:          A pointer to the memory structure.

     @Input             field:          The name of the field to be extracted.

     @Input             ui32TabIndex:           The table index of the field to be extracted.

     @Input             ui32RepIndex:           The repeat index of the field to be extracted.

     @Return    IMG_UINT32:     The value of the field - right aligned.

    ******************************************************************************/
    IMG_UINT32 MEMIO_READ_TABLE_REPEATED_FIELD(IMG_VOID *       vpMem, field, IMG_UINT32 ui32TabIndex, IMG_UINT32 ui32RepIndex);

    /*!
    ******************************************************************************

     @Function  MEMIO_WRITE_FIELD

     @Description

     This macro is used to update the value of a field in a packed memory based
     structure.

     @Input             vpMem:          A pointer to the memory structure.

     @Input             field:          The name of the field to be updated.

     @Input             ui32Value:      The value to be writtem to the field - right aligned.

     @Return    None.

    ******************************************************************************/
    IMG_VOID MEMIO_WRITE_FIELD(IMG_VOID *       vpMem, field, IMG_UINT32        ui32Value);

    /*!
    ******************************************************************************

     @Function  MEMIO_WRITE_TABLE_FIELD

     @Description

     This macro is used to update the field in a table in a packed memory
     based structure.

     @Input             vpMem:          A pointer to the memory structure.

     @Input             field:          The name of the field to be updated.

     @Input             ui32TabIndex:           The table index of the field to be updated.

     @Input             ui32Value:      The value to be writtem to the field - right aligned.

     @Return    None.

    ******************************************************************************/
    IMG_VOID MEMIO_WRITE_TABLE_FIELD(IMG_VOID * vpMem, field, IMG_UINT32 ui32TabIndex, IMG_UINT32       ui32Value);

    /*!
    ******************************************************************************

     @Function  MEMIO_WRITE_REPEATED_FIELD

     @Description

     This macro is used to update a repeated field in a packed memory
     based structure.

     @Input             vpMem:          A pointer to the memory structure.

     @Input             field:          The name of the field to be updated.

     @Input             ui32RepIndex:           The repeat index of the field to be updated.

     @Input             ui32Value:      The value to be writtem to the field - right aligned.

     @Return    None.

    ******************************************************************************/
    IMG_VOID MEMIO_WRITE_REPEATED_FIELD(IMG_VOID *      vpMem, field, IMG_UINT32 ui32RepIndex, IMG_UINT32       ui32Value);


    /*!
    ******************************************************************************

     @Function  MEMIO_WRITE_TABLE_REPEATED_FIELD

     @Description

     This macro is used to update a repeated field in a table in a packed memory
     based structure.

     @Input             vpMem:          A pointer to the memory structure.

     @Input             field:          The name of the field to be updated.

     @Input             ui32TabIndex:           The table index of the field to be updated.

     @Input             ui32RepIndex:           The repeat index of the field to be updated.

     @Input             ui32Value:      The value to be writtem to the field - right aligned.

     @Return    None.

    ******************************************************************************/
    IMG_VOID MEMIO_WRITE_TABLE_REPEATED_FIELD(IMG_VOID *        vpMem, field, IMG_UINT32 ui32TabIndex, IMG_UINT32 ui32RepIndex, IMG_UINT32      ui32Value);

#else

#if WIN32
#define MEMIO_CHECK_ALIGNMENT(vpMem)

#else
#define MEMIO_CHECK_ALIGNMENT(vpMem)            \
        IMG_ASSERT(((IMG_UINTPTR_T)vpMem & 0x3) == 0)
#endif

    /*!
    ******************************************************************************

     @Function  MEMIO_READ_FIELD

    ******************************************************************************/


#if defined __RELEASE_DEBUG__

#define MEMIO_READ_FIELD(vpMem, field)                                                                                                                                                          \
        ( MEMIO_CHECK_ALIGNMENT(vpMem),                                                                                                                                                                 \
        ((IMG_UINT32)(((*((field##_TYPE *)(((IMG_UINTPTR_T)vpMem) + field##_OFFSET))) & field##_MASK) >> field##_SHIFT)) )

#else

#if 1
#define MEMIO_READ_FIELD(vpMem, field)                                                                                                                                                              \
            ((IMG_UINT32)(((*((field##_TYPE *)(((IMG_UINTPTR_T)vpMem) + field##_OFFSET))) & field##_MASK) >> field##_SHIFT))

#else

#define MEMIO_READ_FIELD(vpMem, field)                                                                                                                                                              \
            ((IMG_UINT32)(((*((field##_TYPE *)(((IMG_UINTPTR_T)vpMem) + field##_OFFSET))) >> field##_SHIFT) & field##_LSBMASK) )

#endif

#endif



    /*!
    ******************************************************************************

     @Function  MEMIO_READ_TABLE_FIELD

    ******************************************************************************/
#if defined __RELEASE_DEBUG__

#define MEMIO_READ_TABLE_FIELD(vpMem, field, ui32TabIndex)                                                                                                                                                                                              \
        ( MEMIO_CHECK_ALIGNMENT(vpMem), IMG_ASSERT((ui32TabIndex < field##_NO_ENTRIES) || (field##_NO_ENTRIES == 0)),                                                                           \
        ((IMG_UINT32)(((*((field##_TYPE *)(((IMG_UINTPTR_T)vpMem) + field##_OFFSET + (field##_STRIDE * ui32TabIndex)))) & field##_MASK) >> field##_SHIFT)) )       \

#else

#define MEMIO_READ_TABLE_FIELD(vpMem, field, ui32TabIndex)                                                                                                                                                                                              \
        ((IMG_UINT32)(((*((field##_TYPE *)(((IMG_UINTPTR_T)vpMem) + field##_OFFSET + (field##_STRIDE * ui32TabIndex)))) & field##_MASK) >> field##_SHIFT))         \

#endif


    /*!
    ******************************************************************************

     @Function  MEMIO_READ_REPEATED_FIELD

    ******************************************************************************/
#if defined __RELEASE_DEBUG__

#define MEMIO_READ_REPEATED_FIELD(vpMem, field, ui32RepIndex)                                                                                                                                                                                                                                                           \
        ( MEMIO_CHECK_ALIGNMENT(vpMem), IMG_ASSERT(ui32RepIndex < field##_NO_REPS),                                                                                                                                                                                                                     \
        ((IMG_UINT32)(((*((field##_TYPE *)(((IMG_UINTPTR_T)vpMem) + field##_OFFSET))) & (field##_MASK >> (ui32RepIndex * field##_SIZE))) >> (field##_SHIFT - (ui32RepIndex * field##_SIZE)))) )    \

#else

#define MEMIO_READ_REPEATED_FIELD(vpMem, field, ui32RepIndex)                                                                                                                                                                                                                                                           \
        ( (IMG_UINT32)(((*((field##_TYPE *)(((IMG_UINTPTR_T)vpMem) + field##_OFFSET))) & (field##_MASK >> (ui32RepIndex * field##_SIZE))) >> (field##_SHIFT - (ui32RepIndex * field##_SIZE))) )    \

#endif
    /*!
    ******************************************************************************

     @Function  MEMIO_READ_TABLE_REPEATED_FIELD

    ******************************************************************************/
#if defined __RELEASE_DEBUG__

#define MEMIO_READ_TABLE_REPEATED_FIELD(vpMem, field, ui32TabIndex, ui32RepIndex)                                                                                                                                                                                                                                                                               \
    ( MEMIO_CHECK_ALIGNMENT(vpMem), IMG_ASSERT((ui32TabIndex < field##_NO_ENTRIES) || (field##_NO_ENTRIES == 0)), IMG_ASSERT(ui32RepIndex < field##_NO_REPS), \
    ((IMG_UINT32)(((*((field##_TYPE *)(((IMG_UINTPTR_T)vpMem) + field##_OFFSET + (field##_STRIDE * ui32TabIndex)))) & (field##_MASK >> (ui32RepIndex * field##_SIZE))) >> (field##_SHIFT - (ui32RepIndex * field##_SIZE)))) )      \

#else

#define MEMIO_READ_TABLE_REPEATED_FIELD(vpMem, field, ui32TabIndex, ui32RepIndex)                                                                                                                                                                                                                                                                               \
    ((IMG_UINT32)(((*((field##_TYPE *)(((IMG_UINTPTR_T)vpMem) + field##_OFFSET + (field##_STRIDE * ui32TabIndex)))) & (field##_MASK >> (ui32RepIndex * field##_SIZE))) >> (field##_SHIFT - (ui32RepIndex * field##_SIZE))))        \

#endif

    /*!
    ******************************************************************************

     @Function  MEMIO_WRITE_FIELD

    ******************************************************************************/
#define MEMIO_WRITE_FIELD(vpMem, field, ui32Value)                                                                                                              \
        MEMIO_CHECK_ALIGNMENT(vpMem);                                                                                                                                           \
        (*((field##_TYPE *)(((IMG_UINTPTR_T)vpMem) + field##_OFFSET))) =                                                                           \
        ((*((field##_TYPE *)(((IMG_UINTPTR_T)vpMem) + field##_OFFSET))) & (field##_TYPE)~field##_MASK) |           \
                (field##_TYPE)(( (IMG_UINT32) (ui32Value) << field##_SHIFT) & field##_MASK);

#define MEMIO_WRITE_FIELD_LITE(vpMem, field, ui32Value)                                                                                                 \
        MEMIO_CHECK_ALIGNMENT(vpMem);                                                                                                                                           \
         (*((field##_TYPE *)(((IMG_UINTPTR_T)vpMem) + field##_OFFSET))) =                                                                          \
        ((*((field##_TYPE *)(((IMG_UINTPTR_T)vpMem) + field##_OFFSET))) |                                          \
                (field##_TYPE) (( (IMG_UINT32) (ui32Value) << field##_SHIFT)) );

    /*!
    ******************************************************************************

     @Function  MEMIO_WRITE_TABLE_FIELD
    ******************************************************************************/
#define MEMIO_WRITE_TABLE_FIELD(vpMem, field, ui32TabIndex, ui32Value)                                                                                                                                          \
        MEMIO_CHECK_ALIGNMENT(vpMem); IMG_ASSERT(((ui32TabIndex) < field##_NO_ENTRIES) || (field##_NO_ENTRIES == 0));                                                   \
        (*((field##_TYPE *)(((IMG_UINTPTR_T)vpMem) + field##_OFFSET + (field##_STRIDE * (ui32TabIndex))))) =                                                                               \
                ((*((field##_TYPE *)(((IMG_UINTPTR_T)vpMem) + field##_OFFSET + (field##_STRIDE * (ui32TabIndex))))) & (field##_TYPE)~field##_MASK) |       \
                (field##_TYPE)(( (IMG_UINT32) (ui32Value) << field##_SHIFT) & field##_MASK);

    /*!
    ******************************************************************************

     @Function  MEMIO_WRITE_REPEATED_FIELD

    ******************************************************************************/
#define MEMIO_WRITE_REPEATED_FIELD(vpMem, field, ui32RepIndex, ui32Value)                                                                                                                                       \
        MEMIO_CHECK_ALIGNMENT(vpMem); IMG_ASSERT((ui32RepIndex) < field##_NO_REPS);                                                                                                                             \
        (*((field##_TYPE *)(((IMG_UINTPTR_T)vpMem) + field##_OFFSET))) =                                                                                                                                                   \
        ((*((field##_TYPE *)(((IMG_UINTPTR_T)vpMem) + field##_OFFSET))) & (field##_TYPE)~(field##_MASK >> ((ui32RepIndex) * field##_SIZE)) |                       \
                (field##_TYPE)(( (IMG_UINT32) (ui32Value) << (field##_SHIFT - ((ui32RepIndex) * field##_SIZE))) & (field##_MASK >> ((ui32RepIndex) * field##_SIZE))));

    /*!
    ******************************************************************************

     @Function  MEMIO_WRITE_TABLE_REPEATED_FIELD

    ******************************************************************************/
#define MEMIO_WRITE_TABLE_REPEATED_FIELD(vpMem, field, ui32TabIndex, ui32RepIndex, ui32Value)                                                                                                                                                                           \
        MEMIO_CHECK_ALIGNMENT(vpMem); IMG_ASSERT(((ui32TabIndex) < field##_NO_ENTRIES) || (field##_NO_ENTRIES == 0)); IMG_ASSERT((ui32RepIndex) < field##_NO_REPS);                                             \
        (*((field##_TYPE *)(((IMG_UINTPTR_T)vpMem) + field##_OFFSET + (field##_STRIDE * (ui32TabIndex))))) =                                                                                                                                                               \
                ((*((field##_TYPE *)(((IMG_UINTPTR_T)vpMem) + field##_OFFSET + (field##_STRIDE * (ui32TabIndex))))) & (field##_TYPE)~(field##_MASK >> ((ui32RepIndex) * field##_SIZE))) |          \
                (field##_TYPE)(( (IMG_UINT32) (ui32Value) << (field##_SHIFT - ((ui32RepIndex) * field##_SIZE))) & (field##_MASK >> ((ui32RepIndex) * field##_SIZE)));

#endif


#if (__cplusplus)
}
#endif

#endif

