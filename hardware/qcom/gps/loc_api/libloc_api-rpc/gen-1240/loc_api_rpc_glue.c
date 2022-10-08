/******************************************************************************
  @file  loc_api_rpc_glue.c
  @brief Android Loc API glue code using rpcgen.

  DESCRIPTION
  Loc API glue code for Android

  -----------------------------------------------------------------------------
Copyright (c) 2009, QUALCOMM USA, INC.

All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

·         Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer. 

·         Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution. 

·         Neither the name of the QUALCOMM USA, INC.  nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  -----------------------------------------------------------------------------
 ******************************************************************************/
/*=====================================================================
                        EDIT HISTORY FOR MODULE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

when       who      what, where, why 
--------   ---      ------------------------------------------------------- 
03/05/2009   dx       Initial version

======================================================================*/ 
/*=====================================================================

                     INCLUDE FILES FOR MODULE

======================================================================*/
//#define LOG_NDDEBUG 0

#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>

#include <rpc/rpc.h>
#include <rpc/clnt.h>

/* Include RPC headers */
#include "loc_api_rpc_glue.h"

/* Callback init */
#include "loc_apicb_appinit.h"

/* Logging */
#define LOG_TAG "lib_api_rpc_glue"
#include <utils/Log.h>

/* Comment this out to enable logging */
#undef LOGD
#define LOGD(...) {}

/*=====================================================================
     External declarations
======================================================================*/

CLIENT* loc_api_clnt = NULL;

/* Callback ID and pointer */
#define LOC_API_CB_ID 1 
loc_event_cb_f_type *loc_api_saved_cb = NULL;  /* the only callback of Loc API client */

#define RPC_FUNC_VERSION_BASE(a,b) a ## b
#define RPC_FUNC_VERSION(a,b) RPC_FUNC_VERSION_BASE(a,b)

#define LOC_GLUE_CHECK_INIT(ret_type) \
   if (loc_api_clnt == NULL) { return (ret_type) RPC_LOC_API_RPC_FAILURE; }

#define LOC_GLUE_CHECK_RESULT(stat, ret_type) \
   if (stat != RPC_SUCCESS) { return (ret_type) RPC_LOC_API_RPC_FAILURE; }   

/* Callback functions */
/* Returns 1 if successful */
bool_t rpc_loc_event_cb_f_type_0x00040001_svc(
        rpc_loc_event_cb_f_type_args *argp,
        rpc_loc_event_cb_f_type_rets *ret,
        struct svc_req *req)
{
    /* Callback not registered, or unexpected ID (shouldn't happen) */
    if (loc_api_saved_cb == NULL || argp->cb_id != LOC_API_CB_ID)
    {
        LOGD("Warning: No callback handler.\n");
        ret->loc_event_cb_f_type_result = 0;
        return 1; /* simply return */
    }

    LOGD("proc: %x  prog: %x  vers: %x\n",
            (int) req->rq_proc,
            (int) req->rq_prog,
            (int) req->rq_vers);

    LOGD("Callback received: %x (handle=%d ret_ptr=%d)\n",
            (int) argp->loc_event,
            (int) argp->loc_handle,
            (int) ret);

    /* Forward callback to real callback procedure */
    rpc_loc_client_handle_type        loc_handle = argp->loc_handle;
    rpc_loc_event_mask_type           loc_event  = argp->loc_event;
    const rpc_loc_event_payload_u_type*  loc_event_payload =
        (const rpc_loc_event_payload_u_type*) argp->loc_event_payload;

    int32 rc = loc_api_saved_cb(loc_handle, loc_event, loc_event_payload);
    ret->loc_event_cb_f_type_result = rc;

    return 1; /* ok */
}

int loc_apicbprog_freeresult (SVCXPRT *transp, xdrproc_t xdr_result, caddr_t result)
{
    xdr_free (xdr_result, result);

    /*
     * Insert additional freeing code here, if needed
     */
    // LOGD("***** loc_apicbprog_freeresult\n");

    return 1;
}

int loc_apicbprog_0x00040001_freeresult (SVCXPRT *transp, xdrproc_t xdr_result, caddr_t result)
{
    return loc_apicbprog_freeresult (transp, xdr_result, result);
}

