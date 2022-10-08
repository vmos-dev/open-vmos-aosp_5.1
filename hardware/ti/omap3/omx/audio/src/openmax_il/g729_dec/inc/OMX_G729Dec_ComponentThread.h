
/*
 * Copyright (C) Texas Instruments - http://www.ti.com/
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
/* =============================================================================
 *             Texas Instruments OMAP (TM) Platform Software
 *  (c) Copyright Texas Instruments, Incorporated.  All Rights Reserved.
 *
 *  Use of this software is controlled by the terms and conditions found
 *  in the license agreement under which this software has been supplied.
 * =========================================================================== */
/**
 * @file OMX_G729Dec_ComponentThread.h
 *
 * This header file contains data and function prototypes for G729 DECODER OMX 
 *
 * @path  $(OMAPSW_MPU)\linux\audio\src\openmax_il\g729_dec\inc
 *
 * @rev  0.1
 */
/* ----------------------------------------------------------------------------- 
 *! 
 *! Revision History 
 *! ===================================
 *! Date         Author(s)            Version  Description
 *! ---------    -------------------  -------  ---------------------------------
 *! 03-Jan-2007  A.Donjon             0.1      Code update for G729 DECODER
 *! 
 *!
 * ================================================================================= */
/* ------compilation control switches -------------------------*/

#ifndef OMX_G729_COMPONENTTHREAD__H
#define OMX_G729_COMPONENTTHREAD__H

/* ======================================================================= */
/**
 * @def    EXIT_COMPONENT_THRD              Define Exit Componet Value
 */
/* ======================================================================= */ 
#define EXIT_COMPONENT_THRD  10

/****************************************************************
 *  INCLUDE FILES                                                 
 ****************************************************************/
/* ----- system and platform files ----------------------------*/
/*-------program files ----------------------------------------*/

/****************************************************************
 * EXTERNAL REFERENCES NOTE : only use if not found in header file
 ****************************************************************/
/*--------data declarations -----------------------------------*/
/*--------function prototypes ---------------------------------*/

/****************************************************************
 * PUBLIC DECLARATIONS Defined here, used elsewhere
 ****************************************************************/
/*--------data declarations -----------------------------------*/

/*--------function prototypes ---------------------------------*/

/****************************************************************
 * PRIVATE DECLARATIONS Defined here, used only here
 ****************************************************************/
/*--------data declarations -----------------------------------*/
/*--------function prototypes ---------------------------------*/


void* G729DEC_ComponentThread (void* pThreadData);

#endif
