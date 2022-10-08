/*
 * Copyright (c) 2010, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef _Android

/* #include "OMX_RegLib.h" */
#include "OMX_Component.h"
#include "OMX_Core.h"
#include "OMX_ComponentRegistry.h"

#include "OMX_Core_Wrapper.h"
#include "timm_osal_trace.h"
#undef LOG_TAG
#define LOG_TAG "OMX_CORE"

/** determine capabilities of a component before acually using it */
extern OMX_BOOL TIOMXConfigParser(OMX_PTR aInputParameters,
    OMX_PTR aOutputParameters);

#endif


#ifdef _Android
#ifdef _FROYO
OMX_BOOL TIOMXConfigParserRedirect(OMX_PTR aInputParameters,
    OMX_PTR aOutputParameters)
{
	TIMM_OSAL_Entering("OMXConfigParserRedirect +\n");
	OMX_BOOL Status = OMX_FALSE;

	Status = TIOMXConfigParser(aInputParameters, aOutputParameters);

	TIMM_OSAL_Exiting("OMXConfigParserRedirect -\n");
	return Status;
}
#endif
OMX_ERRORTYPE TIComponentTable_EventHandler(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_PTR pAppData,
    OMX_IN OMX_EVENTTYPE eEvent,
    OMX_IN OMX_U32 nData1, OMX_IN OMX_U32 nData2, OMX_IN OMX_PTR pEventData)
{
	return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE TIComponentTable_EmptyBufferDone(OMX_OUT OMX_HANDLETYPE
    hComponent, OMX_OUT OMX_PTR pAppData,
    OMX_OUT OMX_BUFFERHEADERTYPE * pBuffer)
{
	return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE TIComponentTable_FillBufferDone(OMX_OUT OMX_HANDLETYPE
    hComponent, OMX_OUT OMX_PTR pAppData,
    OMX_OUT OMX_BUFFERHEADERTYPE * pBuffer)
{
	return OMX_ErrorNotImplemented;
}


OMX_API OMX_ERRORTYPE TIOMX_Init(void)
{
	TIMM_OSAL_Entering("TIOMX_Init\n");

	return OMX_Init();
}

OMX_API OMX_ERRORTYPE TIOMX_Deinit(void)
{
	TIMM_OSAL_Entering("TIOMX_Deinit\n");

	return OMX_Deinit();
}

OMX_API OMX_ERRORTYPE TIOMX_ComponentNameEnum(OMX_OUT OMX_STRING
    cComponentName, OMX_IN OMX_U32 nNameLength, OMX_IN OMX_U32 nIndex)
{

	TIMM_OSAL_Entering("TIOMX_ComponentNameEnum\n");

	return OMX_ComponentNameEnum(cComponentName, nNameLength, nIndex);
}

OMX_API OMX_ERRORTYPE TIOMX_GetHandle(OMX_OUT OMX_HANDLETYPE * pHandle,
    OMX_IN OMX_STRING cComponentName,
    OMX_IN OMX_PTR pAppData, OMX_IN OMX_CALLBACKTYPE * pCallBacks)
{

	TIMM_OSAL_Entering("TIOMX_GetHandle\n");

	return OMX_GetHandle(pHandle, cComponentName, pAppData, pCallBacks);
}

OMX_API OMX_ERRORTYPE TIOMX_FreeHandle(OMX_IN OMX_HANDLETYPE hComponent)
{
	TIMM_OSAL_Entering("TIOMX_FreeHandle\n");

	return OMX_FreeHandle(hComponent);
}

OMX_API OMX_ERRORTYPE TIOMX_GetComponentsOfRole(OMX_IN OMX_STRING role,
    OMX_INOUT OMX_U32 * pNumComps, OMX_INOUT OMX_U8 ** compNames)
{

	TIMM_OSAL_Entering("TIOMX_GetComponentsOfRole\n");

	return OMX_GetComponentsOfRole(role, pNumComps, compNames);
}

OMX_API OMX_ERRORTYPE TIOMX_GetRolesOfComponent(OMX_IN OMX_STRING compName,
    OMX_INOUT OMX_U32 * pNumRoles, OMX_OUT OMX_U8 ** roles)
{

	TIMM_OSAL_Entering("TIOMX_GetRolesOfComponent\n");

	return OMX_GetRolesOfComponent(compName, pNumRoles, roles);
}

OMX_API OMX_ERRORTYPE TIOMX_SetupTunnel(OMX_IN OMX_HANDLETYPE hOutput,
    OMX_IN OMX_U32 nPortOutput,
    OMX_IN OMX_HANDLETYPE hInput, OMX_IN OMX_U32 nPortInput)
{

	TIMM_OSAL_Entering("TIOMX_SetupTunnel\n");

	return OMX_SetupTunnel(hOutput, nPortOutput, hInput, nPortInput);
}

OMX_API OMX_ERRORTYPE TIOMX_GetContentPipe(OMX_OUT OMX_HANDLETYPE * hPipe,
    OMX_IN OMX_STRING szURI)
{

	TIMM_OSAL_Entering("TIOMX_GetContentPipe\n");

	//return OMX_GetContentPipe(
	//      hPipe,
	//      szURI);
	return 0;
}
#endif
