/* ====================================================================
*             Texas Instruments OMAP(TM) Platform Software
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
* ==================================================================== */

#include <dlfcn.h>   /* For dynamic loading */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <utils/Log.h>

#undef LOG_TAG
#define LOG_TAG "TIOMX_CORE"

#include "OMX_Component.h"
#include "OMX_Core.h"
#include "OMX_ComponentRegistry.h"

#ifndef NO_OPENCORE
/** determine capabilities of a component before acually using it */
#include "ti_omx_config_parser.h"
#endif

/** size for the array of allocated components.  Sets the maximum 
 * number of components that can be allocated at once */
#define MAXCOMP (50)
#define MAXNAMESIZE (130)
#define EMPTY_STRING "\0"

/** Determine the number of elements in an array */
#define COUNTOF(x) (sizeof(x)/sizeof(x[0]))

/** Array to hold the DLL pointers for each allocated component */
static void* pModules[MAXCOMP] = {0};

/** Array to hold the component handles for each allocated component */
static void* pComponents[COUNTOF(pModules)] = {0};

/** count will be used as a reference counter for OMX_Init()
    so all changes to count should be mutex protected */
int count = 0;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int tableCount = 0;
ComponentTable componentTable[MAX_TABLE_SIZE];
char * sRoleArray[60][20];
char compName[60][200];

char *tComponentName[MAXCOMP][2] = {
    /*video and image components */
    //{"OMX.TI.JPEG.decoder", "image_decoder.jpeg" },
    {"OMX.TI.JPEG.Encoder", "image_encoder.jpeg"},
    //{"OMX.TI.Video.Decoder", "video_decoder.h263"},
    {"OMX.TI.Video.Decoder", "video_decoder.avc"},
    //{"OMX.TI.Video.Decoder", "video_decoder.mpeg2"},
    {"OMX.TI.Video.Decoder", "video_decoder.mpeg4"},
    {"OMX.TI.Video.Decoder", "video_decoder.wmv"},
    {"OMX.TI.Video.encoder", "video_encoder.mpeg4"},
    {"OMX.TI.Video.encoder", "video_encoder.h263"},
    {"OMX.TI.Video.encoder", "video_encoder.avc"},
    //{"OMX.TI.VPP", "iv_renderer.yuv.overlay"},
    //{"OMX.TI.Camera", "camera.yuv"},

    /* Speech components */
/*  {"OMX.TI.G729.encode", NULL},
    {"OMX.TI.G729.decode", NULL},	
    {"OMX.TI.G722.encode", NULL},
    {"OMX.TI.G722.decode", NULL},
    {"OMX.TI.G711.encode", NULL},
    {"OMX.TI.G711.decode", NULL},
    {"OMX.TI.G723.encode", NULL},
    {"OMX.TI.G723.decode", NULL},
    {"OMX.TI.G726.encode", NULL},
    {"OMX.TI.G726.decode", NULL},
    {"OMX.TI.GSMFR.encode", NULL},
    {"OMX.TI.GSMFR.decode", NULL},
*/

    /* Audio components */
#ifdef BUILD_WITH_TI_AUDIO
    {"OMX.TI.MP3.decode", "audio_decoder.mp3"},
    {"OMX.TI.AAC.encode", "audio_encoder.aac"},
    {"OMX.TI.AAC.decode", "audio_decoder.aac"},
    {"OMX.TI.WMA.decode", "audio_decoder.wma"},
    {"OMX.TI.WBAMR.decode", "audio_decoder.amrwb"},
    {"OMX.TI.AMR.decode", "audio_decoder.amrnb"},
    {"OMX.TI.AMR.encode", "audio_encoder.amrnb"},
    {"OMX.TI.WBAMR.encode", "audio_encoder.amrwb"},
#endif
/*  {"OMX.TI.PCM.encode", NULL},
    {"OMX.TI.PCM.decode", NULL},     
    {"OMX.TI.RAG.decode", "audio_decoder.ra"},
    {"OMX.TI.IMAADPCM.decode", NULL},
    {"OMX.TI.IMAADPCM.encode", NULL},
*/

    /* terminate the table */
    {NULL, NULL},
};


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
OMX_ERRORTYPE TIOMX_Init()
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    if(pthread_mutex_lock(&mutex) != 0)
    {
        ALOGE("%d :: Core: Error in Mutex lock\n",__LINE__);
        return OMX_ErrorUndefined;
    }

    count++;
    ALOGD("init count = %d\n", count);

    if (count == 1)
    {
        eError = TIOMX_BuildComponentTable();
    }

    if(pthread_mutex_unlock(&mutex) != 0)
    {
        ALOGE("%d :: Core: Error in Mutex unlock\n",__LINE__);
        return OMX_ErrorUndefined;
    }
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

