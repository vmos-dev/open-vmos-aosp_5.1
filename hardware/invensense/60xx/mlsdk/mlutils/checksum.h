#ifndef MPLCHECKSUM_H
#define MPLCHECKSUM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mltypes.h"
#ifdef INV_INCLUDE_LEGACY_HEADERS
#include "checksum_legacy.h"
#endif

    uint32_t inv_checksum(unsigned char *str, int len);

#ifdef __cplusplus
}
#endif
#endif                          /* MPLCHECKSUM_H */
