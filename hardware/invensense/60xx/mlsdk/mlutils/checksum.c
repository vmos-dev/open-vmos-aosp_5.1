#include "mltypes.h"

/** bernstein hash, from public domain source */

uint32_t inv_checksum(unsigned char *str, int len)
{
    uint32_t hash = 5381;
    int i, c;

    for (i = 0; i < len; i++) {
        c = *(str + i);
        hash = ((hash << 5) + hash) + c;    /* hash * 33 + c */
    }

    return hash;
}
