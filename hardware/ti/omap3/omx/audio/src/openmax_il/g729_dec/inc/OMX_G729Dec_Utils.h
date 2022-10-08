
/*
 * Copyright (C) Texas Instruments - http://www.ti.com/
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
/* =============================================================================
 *             Texas Instruments OMAP (TM) Platform Software
 *  (c) Copyright Texas Instruments, Incorporated.  All Rights Reserved.
 *
 *  Use of this software is controlled by the terms and conditions found
 *  in the license agreement under which this software has been supplied.
 * =========================================================================== */
/**
 * @file OMX_G729Dec_Utils.h
 *
 * This header file contains data and function prototypes for G729 DECODER OMX 
 *
 * @path  $(OMAPSW_MPU)\linux\audio\src\openmax_il\g729_dec\inc
 *
 * @rev  0.1
 */
/* ----------------------------------------------------------------------------- 
 *! 
 *! Revision History 
 *! ===================================
 *! Date         Author(s)            Version  Description
 *! ---------    -------------------  -------  ---------------------------------
 *! 03-Jan-2007  A.Donjon             0.1      Code update for G729 DECODER
 *! 
 *!
 * ================================================================================= */
/* ------compilation control switches -------------------------*/

#ifndef OMX_G729DEC_UTILS__H
#define OMX_G729DEC_UTILS__H

/* ======================================================================= */
/**
 * @def    G729DEC_MAJOR_VER              Define value for "major" version
 */
/* ======================================================================= */
#define  G729DEC_MAJOR_VER 0x1

/* ======================================================================= */
/**
 * @def    G729DEC_MINOR_VER              Define value for "minor" version
 */
/* ======================================================================= */
#define  G729DEC_MINOR_VER 0x1

/* ======================================================================= */
/**
 * @def    NOT_USED                            Define a not used value
 */
/* ======================================================================= */
#define NOT_USED 10

/* ======================================================================= */
/**
 * @def    NORMAL_BUFFER                       Define a normal buffer value
 */
/* ======================================================================= */
#define NORMAL_BUFFER 0

/* ======================================================================= */
/**
 * @def    OMX_G729DEC_DEFAULT_SEGMENT        Define the default segment
 */
/* ======================================================================= */
#define OMX_G729DEC_DEFAULT_SEGMENT (0)

/* ======================================================================= */
/**
 * @def    OMX_G729DEC_SN_TIMEOUT            Define a value for SN Timeout
 */
/* ======================================================================= */
#define OMX_G729DEC_SN_TIMEOUT (-1)

/* ======================================================================= */
/**
 * @def    OMX_G729DEC_SN_PRIORITY           Define a value for SN Priority
 */
/* ======================================================================= */
#define OMX_G729DEC_SN_PRIORITY (10)

/* ======================================================================= */
/**
 * @def    OMX_G729DEC_NUM_DLLS              Define a num of DLLS to be used
 */
/* ======================================================================= */
#define OMX_G729DEC_NUM_DLLS (2)

/* ======================================================================= */
/**
 * @def    G729DEC_USN_DLL_NAME             Path & Name of USN DLL to be used
 *                                           at initialization
 */
/* ======================================================================= */
#ifdef UNDER_CE
#define G729DEC_USN_DLL_NAME "\\windows\\usn.dll64P"
#else
#define G729DEC_USN_DLL_NAME "usn.dll64P"
#endif

/* ======================================================================= */
/**
 * @def    G729DEC_USN_DLL_NAME             Path & Name of DLL to be useda
 *                                           at initialization
 */
/* ======================================================================= */
#ifdef UNDER_CE
#define G729DEC_DLL_NAME "\\windows\\g729dec_sn.dll64P"
#else
#define G729DEC_DLL_NAME "g729dec_sn.dll64P"
#endif

/****************************************************************
 *  INCLUDE FILES                                                 
 ****************************************************************/
/* ----- system and platform files ----------------------------*/

/*-------program files ----------------------------------------*/
#include <OMX_Component.h>
#include "OMX_G729Decoder.h"
/****************************************************************
 * EXTERNAL REFERENCES NOTE : only use if not found in header file
 ****************************************************************/
/*--------data declarations -----------------------------------*/
/*--------function prototypes ---------------------------------*/

/****************************************************************
 * PUBLIC DECLARATIONS Defined here, used elsewhere
 ****************************************************************/
/*--------data declarations -----------------------------------*/

/*--------function prototypes ---------------------------------*/

/****************************************************************
 * PRIVATE DECLARATIONS Defined here, used only here
 ****************************************************************/
/*--------data declarations -----------------------------------*/
/*--------function prototypes ---------------------------------*/


OMX_ERRORTYPE G729DECGetCorresponding_LCMLHeader(G729DEC_COMPONENT_PRIVATE* pComponentPrivate,
                                                 OMX_U8 *pBuffer,
                                                 OMX_DIRTYPE eDir,
                                                 LCML_G729DEC_BUFHEADERTYPE **ppLcmlHdr);

OMX_ERRORTYPE G729DECLCML_Callback (TUsnCodecEvent event,void * args [10]);

OMX_ERRORTYPE G729DECFill_LCMLInitParams(OMX_HANDLETYPE pHandle,
                                         LCML_DSP *plcml_Init,OMX_U16 arr[]);


OMX_ERRORTYPE G729DECGetBufferDirection(OMX_BUFFERHEADERTYPE *pBufHeader, OMX_DIRTYPE *eDir);
OMX_U32 G729DECHandleCommand (G729DEC_COMPONENT_PRIVATE *pComponentPrivate);

OMX_ERRORTYPE G729DECHandleDataBuf_FromApp(OMX_BUFFERHEADERTYPE *pBufHeader,
                                           G729DEC_COMPONENT_PRIVATE *pComponentPrivate);


OMX_ERRORTYPE G729DECHandleDataBuf_FromLCML(G729DEC_COMPONENT_PRIVATE* pComponentPrivate);


void  AddHeader(BYTE **pFileBuf);
void  ResetPtr(BYTE **pFileBuf);
OMX_HANDLETYPE G729DECGetLCMLHandle(G729DEC_COMPONENT_PRIVATE* pComponentPrivate);
OMX_ERRORTYPE G729DECFreeLCMLHandle(G729DEC_COMPONENT_PRIVATE* pComponentPrivate);
OMX_ERRORTYPE G729DEC_CleanupInitParams(OMX_HANDLETYPE pComponent);
void G729DEC_SetPending(G729DEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir);
void G729DEC_ClearPending(G729DEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir) ;
OMX_U32 G729DEC_IsPending(G729DEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir);
OMX_ERRORTYPE G729DECFill_LCMLInitParamsEx(OMX_HANDLETYPE pComponent);
OMX_U32 G729DEC_IsValid(G729DEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_U8 *pBuffer, OMX_DIRTYPE eDir) ;
OMX_ERRORTYPE G729DEC_TransitionToIdle(G729DEC_COMPONENT_PRIVATE *pComponentPrivate);

/*--------macros ----------------------------------------------*/

#endif
