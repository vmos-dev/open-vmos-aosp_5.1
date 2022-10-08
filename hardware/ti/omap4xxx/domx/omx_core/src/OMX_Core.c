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

#include <dlfcn.h>		/* For dynamic loading */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>


/* #include "OMX_RegLib.h" */
#include "OMX_Component.h"
#include "OMX_Core.h"
#include "OMX_ComponentRegistry.h"

#include "timm_osal_types.h"
#include "timm_osal_error.h"
#include "timm_osal_trace.h"
#include "timm_osal_mutex.h"

#ifdef CHECK_SECURE_STATE
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#endif

/** size for the array of allocated components.  Sets the maximum
 * number of components that can be allocated at once */
#define MAXCOMP (50)
#define MAXNAMESIZE (128)
#define EMPTY_STRING "\0"

/** Determine the number of elements in an array */
#define COUNTOF(x) (sizeof(x)/sizeof(x[0]))

/** Array to hold the DLL pointers for each allocated component */
static void *pModules[MAXCOMP] = { 0 };

/** Array to hold the component handles for each allocated component */
static void *pComponents[COUNTOF(pModules)] = { 0 };

/* count for call OMX_Init() */
int count = 0;
pthread_mutex_t mutex;
TIMM_OSAL_PTR pCoreInitMutex = NULL;

int tableCount = 0;
ComponentTable componentTable[MAX_TABLE_SIZE];
char *sRoleArray[60][20];
char compName[60][200];


char *tComponentName[MAXCOMP][MAX_ROLES] = {
    /*video and image components */
    {"OMX.TI.DUCATI1.VIDEO.DECODER", "video_decoder.mpeg4",
        "video_decoder.avc",
        "video_decoder.h263",
        "video_decoder.wmv",
        "video_decoder.vp6",
        "video_decoder.vp7", NULL},
    {"OMX.TI.DUCATI1.VIDEO.DECODER.secure", "video_decoder.mpeg4",
        "video_decoder.avc",
        "video_decoder.h263", NULL},
    {"OMX.TI.DUCATI1.VIDEO.H264D",  "video_decoder.avc", NULL},
    {"OMX.TI.DUCATI1.VIDEO.H264E",  "video_encoder.avc", NULL},
    {"OMX.TI.DUCATI1.VIDEO.MPEG4D", "video_decoder.mpeg4", NULL},
    {"OMX.TI.DUCATI1.VIDEO.MPEG4E", "video_encoder.mpeg4",
                                    "video_encoder.h263",NULL},
    {"OMX.TI.DUCATI1.VIDEO.VP6D",   "video_decoder.vp6", NULL},
    {"OMX.TI.DUCATI1.VIDEO.VP7D",   "video_decoder.vp7", NULL},
    {"OMX.TI.DUCATI1.IMAGE.JPEGD",  "jpeg_decoder.jpeg", NULL},
    {"OMX.TI.DUCATI1.VIDEO.CAMERA",  "camera.omx", NULL},
    /* terminate the table */
    {NULL, NULL},
};

//AD
extern OMX_ERRORTYPE OMX_ComponentInit(OMX_HANDLETYPE hComponent);

#define CORE_assert  CORE_paramCheck
#define CORE_require CORE_paramCheck
#define CORE_ensure  CORE_paramCheck

