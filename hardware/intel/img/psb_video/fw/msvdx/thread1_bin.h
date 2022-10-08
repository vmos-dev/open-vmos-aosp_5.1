
/******************************************************************************

 @File         thread0_bin.h

 @Title

 @Copyright    Copyright (C)  Imagination Technologies Limited. All Rights Reserved.

 @Platform

 @Description

******************************************************************************/
#ifdef DE_ENV
#include "global.h"
#endif
//#if SLICE_SWITCHING_VARIANT
// This file was automatically generated from ../release/thread0.dnl using dnl2c.

/*
extern unsigned long aui32MTXDXVAFWText[];
extern unsigned long ui32MTXDXVAFWTextSize;

extern unsigned long aui32MTXDXVAFWData[];
extern unsigned long ui32MTXDXVAFWDataSize;

extern unsigned long ui32MTXDXVAFWDataLocation;
*/
typedef struct {
    const char* psVersion;
    const char* psBuildDate;
    unsigned int uiTextSize;
    unsigned int uiDataSize;
    unsigned int DataOffset;
    const unsigned char* pui8Text;
    const unsigned char* pui8Data;
} FIRMWARE;

//extern const FIRMWARE sSliceSwitchingFirmware;
extern const FIRMWARE sFirmware1366_FS;
extern const FIRMWARE sFirmware1366_SS;
extern const FIRMWARE sFirmware1419_FS;
extern const FIRMWARE sFirmware_FS;
extern const FIRMWARE sFirmware_SS;
extern const FIRMWARE sFirmware0000_SS;
extern const FIRMWARE sFirmware_SS_DE3_3_20;
extern const FIRMWARE sFirmware_SS_1472_3_8;
extern const FIRMWARE sFirmware_SS_1472_3_10;
extern const FIRMWARE sFirmware_1472_3_20_SS;

#define FIRMWARE_VERSION_DEFINED
#define FIRMWARE_BUILDDATE_DEFINED
//#endif /* SLICE_SWITCHING_VARIANT */
