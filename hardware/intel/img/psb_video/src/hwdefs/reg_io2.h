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


/******************************************************************************

 @File         reg_io2.h

 @Title        MSVDX Offsets

 @Platform     Independent

 @Description  </b>\n

******************************************************************************/
#if !defined (__REG_IO2_H__)
#define __REG_IO2_H__

#if (__cplusplus)
extern "C" {
#endif

#include "img_types.h"

#ifdef DOXYGEN_WILL_SEE_THIS
    /*!
    ******************************************************************************

     @Function  REGIO_READ_FIELD

     @Description

     This macro is used to extract a field from a register.

     @Input             ui32RegValue:           The register value.

     @Input             group:          The name of the group containing the register from which
                                                the field is to be extracted.

     @Input             reg:            The name of the register from which the field is to
                                                be extracted.

     @Input             field:          The name of the field to be extracted.

     @Return    IMG_UINT32:     The value of the field - right aligned.

    ******************************************************************************/
    IMG_UINT32 REGIO_READ_FIELD(IMG_UINT32      ui32RegValue, group, reg, field);

    /*!
    ******************************************************************************

     @Function  REGIO_READ_REPEATED_FIELD

     @Description

     This macro is used to extract the value of a repeated from a register.

     @Input             ui32RegValue:           The register value.

     @Input             group:          The name of the group containing the register from which
                                                the field is to be extracted.

     @Input             reg:            The name of the register from which the field is to
                                                be extracted.

     @Input             field:          The name of the field to be extracted.

     @Input             ui32RepIndex:           The repeat index of the field to be extracted.

     @Return    IMG_UINT32:     The value of the field - right aligned.

    ******************************************************************************/
    IMG_UINT32 REGIO_READ_REPEATED_FIELD(IMG_UINT32     ui32RegValue, group, reg, field, IMG_UINT32 ui32RepIndex);

    /*!
    ******************************************************************************

     @Function  REGIO_READ_REGISTER

     @Description

     This macro is used to read a register.

     @Input             ui32DevId:              The device Id within the group.

     @Input             group:          The name of the group containing the register to be
                                                read.

     @Input             reg:            The name of the register to be read.

     @Return    IMG_UINT32:     The value of the register.

    ******************************************************************************/
    IMG_UINT32 REGIO_READ_REGISTER(IMG_UINT32   ui32DevId, group, reg);

    /*!
    ******************************************************************************

     @Function  REGIO_READ_TABLE_REGISTER

     @Description

     This macro is used to read a register from a table.

     @Input             ui32DevId:              The device Id within the group.

     @Input             group:          The name of the group containing the register to be
                                                read.

     @Input             reg:            The name of the register to be read.

     @Input             ui32TabIndex:   The index of the table entry to be read.

     @Return    IMG_UINT32:     The value of the register.

    ******************************************************************************/
    IMG_UINT32 REGIO_READ_TABLE_REGISTER(IMG_UINT32     ui32DevId, group, reg, IMG_UINT32 ui32TabIndex);

    /*!
    ******************************************************************************

     @Function  REGIO_WRITE_FIELD

     @Description

     This macro is used to update the value of a field in a register.

     @Input             ui32RegValue:   The register value - which gets updated.

     @Input             group:          The name of the group containing the register into which
                                                the field is to be written.

     @Input             reg:            The name of the register into which the field is to
                                                be written.

     @Input             field:          The name of the field to be updated.

     @Input             ui32Value:      The value to be written to the field - right aligned.

     @Return    None.

    ******************************************************************************/
    IMG_VOID REGIO_WRITE_FIELD(IMG_UINT32       ui32RegValue, group, reg, field, IMG_UINT32     ui32Value);

    /*!
    ******************************************************************************

     @Function  REGIO_WRITE_REPEATED_FIELD

     @Description

     This macro is used to update a repeated field in a packed memory
     based structure.

     @Input             ui32RegValue:   The register value - which gets updated.

     @Input             group:          The name of the group containing the register into which
                                                the field is to be written.

     @Input             reg:            The name of the register into which the field is to
                                                be written.

     @Input             field:          The name of the field to be updated.

     @Input             ui32Value:      The value to be written to the field - right aligned.

     @Return    None.

    ******************************************************************************/
    IMG_VOID REGIO_WRITE_REPEATED_FIELD(IMG_UINT32      ui32RegValue, group, reg, field, IMG_UINT32 ui32RepIndex, IMG_UINT32    ui32Value);


    /*!
    ******************************************************************************

     @Function  REGIO_WRITE_REGISTER

     @Description

     This macro is used to write a register.

     @Input             ui32DevId:              The device Id within the group.

     @Input             group:          The name of the group containing the register to be
                                                written.

     @Input             reg:            The name of the register to be written.

     @Input             ui32RegValue:   The value to be written to the register.

     @Return    None.

    ******************************************************************************/
    IMG_VOID REGIO_WRITE_REGISTER(IMG_UINT32    ui32DevId, group, reg, IMG_UINT32 ui32RegValue);

    /*!
    ******************************************************************************

     @Function  REGIO_WRITE_TABLE_REGISTER

     @Description

     This macro is used to wrirte a register in a table.

     @Input             ui32DevId:              The device Id within the group.

     @Input             group:          The name of the group containing the register to be
                                                written.

     @Input             reg:            The name of the register to be written.

     @Input             ui32TabIndex:   The index of the table entry to be written.

     @Input             ui32RegValue:   The value to be written to the register.

     @Return    None.

    ******************************************************************************/
    IMG_VOID REGIO_WRITE_TABLE_REGISTER(IMG_UINT32      ui32DevId, group, reg, IMG_UINT32 ui32TabIndex, IMG_UINT32 ui32RegValue);

    /*!
    ******************************************************************************

     @Function  REGIO_WRITE_OFFSET_REGISTER

     @Description

     This macro is used to write a register at an offset from a given register.

     @Input             ui32DevId:              The device Id within the group.

     @Input             group:          The name of the group containing the register to be
                                                written.

     @Input             reg:            The name of the base register to be written.

     @Input             ui32Offset:     The offset (eg. 0,1,2,...) of the register to be written.

     @Input             ui32RegValue:   The value to be written to the register.

     @Return    None.

    ******************************************************************************/
    IMG_VOID REGIO_WRITE_TABLE_REGISTER(IMG_UINT32      ui32DevId, group, reg, IMG_UINT32 ui32Offset, IMG_UINT32 ui32RegValue);

#else

    /*!
    ******************************************************************************

     @Function  REGIO_READ_FIELD

    ******************************************************************************/
#if 1
#define REGIO_READ_FIELD(ui32RegValue, group, reg, field)                                                       \
        ((ui32RegValue & group##_##reg##_##field##_MASK) >> group##_##reg##_##field##_SHIFT)

#else

#define REGIO_READ_FIELD(ui32RegValue, group, reg, field)                                                       \
        ((ui32RegValue >> group##_##reg##_##field##_SHIFT) & group##_##reg##_##field##_LSBMASK)

#endif

    /*!
    ******************************************************************************

     @Function  REGIO_READ_REPEATED_FIELD

    ******************************************************************************/
#define REGIO_READ_REPEATED_FIELD(ui32RegValue, group, reg, field, ui32RepIndex)                                                                                                                                                                                                        \
        ( IMG_ASSERT(ui32RepIndex < group##_##reg##_##field##_NO_REPS),                                                                                                                                                                                                                                                                 \
        ((ui32RegValue & (group##_##reg##_##field##_MASK >> (ui32RepIndex * group##_##reg##_##field##_SIZE)))   \
        >> (group##_##reg##_##field##_SHIFT - (ui32RepIndex * group##_##reg##_##field##_SIZE))) )

    /*!
    ******************************************************************************

     @Function  REGIO_READ_REGISTER

    ******************************************************************************/
#define REGIO_READ_REGISTER(ui32DevId, group, reg)                      \
        group##_ReadRegister(ui32DevId, group##_##reg##_OFFSET)

    /*!
    ******************************************************************************

     @Function  REGIO_READ_TABLE_REGISTER

     ******************************************************************************/
#define REGIO_READ_TABLE_REGISTER(ui32DevId, group, reg, ui32TabIndex)  \
        ( IMG_ASSERT(ui32TabIndex < group##_##reg##_NO_ENTRIES),                                \
          group##_ReadRegister(ui32DevId, (group##_##reg##_OFFSET+(ui32TabIndex*group##_##reg##_STRIDE))) )

    /*!
    ******************************************************************************

     @Function  REGIO_WRITE_FIELD

    ******************************************************************************/
#define REGIO_WRITE_FIELD(ui32RegValue, group, reg, field, ui32Value)                                       \
        (ui32RegValue) =                                                                                                                                                \
        ((ui32RegValue) & ~(group##_##reg##_##field##_MASK)) |                                                                  \
                (((ui32Value) << (group##_##reg##_##field##_SHIFT)) & (group##_##reg##_##field##_MASK));

#ifndef DEBUG
#define REGIO_ASSERT( x ) ((void)0)
#else
#define REGIO_ASSERT( x ) IMG_ASSERT( x )
#endif

#define REGIO_WRITE_FIELD_MASKEDLITE(ui32RegValue, group, reg, field, ui32Value)                                                                                        \
        do {    \
                REGIO_ASSERT( ((ui32RegValue) & (group##_##reg##_##field##_MASK) ) == 0 );                                                                                      \
                (ui32RegValue) |= (((ui32Value) << (group##_##reg##_##field##_SHIFT)) & (group##_##reg##_##field##_MASK));                      \
        } while(0)

#define REGIO_WRITE_FIELD_LITE(ui32RegValue, group, reg, field, ui32Value)                                                                                              \
        do { \
                REGIO_ASSERT( ((ui32RegValue) & (group##_##reg##_##field##_MASK) ) == 0 );                                                                                      \
                REGIO_ASSERT( ( ( (ui32Value) << (group##_##reg##_##field##_SHIFT) ) & (~(group##_##reg##_##field##_MASK))) == 0);      \
                (ui32RegValue) |=  ( (ui32Value) << (group##_##reg##_##field##_SHIFT) ) ;       \
        } while(0)

    /*!
    ******************************************************************************

     @Function  REGIO_WRITE_REPEATED_FIELD

    ******************************************************************************/
#define REGIO_WRITE_REPEATED_FIELD(ui32RegValue, group, reg, field, ui32RepIndex, ui32Value)                                                                                            \
        IMG_ASSERT(ui32RepIndex < group##_##reg##_##field##_NO_REPS);                                                                                                                                                           \
        ui32RegValue =                                                                                                                                                                                                                                                  \
        (ui32RegValue & ~(group##_##reg##_##field##_MASK >> (ui32RepIndex * group##_##reg##_##field##_SIZE))) |         \
                (ui32Value << (group##_##reg##_##field##_SHIFT - (ui32RepIndex * group##_##reg##_##field##_SIZE)) & (group##_##reg##_##field##_MASK >> (ui32RepIndex * group##_##reg##_##field##_SIZE)));

    /*!
    ******************************************************************************

     @Function  REGIO_WRITE_REGISTER

    ******************************************************************************/
#define REGIO_WRITE_REGISTER(ui32DevId, group, reg, ui32RegValue)       \
        group##_WriteRegister(ui32DevId, (group##_##reg##_OFFSET), (ui32RegValue))

    /*!
    ******************************************************************************

     @Function  REGIO_WRITE_TABLE_REGISTER

    ******************************************************************************/
#define REGIO_WRITE_TABLE_REGISTER(ui32DevId, group, reg, ui32TabIndex, ui32RegValue)           \
                group##_WriteRegister(ui32DevId, (group##_##reg##_OFFSET+(ui32TabIndex*group##_##reg##_STRIDE)), ui32RegValue)


    /*!
    ******************************************************************************

     @Function  REGIO_WRITE_OFFSET_REGISTER

    ******************************************************************************/
#define REGIO_WRITE_OFFSET_REGISTER(ui32DevId, group, reg, ui32Offset, ui32RegValue)            \
          group##_WriteRegister(ui32DevId, (group##_##reg##_OFFSET+(ui32Offset*4)), ui32RegValue)


#endif


#if (__cplusplus)
}
#endif

#endif

