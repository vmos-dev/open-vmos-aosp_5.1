/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */
/*******************************************************************************
 *
 * $Id: mltypes.h 3680 2010-09-04 03:13:32Z mcaramello $
 *
 *******************************************************************************/

#ifndef _MLERRORCODE_H_
#define _MLERRORCODE_H_

#include "mltypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
    Defines 
*/
#define CALL_N_CHECK(f) {                                                   \
    unsigned int r35uLt = f;                                                \
    if(INV_SUCCESS != r35uLt) {                                             \
        MPL_LOGE("Error in file %s, line %d : %s returned code %s (#%d)\n", \
                __FILE__, __LINE__, #f, MLErrorCode(r35uLt), r35uLt);       \
    }                                                                       \
}

#define CALL_CHECK_N_RETURN_ERROR(f) {                                      \
    unsigned int r35uLt = f;                                                \
    if(INV_SUCCESS != r35uLt) {                                             \
        MPL_LOGE("Error in file %s, line %d : %s returned code %s (#%d)\n", \
                __FILE__, __LINE__, #f, MLErrorCode(r35uLt), r35uLt);       \
        return r35uLt;                                                      \
    }                                                                       \
}

// for functions returning void
#define CALL_CHECK_N_RETURN(f) do {                                         \
    unsigned int r35uLt = f;                                                \
    if(INV_SUCCESS != r35uLt) {                                             \
        MPL_LOGE("Error in file %s, line %d : %s returned code %s (#%d)\n", \
                __FILE__, __LINE__, #f, MLErrorCode(r35uLt), r35uLt);       \
        return;                                                             \
    }                                                                       \
    } while(0)

#define CALL_CHECK_N_EXIT(f) {                                              \
    unsigned int r35uLt = f;                                                \
    if(INV_SUCCESS != r35uLt) {                                             \
        MPL_LOGE("Error in file %s, line %d : %s returned code %s (#%d)\n", \
                __FILE__, __LINE__, #f, MLErrorCode(r35uLt), r35uLt);       \
        exit (r35uLt);                                                      \
    }                                                                       \
}

    
#define CALL_CHECK_N_CALLBACK(f, cb) {                                      \
    unsigned int r35uLt = f;                                                \
    if(INV_SUCCESS != r35uLt) {                                             \
        MPL_LOGE("Error in file %s, line %d : %s returned code %s (#%d)\n", \
                __FILE__, __LINE__, #f, MLErrorCode(r35uLt), r35uLt);       \
        cb;                                                                 \
    }                                                                       \
}

#define CALL_CHECK_N_GOTO(f, label) {                                       \
    unsigned int r35uLt = f;                                                \
    if(INV_SUCCESS != r35uLt) {                                             \
        MPL_LOGE("Error in file %s, line %d : %s returned code %s (#%d)\n", \
                __FILE__, __LINE__, #f, MLErrorCode(r35uLt), r35uLt);       \
        goto label;                                                         \
    }                                                                       \
}

char* MLErrorCode(inv_error_t errorcode);

#ifdef __cplusplus
}
#endif

#endif