/*===========================================================================

FUNCTION loc_api_glue_init

DESCRIPTION
   Initiates the RPC client

RETURN VALUE
   1 for success
   0 for failure
   
===========================================================================*/
int loc_api_glue_init(void)
{
    if (loc_api_clnt == NULL)
    {
        /* Print msg */
        LOGD("Trying to create RPC client...\n");
        loc_api_clnt = clnt_create(NULL, LOC_APIPROG, LOC_APIVERS, NULL);
        LOGD("Created loc_api_clnt ---- %x\n", (unsigned int)loc_api_clnt);

        if (loc_api_clnt == NULL)
        {
            fprintf(stderr, "Error: cannot create RPC client.\n");
            return 0;
        }

        /* Init RPC callbacks */
        int rc = loc_apicb_app_init();
        if (rc >= 0)
        {
            LOGD("Loc API callback initialized.\n");
        } else {
            fprintf(stderr, "Loc API callback initialization failed.\n");
            return 0;
        }
    }

    return 1;
}

rpc_loc_client_handle_type loc_open (
        rpc_loc_event_mask_type  event_reg_mask,
        loc_event_cb_f_type      *event_callback
    )
{
    LOC_GLUE_CHECK_INIT(rpc_loc_client_handle_type);

    rpc_loc_open_args args;
    args.event_reg_mask = event_reg_mask;
    args.event_callback = LOC_API_CB_ID;
    loc_api_saved_cb = event_callback;

    rpc_loc_open_rets rets;
    enum clnt_stat stat = RPC_SUCCESS;

    stat = RPC_FUNC_VERSION(rpc_loc_open_, /* LOC_APIVERS */ 0x00040001)(&args, &rets, loc_api_clnt);
    LOC_GLUE_CHECK_RESULT(stat, int32);

    return (rpc_loc_client_handle_type) rets.loc_open_result;
}

int32 loc_close(rpc_loc_client_handle_type handle)
{
    LOC_GLUE_CHECK_INIT(int32);

    rpc_loc_close_args args;
    args.handle = handle;

    rpc_loc_close_rets rets;
    enum clnt_stat stat = RPC_SUCCESS;

    stat = RPC_FUNC_VERSION(rpc_loc_close_, /* LOC_APIVERS */ 0x00040001)(&args, &rets, loc_api_clnt);
    LOC_GLUE_CHECK_RESULT(stat, int32);

    return (int32) rets.loc_close_result;
}

int32 loc_start_fix(rpc_loc_client_handle_type handle)
{
    LOC_GLUE_CHECK_INIT(int32);

    rpc_loc_start_fix_args args;
    args.handle = handle;

    rpc_loc_start_fix_rets rets;
    enum clnt_stat stat = RPC_SUCCESS;

    stat = RPC_FUNC_VERSION(rpc_loc_start_fix_, /* LOC_APIVERS */ 0x00040001)(&args, &rets, loc_api_clnt);
    LOC_GLUE_CHECK_RESULT(stat, int32);

    return (int32) rets.loc_start_fix_result;
}

int32 loc_stop_fix(rpc_loc_client_handle_type handle)
{
    LOC_GLUE_CHECK_INIT(int32);

    rpc_loc_stop_fix_args args;
    args.handle = handle;

    rpc_loc_stop_fix_rets rets;
    enum clnt_stat stat = RPC_SUCCESS;

    stat = RPC_FUNC_VERSION(rpc_loc_stop_fix_, /* LOC_APIVERS */ 0x00040001)(&args, &rets, loc_api_clnt);
    LOC_GLUE_CHECK_RESULT(stat, int32);

    return (int32) rets.loc_stop_fix_result;
}