#define CORE_paramCheck(C, V, S) do {\
    if (!(C)) { eError = V;\
    TIMM_OSAL_Error("failed check: " #C);\
    TIMM_OSAL_Error(" - returning error: " #V);\
    if(S) TIMM_OSAL_Error(" - %s", S);\
    goto EXIT; }\
    } while(0)

/******************************Public*Routine******************************\
* OMX_Init()
*
* Description:This method will initialize the OMX Core.  It is the
* responsibility of the application to call OMX_Init to ensure the proper
* set up of core resources.
*
* Returns:    OMX_NOERROR          Successful
*
* Note
*
\**************************************************************************/
OMX_ERRORTYPE OMX_Init()
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	TIMM_OSAL_ERRORTYPE eOsalError = TIMM_OSAL_ERR_NONE;

	eOsalError = TIMM_OSAL_MutexObtain(pCoreInitMutex, TIMM_OSAL_SUSPEND);
	CORE_assert(eOsalError == TIMM_OSAL_ERR_NONE,
	    OMX_ErrorInsufficientResources, "Mutex lock failed");

	count++;

	if (count == 1)
	{
		pthread_mutex_init(&mutex, NULL);
		eError = OMX_BuildComponentTable();
	}

	eOsalError = TIMM_OSAL_MutexRelease(pCoreInitMutex);
	CORE_assert(eOsalError == TIMM_OSAL_ERR_NONE,
	    OMX_ErrorInsufficientResources, "Mutex release failed");
      EXIT:
	return eError;
}

/******************************Public*Routine******************************\
* OMX_GetHandle
*
* Description: This method will create the handle of the COMPONENTTYPE
* If the component is currently loaded, this method will reutrn the
* hadle of existingcomponent or create a new instance of the component.
* It will call the OMX_ComponentInit function and then the setcallback
* method to initialize the callback functions
* Parameters:
* @param[out] pHandle            Handle of the loaded components
* @param[in] cComponentName     Name of the component to load
* @param[in] pAppData           Used to identify the callbacks of component
* @param[in] pCallBacks         Application callbacks
*
* @retval OMX_ErrorUndefined
* @retval OMX_ErrorInvalidComponentName
* @retval OMX_ErrorInvalidComponent
* @retval OMX_ErrorInsufficientResources
* @retval OMX_NOERROR                      Successful
*
* Note
*
\**************************************************************************/

