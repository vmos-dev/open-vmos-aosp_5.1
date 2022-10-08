#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "omx_proxy_common.h"
#include <timm_osal_interfaces.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

#define COMPONENT_NAME "OMX.TI.DUCATI1.VIDEO.DECODER.secure"

extern OMX_U32 DUCATI_IN_SECURE_MODE;
extern OMX_U32 SECURE_COMPONENTS_RUNNING;

extern OMX_ERRORTYPE OMX_ProxyViddecInit(OMX_HANDLETYPE hComponent);
OMX_ERRORTYPE PROXY_VIDDEC_Secure_ComponentDeInit(OMX_HANDLETYPE hComponent);

OMX_ERRORTYPE OMX_ComponentInit(OMX_HANDLETYPE hComponent)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_COMPONENTTYPE *pHandle = NULL;
	PROXY_COMPONENT_PRIVATE *pComponentPrivate = NULL;
	const OMX_U8 enable = 1, disable = 0;
        OMX_U8 mode;
	int ret;

	pHandle = (OMX_COMPONENTTYPE *) hComponent;

	DOMX_ENTER("");

	DOMX_DEBUG("Component name provided is %s", COMPONENT_NAME);

	pHandle->pComponentPrivate =
	    (PROXY_COMPONENT_PRIVATE *)
	    TIMM_OSAL_Malloc(sizeof(PROXY_COMPONENT_PRIVATE), TIMM_OSAL_TRUE,
	    0, TIMMOSAL_MEM_SEGMENT_INT);

	PROXY_assert(pHandle->pComponentPrivate != NULL,
	    OMX_ErrorInsufficientResources,
	    "ERROR IN ALLOCATING PROXY COMPONENT PRIVATE STRUCTURE");

	pComponentPrivate =
	    (PROXY_COMPONENT_PRIVATE *) pHandle->pComponentPrivate;

	TIMM_OSAL_Memset(pComponentPrivate, 0,
		sizeof(PROXY_COMPONENT_PRIVATE));

	pComponentPrivate->cCompName =
	    TIMM_OSAL_Malloc(MAX_COMPONENT_NAME_LENGTH * sizeof(OMX_U8),
	    TIMM_OSAL_TRUE, 0, TIMMOSAL_MEM_SEGMENT_INT);

	PROXY_assert(pComponentPrivate->cCompName != NULL,
	    OMX_ErrorInsufficientResources,
	    " Error in Allocating space for proxy component table");

	// Copying component Name - this will be picked up in the proxy common
	PROXY_assert(strlen(COMPONENT_NAME) + 1 < MAX_COMPONENT_NAME_LENGTH,
	    OMX_ErrorInvalidComponentName,
	    "Length of component name is longer than the max allowed");
	TIMM_OSAL_Memcpy(pComponentPrivate->cCompName, COMPONENT_NAME,
	    strlen(COMPONENT_NAME) + 1);

	pComponentPrivate->secure_misc_drv_fd = open("/dev/rproc_user", O_SYNC | O_RDWR);
	if (pComponentPrivate->secure_misc_drv_fd < 0)
	{
		DOMX_ERROR("Can't open rproc_user device 0x%x\n", errno);
		eError = OMX_ErrorInsufficientResources;
                goto EXIT;
	}

        ret = write(pComponentPrivate->secure_misc_drv_fd, &enable, sizeof(enable));
        if(ret != 1)
        {
            DOMX_ERROR("errno from setting secure mode = %x",errno);
	    ret = write(pComponentPrivate->secure_misc_drv_fd, &disable, sizeof(disable));
	    if (ret < 0)
	    {
	        DOMX_ERROR("Setting unsecure mode failed");
            }

	    ret = close(pComponentPrivate->secure_misc_drv_fd);
	    if (ret < 0)
	    {
		DOMX_ERROR("Can't close the driver");
	    }
            eError = OMX_ErrorInsufficientResources;
            goto EXIT;
        }

	ret = read(pComponentPrivate->secure_misc_drv_fd, &mode, sizeof(mode));
	PROXY_assert(mode == enable, OMX_ErrorUndefined,"ERROR: We are not in secure mode");
	DOMX_DEBUG("secure mode recieved from Misc driver for secure playback = 0x%x\n", mode);

	eError = OMX_ProxyViddecInit(hComponent);
	pHandle->ComponentDeInit = PROXY_VIDDEC_Secure_ComponentDeInit;

#ifdef USE_ION
	pComponentPrivate->bUseIon = OMX_TRUE;
	pComponentPrivate->bMapIonBuffers = OMX_FALSE;
#endif
	EXIT:
	    if (eError != OMX_ErrorNone)
	    {
		DOMX_DEBUG("Error in Initializing Proxy");
		if (pComponentPrivate->cCompName != NULL)
		{
			TIMM_OSAL_Free(pComponentPrivate->cCompName);
			pComponentPrivate->cCompName = NULL;
		}
		if (pComponentPrivate != NULL)
		{
			TIMM_OSAL_Free(pComponentPrivate);
			pComponentPrivate = NULL;
		}
	    }
	    return eError;
}

OMX_ERRORTYPE PROXY_VIDDEC_Secure_ComponentDeInit(OMX_HANDLETYPE hComponent)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_COMPONENTTYPE *pHandle = NULL;
	PROXY_COMPONENT_PRIVATE *pComponentPrivate = NULL;
	int ret;
	const OMX_U8 disable = 0;
        int secure_misc_drv_fd;

	pHandle = (OMX_COMPONENTTYPE *) hComponent;

	pComponentPrivate =
	    (PROXY_COMPONENT_PRIVATE *) pHandle->pComponentPrivate;

        secure_misc_drv_fd = pComponentPrivate->secure_misc_drv_fd;

        eError = PROXY_ComponentDeInit(hComponent);
        if(eError != OMX_ErrorNone)
        {
                DOMX_ERROR("Proxy common deinit returned error = %x",eError);
        }
        pComponentPrivate = NULL;

        ret = write(secure_misc_drv_fd, &disable, sizeof(disable));
	if (ret < 0)
	{
	        DOMX_ERROR("Setting unsecure mode failed");
        }

	ret = close(secure_misc_drv_fd);
	if (ret < 0)
	{
		DOMX_ERROR("Can't close the driver");
	}

	return eError;
}

