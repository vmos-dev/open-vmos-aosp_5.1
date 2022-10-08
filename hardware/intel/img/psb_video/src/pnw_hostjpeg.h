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
 *
 */


#ifndef _PNW_HOST_JPEG_H_
#define _PNW_HOST_JPEG_H_

#include <img_types.h>

#define QUANT_TABLE_SIZE_BYTES  (64)             //!<JPEG input quantization table size

#define MTX_MAX_COMPONENTS              (3)                             //<!JPEG input parameter sizes

#define PNW_JPEG_COMPONENTS_NUM (3)

#define PNW_JPEG_HEADER_MAX_SIZE (1024)
/*Limit the scan size to maximum useable (due to it being used as the
 * 16 bit field for Restart Intervals) = 0xFFFF MCUs
 * In reality, worst case allocatable bytes is less than this, something
 * around 0x159739C == 0x4b96 MCUs = 139 x 139 MCUS = 2224 * 2224 pixels, approx.
 * We'll give this upper limit some margin for error, and limit our
 * MCUsPerScan to 2000 * 2000 pixels = 125 * 125 MCUS = 0x3D09 MCUS
 * = 0x116F322 bytes (1170 worst case per MCU)*/
#define JPEG_MAX_MCU_PER_SCAN 0x3D09

#define JPEG_MCU_NUMBER(width, height, eFormat) \
    ((((width) + 15) / 16) * (((height) + 15) / 16) * \
     (((eFormat) == IMG_CODEC_YV16) ? 2 : 1))

#define JPEG_MCU_PER_CORE(width, height, core, eFormat) \
        ((core) > 1 ? (((uint32_t)JPEG_MCU_NUMBER(width, height, eFormat) + (core) - 1) / (core))\
         :(uint32_t)JPEG_MCU_NUMBER(width, height, eFormat))

#define JPEG_SCANNING_COUNT(width, height, core, eFormat) \
    ((uint32_t)(JPEG_MCU_PER_CORE(width, height, core, eFormat) > JPEG_MAX_MCU_PER_SCAN) ? \
      ((uint32_t)(JPEG_MCU_NUMBER(width, height, eFormat) + JPEG_MAX_MCU_PER_SCAN - 1) \
        / JPEG_MAX_MCU_PER_SCAN) \
       : (core))

#define JPEG_MCU_PER_SCAN(width, height, core, eFormat) \
    ((JPEG_MCU_PER_CORE(width, height, core, eFormat) > JPEG_MAX_MCU_PER_SCAN) ? \
    JPEG_MAX_MCU_PER_SCAN : JPEG_MCU_PER_CORE(width, height, core, eFormat))

/*The start address of every segment must align 128bits -- DMA burst width*/
#define JPEG_CODED_BUF_SEGMENT_SIZE(total, width, height, core, eFormat) \
    (((total) - PNW_JPEG_HEADER_MAX_SIZE) / \
     JPEG_SCANNING_COUNT(ctx->Width, ctx->Height, ctx->NumCores, eFormat) \
     & (~0xf))

/*pContext->sScan_Encode_Info.ui32NumberMCUsToEncodePerScan=(pContext->sScan_Encode_Info.ui32NumberMCUsToEncode+pEncContext->i32NumCores-1)/pEncContext->i32NumCores;
 *pContext->sScan_Encode_Info.aBufferTable[ui8Loop].ui32DataBufferSizeBytes = (DATA_BUFFER_SIZE(pContext->sScan_Encode_Info.ui32NumberMCUsToEncodePerScan) +sizeof(BUFFER_HEADER)) + 3 & ~3;
 ui32NumberMCUsToEncode is equal (width/16) * (height/16)
 MAX_MCU_SIZE is 1170
 For 352x288, size of data buffer is 231676.
 The number of data buffer is equal to the number of cores minus one*/
#define PNW_JPEG_CODED_BUF_SIZE(width, height, NumCores)  ((((((width) + 15) / 16) * (((height) + 15) / 16) * MAX_MCU_SIZE ) + 0xf) & ~0xf)


typedef struct {
    unsigned int        ui32Width;              //!< Width of the image component
    unsigned int        ui32Stride;             //!< Stride of the image component
    unsigned int        ui32Step;               //!< Step of the image component
    unsigned int        ui32Height;             //!< Height of the image component

} COMP_INFO;

typedef struct {
    unsigned int        ui32OutputWidth;                //!< Width of the JPEG image
    unsigned int        ui32OutputHeight;               //!< Height of the JPEG image
    unsigned int        ui32Components;                 //!< Number of components in the image ( 1 or 3 )
    COMP_INFO   sCompInfo[3];                   //!< Array containing component info

} IMG_JPEG_INFO;