int32 loc_ioctl(
    rpc_loc_client_handle_type           handle,
    rpc_loc_ioctl_e_type                 ioctl_type,
    rpc_loc_ioctl_data_u_type*           ioctl_data
    )
{
    LOC_GLUE_CHECK_INIT(int32);

    rpc_loc_ioctl_args args;
    args.handle = handle;
    args.ioctl_data = ioctl_data;
    args.ioctl_type = ioctl_type;
    if (ioctl_data != NULL)
    {
        /* Assign ioctl union discriminator */
        ioctl_data->disc = ioctl_type;

        /* In case the user hasn't filled in other disc fields,
           automatically fill them in here */
        switch (ioctl_type)
        {
            case RPC_LOC_IOCTL_GET_API_VERSION:
            case RPC_LOC_IOCTL_SET_FIX_CRITERIA:
            case RPC_LOC_IOCTL_GET_FIX_CRITERIA:
            case RPC_LOC_IOCTL_INFORM_NI_USER_RESPONSE:
            case RPC_LOC_IOCTL_INJECT_PREDICTED_ORBITS_DATA:
            case RPC_LOC_IOCTL_QUERY_PREDICTED_ORBITS_DATA_VALIDITY:
            case RPC_LOC_IOCTL_QUERY_PREDICTED_ORBITS_DATA_SOURCE:
            case RPC_LOC_IOCTL_SET_PREDICTED_ORBITS_DATA_AUTO_DOWNLOAD:
            case RPC_LOC_IOCTL_INJECT_UTC_TIME:
            case RPC_LOC_IOCTL_INJECT_RTC_VALUE:
            case RPC_LOC_IOCTL_INJECT_POSITION:
            case RPC_LOC_IOCTL_QUERY_ENGINE_STATE:
            case RPC_LOC_IOCTL_INFORM_SERVER_OPEN_STATUS:
            case RPC_LOC_IOCTL_INFORM_SERVER_CLOSE_STATUS:
            case RPC_LOC_IOCTL_SET_ENGINE_LOCK:
            case RPC_LOC_IOCTL_GET_ENGINE_LOCK:
            case RPC_LOC_IOCTL_SET_SBAS_CONFIG:
            case RPC_LOC_IOCTL_GET_SBAS_CONFIG:
            case RPC_LOC_IOCTL_SET_NMEA_TYPES:
            case RPC_LOC_IOCTL_GET_NMEA_TYPES:
                break;
            case RPC_LOC_IOCTL_SET_CDMA_PDE_SERVER_ADDR:
            case RPC_LOC_IOCTL_SET_CDMA_MPC_SERVER_ADDR:
            case RPC_LOC_IOCTL_SET_UMTS_SLP_SERVER_ADDR:
                args.ioctl_data->rpc_loc_ioctl_data_u_type_u.server_addr.addr_info.disc =
                    args.ioctl_data->rpc_loc_ioctl_data_u_type_u.server_addr.addr_type;
                break;
            case RPC_LOC_IOCTL_GET_CDMA_PDE_SERVER_ADDR:
            case RPC_LOC_IOCTL_GET_CDMA_MPC_SERVER_ADDR:
            case RPC_LOC_IOCTL_GET_UMTS_SLP_SERVER_ADDR:
                break;
            case RPC_LOC_IOCTL_SET_ON_DEMAND_LPM:
            case RPC_LOC_IOCTL_GET_ON_DEMAND_LPM:
            case RPC_LOC_IOCTL_DELETE_ASSIST_DATA:
            case RPC_LOC_IOCTL_SET_CUSTOM_PDE_SERVER_ADDR:
            case RPC_LOC_IOCTL_GET_CUSTOM_PDE_SERVER_ADDR:
            default:
                break;
        } /* switch */
    } /* ioctl_data != NULL */

    rpc_loc_ioctl_rets rets;
    enum clnt_stat stat = RPC_SUCCESS;

    stat = RPC_FUNC_VERSION(rpc_loc_ioctl_, /* LOC_APIVERS */ 0x00040001)(&args, &rets, loc_api_clnt);
    LOC_GLUE_CHECK_RESULT(stat, int32);

    return (int32) rets.loc_ioctl_result;
}

/* Returns 0 if error */
int32 loc_api_null(void)
{
    LOC_GLUE_CHECK_INIT(int32);

    int32 rets;
    enum clnt_stat stat = RPC_SUCCESS;

    stat = RPC_FUNC_VERSION(rpc_loc_api_null_, LOC_APIVERS)(NULL, &rets, loc_api_clnt);
    LOC_GLUE_CHECK_RESULT(stat, int32);

    return (int32) rets;
}
