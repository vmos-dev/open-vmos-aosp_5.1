/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */

/*******************************************************************************
 *
 * $Id: testsupport.h 5629 2011-06-11 03:13:08Z mcaramello $
 *
 ******************************************************************************/
 
#ifndef _TESTSUPPORT_H_
#define _TESTSUPPORT_H_

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------
    Includes
---------------------------*/

#include "mltypes.h"
#include "mlerrorcode.h"

#include "mlsl.h"
#include "log.h"

/*---------------------------
    Defines
---------------------------*/

/*---------------------------
    p-Types
---------------------------*/
#ifdef TESTING_SUPPORT
    void            SetHandle           (void *sl_handle);
    void            CommandPrompt       (void *sl_handle);
    void            RegisterMap         (void *sl_handle);
    void            DataLogger          (const unsigned long flag);
    void            DataLoggerSelector  (const unsigned long flag);
    void            DataLoggerCb        (void);
    unsigned short  KeyboardHandler     (unsigned char key);
    char*           CompassStateName    (char* out, int state);
#else
#define DataLoggerSelector(x)           //
#define DataLogger(x)                   //
#define DataLoggerCb                    NULL
#endif

#ifdef __cplusplus
}
#endif

#endif // _TESTSUPPORT_H_

