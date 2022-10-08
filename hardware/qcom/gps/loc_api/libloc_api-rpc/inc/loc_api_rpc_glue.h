/******************************************************************************
  @file:  loc_api_rpc_glue.h
  @brief:  Loc API Android glue code header

  DESCRIPTION
     Loc API Android glue code header

  INITIALIZATION AND SEQUENCING REQUIREMENTS

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
03/17/09   dx       Initial version

$Id: //source/qcom/qct/modem/gps/cgps/dev/locationmiddleware/locapi/app-linux/source/test/loc_api_test/loc_api_cb_log.h#4 $
======================================================================*/

#ifndef LOC_API_RPC_GLUE_H
#define LOC_API_RPC_GLUE_H

/* Include RPC headers */
#include "loc_api_common.h"
#include "loc_api.h"
#include "loc_api_cb.h"
#include "loc_api_fixup.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* Boolean */
/* Other data types in comdef.h are defined in rpc stubs, so fix it here */
typedef unsigned char boolean;
#define TRUE 1
#define FALSE 0

extern int loc_api_glue_init(void);
extern int loc_api_null(void);

typedef int32 (loc_event_cb_f_type)(
      rpc_loc_client_handle_type            loc_handle,             /* handle of the client */
      rpc_loc_event_mask_type               loc_event,              /* event mask           */
      const rpc_loc_event_payload_u_type*   loc_event_payload       /* payload              */
);

extern rpc_loc_client_handle_type loc_open(
      rpc_loc_event_mask_type       event_reg_mask,
      loc_event_cb_f_type       *event_callback
);

extern int32 loc_close
(
      rpc_loc_client_handle_type handle
);

extern int32 loc_start_fix
(
      rpc_loc_client_handle_type handle
);

extern int32 loc_stop_fix
(
      rpc_loc_client_handle_type handle
);

extern int32 loc_ioctl
(
      rpc_loc_client_handle_type           handle,
      rpc_loc_ioctl_e_type                 ioctl_type,
      rpc_loc_ioctl_data_u_type*           ioctl_data
);

#ifdef __cplusplus
}
#endif

#endif /* LOC_API_RPC_GLUE_H */
