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
* @file OMXDccDataSave.cpp
*
* This file contains functionality for handling DCC data save
*
*/

#include "CameraHal.h"
#include "OMXCameraAdapter.h"


namespace Ti {
namespace Camera {

status_t OMXCameraAdapter::initDccFileDataSave(OMX_HANDLETYPE* omxHandle, int portIndex)
{
    OMX_CONFIG_EXTRADATATYPE extraDataControl;
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    LOG_FUNCTION_NAME;

    OMX_INIT_STRUCT_PTR (&extraDataControl, OMX_CONFIG_EXTRADATATYPE);
    extraDataControl.nPortIndex = portIndex;
    extraDataControl.eExtraDataType = OMX_TI_DccData;
    extraDataControl.bEnable = OMX_TRUE;

    eError =  OMX_SetConfig(*omxHandle,
                        ( OMX_INDEXTYPE ) OMX_IndexConfigOtherExtraDataControl,
                        &extraDataControl);

    if ( OMX_ErrorNone != eError )
        {
        CAMHAL_LOGEB("Error while configuring dcc data overwrite extra data 0x%x",
                    eError);

        ret =  NO_INIT;
        }

    if (mDccData.pData) {
        free(mDccData.pData);
        mDccData.pData = NULL;
    }
    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::sniffDccFileDataSave(OMX_BUFFERHEADERTYPE* pBuffHeader)
{
    OMX_OTHER_EXTRADATATYPE *extraData;
    OMX_TI_DCCDATATYPE* dccData;
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    android::AutoMutex lock(mDccDataLock);

    if ( NULL == pBuffHeader ) {
        CAMHAL_LOGEA("Invalid Buffer header");
        LOG_FUNCTION_NAME_EXIT;
        return -EINVAL;
    }

    extraData = getExtradata(pBuffHeader->pPlatformPrivate,
                             (OMX_EXTRADATATYPE)OMX_TI_DccData);

    if ( NULL != extraData ) {
        CAMHAL_LOGVB("Size = %d, sizeof = %d, eType = 0x%x, nDataSize= %d, nPortIndex = 0x%x, nVersion = 0x%x",
                     extraData->nSize,
                     sizeof(OMX_OTHER_EXTRADATATYPE),
                     extraData->eType,
                     extraData->nDataSize,
                     extraData->nPortIndex,
                     extraData->nVersion);
    } else {
        CAMHAL_LOGVA("Invalid OMX_TI_DCCDATATYPE");
        LOG_FUNCTION_NAME_EXIT;
        return NO_ERROR;
    }

    dccData = ( OMX_TI_DCCDATATYPE * ) extraData->data;

    if (NULL == dccData) {
        CAMHAL_LOGVA("OMX_TI_DCCDATATYPE is not found in extra data");
        LOG_FUNCTION_NAME_EXIT;
        return NO_ERROR;
    }

    if (mDccData.pData) {
        free(mDccData.pData);
    }

    memcpy(&mDccData, dccData, sizeof(mDccData));

    int dccDataSize = (int)dccData->nSize - (int)(&(((OMX_TI_DCCDATATYPE*)0)->pData));

    mDccData.pData = (OMX_PTR)malloc(dccDataSize);

    if (NULL == mDccData.pData) {
        CAMHAL_LOGVA("not enough memory for DCC data");
        LOG_FUNCTION_NAME_EXIT;
        return NO_ERROR;
    }

    memcpy(mDccData.pData, &(dccData->pData), dccDataSize);

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

// Recursively searches given directory contents for the correct DCC file.
// The directory must be opened and its stream pointer + path passed
// as arguments. As this function is called recursively, to avoid excessive
// stack usage the path param is reused -> this MUST be char array with
// enough length!!! (260 should suffice). Path must end with "/".
// The directory must also be closed in the caller function.
// If the correct camera DCC file is found (based on the OMX measurement data)
// its file stream pointer is returned. NULL is returned otherwise
FILE * OMXCameraAdapter::parseDCCsubDir(DIR *pDir, char *path)
{
    FILE *pFile;
    DIR *pSubDir;
    struct dirent *dirEntry;
    int initialPathLength = strlen(path);

    LOG_FUNCTION_NAME;

    /* check each directory entry */
    while ((dirEntry = readdir(pDir)) != NULL)
    {
        if (dirEntry->d_name[0] == '.')
            continue;

        strcat(path, dirEntry->d_name);
        // dirEntry might be sub directory -> check it
        pSubDir = opendir(path);
        if (pSubDir) {
            // dirEntry is sub directory -> parse it
            strcat(path, "/");
            pFile = parseDCCsubDir(pSubDir, path);
            closedir(pSubDir);
            if (pFile) {
                // the correct DCC file found!
                LOG_FUNCTION_NAME_EXIT;
                return pFile;
            }
        } else {
            // dirEntry is file -> open it
            pFile = fopen(path, "rb");
            if (pFile) {
                // now check if this is the correct DCC file for that camera
                OMX_U32 dccFileIDword;
                OMX_U32 *dccFileDesc = (OMX_U32 *) &mDccData.nCameraModuleId;
                int i;

                // DCC file ID is 3 4-byte words
                for (i = 0; i < 3; i++) {
                    if (fread(&dccFileIDword, sizeof(OMX_U32), 1, pFile) != 1) {
                        // file too short
                        break;
                    }
                    if (dccFileIDword != dccFileDesc[i]) {
                        // DCC file ID word i does not match
                        break;
                    }
                }

                fclose(pFile);
                if (i == 3) {
                    // the correct DCC file found!
                    CAMHAL_LOGDB("DCC file to be updated: %s", path);
                    // reopen it for modification
                    pFile = fopen(path, "rb+");
                    if (!pFile)
                        CAMHAL_LOGEB("ERROR: DCC file %s failed to open for modification", path);
                    LOG_FUNCTION_NAME_EXIT;
                    return pFile;
                }
            } else {
                CAMHAL_LOGEB("ERROR: Failed to open file %s for reading", path);
            }
        }
        // restore original path
        path[initialPathLength] = '\0';
    }

    LOG_FUNCTION_NAME_EXIT;

    // DCC file not found in this directory tree
    return NULL;
}

// Finds the DCC file corresponding to the current camera based on the
// OMX measurement data, opens it and returns the file stream pointer
// (NULL on error or if file not found).
// The folder string dccFolderPath must end with "/"
FILE * OMXCameraAdapter::fopenCameraDCC(const char *dccFolderPath)
{
    FILE *pFile;
    DIR *pDir;
    char dccPath[260];

    LOG_FUNCTION_NAME;

    strcpy(dccPath, dccFolderPath);

    pDir = opendir(dccPath);
    if (!pDir) {
        CAMHAL_LOGEB("ERROR: Opening DCC directory %s failed", dccPath);
        LOG_FUNCTION_NAME_EXIT;
        return NULL;
    }

    pFile = parseDCCsubDir(pDir, dccPath);
    closedir(pDir);
    if (pFile) {
        CAMHAL_LOGDB("DCC file %s opened for modification", dccPath);
    }

    LOG_FUNCTION_NAME_EXIT;

    return pFile;
}

// Positions the DCC file stream pointer to the correct offset within the
// correct usecase based on the OMX mesurement data. Returns 0 on success
status_t OMXCameraAdapter::fseekDCCuseCasePos(FILE *pFile)
{
    OMX_U32 dccNumUseCases = 0;
    OMX_U32 dccUseCaseData[3];
    OMX_U32 i;

    LOG_FUNCTION_NAME;

    // position the file pointer to the DCC use cases section
    if (fseek(pFile, 80, SEEK_SET)) {
        CAMHAL_LOGEA("ERROR: Unexpected end of DCC file");
        LOG_FUNCTION_NAME_EXIT;
        return -EINVAL;
    }

    if (fread(&dccNumUseCases, sizeof(OMX_U32), 1, pFile) != 1 ||
        dccNumUseCases == 0) {
        CAMHAL_LOGEA("ERROR: DCC file contains 0 use cases");
        LOG_FUNCTION_NAME_EXIT;
        return -EINVAL;
    }

    for (i = 0; i < dccNumUseCases; i++) {
        if (fread(dccUseCaseData, sizeof(OMX_U32), 3, pFile) != 3) {
            CAMHAL_LOGEA("ERROR: Unexpected end of DCC file");
            LOG_FUNCTION_NAME_EXIT;
            return -EINVAL;
        }

        if (dccUseCaseData[0] == mDccData.nUseCaseId) {
            // DCC use case match!
            break;
        }
    }

    if (i == dccNumUseCases) {
        CAMHAL_LOGEB("ERROR: Use case ID %lu not found in DCC file", mDccData.nUseCaseId);
        LOG_FUNCTION_NAME_EXIT;
        return -EINVAL;
    }

    // dccUseCaseData[1] is the offset to the beginning of the actual use case
    // from the beginning of the file
    // mDccData.nOffset is the offset within the actual use case (from the
    // beginning of the use case to the data to be modified)

    if (fseek(pFile,dccUseCaseData[1] + mDccData.nOffset, SEEK_SET ))
    {
        CAMHAL_LOGEA("ERROR: Error setting the correct offset");
        LOG_FUNCTION_NAME_EXIT;
        return -EINVAL;
    }

    LOG_FUNCTION_NAME_EXIT;

    return NO_ERROR;
}

status_t OMXCameraAdapter::saveDccFileDataSave()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    android::AutoMutex lock(mDccDataLock);

    if (mDccData.pData)
        {
        FILE *fd = fopenCameraDCC(DCC_PATH);

        if (fd)
            {
            if (!fseekDCCuseCasePos(fd))
                {
                int dccDataSize = (int)mDccData.nSize - (int)(&(((OMX_TI_DCCDATATYPE*)0)->pData));

                if (fwrite(mDccData.pData, dccDataSize, 1, fd) != 1)
                    {
                    CAMHAL_LOGEA("ERROR: Writing to DCC file failed");
                    }
                else
                    {
                    CAMHAL_LOGDA("DCC file successfully updated");
                    }
                }
            fclose(fd);
            }
        else
            {
            CAMHAL_LOGEA("ERROR: Correct DCC file not found or failed to open for modification");
            }
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::closeDccFileDataSave()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    android::AutoMutex lock(mDccDataLock);

    if (mDccData.pData) {
        free(mDccData.pData);
        mDccData.pData = NULL;
    }
    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

} // namespace Camera
} // namespace Ti
