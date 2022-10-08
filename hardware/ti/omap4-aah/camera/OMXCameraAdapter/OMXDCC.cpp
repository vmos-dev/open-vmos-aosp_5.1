/*
 * Copyright (C) Texas Instruments - http://www.ti.com/
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
* @file OMXDCC.cpp
*
* This file contains functionality for loading the DCC binaries.
*
*/

#include "CameraHal.h"
#include "OMXCameraAdapter.h"
#include "ErrorUtils.h"
#include "OMXDCC.h"
#include <utils/String8.h>
#include <utils/Vector.h>

namespace Ti {
namespace Camera {

android::String8 DCCHandler::DCCPath("/data/misc/camera/");
bool DCCHandler::mDCCLoaded = false;

status_t DCCHandler::loadDCC(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE dccError = OMX_ErrorNone;

    if (!mDCCLoaded) {
        dccError = initDCC(hComponent);
        if (dccError != OMX_ErrorNone) {
            CAMHAL_LOGE(" Error in DCC Init");
        }

        mDCCLoaded = true;
    }

    return Utils::ErrorUtils::omxToAndroidError(dccError);
}

OMX_ERRORTYPE DCCHandler::initDCC(OMX_HANDLETYPE hComponent)
{
    OMX_TI_PARAM_DCCURIINFO param;
    OMX_PTR ptempbuf;
    OMX_U16 nIndex = 0;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    int ret;
    OMX_S32 status = 0;
    android::Vector<android::String8 *> dccDirs;
    OMX_U16 i;
    MemoryManager memMgr;
    CameraBuffer *dccBuffer = NULL;
    int dccbuf_size = 0;
    OMX_INIT_STRUCT_PTR(&param, OMX_TI_PARAM_DCCURIINFO);

    // Read the the DCC URI info
    for (nIndex = 0; eError != OMX_ErrorNoMore; nIndex++) {
        param.nIndex = nIndex;
        eError = OMX_GetParameter(hComponent,
                                  ( OMX_INDEXTYPE )OMX_TI_IndexParamDccUriInfo,
                                  &param);

        if (eError == OMX_ErrorNone) {
            CAMHAL_LOGD("DCC URI's %s ", param.sDCCURI);
            android::String8 *dccDir = new android::String8();
            if ( NULL != dccDir ) {
                dccDir->clear();
                dccDir->append(DCCPath);
                dccDir->append((const char *) param.sDCCURI);
                dccDir->append("/");
                dccDirs.add(dccDir);
            } else {
                CAMHAL_LOGE("DCC URI not allocated");
                eError = OMX_ErrorInsufficientResources;
                goto EXIT;
            }
        }
    }

    // setting  back errortype OMX_ErrorNone
    if (eError == OMX_ErrorNoMore) {
        eError = OMX_ErrorNone;
    }

    dccbuf_size = readDCCdir(NULL, dccDirs);
    if(dccbuf_size <= 0) {
        CAMHAL_LOGE("No DCC files found, switching back to default DCC");
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    dccbuf_size = ((dccbuf_size + 4095 )/4096)*4096;

    if ( memMgr.initialize() != NO_ERROR ) {
        CAMHAL_LOGE("DCC memory manager initialization failed!!!");
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    dccBuffer = memMgr.allocateBufferList(0, 0, NULL, dccbuf_size, 1);
    if ( NULL == dccBuffer ) {
        CAMHAL_LOGE("DCC buffer allocation failed!!!");
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    dccbuf_size = readDCCdir(dccBuffer[0].mapped, dccDirs);
    CAMHAL_ASSERT_X(dccbuf_size > 0,"ERROR in copy DCC files into buffer");

    eError = sendDCCBufPtr(hComponent, dccBuffer);

EXIT:

    for (i = 0; i < dccDirs.size(); i++) {
        android::String8 *dccDir = dccDirs.itemAt(0);
        dccDirs.removeAt(0);
        delete dccDir;
    }

    if ( NULL != dccBuffer ) {
        memMgr.freeBufferList(dccBuffer);
    }

     return eError;
}

OMX_ERRORTYPE DCCHandler::sendDCCBufPtr(OMX_HANDLETYPE hComponent,
                                    CameraBuffer *dccBuffer)
{
    OMX_TI_CONFIG_SHAREDBUFFER uribufparam;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_INIT_STRUCT_PTR(&uribufparam, OMX_TI_CONFIG_SHAREDBUFFER);

    CAMHAL_ASSERT_X(dccBuffer != NULL,"ERROR invalid DCC buffer");

    uribufparam.nPortIndex = OMX_ALL;
    uribufparam.nSharedBuffSize = dccBuffer->size;
    uribufparam.pSharedBuff = (OMX_U8 *) camera_buffer_get_omx_ptr(dccBuffer);

    eError = OMX_SetParameter(hComponent,
                                ( OMX_INDEXTYPE )OMX_TI_IndexParamDccUriBuffer,
                                &uribufparam);
    if (eError != OMX_ErrorNone) {
        CAMHAL_LOGEB(" Error in SetParam for DCC Uri Buffer 0x%x", eError);
    }

    return eError;
}

size_t DCCHandler::readDCCdir(OMX_PTR buffer,
                          const android::Vector<android::String8 *> &dirPaths)
{
    FILE *pFile;
    OMX_S32 lSize;
    OMX_S32 dcc_buf_size = 0;
    size_t result;
    OMX_STRING filename;
    android::String8 temp;
    const char *dotdot = "..";
    DIR *d;
    struct dirent *dir;
    OMX_U16 i = 0;
    status_t stat = NO_ERROR;
    size_t ret = 0;

    for (i = 0; i < dirPaths.size(); i++) {
        d = opendir(dirPaths.itemAt(i)->string());
        if (d) {
            // read each filename
            while ((dir = readdir(d)) != NULL) {
                filename = dir->d_name;
                temp.clear();
                temp.append(dirPaths.itemAt(i)->string());
                temp.append(filename);
                if ((*filename != *dotdot)) {
                    pFile = fopen(temp.string(), "rb");
                    if (pFile == NULL) {
                        stat = -errno;
                    } else {
                        fseek(pFile, 0, SEEK_END);
                        lSize = ftell(pFile);
                        rewind(pFile);
                        // buffer is not NULL then copy all the DCC profiles into buffer
                        // else return the size of the DCC directory.
                        if (buffer) {
                            // copy file into the buffer:
                            result = fread(buffer, 1, lSize, pFile);
                            if (result != (size_t) lSize) {
                                stat = INVALID_OPERATION;
                            }
                            buffer = buffer + lSize;
                        }
                        // getting the size of the total dcc files available in FS */
                        dcc_buf_size = dcc_buf_size + lSize;
                        // terminate
                        fclose(pFile);
                    }
                }
            }
            closedir(d);
        }
    }

    if (stat == NO_ERROR) {
        ret = dcc_buf_size;
    }

    return ret;
}

} // namespace Camera
} // namespace Ti