typedef enum _img_format_ {
    IMG_CODEC_IYUV, /* IYUV */
    IMG_CODEC_IMC2, /* IMC2 */
    IMG_CODEC_PL8,
    IMG_CODEC_PL12,
    IMG_CODEC_NV12,
    IMG_CODEC_YV16,
} IMG_FORMAT;

typedef struct {
    IMG_UINT32  ui32BytesUsed;          //!<
    IMG_UINT32  ui32BytesEncoded;       //!<
    IMG_UINT32  ui32BytesToEncode;      //!<
    IMG_UINT32  ui32Reserved3;          //!<

} BUFFER_HEADER;

typedef enum {
    IMG_ERR_OK                                  = 0,    //!< OK
    IMG_ERR_SURFACE_LOCKED              = 1,    //!< The requested surface was locked
    IMG_ERR_MEMORY                              = 2,    //!< A memory error occured
    IMG_ERR_FILE                                = 3,    //!< A file error occured
    IMG_ERR_NOBUFFERAVAILABLE   = 4,    //!< No buffer was available
    IMG_ERR_COMPLETE                    = 5,    //!< Command is complete
    IMG_ERR_INVALID_CONTEXT             = 6,    //!< An invalid context was given
    IMG_ERR_INVALID_SIZE                = 7,    //!< An invalid size was given
    IMG_ERR_TIMEOUT                             = 8,    //!< Timeout
    IMG_ERR_UNDEFINED                   = -1

} IMG_ERRORCODE;

/*!
 *  *****************************************************************************
 *
 * @details    Struct sent with the MTX_CMDID_ISSUEBUFF command detailing
 *                                 where a scan encode should begin (calculated from the total count of MCUs)
 *                                 and how many MCU's should be processed in this scan.
 *
 * @brief          JPEG structure defining scan start position and how many MCUs to process
 *
 ****************************************************************************/
typedef struct {
    IMG_UINT32  ui32CurrentMTXScanMCUPosition;  //!< Scan start position in MCUs
    IMG_UINT32  ui32MCUCntAndResetFlag;         //!< [32:2] Number of MCU's to encode or decode, [1] Reset predictors (1=Reset, 0=No Reset)

} MTX_ISSUE_BUFFERS;
/*!
 *  *****************************************************************************
 *
 * @details    Struct describing surface component info
 *
 * @brief          Surface component info
 *
 *****************************************************************************/
typedef struct {
    IMG_UINT32 ui32Step;
    IMG_UINT32 ui32Width;
    IMG_UINT32 ui32Height;
    IMG_UINT32 ui32PhysWidth;
    IMG_UINT32 ui32PhysHeight;

} IMG_SURF_COMPONENT_INFO;

/*!
 *  *****************************************************************************
 *
 * @details    Enum describing buffer lock status
 *
 * @brief          Buffer lock status
 *
 ****************************************************************************/
typedef enum {
    BUFFER_FREE = 1,  //!< Buffer is not locked
    HW_LOCK,          //!< Buffer is locked by hardware
    SW_LOCK,          //!< Buffer is locked by software
    NOTDEVICEMEMORY,    //!< Buffer is not a device memory buffer
} LOCK_STATUS;


/*!v
 *  *****************************************************************************
 *
 * @details    Struct describing a coded data buffer
 *
 * @brief          Coded data buffer
 *
 * ****************************************************************************/
typedef struct {
    void* pMemInfo;   //!< Pointer to the memory handle for the buffer
    LOCK_STATUS sLock;                  //!< Lock status for the buffer
    IMG_UINT32  ui32Size;               //!< Size in bytes of the buffer
    IMG_UINT32  ui32BytesWritten;       //!< Number of bytes written into buffer
} IMG_BUFFER, IMG_CODED_BUFFER;



/*!
 *  *****************************************************************************
 *
 * @details    Struct describing a frame
 *
 * @brief          Frame information
 *
 ****************************************************************************/