OMX_ERRORTYPE TIOMX_GetHandle( OMX_HANDLETYPE* pHandle, OMX_STRING cComponentName,
    OMX_PTR pAppData, OMX_CALLBACKTYPE* pCallBacks)
{
    static const char prefix[] = "lib";
    static const char postfix[] = ".so";
    OMX_ERRORTYPE (*pComponentInit)(OMX_HANDLETYPE*);
    OMX_ERRORTYPE err = OMX_ErrorNone;
    OMX_COMPONENTTYPE *componentType;
    const char* pErr = dlerror();

    if(pthread_mutex_lock(&mutex) != 0)
    {
        ALOGE("%d :: Core: Error in Mutex lock\n",__LINE__);
        return OMX_ErrorUndefined;
    }

    if ((NULL == cComponentName) || (NULL == pHandle) || (NULL == pCallBacks)) {
        err = OMX_ErrorBadParameter;
        goto UNLOCK_MUTEX;
    }

    /* Verify that the name is not too long and could cause a crash.  Notice
     * that the comparison is a greater than or equals.  This is to make
     * sure that there is room for the terminating NULL at the end of the
     * name. */
    if(strlen(cComponentName) >= MAXNAMESIZE) {
        err = OMX_ErrorInvalidComponentName;
        goto UNLOCK_MUTEX;
    }
    /* Locate the first empty slot for a component.  If no slots
     * are available, error out */
    int i = 0;
    for(i=0; i< COUNTOF(pModules); i++) {
        if(pModules[i] == NULL) break;
    }
    if(i == COUNTOF(pModules)) {
        err = OMX_ErrorInsufficientResources;
        goto UNLOCK_MUTEX;
    }

    int refIndex = 0;
    for (refIndex=0; refIndex < MAX_TABLE_SIZE; refIndex++) {
        //get the index for the component in the table
        if (strcmp(componentTable[refIndex].name, cComponentName) == 0) {
            ALOGD("Found component %s with refCount %d\n",
                  cComponentName, componentTable[refIndex].refCount);

            /* check if the component is already loaded */
            if (componentTable[refIndex].refCount >= MAX_CONCURRENT_INSTANCES) {
                err = OMX_ErrorInsufficientResources;
                ALOGE("Max instances of component %s already created.\n", cComponentName);
                goto UNLOCK_MUTEX;
            } else {  // we have not reached the limit yet
                /* do what was done before need to limit concurrent instances of each component */

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

                /* the lengths are defined herein or have been
                 * checked already, so strcpy and strcat are
                 * are safe to use in this context. */
                char buf[sizeof(prefix) + MAXNAMESIZE + sizeof(postfix)];
                strcpy(buf, prefix);
                strcat(buf, cComponentName);
                strcat(buf, postfix);

                pModules[i] = dlopen(buf, RTLD_LAZY | RTLD_GLOBAL);
                if( pModules[i] == NULL ) {
                    ALOGE("dlopen %s failed because %s\n", buf, dlerror());
                    err = OMX_ErrorComponentNotFound;
                    goto UNLOCK_MUTEX;
                }

                /* Get a function pointer to the "OMX_ComponentInit" function.  If
                 * there is an error, we can't go on, so set the error code and exit */
                pComponentInit = dlsym(pModules[i], "OMX_ComponentInit");
                pErr = dlerror();
                if( (pErr != NULL) || (pComponentInit == NULL) ) {
                    ALOGE("%d:: dlsym failed for module %p\n", __LINE__, pModules[i]);
                    err = OMX_ErrorInvalidComponent;
                    goto CLEAN_UP;
                }

               /* We now can access the dll.  So, we need to call the "OMX_ComponentInit"
                * method to load up the "handle" (which is just a list of functions to
                * call) and we should be all set.*/
                *pHandle = malloc(sizeof(OMX_COMPONENTTYPE));
                if(*pHandle == NULL) {
                    err = OMX_ErrorInsufficientResources;
                    ALOGE("%d:: malloc of pHandle* failed\n", __LINE__);
                    goto CLEAN_UP;
                }

                pComponents[i] = *pHandle;
                componentType = (OMX_COMPONENTTYPE*) *pHandle;
                componentType->nSize = sizeof(OMX_COMPONENTTYPE);
                err = (*pComponentInit)(*pHandle);
                if (OMX_ErrorNone == err) {
                    err = (componentType->SetCallbacks)(*pHandle, pCallBacks, pAppData);
                    if (err != OMX_ErrorNone) {
                        ALOGE("%d :: Core: SetCallBack failed %d\n",__LINE__, err);
                        goto CLEAN_UP;
                    }
                    /* finally, OMX_ComponentInit() was successful and
                       SetCallbacks was successful, we have a valid instance,
                       so no we increment refCount */
                    componentTable[refIndex].pHandle[componentTable[refIndex].refCount] = *pHandle;
                    componentTable[refIndex].refCount += 1;
                    goto UNLOCK_MUTEX;  // Component is found, and thus we are done
                }
                else if (err == OMX_ErrorInsufficientResources) {
                        ALOGE("%d :: Core: Insufficient Resources for Component %d\n",__LINE__, err);
                        goto CLEAN_UP;
                }
            }
        }
    }

    // If we are here, we have not found the component
    err = OMX_ErrorComponentNotFound;
    goto UNLOCK_MUTEX;
CLEAN_UP:
    if(*pHandle != NULL)
    /* cover the case where we error out before malloc'd */
    {
        free(*pHandle);
        *pHandle = NULL;
    }
    pComponents[i] = NULL;
    dlclose(pModules[i]);
    pModules[i] = NULL;

UNLOCK_MUTEX:
    if(pthread_mutex_unlock(&mutex) != 0)
    {
        ALOGE("%d :: Core: Error in Mutex unlock\n",__LINE__);
        err = OMX_ErrorUndefined;
    }
    return (err);
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
OMX_ERRORTYPE TIOMX_FreeHandle (OMX_HANDLETYPE hComponent)
{

    OMX_ERRORTYPE retVal = OMX_ErrorUndefined;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)hComponent;

    if(pthread_mutex_lock(&mutex) != 0)
    {
        ALOGE("%d :: Core: Error in Mutex lock\n",__LINE__);
        return OMX_ErrorUndefined;
    }

    /* Locate the component handle in the array of handles */
    int i = 0;
    for(i=0; i< COUNTOF(pModules); i++) {
        if(pComponents[i] == hComponent) break;
    }

    if(i == COUNTOF(pModules)) {
        ALOGE("%d :: Core: component %p is not found\n", __LINE__, hComponent);
        retVal = OMX_ErrorBadParameter;
        goto EXIT;
    }

    retVal = pHandle->ComponentDeInit(hComponent);
    if (retVal != OMX_ErrorNone) {
        ALOGE("%d :: ComponentDeInit failed %d\n",__LINE__, retVal);
        goto EXIT;
    }

    int refIndex = 0, handleIndex = 0;
    for (refIndex=0; refIndex < MAX_TABLE_SIZE; refIndex++) {
        for (handleIndex=0; handleIndex < componentTable[refIndex].refCount; handleIndex++){
            /* get the position for the component in the table */
            if (componentTable[refIndex].pHandle[handleIndex] == hComponent){
                ALOGD("Found matching pHandle(%p) at index %d with refCount %d",
                      hComponent, refIndex, componentTable[refIndex].refCount);
                if (componentTable[refIndex].refCount) {
                    componentTable[refIndex].refCount -= 1;
                }
                componentTable[refIndex].pHandle[handleIndex] = NULL;
                dlclose(pModules[i]);
                pModules[i] = NULL;
                free(pComponents[i]);
                pComponents[i] = NULL;
                retVal = OMX_ErrorNone;
                goto EXIT;
            }
        }
    }

    // If we are here, we have not found the matching component
    retVal = OMX_ErrorComponentNotFound;

EXIT:
    /* The unload is now complete, so set the error code to pass and exit */
    if(pthread_mutex_unlock(&mutex) != 0)
    {
        ALOGE("%d :: Core: Error in Mutex unlock\n",__LINE__);
        return OMX_ErrorUndefined;
    }

    return retVal;
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
OMX_ERRORTYPE TIOMX_Deinit()
{
    if(pthread_mutex_lock(&mutex) != 0) {
        ALOGE("%d :: Core: Error in Mutex lock\n",__LINE__);
        return OMX_ErrorUndefined;
    }

    if (count) {
        count--;
    }

    ALOGD("deinit count = %d\n", count);

    if(pthread_mutex_unlock(&mutex) != 0) {
        ALOGE("%d :: Core: Error in Mutex unlock\n",__LINE__);
        return OMX_ErrorUndefined;
    }

    return OMX_ErrorNone;
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
OMX_API OMX_ERRORTYPE OMX_APIENTRY TIOMX_SetupTunnel(
    OMX_IN  OMX_HANDLETYPE hOutput,
    OMX_IN  OMX_U32 nPortOutput,
    OMX_IN  OMX_HANDLETYPE hInput,
    OMX_IN  OMX_U32 nPortInput)
{
    OMX_ERRORTYPE eError = OMX_ErrorNotImplemented;
    OMX_COMPONENTTYPE *pCompIn, *pCompOut;
    OMX_TUNNELSETUPTYPE oTunnelSetup;

    if (hOutput == NULL && hInput == NULL)
        return OMX_ErrorBadParameter;

    oTunnelSetup.nTunnelFlags = 0;
    oTunnelSetup.eSupplier = OMX_BufferSupplyUnspecified;

    pCompOut = (OMX_COMPONENTTYPE*)hOutput;

    if (hOutput)
    {
        eError = pCompOut->ComponentTunnelRequest(hOutput, nPortOutput, hInput, nPortInput, &oTunnelSetup);
    }


    if (eError == OMX_ErrorNone && hInput) 
    {  
        pCompIn = (OMX_COMPONENTTYPE*)hInput;
        eError = pCompIn->ComponentTunnelRequest(hInput, nPortInput, hOutput, nPortOutput, &oTunnelSetup);
        if (eError != OMX_ErrorNone && hOutput) 
        {
            /* cancel tunnel request on output port since input port failed */
            pCompOut->ComponentTunnelRequest(hOutput, nPortOutput, NULL, 0, NULL);
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
OMX_API OMX_ERRORTYPE OMX_APIENTRY TIOMX_ComponentNameEnum(
    OMX_OUT OMX_STRING cComponentName,
    OMX_IN  OMX_U32 nNameLength,
    OMX_IN  OMX_U32 nIndex)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    if (nIndex >=  tableCount)
    {
        eError = OMX_ErrorNoMore;
     }
    else
    {
        strcpy(cComponentName, componentTable[nIndex].name);
    }
    
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
OMX_API OMX_ERRORTYPE TIOMX_GetRolesOfComponent (
    OMX_IN      OMX_STRING cComponentName,
    OMX_INOUT   OMX_U32 *pNumRoles,
    OMX_OUT     OMX_U8 **roles)
{

    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 i = 0;
    OMX_U32 j = 0;
    OMX_BOOL bFound = OMX_FALSE;

    if (cComponentName == NULL || pNumRoles == NULL)
    {
        if (cComponentName == NULL)
        {
            ALOGE("cComponentName is NULL\n");
        }
        if (pNumRoles == NULL)
        {
            ALOGE("pNumRoles is NULL\n");
        }
        eError = OMX_ErrorBadParameter;
        goto EXIT;       
    }
    while (i < tableCount)
    {
        if (strcmp(cComponentName, componentTable[i].name) == 0)
        {
            bFound = OMX_TRUE;
            break;
        }
        i++;
    }
    if (!bFound)
    {
        eError = OMX_ErrorComponentNotFound;
        ALOGE("component %s not found\n", cComponentName);
        goto EXIT;
    } 
    if (roles == NULL)
    { 
        *pNumRoles = componentTable[i].nRoles;
    }
    else
    {
        /* must be second of two calls,
           pNumRoles is input in this context.
           If pNumRoles is < actual number of roles
           than we return an error */
        if (*pNumRoles >= componentTable[i].nRoles)
        {
            for (j = 0; j<componentTable[i].nRoles; j++) 
            {
                strcpy((OMX_STRING)roles[j], componentTable[i].pRoleArray[j]);
            }
            *pNumRoles = componentTable[i].nRoles;
        }
        else
        {
            eError = OMX_ErrorBadParameter;
            ALOGE("pNumRoles (%d) is less than actual number (%d) of roles \
                   for this component %s\n", *pNumRoles, componentTable[i].nRoles, cComponentName);
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
OMX_API OMX_ERRORTYPE TIOMX_GetComponentsOfRole ( 
    OMX_IN      OMX_STRING role,
    OMX_INOUT   OMX_U32 *pNumComps,
    OMX_INOUT   OMX_U8  **compNames)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 i = 0;
    OMX_U32 j = 0;
    OMX_U32 k = 0;
    OMX_U32 compOfRoleCount = 0;

    if (role == NULL || pNumComps == NULL)
    {
       if (role == NULL)
       {
           ALOGE("role is NULL");
       }
       if (pNumComps == NULL)
       {
           ALOGE("pNumComps is NULL\n");
       }
       eError = OMX_ErrorBadParameter;
       goto EXIT;
    }

   /* This implies that the componentTable is not filled */
    if (!tableCount)
    {
        eError = OMX_ErrorUndefined;
        ALOGE("Component table is empty. Please reload OMX Core\n");
        goto EXIT;
    }

    /* no matter, we always want to know number of matching components
       so this will always run */ 
    for (i = 0; i < tableCount; i++)
    {
        for (j = 0; j < componentTable[i].nRoles; j++) 
        { 
            if (strcmp(componentTable[i].pRoleArray[j], role) == 0)
            {
                /* the first call to this function should only count the number
                   of roles 
                */
                compOfRoleCount++;
            }
        }
    }
    if (compOfRoleCount == 0)
    {
        eError = OMX_ErrorComponentNotFound;
        ALOGE("Component supporting role %s was not found\n", role);
    }
    if (compNames == NULL)
    {
        /* must be the first of two calls */
        *pNumComps = compOfRoleCount;
    }
    else
    {
        /* must be the second of two calls */
        if (*pNumComps < compOfRoleCount)
        {
            /* pNumComps is input in this context,
               it can not be less, this would indicate
               the array is not large enough
            */
            eError = OMX_ErrorBadParameter;
            ALOGE("pNumComps (%d) is less than the actual number (%d) of components \
                  supporting role %s\n", *pNumComps, compOfRoleCount, role);
        }
        else
        {
            k = 0;
            for (i = 0; i < tableCount; i++)
            {
                for (j = 0; j < componentTable[i].nRoles; j++) 
                { 
                    if (strcmp(componentTable[i].pRoleArray[j], role) == 0)
                    {
                        /*  the second call compNames can be allocated
                            with the proper size for that number of roles.
                        */
                        compNames[k] = (OMX_U8*)componentTable[i].name;
                        k++;
                        if (k == compOfRoleCount)
                        {
                            /* there are no more components of this role
                               so we can exit here */
                            *pNumComps = k;
                            goto EXIT;
                        } 
                    }
                }
            }
        }        
    }

    EXIT:
    return eError;
}


OMX_ERRORTYPE TIOMX_BuildComponentTable()
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CALLBACKTYPE sCallbacks;
    int j = 0;
    int numFiles = 0;
    int i;

    for (i = 0, numFiles = 0; i < MAXCOMP; i ++) {
        if (tComponentName[i][0] == NULL) {
            break;
        }
        if (numFiles <= MAX_TABLE_SIZE){
            for (j = 0; j < numFiles; j ++) {
                if (!strcmp(componentTable[j].name, tComponentName[i][0])) {
                    /* insert the role */
                    if (tComponentName[i][1] != NULL)
                    {
                        componentTable[j].pRoleArray[componentTable[j].nRoles] = tComponentName[i][1];
                        componentTable[j].pHandle[componentTable[j].nRoles] = NULL; //initilize the pHandle element
                        componentTable[j].nRoles ++;
                    }
                    break;
                }
            }
            if (j == numFiles) { /* new component */
                if (tComponentName[i][1] != NULL){
                    componentTable[numFiles].pRoleArray[0] = tComponentName[i][1];
                    componentTable[numFiles].nRoles = 1;
                }
                strcpy(compName[numFiles], tComponentName[i][0]);
                componentTable[numFiles].name = compName[numFiles];
                componentTable[numFiles].refCount = 0; //initialize reference counter.
                numFiles ++;
            }
        }
    }
    tableCount = numFiles;
    if (eError != OMX_ErrorNone){
        ALOGE("Could not build Component Table\n");
    }

    return eError;
}

OMX_BOOL TIOMXConfigParserRedirect(
    OMX_PTR aInputParameters,
    OMX_PTR aOutputParameters)

{
    OMX_BOOL Status = OMX_FALSE;
#ifndef NO_OPENCORE
    Status = TIOMXConfigParser(aInputParameters, aOutputParameters);
#endif
    return Status;
}
