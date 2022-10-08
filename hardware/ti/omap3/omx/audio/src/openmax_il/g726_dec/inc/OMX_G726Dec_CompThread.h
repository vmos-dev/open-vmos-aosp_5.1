
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
 *             Texas Instruments OMAP(TM) Platform Software
 *  (c) Copyright Texas Instruments, Incorporated.  All Rights Reserved.
 *
 *  Use of this software is controlled by the terms and conditions found
 *  in the license agreement under which this software has been supplied.
 * ============================================================================ */
/**
 * @file OMX_G726Dec_CompThread.h
 *
 * This is an header file for an audio G726 decoder that is fully
 * compliant with the OMX Audio specification.
 * This the file is used internally by the component
 * in its code.
 *
 * @path  $(CSLPATH)\OMAPSW_MPU\linux\audio\src\openmax_il\g726_dec\inc\
 *
 * @rev 1.0
 */
/* --------------------------------------------------------------------------- */

#ifndef OMX_G726_COMPONENTTHREAD__H
#define OMX_G726_COMPONENTTHREAD__H

#define EXIT_COMPONENT_THRD  10

/* ================================================================================= * */
/**
 * G726DEC_ComponentThread() This is component thread of the component which keeps
 * running or lsitening from the application unless component is deinitialized
 * from by the application i.e. component is transitioned from Idle to Loaded
 * state.
 *
 * @param pHandle This is component handle allocated by the OMX core. 
 *
 * @pre          None
 *
 * @post         None
 *
 *  @return      OMX_ErrorNone = Successful Inirialization of the component\n
 *               OMX_ErrorInsufficientResources = Not enough memory
 *
 *  @see         None
 */
/* ================================================================================ * */
void* G726DEC_ComponentThread (void* pThreadData);

#endif