typedef struct {
    IMG_BUFFER *psBuffer;                           //!< pointer to the image buffer
    IMG_UINT32 ui32Width;                                                       //!< stride of pBuffer
    IMG_UINT32 ui32Height;                                                      //!< height of picture in pBuffer

    IMG_UINT32 ui32ComponentCount;                                      //!< number of colour components used
    IMG_FORMAT eFormat;                                                         //!< Format of the surface

    IMG_UINT32  aui32ComponentOffset[3];                        //!< Offset of the planes within the surface
    IMG_SURF_COMPONENT_INFO     aui32ComponentInfo[3];  //!< Component plane information

} IMG_FRAME, JPEG_SOURCE_SURFACE;
#define MAX_NUMBER_OF_MTX_UNITS 4 // Number of MTX units
/* Number of buffers for encode coded data*/
#define NUMBER_OF_BUFFS 3 // Should be at least equal to number of MTX's for optimal performance
#define SIM_TEST_NUMBER_OF_BUFFS 3 // Should be at least equal to number of MTX's for optimal performance
#define BLOCKCOUNTMAXPERCOLOURPLANE 6 // Would be 10 for max theoretically possible.. 6 is our actual max
#define MAX_MCU_SIZE                    (((8*8)+1)*3 * BLOCKCOUNTMAXPERCOLOURPLANE)
#define SIM_TEST_MCUS_IN_BUFFER 22
#define RL_END_OF_BLOCK         0xff
#define DATA_BUFFER_SIZE(mcus_in_buffer) (MAX_MCU_SIZE*mcus_in_buffer)
#define DMEMORYMCUSATSIXSEVENTY_ESTIMATE 55488

#define C_INTERLEAVE 1
#define LC_UVINTERLEAVE 2
#define LC_VUINTERLEAVE 3

//#define ISCHROMAINTERLEAVED(eSurfaceFormat) ((IMG_UINT) (eSurfaceFormat==IMG_CODEC_Y0UY1V_8888)*LC_UVINTERLEAVE)+((IMG_UINT) (eSurfaceFormat==IMG_CODEC_Y0VY1U_8888)*LC_VUINTERLEAVE)+((IMG_UINT) (eSurfaceFormat==IMG_CODEC_PL12 || eSurfaceFormat==IMG_CODEC_422_PL12)*C_INTERLEAVE)

#define ISCHROMAINTERLEAVED(eSurfaceFormat) ((IMG_UINT) (eSurfaceFormat==IMG_CODEC_PL12 || eSurfaceFormat==IMG_CODEC_NV12)*C_INTERLEAVE)
/*****************************************************************************/
/*  \brief   MAX_COMP_IN_SCAN                                                */
/*                                                                           */
/*  Maximum components that can be encoded in a scan as specified by the     */
/*  JPEG specification.                                                      */
/*****************************************************************************/
#define MAX_COMP_IN_SCAN                4
//#define MTX_CMD_BUF_SIZE      (32)
//#define MTX_MAX_COMPONENTS    (3)
#define PERF            1

/******************************************************************************
  General definitions for static header offsets (used for MJPEG A headers)
 ******************************************************************************/
#define H_QT_OFFSET 46
#define H_HT_OFFSET 184
#define H_SOF_OFFSET 622
#define H_SOS_OFFSET 642
#define H_SOI_OFFSET 656



/******************************************************************************
  General definitions
 ******************************************************************************/
#define BYTE                            8
#define BYTES_IN_INT                    4
#define BITS_IN_INT                     32
#define BLOCK_SIZE                      8
#define PELS_IN_BLOCK                   64

/******************************************************************************
  MJPEG A marker definitions
 ******************************************************************************/
#define MJPEG_APP1                                      0xFFE1

/******************************************************************************
  JPEG marker definitions
 ******************************************************************************/
#define START_OF_IMAGE              0xFFD8
#define SOF_BASELINE_DCT            0xFFC0
#define END_OF_IMAGE                0xFFD9
#define START_OF_SCAN               0xFFDA




/* Definitions for the huffman table specification in the Marker segment */
#define DHT_MARKER                  0xFFC4
#define LH_DC                       0x001F
#define LH_AC                       0x00B5
#define LEVEL_SHIFT                 128

/* Definitions for the quantization table specification in the Marker segment */
#define DQT_MARKER                  0xFFDB
#define ACMAX                       0x03FF
#define DCMAX                       0x07FF
/* Length and precision of the quantization table parameters */
#define LQPQ                        0x00430
#define QMAX                        255
#define CLIP(Number,Max,Min)    if((Number) > (Max)) (Number) = (Max); \
    else if((Number) < (Min)) (Number) = (Min)
#define AVAILABLE                       ( 0 )
#define JPEG_ENCODE_LOCK        ( 0x20 )



//////////////////////////////////////////////////////////////////////////////////////////////
// Structures unchanged by TopazSc
//////////////////////////////////////////////////////////////////////////////////////////////

//typedef struct
//{
//      IMG_UINT32 ui32BytesUsed;
//      IMG_UINT32 ui32BytesEncoded;
//      IMG_UINT32 ui32Reserved2;
//      IMG_UINT32 ui32Reserved3;
//} BUFFER_HEADER;

