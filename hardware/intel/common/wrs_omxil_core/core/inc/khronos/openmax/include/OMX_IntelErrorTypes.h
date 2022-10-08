#ifndef OMX_IntelErrorTypes_h
#define OMX_IntelErrorTypes_h

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#include <OMX_Core.h>

typedef enum OMX_INTELERRORTYPE
{
      OMX_ErrorIntelExtSliceSizeOverflow = OMX_ErrorVendorStartUnused + (OMX_S32)0x00001001,
      OMX_ErrorIntelVideoNotPermitted    = OMX_ErrorVendorStartUnused + (OMX_S32)0x00001002,
      OMX_ErrorIntelProcessStream        = OMX_ErrorVendorStartUnused + (OMX_S32)0x00001003,
      OMX_ErrorIntelMissingConfig        = OMX_ErrorVendorStartUnused + (OMX_S32)0x00001004,

} OMX_INTELERRORTYPE;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
/* File EOF */