OMX_ERRORTYPE OMX_GetHandle(OMX_HANDLETYPE * pHandle,
    OMX_STRING cComponentName, OMX_PTR pAppData,
    OMX_CALLBACKTYPE * pCallBacks)
{
	static const char prefix[] = "lib";
	static const char postfix[] = ".so";
	OMX_ERRORTYPE(*pComponentInit) (OMX_HANDLETYPE *);
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_COMPONENTTYPE *componentType;
	int i;
	char buf[sizeof(prefix) + MAXNAMESIZE + sizeof(postfix)];
	const char *pErr = dlerror();
	char *dlError = NULL;
#ifdef CHECK_SECURE_STATE
        int secure_misc_drv_fd,ret;
        OMX_U8 mode, enable=1;
#endif
	if (pthread_mutex_lock(&mutex) != 0)
	{
		TIMM_OSAL_Error("Core: Error in Mutex lock");
	}

	CORE_require(NULL != cComponentName, OMX_ErrorBadParameter, NULL);
	CORE_require(NULL != pHandle, OMX_ErrorBadParameter, NULL);
	CORE_require(NULL != pCallBacks, OMX_ErrorBadParameter, NULL);
	CORE_require(count > 0, OMX_ErrorUndefined,
	    "OMX_GetHandle called without calling OMX_Init first");

	/* Verify that the name is not too long and could cause a crash.  Notice
	 * that the comparison is a greater than or equals.  This is to make
	 * sure that there is room for the terminating NULL at the end of the
	 * name. */
	CORE_require(strlen(cComponentName) < MAXNAMESIZE,
	    OMX_ErrorInvalidComponentName, NULL);

	/* Locate the first empty slot for a component.  If no slots
	 * are available, error out */
	for (i = 0; i < COUNTOF(pModules); i++)
	{
		if (pModules[i] == NULL)
			break;
	}
	CORE_assert(i != COUNTOF(pModules), OMX_ErrorInsufficientResources,
	    NULL);

	/* load the component and check for an error.  If filename is not an
	 * absolute path (i.e., it does not  begin with a "/"), then the
	 * file is searched for in the following locations:
	 *
	 *     The LD_LIBRARY_PATH environment variable locations
	 *     The library cache, /etc/ld.so.cache.
	 *     /lib
	 *     /usr/lib
	 *
	 * If there is an error, we can't go on, so set the error code and exit */
	strcpy(buf, prefix);	/* the lengths are defined herein or have been */
	strcat(buf, cComponentName);	/* checked already, so strcpy and strcat are  */
	strcat(buf, postfix);	/* are safe to use in this context. */

#ifdef CHECK_SECURE_STATE
        //Dont return errors from misc driver to the user if any.
        //Since this affects all usecases, secure and non-secure.
        //Do log the errors though.
        secure_misc_drv_fd = open("/dev/rproc_user", O_SYNC | O_RDONLY);
	if (secure_misc_drv_fd < 0)
	{
		TIMM_OSAL_Error("Can't open misc driver device 0x%x\n", errno);
	}

	ret = read(secure_misc_drv_fd, &mode, sizeof(mode));
	if (ret < 0)
	{
		TIMM_OSAL_Error("Can't read from the misc driver");
	}
        if(mode == enable && strstr(cComponentName,"secure") == NULL)
	{
		TIMM_OSAL_Error("non-secure component not supported in secure mode");
		eError = OMX_ErrorComponentNotFound;
	}
	ret = close(secure_misc_drv_fd);
	if (ret < 0)
	{
		TIMM_OSAL_Error("Can't close the misc driver");
	}
        //Dont allow non-secure usecases if we are in secure state.
        //Else some of the memory regions will be unexpected firewalled.
        //This provides a clean exit in case we are in secure mode.
        if(eError == OMX_ErrorComponentNotFound)
        {
                goto EXIT;
        }
#endif

//#if 0
	pModules[i] = dlopen(buf, RTLD_LAZY | RTLD_GLOBAL);
	if (pModules[i] == NULL)
	{
		dlError = dlerror();
		TIMM_OSAL_Error("Failed because %s", dlError);
		eError = OMX_ErrorComponentNotFound;
		goto EXIT;
	}

	/* Get a function pointer to the "OMX_ComponentInit" function.  If
	 * there is an error, we can't go on, so set the error code and exit */
	pComponentInit = dlsym(pModules[i], "OMX_ComponentInit");
	pErr = dlerror();
	CORE_assert(((pErr == NULL) && (pComponentInit != NULL)),
	    OMX_ErrorInvalidComponent, NULL);
//#endif

	/* We now can access the dll.  So, we need to call the "OMX_ComponentInit"
	 * method to load up the "handle" (which is just a list of functions to
	 * call) and we should be all set.*/
	*pHandle = malloc(sizeof(OMX_COMPONENTTYPE));
	CORE_assert((*pHandle != NULL), OMX_ErrorInsufficientResources,
	    "Malloc of pHandle* failed");

	pComponents[i] = *pHandle;
	componentType = (OMX_COMPONENTTYPE *) * pHandle;
	componentType->nSize = sizeof(OMX_COMPONENTTYPE);

	componentType->nVersion.s.nVersionMajor = 1;
	componentType->nVersion.s.nVersionMinor = 1;
	componentType->nVersion.s.nRevision = 0;
	componentType->nVersion.s.nStep = 0;

	eError = (*pComponentInit) (*pHandle);
//eError = OMX_ComponentInit(*pHandle);
	if (OMX_ErrorNone == eError)
	{
		eError =
		    (componentType->SetCallbacks) (*pHandle, pCallBacks,
		    pAppData);
		CORE_assert(eError == OMX_ErrorNone, eError,
		    "Core: Error returned from component SetCallBack");
	} else
	{
		/* when the component fails to initialize, release the
		   component handle structure */
		free(*pHandle);
		/* mark the component handle as NULL to prevent the caller from
		   actually trying to access the component with it, should they
		   ignore the return code */
		*pHandle = NULL;
		pComponents[i] = NULL;
		dlclose(pModules[i]);
		goto EXIT;
	}
	eError = OMX_ErrorNone;
      EXIT:
	if (pthread_mutex_unlock(&mutex) != 0)
	{
		TIMM_OSAL_Error("Core: Error in Mutex unlock");
	}
	return (eError);
}