/*****************************************************************************/
/*  STREAMTYPEW                                                                          */
/*  Container to store the stream context                                    */
/*****************************************************************************/
typedef struct {
    IMG_UINT8 *Buffer; /*!< Ptr to the bitstream buffer */
    IMG_UINT32 Offset;  /*!< Offset in the bitstream buffer */
    IMG_UINT32 Limit;
} STREAMTYPEW;

//typedef struct
//{
//      IMG_UINT32 ui32WidthBlocks ;    /* Width in pixels, shall be a multiple of 8*/
//      IMG_UINT32 ui32HeightBlocks ;   /* Height in pixels, shall be a multiple of 8*/
//      IMG_UINT32 ui32XLimit;                  /* Blocks will not be encoded beyond this */
//      IMG_UINT32 ui32YLimit;                  /* Blocks will not be encoded beyond this */
//} MCUCOMPONENT;
//
//typedef struct
//{
//      IMG_UINT32 ui32PhysAddr;                /* Physical address Component plane in shared memory*/
//      IMG_UINT32 ui32Stride;                  /* Stride of source plane */
//      IMG_UINT32 ui32Height;                  /* Height of avaliable data in plane.  shall be a minumum of one MCU high */
//} COMPONENTPLANE;
//
//
//typedef struct
//{
///// The following values are DMA'd straight across from host
//      COMPONENTPLANE  ComponentPlane[MTX_MAX_COMPONENTS];
//      MCUCOMPONENT    MCUComponent[MTX_MAX_COMPONENTS];
//      IMG_UINT32 ui32ComponentsInScan;        /* Number of compnents */
//      IMG_UINT32 ui32TableA;          /* Quantisation table for Luma component*/
//      IMG_UINT32 ui32DataInterleaveStatus; /*What kind of interleaving is required*/
//} JPEG_MTX_DMA_SETUP;


//typedef struct
//{
////    IMG_UINT32  ui32BufferPhysAddr;         /*  Physical address of buffer table in shared memory*/
//      IMG_UINT32  ui32CurrentMTXScanMCUPosition; // Scan start position in MCUs
//    IMG_UINT32  ui32SizeAndProcMCUCount;/*    [31:16] Valid data in bytes
//                                                                                      [15:0] Number of MCU's to encode or decode */
//} MTX_ISSUE_BUFFERS;




//////////////////////////////////////////////////////////////////////////////////////////////
// Old Topaz structures (retained for backward compatibility with sim tests)
//////////////////////////////////////////////////////////////////////////////////////////////

typedef struct {
    IMG_UINT32 ui32BytesPendingEDMA;
    IMG_UINT32 ui32BytesEncoded;
    IMG_UINT32 ui32BlocksEncoded;
} LEGACY_ENCODE_HEADER;

typedef enum {
    BUFF_BUSY_IN_HW_0,  /* Indicates that this buffer is in use by the hardware */
    BUFF_BUSY_IN_SW,    /* Indicates that this buffer is in use by sw */
    BUFF_AVALIABLE,             /* Indicates this buffer can be submitted to hw */
} LEGACY_BUFFER_STATUS;

/*****************************************************************************/
/*  \brief   _EncType                                                        */
/*                                                                           */
/*  JPEG low level encoder context structure                                           */
/*****************************************************************************/
typedef struct {
    //    STREAMTYPEW streamW;    /*!< Ptr to the stream context */
    const IMG_UINT16 *ACCode[2];     /*!< Ptr to the huffman tables to code AC coeffs */
    const IMG_UINT8  *ACSize[2];     /*!< Ptr to the huffman tables to code AC coeffs */
    const IMG_UINT16 *DCCode[2];     /*!< Ptr to the huffman tables to code DC coeffs */
    const IMG_UINT8  *DCSize[2];     /*!< Ptr to the huffman tables to code DC coeffs */

    IMG_UINT8  *Qmatrix[MAX_COMP_IN_SCAN];/*!< Ptr to the Quant tables */
    IMG_UINT8  uc_num_mcu_x;   /*!< Number of MCUs in the horizontal direction */
    IMG_UINT8  uc_num_mcu_y;   /*!< Number of MCUs in the vertical direction */

    IMG_UINT8  aui8QuantTable[MAX_COMP_IN_SCAN][64]; /* Area for creating tables */
} LEGACY_ENCTYPE;

typedef struct {
    IMG_UINT8*  pData;
    IMG_UINT32  ui32DataBufferSize;
    IMG_UINT32  OpNum;
    IMG_UINT32  ui32numberOfMCUs;
    LEGACY_BUFFER_STATUS        eBufferStatus;
    struct MEMORY_INFO_TAG* pMemInfo;

    IMG_UINT32  ui32CurrentIndexAccessPos;
    IMG_UINT32  ui32CurrentIndexHeaderPos;
#ifdef PERF
    IMG_UINT32  ui32IssueTime;
#endif

} LEGACY_BUFFERINFO;


