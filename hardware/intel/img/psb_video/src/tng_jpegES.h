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
*/

#ifndef _PTG_JPEG_H_
#define _PTG_JPEG_H_

#include "img_types.h"
#include "tng_hostdefs.h"
#include "va/va_enc_jpeg.h"

#define QUANT_TABLE_SIZE_BYTES  (64)
#define MTX_MAX_COMPONENTS  (3)
#define MAX_NUMBER_OF_MTX_UNITS 4 // Number of MTX units

typedef enum {
    IMG_ERR_OK                  = 0,    //!< OK
    IMG_ERR_SURFACE_LOCKED      = 1,    //!< The requested surface was locked
    IMG_ERR_MEMORY              = 2,    //!< A memory error occured
    IMG_ERR_FILE                = 3,    //!< A file error occured
    IMG_ERR_NOBUFFERAVAILABLE   = 4,    //!< No buffer was available
    IMG_ERR_COMPLETE            = 5,    //!< Command is complete
    IMG_ERR_INVALID_CONTEXT     = 6,    //!< An invalid context was given
    IMG_ERR_INVALID_SIZE        = 7,    //!< An invalid size was given
    IMG_ERR_TIMEOUT             = 8,    //!< Timeout
    IMG_ERR_UNDEFINED           = -1

} IMG_ERRORCODE;

/*****************************************************************************/
/*  STREAMTYPEW                                                              */
/*  Container to store the stream context                                    */
/*****************************************************************************/
typedef struct {
    IMG_UINT8 *Buffer; /*!< Ptr to the bitstream buffer */
    IMG_UINT32 Offset;  /*!< Offset in the bitstream buffer */
    IMG_UINT32 Limit;
} STREAMTYPEW;


typedef struct {
    IMG_UINT8   aui8LumaQuantParams[QUANT_TABLE_SIZE_BYTES];    //!< Luma quant params
    IMG_UINT8   aui8ChromaQuantParams[QUANT_TABLE_SIZE_BYTES];  //!< Chroma quant params

} JPEG_MTX_QUANT_TABLE;

typedef struct {
    IMG_UINT32  ui32MCUPositionOfScanAndPipeNo; //!< Scan start position in MCUs
    IMG_UINT32  ui32MCUCntAndResetFlag;     //!< [32:2] Number of MCU's to encode or decode, [1] Reset predictors (1=Reset, 0=No Reset)

} MTX_ISSUE_BUFFERS;


typedef struct {
    IMG_UINT32  ui32PhysAddr;   //!< Physical address Component plane in shared memory
    IMG_UINT32  ui32Stride;     //!< Stride of source plane */
    IMG_UINT32  ui32Height;     //!< Height of avaliable data in plane.  shall be a minumum of one MCU high

} COMPONENTPLANE;

typedef struct {
    IMG_UINT32  ui32WidthBlocks;    //!< Width in pixels, shall be a multiple of 8
    IMG_UINT32  ui32HeightBlocks;   //!< Height in pixels, shall be a multiple of 8
    IMG_UINT32  ui32XLimit;         //!< Blocks will not be encoded beyond this
    IMG_UINT32  ui32YLimit;         //!< Blocks will not be encoded beyond this

} MCUCOMPONENT;


typedef struct {
    COMPONENTPLANE  ComponentPlane[MTX_MAX_COMPONENTS]; //!< Array of component plane source information (detailing physical address, stride and height)
    MCUCOMPONENT    MCUComponent[MTX_MAX_COMPONENTS];   //!< Array of Minimum Coded Unit information for each component plane
    IMG_UINT32      ui32ComponentsInScan;               //!< Number of components
    IMG_UINT32      ui32TableA;                         //!< Quantisation table for Luma component
    IMG_UINT16      ui16DataInterleaveStatus;           //!< Source component interleave status (0, C_INTERLEAVE, LC_UVINTERLEAVE or LC_VUINTERLEAVE)
    IMG_UINT16      ui16MaxPipes;                       //!< Maximum number of pipes to use in the encode
} JPEG_MTX_DMA_SETUP;