/******************************Public*Routine******************************\
* OMX_FreeHandle()
*
* Description:This method will unload the OMX component pointed by
* OMX_HANDLETYPE. It is the responsibility of the calling method to ensure that
* the Deinit method of the component has been called prior to unloading component
*
* Parameters:
* @param[in] hComponent the component to unload
*
* Returns:    OMX_NOERROR          Successful
*
* Note
*
\**************************************************************************/
OMX_ERRORTYPE OMX_FreeHandle(OMX_HANDLETYPE hComponent)
{

	OMX_ERRORTYPE eError = OMX_ErrorUndefined;
	OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *) hComponent;
	int i;

	if (pthread_mutex_lock(&mutex) != 0)
	{
		TIMM_OSAL_Error("Core: Error in Mutex lock");
	}

	CORE_require(pHandle != NULL, OMX_ErrorBadParameter, NULL);
	CORE_require(count > 0, OMX_ErrorUndefined,
	    "OMX_FreeHandle called without calling OMX_Init first");

	/* Locate the component handle in the array of handles */
	for (i = 0; i < COUNTOF(pModules); i++)
	{
		if (pComponents[i] == hComponent)
			break;
	}

	CORE_assert(i != COUNTOF(pModules), OMX_ErrorBadParameter, NULL);

	eError = pHandle->ComponentDeInit(hComponent);
	if (eError != OMX_ErrorNone)
	{
		TIMM_OSAL_Error("Error From ComponentDeInit..");
	}

	/* release the component and the component handle */
	dlclose(pModules[i]);
	pModules[i] = NULL;
	free(pComponents[i]);

	pComponents[i] = NULL;
	eError = OMX_ErrorNone;

      EXIT:
	/* The unload is now complete, so set the error code to pass and exit */
	if (pthread_mutex_unlock(&mutex) != 0)
	{
		TIMM_OSAL_Error("Core: Error in Mutex unlock");
	}

	return eError;
}

/******************************Public*Routine******************************\
* OMX_DeInit()
*
* Description:This method will release the resources of the OMX Core.  It is the
* responsibility of the application to call OMX_DeInit to ensure the clean up of these
* resources.
*
* Returns:    OMX_NOERROR          Successful
*
* Note
*
\**************************************************************************/
OMX_ERRORTYPE OMX_Deinit()
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	TIMM_OSAL_ERRORTYPE eOsalError = TIMM_OSAL_ERR_NONE;

	eOsalError = TIMM_OSAL_MutexObtain(pCoreInitMutex, TIMM_OSAL_SUSPEND);
	if (eOsalError != TIMM_OSAL_ERR_NONE)
	{
		TIMM_OSAL_Error("Mutex lock failed");
	}
	/*Returning error none because of OMX spec limitation on error codes that
	   can be returned by OMX_Deinit */
	CORE_assert(count > 0, OMX_ErrorNone,
	    "OMX_Deinit being called without a corresponding OMX_Init");
	count--;

	if (pthread_mutex_lock(&mutex) != 0)
		TIMM_OSAL_Error("Core: Error in Mutex lock");

	if (count == 0)
	{
		if (pthread_mutex_unlock(&mutex) != 0)
			TIMM_OSAL_Error("Core: Error in Mutex unlock");
		if (pthread_mutex_destroy(&mutex) != 0)
		{
			/*printf("%d :: Core: Error in Mutex destroy\n",__LINE__); */
		}
	} else
	{
		if (pthread_mutex_unlock(&mutex) != 0)
			TIMM_OSAL_Error("Core: Error in Mutex unlock");
	}

      EXIT:
	eOsalError = TIMM_OSAL_MutexRelease(pCoreInitMutex);
	if (eOsalError != TIMM_OSAL_ERR_NONE)
	{
		TIMM_OSAL_Error("Mutex release failed");
	}
	return eError;
}