/*****************************************************************************/
/*  LEGACY_JPEGENC_ITTIAM_PARAMS                                                 */
/*                                                                           */
/*  Pointer to the JPEG encoder parameter context. This structure is used by */
/*  the sample application to pass configuration parameters to the encoder.  */
/*****************************************************************************/
typedef struct {
    IMG_UINT8  uc_num_q_tables;        /*!< Number of Q tables */
    IMG_UINT16 ui16_q_factor;          /*!< Quality factor */
    IMG_UINT8  puc_q_table_id[4];      /*!< Q table ID */
    IMG_UINT8  puc_q_table[4][64];     /*!< Q tables */
    IMG_UINT8  uc_isAbbreviated;       /*!< 0: Interchange ; 1: Abbreviated */
    IMG_UINT8  uc_num_comp_in_scan;    /*!< Number of components in scan (<= 3) */
    IMG_UINT8  puc_comp_id[MAX_COMP_IN_SCAN]; /*!< Component identifier */
    IMG_UINT16 ui16_width;             /*!< Width of the JPEG image */
    IMG_UINT16 ui16_height;            /*!< Height of the JPEG image */
} LEGACY_JPEGENC_ITTIAM_PARAMS;

typedef struct {
    IMG_UINT8  uc_num_comp_in_img;    /*!< Number of components in image */
    IMG_UINT8  puc_comp_id[255];      /*!< Component identifier */
    IMG_UINT8  puc_q_table_id[255];   /*!< Q table id to use */
    IMG_UINT8  puc_huff_table_id[255];/*!< Huff table id to use */
    IMG_UINT8 *puc_comp_buff[255];    /*!< Ptr to the component buff */
    IMG_UINT16 pui16_comp_width[255]; /*!< Width of the component buff */
    IMG_UINT16 pui16_comp_height[255];/*!< Height of the component buff */
    IMG_UINT8  puc_horiz_scale[MAX_COMP_IN_SCAN];/*!< Component horizontal scale factor */
    IMG_UINT8  puc_vert_scale[MAX_COMP_IN_SCAN]; /*!< Component vertical scale factor */

    IMG_UINT8 CompIdtoIndex[MAX_COMP_IN_SCAN];

} LEGACY_JPEGENC_ITTIAM_COMPONENT;

typedef enum {
    LEGACY_JPEG_API_CURRENT_ACTIVE_NONE,
    LEGACY_JPEG_API_CURRENT_ACTIVE_ENCODE,
    LEGACY_JPEG_API_CURRENT_ACTIVE_DECODE,

} LEGACY_JPEG_API_CURRENT_ACTIVE;


/*!
 *  *****************************************************************************
 *
 * @details    Struct describing Minimum Coded Unit information for a single JPEG component plane.
 *  Details the size of blocks to be taken from the plane and the maximum block positions.
 *  Send to firmware in the MTX_CMDID_SETUP command as part of the JPEG_MTX_DMA_SETUP structure
 *
 * @brief          JPEG Minimum Coded Unit Information
 *
 *****************************************************************************/
typedef struct {
    IMG_UINT32  ui32WidthBlocks;        //!< Width in pixels, shall be a multiple of 8
    IMG_UINT32  ui32HeightBlocks;       //!< Height in pixels, shall be a multiple of 8
    IMG_UINT32  ui32XLimit;                     //!< Blocks will not be encoded beyond this
    IMG_UINT32  ui32YLimit;                     //!< Blocks will not be encoded beyond this

} MCUCOMPONENT;


/*!
 *  *****************************************************************************
 *
 *   @details    Struct describing essential information about a single JPEG component plane, defines the
 *         Physical address of the colour plane, its stride and its height.
 *      Send to firmware in the MTX_CMDID_SETUP command as part of the JPEG_MTX_DMA_SETUP structure.
 *
 *   @brief          Basic information for a single JPEG component plane, passed to firmware.
 *
 *****************************************************************************/
typedef struct {
    IMG_UINT32  ui32PhysAddr;   //!< Physical address Component plane in shared memory
    IMG_UINT32  ui32Stride;             //!< Stride of source plane */
    IMG_UINT32  ui32Height;             //!< Height of avaliable data in plane.  shall be a minumum of one MCU high

} COMPONENTPLANE;


/*!
 *  *****************************************************************************
 *
 *  @details    Struct describing essential information required by firmware to encode a scan.
 *         Send to firmware in the MTX_CMDID_SETUP command.
 *
 *  @brief          Setup information for a single JPEG scan.
 *
 *****************************************************************************/
