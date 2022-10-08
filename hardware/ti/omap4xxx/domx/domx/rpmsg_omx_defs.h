/*
 * OMX offloading remote processor driver
 */

#ifndef RPMSG_OMX_DEFS_H
#define RPMSG_OMX_DEFS_H

#include <linux/rpmsg_omx.h>

//AD - from SDO
/*
 *  ======== OMX_Packet ========
 *
 *  OMX_Packet.desc: the package descriptor field. Note that the
 *  format is different for out-bound and in-bound messages.
 *
 *  out-bound message descriptor
 *
 *  Bits    Description
 *  --------------------------------------------------------------------
 *  [15:12] reserved
 *  [11:8]  omx message type
 *  [7:0]   omx client protocol version
 *
 *
 *  in-bound message descriptor
 *
 *  Bits    Description
 *  --------------------------------------------------------------------
 *  [15:12] reserved
 *  [11:8]  omx server status code
 *  [7:0]   omx server protocol version
 */
/* message type values */
#define OMX_DESC_MSG        0x1       // exec sync command
#define OMX_DESC_SYM_ADD    0x3       // symbol add message
#define OMX_DESC_SYM_IDX    0x4       // query symbox index
#define OMX_DESC_CMD        0x5       // exec non-blocking command.
#define OMX_DESC_TYPE_MASK  0x0F00    // field mask
#define OMX_DESC_TYPE_SHIFT 8         // field shift width

/* omx server status codes must be 0 - 15, it has to fit in a 4-bit field */
#define OMXSERVER_STATUS_SUCCESS          ((uint16_t)0) // success
#define OMXSERVER_STATUS_INVALID_FXN      ((uint16_t)1) // invalid fxn index
#define OMXSERVER_STATUS_SYMBOL_NOT_FOUND ((uint16_t)2) // symbol not found
#define OMXSERVER_STATUS_INVALID_MSG_TYPE ((uint16_t)3) // invalid msg type
#define OMXSERVER_STATUS_MSG_FXN_ERR      ((uint16_t)4) // msg function error
#define OMXSERVER_STATUS_ERROR            ((uint16_t)5) // general failure
#define OMXSERVER_STATUS_UNPROCESSED      ((uint16_t)6) // unprocessed message

#define OMX_POOLID_JOBID_DEFAULT (0x00008000)
#define OMX_INVALIDFXNIDX ((uint32_t)(0xFFFFFFFF))

#endif /* RPMSG_OMX_DEFS_H */