/*************************************************************************
* OMX_SetupTunnel()
*
* Description: Setup the specified tunnel the two components
*
* Parameters:
* @param[in] hOutput     Handle of the component to be accessed
* @param[in] nPortOutput Source port used in the tunnel
* @param[in] hInput      Component to setup the tunnel with.
* @param[in] nPortInput  Destination port used in the tunnel
*
* Returns:    OMX_NOERROR          Successful
*
* Note
*
**************************************************************************/
/* OMX_SetupTunnel */
OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_SetupTunnel(OMX_IN OMX_HANDLETYPE
    hOutput, OMX_IN OMX_U32 nPortOutput, OMX_IN OMX_HANDLETYPE hInput,
    OMX_IN OMX_U32 nPortInput)
{
	OMX_ERRORTYPE eError = OMX_ErrorNotImplemented;
	OMX_COMPONENTTYPE *pCompIn, *pCompOut;
	OMX_TUNNELSETUPTYPE oTunnelSetup;

	if (hOutput == NULL && hInput == NULL)
		return OMX_ErrorBadParameter;

	oTunnelSetup.nTunnelFlags = 0;
	oTunnelSetup.eSupplier = OMX_BufferSupplyUnspecified;

	pCompOut = (OMX_COMPONENTTYPE *) hOutput;

	if (hOutput)
	{
		eError =
		    pCompOut->ComponentTunnelRequest(hOutput, nPortOutput,
		    hInput, nPortInput, &oTunnelSetup);
	}


	if (eError == OMX_ErrorNone && hInput)
	{
		pCompIn = (OMX_COMPONENTTYPE *) hInput;
		eError =
		    pCompIn->ComponentTunnelRequest(hInput, nPortInput,
		    hOutput, nPortOutput, &oTunnelSetup);
		if (eError != OMX_ErrorNone && hOutput)
		{
			/* cancel tunnel request on output port since input port failed */
			pCompOut->ComponentTunnelRequest(hOutput, nPortOutput,
			    NULL, 0, NULL);
		}
	}

	return eError;
}

/*************************************************************************
* OMX_ComponentNameEnum()
*
* Description: This method will provide the name of the component at the given nIndex
*
*Parameters:
* @param[out] cComponentName       The name of the component at nIndex
* @param[in] nNameLength                The length of the component name
* @param[in] nIndex                         The index number of the component
*
* Returns:    OMX_NOERROR          Successful
*
* Note
*
**************************************************************************/
OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_ComponentNameEnum(OMX_OUT OMX_STRING
    cComponentName, OMX_IN OMX_U32 nNameLength, OMX_IN OMX_U32 nIndex)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;

	CORE_require(cComponentName != NULL, OMX_ErrorBadParameter, NULL);
	CORE_require(count > 0, OMX_ErrorUndefined,
	    "OMX_GetHandle called without calling OMX_Init first");

	if (nIndex >= tableCount)
	{
		eError = OMX_ErrorNoMore;
	} else
	{
		strcpy(cComponentName, componentTable[nIndex].name);
	}
      EXIT:
	return eError;
}