typedef struct {
    COMPONENTPLANE      ComponentPlane[MTX_MAX_COMPONENTS];     //!< Array of component plane source information (detailing physical address, stride and height)
    MCUCOMPONENT        MCUComponent[MTX_MAX_COMPONENTS];       //!< Array of Minimum Coded Unit information for each component plane
    IMG_UINT32          ui32ComponentsInScan;                           //!< Number of components
    IMG_UINT32          ui32TableA;                                                     //!< Quantisation table for Luma component
    IMG_UINT32          ui32DataInterleaveStatus;                       //!< Source component interleave status (0, C_INTERLEAVE, LC_UVINTERLEAVE or LC_VUINTERLEAVE)

} JPEG_MTX_DMA_SETUP;

/* JPEG HW Interface state structure */
typedef struct {
    struct MEMORY_INFO_TAG* pMemInfoMTXSetup;
    JPEG_MTX_DMA_SETUP* pMTXSetup;

    struct MEMORY_INFO_TAG* pMemInfoTableBlock;
    IMG_UINT8           *pui8TablesBlock;

    LEGACY_ENCODE_HEADER* pLastEncodeHeader;

    LEGACY_BUFFERINFO           asRLCBuffers[SIM_TEST_NUMBER_OF_BUFFS];

    IMG_UINT8*          pLocalMemBuffer;

    IMG_UINT32          ui32CurrentIndex;

    /* These are used by by encode */
    IMG_UINT32          rlcWriteIndex;
    IMG_UINT32          rlcReadIndex;

    /* It would be nice to get rid of this */
    LEGACY_BUFFERINFO*  pCurrBuff;
    IMG_UINT8*          pReadFromRLE;

    IMG_UINT32          uiLastDCValue;

    IMG_BOOL bNeedRestart;

#ifdef PERF
    IMG_UINT32 ui32PendingBufferTime;
    IMG_UINT32 ui32ProccessDelay;
    IMG_UINT32 ui32TotalBytesTX;
    IMG_UINT32 ui32TotalTXTime;
#endif

} LEGACY_MTXJPEG_HOST_STATE;

typedef struct {
    LEGACY_JPEGENC_ITTIAM_PARAMS JPEGEncoderParams;
    LEGACY_JPEGENC_ITTIAM_COMPONENT sJPEGEncoderComp;
    IMG_UINT32 BytesUsed;
    IMG_UINT8* pui8ByteBuffer;
    IMG_JPEG_INFO       sJpegInfo ;
    LEGACY_ENCTYPE*     pvLowLevelEncContext;
    LEGACY_JPEG_API_CURRENT_ACTIVE      eCurrentActive;
    struct MEMORY_INFO_TAG *pHWSync;
    LEGACY_MTXJPEG_HOST_STATE* gpMTXHostState;
    IMG_UINT32 ui32MCUsWaitingToBeSentToHW;
    IMG_UINT32 ui32MCUsPendingHWEnc;
    IMG_UINT32 ui32BlocksHWEncoded;
    IMG_UINT8 ui32JPEGBytesWritten;
    IMG_UINT32 MCUsToDo;
    IMG_UINT16 ui16Quality;
    IMG_HANDLE ui32Handles[MTX_MAX_COMPONENTS];
    IMG_UINT32 ui32Offsets[MTX_MAX_COMPONENTS];
    JPEG_SOURCE_SURFACE sSurf;

} LEGACY_JPEG_ENCODER_CONTEXT;



//////////////////////////////////////////////////////////////////////////////////////////////
// New TopazSc structures
//////////////////////////////////////////////////////////////////////////////////////////////

typedef struct {
    unsigned char * pMemInfo;
    IMG_UINT16 ui16ScanNumber;
    IMG_UINT32 ui32WriteBackVal;
    IMG_INT8 i8MTXNumber; // Doubles as status indicator ( <0 = Awaiting output to CB, 0 = Idle, >0 = Being filled by MTX)
    IMG_UINT32  MTXOpNum; // Handle returned from MTX issuebuff command, can be used to check for completion
    IMG_UINT32 ui32DataBufferSizeBytes;
    IMG_UINT32 ui32DataBufferUsedBytes;
} TOPAZSC_JPEG_BUFFER_INFO;