typedef struct
{
	IMG_UINT32	apWritebackRegions[WB_FIFO_SIZE];       //!< Data section
} JPEG_MTX_WRITEBACK_MEMORY;

typedef struct {
    IMG_UINT32  ui32BytesUsed;      //!<
    IMG_UINT32  ui32BytesEncoded;   //!<
    IMG_UINT32  ui32BytesToEncode;  //!<
    IMG_UINT32  ui32Reserved3;      //!<

} BUFFER_HEADER;

typedef enum {
    BUFFER_FREE = 1,  //!< Buffer is not locked
    HW_LOCK,          //!< Buffer is locked by hardware
    SW_LOCK,          //!< Buffer is locked by software
    NOTDEVICEMEMORY,  //!< Buffer is not a device memory buffer

} LOCK_STATUS;

typedef struct {
    void* pMemInfo;   //!< Pointer to the memory handle for the buffer
    LOCK_STATUS sLock;                  //!< Lock status for the buffer
    IMG_UINT32  ui32Size;               //!< Size in bytes of the buffer
    IMG_UINT32  ui32BytesWritten;       //!< Number of bytes written into buffer

} IMG_BUFFER, IMG_CODED_BUFFER;

typedef struct {
    void *pMemInfo;
    IMG_UINT16 ui16ScanNumber;
    IMG_UINT32 ui32WriteBackVal;
    IMG_INT8 i8PipeNumber; // Doubles as status indicator ( <0 = Awaiting output to CB, 0 = Idle, >0 = Being filled by MTX)
    IMG_UINT32  MTXOpNum; // Handle returned from MTX issuebuff command, can be used to check for completion
    IMG_UINT32 ui32DataBufferSizeBytes;
    IMG_UINT32 ui32DataBufferUsedBytes;
} TOPAZHP_JPEG_BUFFER_INFO;

typedef struct {
    IMG_UINT16 ui16CScan;
    IMG_UINT16 ui16SScan;
    IMG_UINT16 ui16ScansInImage;
    IMG_UINT8 ui8MTXIdleCnt;
    IMG_UINT8 aui8MTXIdleTable[MAX_NUMBER_OF_MTX_UNITS];
    TOPAZHP_JPEG_BUFFER_INFO *aBufferTable;
    IMG_UINT32 ui32NumberMCUsX;
    IMG_UINT32 ui32NumberMCUsY;
    IMG_UINT32 ui32NumberMCUsToEncode;
    IMG_UINT32 ui32NumberMCUsToEncodePerScan;
    IMG_UINT8 ui8NumberOfCodedBuffers;
} TOPAZHP_SCAN_ENCODE_INFO;

typedef struct {
    IMG_UINT32 eFormat;
    IMG_UINT16 ui16Quality;
    IMG_UINT32 ui32OutputWidth;
    IMG_UINT32 ui32OutputHeight;
    IMG_UINT32 ui32InitialCBOffset;

    object_surface_p pSourceSurface;
    void *pMemInfoMTXSetup;
    JPEG_MTX_DMA_SETUP*    pMTXSetup;

    void *pMemInfoWritebackMemory;
    JPEG_MTX_WRITEBACK_MEMORY *pMTXWritebackMemory;
 
    void *pMemInfoTableBlock;
    JPEG_MTX_QUANT_TABLE   *psTablesBlock;

    IMG_UINT32 ui32Offsets[MTX_MAX_COMPONENTS];

    TOPAZHP_SCAN_ENCODE_INFO sScan_Encode_Info;

    /* New added elements after porting */
    void *ctx;
    IMG_INT32 NumCores;
    IMG_CODED_BUFFER jpeg_coded_buf;
    IMG_UINT32 ui32SizePerCodedBuffer;
    MCUCOMPONENT MCUComponent[MTX_MAX_COMPONENTS];
} TOPAZHP_JPEG_ENCODER_CONTEXT;

#define PTG_JPEG_MAX_SCAN_NUM 7
extern struct format_vtable_s tng_JPEGES_vtable;
extern VAStatus tng_jpeg_AppendMarkers(object_context_p obj_context, void *raw_coded_buf);

#endif //_PTG_JPEG_H_