/*************************************************************************
* OMX_GetRolesOfComponent()
*
* Description: This method will query the component for its supported roles
*
*Parameters:
* @param[in] cComponentName     The name of the component to query
* @param[in] pNumRoles     The number of roles supported by the component
* @param[in] roles		The roles of the component
*
* Returns:    OMX_NOERROR          Successful
*                 OMX_ErrorBadParameter		Faliure due to a bad input parameter
*
* Note
*
**************************************************************************/
OMX_API OMX_ERRORTYPE OMX_GetRolesOfComponent(OMX_IN OMX_STRING
    cComponentName, OMX_INOUT OMX_U32 * pNumRoles, OMX_OUT OMX_U8 ** roles)
{

	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_U32 i = 0;
	OMX_U32 j = 0;
	OMX_BOOL bFound = OMX_FALSE;

	CORE_require(cComponentName != NULL, OMX_ErrorBadParameter, NULL);
	CORE_require(pNumRoles != NULL, OMX_ErrorBadParameter, NULL);
	CORE_require(strlen(cComponentName) < MAXNAMESIZE,
	    OMX_ErrorInvalidComponentName, NULL);
	CORE_require(count > 0, OMX_ErrorUndefined,
	    "OMX_GetHandle called without calling OMX_Init first");

	while (!bFound && i < tableCount)
	{
		if (strcmp(cComponentName, componentTable[i].name) == 0)
		{
			bFound = OMX_TRUE;
		} else
		{
			i++;
		}
	}
	if (roles == NULL)
	{
		*pNumRoles = componentTable[i].nRoles;
		goto EXIT;
	} else
	{
		if (bFound && (*pNumRoles == componentTable[i].nRoles))
		{
			for (j = 0; j < componentTable[i].nRoles; j++)
			{
				strcpy((OMX_STRING) roles[j],
				    componentTable[i].pRoleArray[j]);
			}
		}
	}
      EXIT:
	return eError;
}

/*************************************************************************
* OMX_GetComponentsOfRole()
*
* Description: This method will query the component for its supported roles
*
*Parameters:
* @param[in] role     The role name to query for
* @param[in] pNumComps     The number of components supporting the given role
* @param[in] compNames      The names of the components supporting the given role
*
* Returns:    OMX_NOERROR          Successful
*
* Note
*
**************************************************************************/
OMX_API OMX_ERRORTYPE OMX_GetComponentsOfRole(OMX_IN OMX_STRING role,
    OMX_INOUT OMX_U32 * pNumComps, OMX_INOUT OMX_U8 ** compNames)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_U32 i = 0;
	OMX_U32 j = 0;
	OMX_U32 k = 0;

	CORE_require(role != NULL, OMX_ErrorBadParameter, NULL);
	CORE_require(pNumComps != NULL, OMX_ErrorBadParameter, NULL);
	CORE_require(count > 0, OMX_ErrorUndefined,
	    "OMX_GetHandle called without calling OMX_Init first");

	/* This implies that the componentTable is not filled */
	CORE_assert(componentTable[i].pRoleArray[j] != NULL,
	    OMX_ErrorBadParameter, NULL);

	for (i = 0; i < tableCount; i++)
	{
		for (j = 0; j < componentTable[i].nRoles; j++)
		{
			if (strcmp(componentTable[i].pRoleArray[j],
				role) == 0)
			{
				/* the first call to this function should only count the number
				   of roles so that for the second call compNames can be allocated
				   with the proper size for that number of roles */
				if (compNames != NULL)
				{
					strncpy((OMX_STRING) (compNames[k]),
					    (OMX_STRING) componentTable[i].
					    name, MAXNAMESIZE);
				}
				k++;
			}
		}
		*pNumComps = k;
	}

      EXIT:
	return eError;
}


/***************************************
PRINT TABLE FOR DEBUGGING PURPOSES ONLY
***************************************/

OMX_API OMX_ERRORTYPE OMX_PrintComponentTable()
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	int i = 0;
	int j = 0;

	TIMM_OSAL_Info
	    ("--------Component Table:: %d Components found-------------",
	    tableCount);

	for (i = 0; i < tableCount; i++)
	{
		TIMM_OSAL_Info("%i:: %s", i, componentTable[i].name);
		for (j = 0; j < componentTable[i].nRoles; j++)
		{
			TIMM_OSAL_Info("        %s",
			    componentTable[i].pRoleArray[j]);
		}
	}

	TIMM_OSAL_Info
	    ("-----------------End Component Table ------------------");

	return eError;

}