typedef struct {
    IMG_UINT16 ui16CScan; /*The number of scans to be done, ui32NumberMCUsToEncode / ui32NumberMCUsToEncodePerScan*/
    IMG_UINT16 ui16SScan; /*The current index of scan*/
    IMG_UINT16 ui16ScansInImage;
    IMG_UINT8 ui8MTXIdleCnt;
    IMG_UINT8 aui8MTXIdleTable[MAX_NUMBER_OF_MTX_UNITS];
    TOPAZSC_JPEG_BUFFER_INFO *aBufferTable;
    IMG_UINT32 ui32NumberMCUsX; /* Width / 16*/
    IMG_UINT32 ui32NumberMCUsY; /* Height / 16*/
    IMG_UINT32 ui32NumberMCUsToEncode; /*Total number of MCUs to encode*/
    IMG_UINT32 ui32NumberMCUsToEncodePerScan; /*Number of MCUs per scan, should be  ui32NumberMCUsX * ui32NumberMCUsY*/
    IMG_UINT8 ui8NumberOfCodedBuffers;
    IMG_UINT32 ui32CurMCUsOffset;
} TOPAZSC_SCAN_ENCODE_INFO;

typedef struct {
    IMG_UINT8   aui8LumaQuantParams[QUANT_TABLE_SIZE_BYTES];    //!< Luma quant params
    IMG_UINT8   aui8ChromaQuantParams[QUANT_TABLE_SIZE_BYTES];  //!< Chroma quant params

} JPEG_MTX_QUANT_TABLE;

typedef struct context_jpeg_ENC_s {

    IMG_FORMAT eFormat;
    /*IMG_UINT16 ui16Quality;*/
    IMG_UINT32 ui32OutputWidth;
    IMG_UINT32 ui32OutputHeight;
    IMG_UINT32 ui32InitialCBOffset;

    object_surface_p pSourceSurface;
    unsigned char * pMemInfoMTXSetup;
    JPEG_MTX_DMA_SETUP* pMTXSetup;

    unsigned char * pMemInfoTableBlock;
    JPEG_MTX_QUANT_TABLE        *psTablesBlock;

    IMG_UINT32 ui32Offsets[MTX_MAX_COMPONENTS];

    TOPAZSC_SCAN_ENCODE_INFO sScan_Encode_Info;

    IMG_CODED_BUFFER jpeg_coded_buf;

    unsigned char *ctx;
    IMG_UINT32 ui32SizePerCodedBuffer;
    IMG_UINT8  ui8ScanNum;
} TOPAZSC_JPEG_ENCODER_CONTEXT;

//////////////////////////////////////////////////////
//Function Declarations unchanged by TopazSC
//////////////////////////////////////////////////////
int customize_quantization_tables(unsigned char *luma_matrix,
                                  unsigned char *chroma_matrix,
                                  unsigned int ui32Quality);

void SetCompInfoFromFormat(IMG_FORMAT eFormat, IMG_UINT32 ui32Width, IMG_UINT32 ui32Height, IMG_JPEG_INFO       *psJpegInfo);
/* This function will copy data from one buffer to another where the destination has a different stride */
void APP_CopyToWithStride(IMG_UINT8* pSrc, IMG_UINT8* pDest , IMG_UINT32 ui32SrcWidth, IMG_UINT32 ui32SrcPhysWidth, IMG_UINT32 ui32SrcStep, IMG_UINT32 ui32SrcHeight, IMG_UINT32 ui32DestPhysWidth, IMG_UINT32 ui32DestPhysHeight);
IMG_ERRORCODE SetupJPEGSourceSurface(IMG_FORMAT eSurfaceFormat, IMG_UINT32 ui32FrameWidth, IMG_UINT32 ui32FrameHeight, JPEG_SOURCE_SURFACE *pSurf);
//void FlushByteAlignedBuffer(IMG_UINT32 *pui_buff, IMG_UINT32 *ui_bytes_to_flush, FILE *pf_out);

