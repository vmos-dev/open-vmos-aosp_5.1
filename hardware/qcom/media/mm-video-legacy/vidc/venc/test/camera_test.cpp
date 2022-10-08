/*--------------------------------------------------------------------------
Copyright (c) 2010-2011, Code Aurora Forum. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Code Aurora nor
      the names of its contributors may be used to endorse or promote
      products derived from this software without specific prior written
      permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
--------------------------------------------------------------------------*/
#include "camera_test.h"
// #include "camera_class.h"

EXTERN_C_START

// static android::Camera* pCamera = NULL;

int CameraTest_Initialize(int nFrameRate,
                          int nFrameWidth,
                          int nFrameHeight,
                          CameraPreviewCallback pfnPreviewCallback)
{
   int result = 0;

//    pCamera = new android::Camera;
//    if (!pCamera)
//       return 1;

//    pCamera->init_camera(nFrameWidth,
//                         nFrameHeight,
//                         nFrameRate,
//                         pfnPreviewCallback);

   return result;
}

int CameraTest_Run()
{
   int result = 0;

//    if (pCamera)
//       pCamera->startPreview();
//    else
//       return 1;

   return result;
}

int CameraTest_ReleaseFrame(void* pPhys, void* pVirt)
{
   int result = 0;

//    if (pCamera)
//       pCamera->releaseFrame(pPhys, pVirt);
//    else
//       return 1;

   return result;
}

int CameraTest_Exit()
{
   int result = 0;

//    if (pCamera)
//    {
//       pCamera->stopPreview();
//       delete pCamera;
//    }
//    pCamera = NULL;

   return result;
}


EXTERN_C_END
