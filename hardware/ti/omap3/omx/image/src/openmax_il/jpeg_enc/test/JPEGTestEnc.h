
/*
 *  Copyright 2001-2008 Texas Instruments - http://www.ti.com/
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 * limitations under the License.
 */
/* ================================================================================
*             Texas Instruments OMAP(TM) Platform Software
*  (c) Copyright Texas Instruments, Incorporated.  All Rights Reserved.
*
*  Use of this software is controlled by the terms and conditions found
*  in the license agreement under which this software has been supplied.
* ================================================================================= */
/**
* @file JPEGTest.h
*
* This file implements OMX Component for JPEG encoder that
* is fully compliant with the OMX specification 1.5.
*
* @path  $(CSLPATH)\src
*
* @rev  0.1
*/
/* -------------------------------------------------------------------------------- */
/* ================================================================================
*!
*! Revision History
*! ===================================
*!
*! 22-May-2006 mf: Revisions appear in reverse chronological order;
*! that is, newest first.  The date format is dd-Mon-yyyy.
* ================================================================================= */
#ifndef OMX_JPEGTEST_H
#define OMX_JPEGTEST_H


typedef struct IMAGE_INFO {
    int nWidth;
    int nHeight ;
    int nFormat;
    int nComment;
    char* pCommentString;
    int nThumbnailWidth_app0;
    int nThumbnailHeight_app0;
    int nThumbnailWidth_app1;
    int nThumbnailHeight_app1;
    int nThumbnailWidth_app5;
    int nThumbnailHeight_app5;
    int nThumbnailWidth_app13;
    int nThumbnailHeight_app13;
    int nDRI;
    OMX_BOOL bAPP0;
    OMX_BOOL bAPP1;
    OMX_BOOL bAPP5;
    OMX_BOOL bAPP13;
} IMAGE_INFO;

typedef struct JPEGE_EVENTPRIVATE {
    OMX_EVENTTYPE eEvent;
    OMX_PTR pAppData;
    OMX_PTR pEventInfo;
    OMX_U32 nData1;
    OMX_U32 nData2;
}JPEGE_EVENTPRIVATE;

#define MALLOC(_pStruct_, _sName_)  \
    _pStruct_ = (_sName_*)malloc(sizeof(_sName_));  \
    if(_pStruct_ == NULL){  \
        error = OMX_ErrorInsufficientResources;  \
        goto EXIT;  \
    }\
    memset((void *)_pStruct_, 0, sizeof(_sName_))

#define FREE(_ptr)   \
{                     \
    if (_ptr != NULL) { \
        free(_ptr);   \
        _ptr = NULL; \
    }                \
}

#define OMX_CONF_CHECK_CMD(_ptr1, _ptr2, _ptr3) \
{                       \
    if(!_ptr1 || !_ptr2 || !_ptr3){     \
        eError = OMX_ErrorBadParameter;     \
    goto OMX_CONF_CMD_BAIL;         \
    }                       \
}
#ifdef UNDER_CE
#define sleep Sleep
#endif


/*#define OMX_DEB*/
#ifdef OMX_DEB
    #define PRINT(...) fprintf(stdout,__VA_ARGS__)
#else
    #define PRINT(...)
#endif

#define DSP_MMU_FAULT_HANDLING

#endif /* OMX_JPEGTEST_H */