//////////////////////////////////////////////////////
//Legacy function declarations - most of these will eventually be removed
//////////////////////////////////////////////////////
IMG_ERRORCODE Legacy_PrepareHeader(LEGACY_JPEG_ENCODER_CONTEXT * pContext, IMG_CODED_BUFFER *pCBuffer);
//IMG_ERRORCODE Legacy_AllocateCodedDataBuffers(LEGACY_JPEG_ENCODER_CONTEXT *pContext);
//IMG_ERRORCODE Legacy_host_InitialiseHardware(LEGACY_JPEG_ENCODER_CONTEXT *pContext);
//IMG_ERRORCODE Legacy_IMG_JPEG_EndPicture(IMG_HENC_CONTEXT hEncContext, IMG_CODED_BUFFER *pCBuffer);
//IMG_ERRORCODE Legacy_IMG_JPEG_FreeBuffer(LEGACY_JPEG_ENCODER_CONTEXT * pContext, IMG_CODED_BUFFER **ppCBuffer);
//IMG_ERRORCODE Legacy_IMG_JPEG_DeAllocateFrames(LEGACY_JPEG_ENCODER_CONTEXT *pContext,IMG_FRAME_ARRAY **pFrameArray);
//IMG_ERRORCODE Legacy_IMG_JPEG_EncoderInitialise(IMG_UINT16 ui16Width, IMG_UINT16 ui16Height, IMG_FORMAT eFormat, LEGACY_JPEG_ENCODER_CONTEXT ** ppContext);
//IMG_ERRORCODE Legacy_IMG_JPEG_EncoderDeInitialise(LEGACY_JPEG_ENCODER_CONTEXT * pContext);
//IMG_ERRORCODE Legacy_IMG_JPEG_AllocateFrames(LEGACY_JPEG_ENCODER_CONTEXT *pContext, IMG_UINT16 ui16ArraySize,IMG_FRAME_ARRAY **ppFrameArray);
//IMG_ERRORCODE Legacy_IMG_JPEG_AllocateCodedBuffer(LEGACY_JPEG_ENCODER_CONTEXT * pContext, IMG_UINT32 ui32CBufferSize, IMG_CODED_BUFFER **ppCBuffer);
//IMG_ERRORCODE Legacy_IMG_JPEG_StartPicture(IMG_HENC_CONTEXT hEncContext, IMG_UINT16 ui16Quality, IMG_CODED_BUFFER *pCBuffer,IMG_FRAME *pTFrame);

//////////////////////////////////////////////////////
//TopazSc Function Declarations
//////////////////////////////////////////////////////
IMG_ERRORCODE PrepareHeader(TOPAZSC_JPEG_ENCODER_CONTEXT * pContext, IMG_CODED_BUFFER *pCBuffer, IMG_UINT32 ui32StartOffset, IMG_BOOL bIncludeHuffmanTables);
IMG_ERRORCODE AllocateCodedDataBuffers(TOPAZSC_JPEG_ENCODER_CONTEXT *pContext);
//IMG_ERRORCODE IMG_JPEG_EndPicture(IMG_HENC_CONTEXT hEncContext, IMG_CODED_BUFFER *pCBuffer);
//IMG_ERRORCODE IMG_JPEG_FreeBuffer(TOPAZSC_JPEG_ENCODER_CONTEXT * pContext, IMG_CODED_BUFFER **ppCBuffer);
//IMG_ERRORCODE IMG_JPEG_DeAllocateFrames(TOPAZSC_JPEG_ENCODER_CONTEXT *pContext,IMG_FRAME_ARRAY **pFrameArray);
//IMG_ERRORCODE IMG_JPEG_EncoderInitialise(IMG_HENC_CONTEXT hEncContext,IMG_UINT16 ui16Width, IMG_UINT16 ui16Height, IMG_FORMAT eFormat, TOPAZSC_JPEG_ENCODER_CONTEXT ** ppContext);
//IMG_ERRORCODE IMG_JPEG_EncoderDeInitialise(TOPAZSC_JPEG_ENCODER_CONTEXT * pContext);
//IMG_ERRORCODE IMG_JPEG_AllocateFrames(TOPAZSC_JPEG_ENCODER_CONTEXT *pContext, IMG_UINT32 ui32ArraySize,IMG_FRAME_ARRAY **ppFrameArray);
IMG_ERRORCODE IMG_JPEG_AllocateCodedBuffer(IMG_UINT32 ui32CBufferSize, IMG_CODED_BUFFER **ppCBuffer);
//IMG_ERRORCODE IMG_JPEG_StartPicture(IMG_HENC_CONTEXT hEncContext, IMG_UINT16 ui16Quality, IMG_CODED_BUFFER *pCBuffer,IMG_FRAME *pTFrame, IMG_UINT32 ui32StartOffset, IMG_BOOL bIncludeHuffmanTables);

IMG_ERRORCODE InitializeJpegEncode(TOPAZSC_JPEG_ENCODER_CONTEXT * pContext, object_surface_p pTFrame);
IMG_ERRORCODE SetupJPEGTables(TOPAZSC_JPEG_ENCODER_CONTEXT * pContext, IMG_CODED_BUFFER *pCBuffer,  object_surface_p pTFrame);
IMG_ERRORCODE SubmitScanToMTX(TOPAZSC_JPEG_ENCODER_CONTEXT *pContext, IMG_UINT16 ui16BCnt, IMG_INT8 i8MTXNumber, IMG_UINT32 ui32NoMCUsToEncode);
void pnw_jpeg_set_default_qmatix(unsigned char *pMemInfoTableBlock);
void fPutBitsToBuffer(STREAMTYPEW *BitStream, IMG_UINT8 NoOfBytes, IMG_UINT32 ActualBits);
#endif /*_HOST_JPEG_H_*/