OMX_ERRORTYPE OMX_BuildComponentTable()
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_CALLBACKTYPE sCallbacks;
#ifndef STATIC_TABLE
	OMX_HANDLETYPE hComp = 0;
	OMX_U8 cRole[MAXNAMESIZE];
	OMX_STRING tempName = NULL;
	OMX_STRING temp = NULL;
	static OMX_STRING namePrefix = "OMX";
	static OMX_STRING filePrefix = "libOMX.";
	static OMX_STRING suffix = ".so";
#endif
	int j = 0;
	int numFiles = 0;
	int i, k;
	int componentfound = 0;

	/* set up dummy call backs */
	sCallbacks.EventHandler = ComponentTable_EventHandler;
	sCallbacks.EmptyBufferDone = ComponentTable_EmptyBufferDone;
	sCallbacks.FillBufferDone = ComponentTable_FillBufferDone;

#ifndef STATIC_TABLE
	/* allocate the name table */
	/*
	   compName = (OMX_STRING *) malloc(MAX_TABLE_SIZE * sizeof(OMX_STRING));
	   sRoleArray = (OMX_STRING**) malloc(MAX_TABLE_SIZE * sizeof(OMX_STRING));
	 */

	/* scan the target/lib directory and create a list of files in the directory */
	numFiles = scandir(libdir, &namelist, 0, 0);
	tableCount = 0;
	while (numFiles--)
	{
		/*  check if the file is an OMX component */
		if (strncmp(namelist[numFiles]->d_name, filePrefix,
			strlen(filePrefix)) == 0)
		{

			/* if the file is an OMX component, trim the prefix and suffix */
			tempName = (OMX_STRING) malloc(sizeof(namelist[numFiles]->d_name) + 1);	/* adding one ensures */
			memset(tempName, 0x00, sizeof(namelist[numFiles]->d_name) + 1);	/*  that a null terminator will */
			/*  always be present */
			/* copy only the name without the suffix */
			strncpy(tempName, namelist[numFiles]->d_name,
			    strlen(namelist[numFiles]->d_name) -
			    strlen(suffix));
			/* set a pointer to be after the lib prefix, i.e the beginning of the component name */
			temp = strstr(tempName, namePrefix);

			/* then copy the component name to the table */
			/*
			   compName[tableCount]= (OMX_STRING) malloc(MAXNAMESIZE);
			 */
			strncpy(compName[tableCount], temp, strlen(temp) + 1);
			componentTable[tableCount].name =
			    compName[tableCount];

			/* get the handle for the component and query for the roles of each component */
			eError =
			    OMX_GetHandle(&hComp,
			    componentTable[tableCount].name, 0x0,
			    &sCallbacks);
			if (eError == OMX_ErrorNone)
			{
				j = 0;
				while (eError != OMX_ErrorNoMore)
				{
					eError =
					    ((OMX_COMPONENTTYPE *) hComp)->
					    ComponentRoleEnum(hComp, cRole,
					    j++);
					if (eError == OMX_ErrorNotImplemented)
					{
						j = 1;
						break;
					}
				}
				nRoles = j - 1;
				componentTable[tableCount].nRoles = nRoles;
				/* sRoleArray[tableCount] = (OMX_STRING *) malloc(nRoles * sizeof(OMX_STRING)); */
				if (nRoles > 0)
				{
					/* sRoleArray[tableCount] = (OMX_STRING *) malloc(nRoles * sizeof(OMX_STRING)); */
					for (j = 0; j < nRoles; j++)
					{
						sRoleArray[tableCount][j] =
						    (OMX_STRING)
						    malloc(sizeof(OMX_U8) *
						    MAXNAMESIZE);
						((OMX_COMPONENTTYPE *)
						    hComp)->
						    ComponentRoleEnum(hComp,
						    (OMX_U8 *)
						    sRoleArray[tableCount][j],
						    j);
						componentTable[tableCount].
						    pRoleArray[j] =
						    sRoleArray[tableCount][j];
					}
				} else
				{
					/* sRoleArray[tableCount] = (OMX_STRING *) malloc(sizeof(OMX_STRING));    */
					sRoleArray[tableCount][j] =
					    (OMX_STRING) malloc(sizeof(OMX_U8)
					    * MAXNAMESIZE);
					strcpy(sRoleArray[tableCount][j],
					    EMPTY_STRING);
					componentTable[tableCount].
					    pRoleArray[j] =
					    sRoleArray[tableCount][j];
				}
			}
			if (hComp)
			{
				/* free the component handle */
				eError = OMX_FreeHandle(hComp);
				if (eError != OMX_ErrorNone)
				{
					goto EXIT;
				}
			}
			/* increment the table counter index only if above was successful */
			tableCount++;
			if (tempName != NULL)
			{
				free(tempName);
			}

		}
	}

