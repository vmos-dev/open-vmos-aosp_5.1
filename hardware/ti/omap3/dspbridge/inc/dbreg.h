/*
 *  Copyright 2001-2008 Texas Instruments - http://www.ti.com/
 * 
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

/*
 *  ======== dbreg.h ========
 *  DSP-BIOS Bridge driver support functions for TI OMAP processors.
 *  Purpose:
 *      Registry keys for use in Linux.  This is the clearinghouse for
 *      registry definitions, hopefully eliminating overlapping between 
 *      modules.
 *
 *! Revision History:
 *! ================
 *! 10-Apr-2003 vp:  Added macro for subkey TCWORDSWAP.
 *! 21-Mar-2003 sb:  Added macro for subkey SHMSize
 *! 27-Aug-2001 jeh  Added WSXREG_LOADERFILENAME.
 *! 13-Feb-2001 kc:  DSP/BIOS Bridge name updates.
 *! 29-Nov-2000 rr:  Added WSXREG_DSPTYPE_55 as 6.
 *! 06-Sep-2000 jeh: Added WSXREG_CHNLOFFSET, WSXREG_NUMCHNLS,
 *!                  WSXREG_CHNLBUFSIZE.
 *! 26-Aug-2000 rr:  MEMBASE expanded to 9 entries.
 *! 26-Jul-2000 rr:  Added WSXREG_DCDNAME for the DCD Dll name. It will
 *!                  live under WSXREG_WINSPOXCONFIG.
 *! 17-Jul-2000 rr:  REG_MGR_OBJECT and REG_DRV_OBJECT defined. They
 *!                  are stored in the Registrty under WSXREG_WINSPOXCONFIG
 *!                  when they are created in DSP_Init. WSXREG_DEVOBJECT
 *!                  and WSXREG_MGROBJECT defined.
 *! 11-Dec-1999 ag:  Renamed Isa to IsaBus due to conflict with ceddk.h.
 *! 12-Nov-1999 rr:  New Registry Defnitions.
 *! 15-Oct-1999 rr:  New entry for DevObject created. WSXREG_DEVOBJECT
 *!                  under WSXREG_DDSPDRIVERPATH
 *! 10-Nov-1997 cr:  Added WSXREG_INFPATH, WSXREG_WINDEVICEPATH,
 *!                  WSXREG_WINCURVERSION
 *! 21-Oct-1997 cr:  Added WSXREG_BUSTYPE.
 *! 08-Sep-1997 cr:  Added WSXREG_SERVICES, WSXREG_SERVICENAME and
 *!                  WSXREG_CLASSINDEX.
 *! 30-Aug-1997 cr:  Added WSXREG_SOFTWAREPATHNT & WSXREG_WBCLASSGUID.
 *! 24-Mar-1997 gp:  Added MAXCHIPINFOSUBKEY def.
 *! 18-Feb-1997 cr:  Changed Version1.1 -> Version1.0
 *! 12-Feb-1997 cr:  Changed WinSPOX -> WinBRIDGE.
 *! 11-Dec-1996 gp:  Added Perf key name in WinSPOX Config.
 *! 22-Jul-1996 gp:  Added Trace key name.
 *! 30-May-1996 cr:  Created.
 */

#ifndef DBREG_
#define DBREG_ 1		/* Defined as "1" so InstallShield programs compile. */

#ifdef __cplusplus
extern "C" {
#endif

#define REG_MGR_OBJECT      1
#define REG_DRV_OBJECT      2
/* general registry definitions */
#define MAXREGPATHLENGTH    255	/* Max registry path length. Also the
				   max registry value length. */
#define DSPTYPE_55          6	/* This is the DSP Chip type for 55 */
#define DSPTYPE_64          0x99
#define IVA_ARM7          	0x97	/* This is the DSP Chip type for IVA/ARM7 */

#define DSPPROCTYPE_C55		5510
#define DSPPROCTYPE_C64		6410
#define IVAPROCTYPE_ARM7	470
/* registry */
#define DEVNODESTRING    "DevNode"	/* DWORD devnode */
#define CONFIG           "Software\\TexasInstruments\\DirectDSP\\Config"
#define DRVOBJECT        "DrvObject"
#define MGROBJECT        "MgrObject"
#define CLASS            "Device"	/*  device class */
#define TRACE            "Trace"	/* GT Trace settings.  */
#define PERFR            "Perf"	/* Enable perf Bool.  */
#define ROOT             "Root"	/*  root dir */

/* MiniDriver related definitions */
/* The following definitions are under "Drivers\\DirectDSP\\Device\\XXX "
 * Where XXX is the device or board name 
 */

#define WMDFILENAME      "MiniDriver"	/* WMD entry name */
#define CHIPTYPE         "ChipType"	/* Chip type */
#define CHIPNUM          "NumChips"	/* Number of chips */
#define NUMPROCS         "NumOfProcessors"	/* Number of processors */
#define DEFEXEC          "DefaultExecutable"	/* Default executable */
#define AUTOSTART        "AutoStart"	/* Statically load flag */
#define IVAAUTOSTART     "IvaAutoStart"	/* Statically load flag */
#define BOARDNAME        "BoardName"	/* Name of the Board */
#define UNITNUMBER       "UnitNumber"	/* Unit # of the Board */
#define BUSTYPE          "BusType"	/* Bus type board is on */
#define BUSNUMBER        "BusNumber"	/* Bus number board is on */
#define CURRENTCONFIG    "CurrentConfig"	/* Current resources */
#define PCIVENDEVID      "VendorDeviceId"	/* The board's id */
#define INFPATH          "InfPath"	/* wmd's inf filename */
#define DEVOBJECT        "DevObject"
#define ZLFILENAME       "ZLFileName"	/* Name of ZL file */
#define WORDSIZE         "WordSize"	/* NumBytes in DSP Word */
#define SHMSIZE          "SHMSize"	/* Size of SHM reservd on MPU */
#define IVAEXTMEMSIZE    "IVAEXTMEMSize"	/* IVA External Memeory size  */
#define TCWORDSWAP       "TCWordSwap"	/* Traffic Contoller Word Swap */
#define DSPRESOURCES     "DspTMSResources"	/* C55 DSP resurces on OMAP */
#define IVA1RESOURCES    "ARM7IvaResources"	/* ARM7 IVA resurces on OMAP */
#define PHYSMEMPOOLBASE  "PhysicalMemBase"	/* Physical mem passed to driver */
#define PHYSMEMPOOLSIZE  "PhysicalMemSize"	/* Physical mem passed to driver */
#ifdef __cplusplus
}
#endif
#endif				/* DBREG_ */