#endif

	for (i = 0, numFiles = 0; i < MAXCOMP; i++)
	{
		if (tComponentName[i][0] == NULL)
		{
			break;
		}

		for (j = 0; j < numFiles; j++)
		{
			if (!strcmp(componentTable[j].name,
				tComponentName[i][0]))
			{
				componentfound = 1;
				break;
			}
		}
		if (componentfound == 1)
		{
			continue;
		}

		if (j == numFiles)
		{		/* new component */
			k = 1;
			while (tComponentName[i][k] != NULL)
			{
				componentTable[numFiles].pRoleArray[k - 1] =
				    tComponentName[i][k];
				k++;
			}
			componentTable[numFiles].nRoles = k - 1;
			strcpy(compName[numFiles], tComponentName[i][0]);
			componentTable[numFiles].name = compName[numFiles];
			numFiles++;
		}
	}
	tableCount = numFiles;

	CORE_assert(eError == OMX_ErrorNone, eError,
	    "Could not build Component Table");
      EXIT:
	return eError;
}

OMX_ERRORTYPE ComponentTable_EventHandler(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_PTR pAppData,
    OMX_IN OMX_EVENTTYPE eEvent,
    OMX_IN OMX_U32 nData1, OMX_IN OMX_U32 nData2, OMX_IN OMX_PTR pEventData)
{
	return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE ComponentTable_EmptyBufferDone(OMX_OUT OMX_HANDLETYPE
    hComponent, OMX_OUT OMX_PTR pAppData,
    OMX_OUT OMX_BUFFERHEADERTYPE * pBuffer)
{
	return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE ComponentTable_FillBufferDone(OMX_OUT OMX_HANDLETYPE hComponent,
    OMX_OUT OMX_PTR pAppData, OMX_OUT OMX_BUFFERHEADERTYPE * pBuffer)
{
	return OMX_ErrorNotImplemented;
}



/*===============================================================*/
/** @fn Core_Setup : This function is called when the the OMX Core library is
 *                  loaded. It creates a mutex, which is used during OMX_Init()
 */
/*===============================================================*/
void __attribute__ ((constructor)) Core_Setup(void)
{
	TIMM_OSAL_ERRORTYPE eError = TIMM_OSAL_ERR_NONE;

	eError = TIMM_OSAL_MutexCreate(&pCoreInitMutex);
	if (eError != TIMM_OSAL_ERR_NONE)
	{
		TIMM_OSAL_Error("Creation of default mutex failed");
	}
}



/*===============================================================*/
/** @fn Core_Destroy : This function is called when the the OMX Core library is
 *                    unloaded. It destroys the mutex which was created by
 *                    Core_Setup().
 *
 */
/*===============================================================*/
void __attribute__ ((destructor)) Core_Destroy(void)
{
	TIMM_OSAL_ERRORTYPE eError = TIMM_OSAL_ERR_NONE;

	eError = TIMM_OSAL_MutexDelete(pCoreInitMutex);
	if (eError != TIMM_OSAL_ERR_NONE)
	{
		TIMM_OSAL_Error("Destruction of default mutex failed");
	}
}
