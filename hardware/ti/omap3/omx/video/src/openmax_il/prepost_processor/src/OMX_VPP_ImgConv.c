
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
/* ==============================================================================
*             Texas Instruments OMAP (TM) Platform Software
*  (c) Copyright Texas Instruments, Incorporated.  All Rights Reserved.
*
*  Use of this software is controlled by the terms and conditions found 
*  in the license agreement under which this software has been supplied.
* ============================================================================ */
/**
* @file OMX_VPP_imgConv.c
*
* This file implements OMX Component for VPP that 
* is  compliant with the OMX khronos 1.0.
*
* @path  $(CSLPATH)\
*
* @rev  1.0
*/
/* ---------------------------------------------------------------------------- 
*! 
*! Revision History 
*! ===================================
*! 17-april-2005 mf:  Initial Version. Change required per OMAPSWxxxxxxxxx
*! to provide _________________.
*!
* ============================================================================= */
#ifdef UNDER_CE 
#include <windows.h>
#include <oaf_osal.h>
#include <omx_core.h>
#include <stdlib.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <malloc.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif
#include <dbapi.h>
#include <string.h>
#include <stdio.h>

#include "OMX_VPP.h"
#include "OMX_VPP_Utils.h"
#include <OMX_Component.h>

typedef enum {
  ENoFilter,EScanAlgo
}eFilterAlgoOption; 

const OMX_S32 KDeepFiltering        = 3 ;   /* Number of chrominance artefact redution algorithm scans */
const OMX_U8  KColorKeyTolerence    = 50 ;  /* Tolerence on Color key detection                        */
const OMX_S32 KColorKeyChannelPred  = 150 ; /* Color channel predominance detection                    */
const OMX_S32 KColorKeyChannelMin   = 75 ;  /* Color channel predominance detection                    */
const OMX_S32 KAlgoLumaTolerence    = 600 ; /* Tolerence on luminance to detect pixel near color key   */
const OMX_S32 KAlgoChromaTolerance  = 50 ;  /* Tolerence on chrominance to detect pixel near color key */
const OMX_S32 KqCifWidth            = 176 ;
const OMX_S32 KqCifHeight           = 144 ;
const OMX_S32 KCifWidth             = 352 ;
const OMX_S32 KCifHeight            = 288 ;
const OMX_S32 KInterlacedTiFormat   = 1 ;
const OMX_S32 iFilteringAlgoEnable  = EScanAlgo;



static void ConvertChromReduction(VPP_COMPONENT_PRIVATE *pComponentPrivate);
static void ConvertFormatFromPlanar(OMX_U8 *apInBufferYUV420W, OMX_U8 *apTIinternalFormat);
static void ConvertNoChromReduction(VPP_COMPONENT_PRIVATE *pComponentPrivate);



OMX_ERRORTYPE ComputeTiOverlayImgFormat (VPP_COMPONENT_PRIVATE *pComponentPrivate,OMX_U8* aPictureArray, OMX_U8* aOutImagePtr, OMX_U8* aTransparencyKey )
{

    OMX_ERRORTYPE eError = OMX_ErrorUndefined; 
    OMX_U32 iHeight;
    OMX_U32 iWidth;

    /*If pointer was allocated in a previous call, free it to avoid memory leaks*/
    if(pComponentPrivate->overlay){
        if(pComponentPrivate->overlay->iOvlyConvBufPtr){
            OMX_FREE(pComponentPrivate->overlay->iOvlyConvBufPtr);
            pComponentPrivate->overlay->iOvlyConvBufPtr = NULL;
    }  
    OMX_FREE(pComponentPrivate->overlay);
    pComponentPrivate->overlay=NULL;
    }

    OMX_MALLOC(pComponentPrivate->overlay, sizeof(VPP_OVERLAY));
    pComponentPrivate->overlay->iRBuff = NULL ; 
    pComponentPrivate->overlay->iGBuff =  NULL;
    pComponentPrivate->overlay->iBBuff =  NULL;
    pComponentPrivate->overlay->iOvlyConvBufPtr =  NULL; 
    pComponentPrivate->overlay->iRKey = 0 ;
    pComponentPrivate->overlay->iGKey = 0;
    pComponentPrivate->overlay->iBKey = 0 ;
    pComponentPrivate->overlay->iAlign =1 ;

    iHeight = pComponentPrivate->sCompPorts[1].pPortDef.format.video.nFrameHeight;
    iWidth  = pComponentPrivate->sCompPorts[1].pPortDef.format.video.nFrameWidth;

    VPP_DPRINT("CMMFVideoImageConv::Picture Size w = %d x  h= %d", iWidth, iHeight);

    OMX_MALLOC(pComponentPrivate->overlay->iOvlyConvBufPtr, ((2*iWidth*iHeight)+ (2*(iWidth+2)*(iHeight+3*KDeepFiltering))));

    /* if odd buffer, must align it adding a copy column on left from the last image column */
    if((iHeight & 1) !=0)        
        pComponentPrivate->overlay->iAlign++;
     
    /* Only RGB 24 bits and BGR 24 bits formats are supported */
    if(pComponentPrivate->sCompPorts[1].pPortDef.format.video.eColorFormat==OMX_COLOR_Format24bitRGB888)                           
    {   
        pComponentPrivate->overlay->iRBuff = (OMX_U8*)(aPictureArray)+((iHeight-1)*iWidth*3)+0;
        pComponentPrivate->overlay->iGBuff = (OMX_U8*)(aPictureArray)+((iHeight-1)*iWidth*3)+1;
        pComponentPrivate->overlay->iBBuff = (OMX_U8*)(aPictureArray)+((iHeight-1)*iWidth*3)+2;
    }
    else 
    {
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }
  
    pComponentPrivate->overlay->iRKey = *aTransparencyKey++;
    pComponentPrivate->overlay->iGKey = *aTransparencyKey++;
    pComponentPrivate->overlay->iBKey = *aTransparencyKey++;

    if(iFilteringAlgoEnable == EScanAlgo)
        ConvertChromReduction(pComponentPrivate);
    else
        ConvertNoChromReduction(pComponentPrivate);
  
    if (KInterlacedTiFormat)
        ConvertFormatFromPlanar((pComponentPrivate->overlay->iOvlyConvBufPtr+(2*(iWidth+pComponentPrivate->overlay->iAlign)*(iHeight+3*KDeepFiltering))), 
                                aOutImagePtr);
    eError = OMX_ErrorNone;
EXIT:
    if(eError != OMX_ErrorNone){
        if(pComponentPrivate->overlay){
            OMX_FREE(pComponentPrivate->overlay->iOvlyConvBufPtr);
        }
        OMX_FREE(pComponentPrivate->overlay);
    }
    return eError;
}

static OMX_U32 iWidth ;
static OMX_U32 iHeight ;
static OMX_U8  iRKey ;
static OMX_U8  iGKey;
static OMX_U8  iBKey;
static OMX_U8  iAlign;

/* PRE PROSESSING OVERLAYING ALGORITHM WITH CHROMINANCE ARTEFACT REDUCTION ALGORITH 
One 444 frame buffer allocation for chrominance 
Adding 3 line to use the same buffer for each filtering pass avoid the need 
 to allocate a second frame buffer in 444 YUV space */
static void ConvertChromReduction(VPP_COMPONENT_PRIVATE *pComponentPrivate)
{
  
  OMX_U8 *y, *u, *v, *w;                  /* Pointers on Y U V buffers and Weight buffer */
  OMX_U8 *uu, *vv;                        /* U and V buffer in 444 space */
  OMX_U8 *puu,*pvv,*pyy;                  /* pointers on U,V, and Y on 444 YUV buffers */
  OMX_U8 *uuOut,*vvOut;                   /* U and V buffer in 444 space shifted on 3 lines */
  OMX_U8 *puOut,*pvOut;                   /* Pointers on U,V, and Y on 444 YUV buffers shifted on 3 lines */
  OMX_U8 *pv1, *pv2,*pu1, *pu2;           /* Pointers to 444 U and V buffers for to convert in 420 */
  OMX_U8 yKey,uKey,vKey;                  /* Color Key in YUV color space */
  OMX_U8 nKeyMax1,nKeyMax2,nKeyMax3;      /* Color Key range used in RVB to detect Color Key an in YUV to detect Near Color Key */
  OMX_U8 nKeyMin1,nKeyMin2,nKeyMin3;      
  OMX_U8 nIncAlign;                       /* The buffer need to have a additional line on left if the width is even */
  OMX_U8 nKeyErrorSize = KColorKeyTolerence; /* Color Key error acceptable in percent */
  OMX_U32 wCpt,hCpt;
  OMX_S32 i;
  iHeight = pComponentPrivate->sCompPorts[1].pPortDef.format.video.nFrameHeight;
  iWidth  = pComponentPrivate->sCompPorts[1].pPortDef.format.video.nFrameWidth;
  iAlign  = pComponentPrivate->overlay->iAlign;
  iRKey   = pComponentPrivate->overlay->iRKey;
  iGKey   = pComponentPrivate->overlay->iGKey;
  iBKey   = pComponentPrivate->overlay->iBKey;
   

    y = pComponentPrivate->overlay->iOvlyConvBufPtr + 2*(iWidth+iAlign)*(iHeight+3*KDeepFiltering);

    /* Cb buffer in 444         */
    uuOut = pComponentPrivate->overlay->iOvlyConvBufPtr;    
                                                            
    /* Cr buffer int 444    */
    vvOut = (pComponentPrivate->overlay->iOvlyConvBufPtr+(iWidth+iAlign)*(iHeight+3*KDeepFiltering));
                                                                                
    /* Initalized pointer on line 4 of frame buffer       */
    uu = uuOut+3*KDeepFiltering*(iWidth+iAlign);      
                                
    /* for the first image scan the buffer begin a line 4 */
    vv = vvOut+3*KDeepFiltering*(iWidth+iAlign);                                  

    puu = uu;
    pvv = vv;
  

    /* Dimension reduction for U and V components */
    u = (y+iWidth*iHeight);   /* Initialise pointer on YUV420 output buffers */
    v = (u+(iWidth*iHeight)/4);
    w = (v+(iWidth*iHeight)/4);

    /* Compute color key acceptable range depending on nKeyErrorSize */
    if(iRKey>KColorKeyChannelPred)
    {
        nKeyMax1 = ((iRKey+nKeyErrorSize)<255)?(iRKey+nKeyErrorSize*2):255;
        nKeyMin1 = ((nKeyErrorSize)<iRKey)?(iRKey-nKeyErrorSize*2):0;
    }
    else
    {
        nKeyMax1 = ((iRKey+nKeyErrorSize/2)<255)?(iRKey+nKeyErrorSize/2):255;
        nKeyMin1 = ((nKeyErrorSize/2)<iRKey)?(iRKey-nKeyErrorSize/2):0;
    }

    if(iGKey>KColorKeyChannelPred)
    {
        nKeyMax2 = ((iGKey+nKeyErrorSize)<255)?(iGKey+nKeyErrorSize*2):255;
        nKeyMin2 = ((nKeyErrorSize)<iGKey)?(iGKey-nKeyErrorSize*2):0;
    }
    else
    {
        nKeyMax2 = ((iGKey+nKeyErrorSize/2)<255)?(iGKey+nKeyErrorSize/2):255;
        nKeyMin2 = ((nKeyErrorSize/2)<iGKey)?(iGKey-nKeyErrorSize/2):0;
    }


    if(iBKey>KColorKeyChannelPred)
    {
        nKeyMax3 = ((iBKey+nKeyErrorSize)<255)?(iBKey+nKeyErrorSize*2):255;
        nKeyMin3 = ((nKeyErrorSize)<iBKey)?(iBKey-nKeyErrorSize*2):0;
    }
    else
    {
        nKeyMax3 = ((iBKey+nKeyErrorSize/2)<255)?(iBKey+nKeyErrorSize/2):255;
        nKeyMin3 = ((nKeyErrorSize/2)<iBKey)?(iBKey-nKeyErrorSize/2):0;
    }
    
    /* FIRST IMAGE SCAN ALGORITHM TO COMPUTR 444 UYV buffer from RGB buffer converting the color key */
    /* compute 444 YUV buffers from RGB input buffer converting RGB color key to an Y color key set at value 0 and and UV color key set at value (0,0) */
    for(hCpt=0;hCpt<iHeight;hCpt++)
    {   
        nIncAlign =0; /* alignement incremental set */
        for (wCpt=0;wCpt<(iWidth+iAlign);wCpt++)
        {
            
            if( (*pComponentPrivate->overlay->iRBuff<=nKeyMax1 && 
                 *pComponentPrivate->overlay->iRBuff>=nKeyMin1) &&
                (*pComponentPrivate->overlay->iGBuff<=nKeyMax2 && 
                 *pComponentPrivate->overlay->iGBuff>=nKeyMin2) &&
                (*pComponentPrivate->overlay->iBBuff<=nKeyMax3 && 
                 *pComponentPrivate->overlay->iBBuff>=nKeyMin3) )
            {
                *y   = 0;                                   /* set pixel at Y Color Key  */
                *puu = 0;                                   /* set pixel at UV Color Key */
                *pvv = 0;
            }
            else
            {
                *y=(OMX_U8)((77*(OMX_S32)(*pComponentPrivate->overlay->iRBuff) + 
                             150*(OMX_S32)(*pComponentPrivate->overlay->iGBuff) + 
                             29*(OMX_S32)(*pComponentPrivate->overlay->iBBuff))>>8);
                *puu=(OMX_U8)(((160*((OMX_S32)(*pComponentPrivate->overlay->iRBuff) - (OMX_S32)(*y)))>>8) + 128); 
                *pvv=(OMX_U8)(((126*((OMX_S32)(*pComponentPrivate->overlay->iBBuff) - (OMX_S32)(*y)))>>8) + 128);
                
                if(*y == 0)
                    (*y)++;                                 /* avoid zero almost blackbecause is used by the Y color key   */
                if(*puu == 0 && *pvv == 0)                  /* avoid zero almost black because is used by the UV color key */
                    (*puu)++;
            }
            puu++;
            pvv++;

            if(wCpt>iWidth)
                nIncAlign=0;
            
            y        += nIncAlign;
            pComponentPrivate->overlay->iRBuff   += 3*nIncAlign;
            pComponentPrivate->overlay->iGBuff   += 3*nIncAlign;
            pComponentPrivate->overlay->iBBuff   += 3*nIncAlign;
            nIncAlign = 1;
        }   
        pComponentPrivate->overlay->iRBuff -= 3*iWidth*2;
        pComponentPrivate->overlay->iGBuff -= 3*iWidth*2;
        pComponentPrivate->overlay->iBBuff -= 3*iWidth*2;
        
    }

    /* SECOND IMAGE SCAN ALGORITHM TO REMOVE COLOR KEY RESIDUALS ARTEFACTS */
    yKey     = (OMX_U8)((77*(OMX_S32)(iRKey) + 150*(OMX_S32)(iGKey) + 29*(OMX_S32)(iBKey))>>8); /* convert RGB color key in YUV space */
    uKey     = (OMX_U8)(((160*((OMX_S32)(iRKey) - (OMX_S32)(nKeyMin1)))>>8) + 128); 
    vKey     = (OMX_U8)(((126*((OMX_S32)(iBKey) - (OMX_S32)(nKeyMin1)))>>8) + 128);
    
    nKeyMax1 = (OMX_U8)(((yKey+KAlgoLumaTolerence)<255)?(yKey+KAlgoLumaTolerence):255);
    /*nKeyMin1 = ((KAlgoLumaTolerence)<yKey)?(yKey-KAlgoLumaTolerence):0;*/
    nKeyMin1 = (OMX_U8)(yKey-KAlgoLumaTolerence);

    if(uKey>KColorKeyChannelPred && vKey>KColorKeyChannelPred)
    {
        nKeyMax2 = (OMX_U8)(((uKey+KAlgoChromaTolerance)<255)?(uKey+KAlgoChromaTolerance):255);
        nKeyMax3 = (OMX_U8)(((vKey+KAlgoChromaTolerance)<255)?(vKey+KAlgoChromaTolerance):255);

        nKeyMin2 = (OMX_U8)(((KAlgoChromaTolerance)<uKey)?(uKey-KAlgoChromaTolerance):0);
        nKeyMin3 = (OMX_U8)(((KAlgoChromaTolerance)<vKey)?(vKey-KAlgoChromaTolerance):0);   
    }
    else if(uKey>KColorKeyChannelPred && vKey<KColorKeyChannelMin)
    {
        nKeyMax2 = (OMX_U8)(((uKey+KAlgoChromaTolerance/2)<255)?(uKey+KAlgoChromaTolerance/2):255);
        nKeyMin2 = (OMX_U8)(((KAlgoChromaTolerance/2)<uKey)?(uKey-KAlgoChromaTolerance/2):0);
        nKeyMax3 = 255;
        nKeyMin3 = 0;
    }
    else if(vKey>KColorKeyChannelPred && uKey<KColorKeyChannelMin)
    {
        nKeyMax3 = (OMX_U8)(((uKey+KAlgoChromaTolerance/2)<255)?(uKey+KAlgoChromaTolerance/2):255);
        nKeyMin3 = (OMX_U8)(((KAlgoChromaTolerance/2)<uKey)?(uKey-KAlgoChromaTolerance/2):0);
        nKeyMax2 = 255;
        nKeyMin2 = 0;
    }
    else
    {
        nKeyMax2 = (OMX_U8)(((uKey+KAlgoChromaTolerance/2)<255)?(uKey+KAlgoChromaTolerance/2):255);
        nKeyMax3 = (OMX_U8)(((vKey+KAlgoChromaTolerance/2)<255)?(vKey+KAlgoChromaTolerance/2):255);

        nKeyMin2 = (OMX_U8)(((KAlgoChromaTolerance/2)<uKey)?(uKey-KAlgoChromaTolerance/2):0);
        nKeyMin3 = (OMX_U8)(((KAlgoChromaTolerance/2)<vKey)?(vKey-KAlgoChromaTolerance/2):0);   
    }

    for( i =KDeepFiltering;i>0;i--)
    {                                                       /* and on the next image scan the buffer start at line */
        uu    = uuOut+3*i*(iWidth+iAlign);
        vv    = vvOut+3*i*(iWidth+iAlign);
        puu   = uu;
        pvv   = vv;
        pyy   = (pComponentPrivate->overlay->iOvlyConvBufPtr + 2*(iWidth+iAlign)*(iHeight+3*KDeepFiltering)) + 1 + iWidth;
        puOut = uuOut+3*(i-1)*(iWidth+iAlign);
        pvOut = vvOut+3*(i-1)*(iWidth+iAlign);
    
        memcpy(puOut,puu,iWidth+iAlign);        /* recopy the first line which is not scanned during algorithm */
        memcpy(pvOut,pvv,iWidth+iAlign);
        
        puOut += iWidth+iAlign;             /* initalize pointers on second line */
        pvOut += iWidth+iAlign;
        puu   += iWidth+iAlign;              /* initalize pointers on second line */
        pvv   += iWidth+iAlign;
    

        for(hCpt=1;hCpt<(iHeight-1);hCpt++)
        {   
            *puOut++ = *puu++; 
            *pvOut++ = *pvv++;
            *puOut++ = *puu++;
            *pvOut++ = *pvv++;

            for (wCpt=1;wCpt<(iWidth-1);wCpt++)
            {

                *puOut = *puu;
                *pvOut = *pvv;
                /* check if the pixel is near the color key */
                if(((*pyy)<=nKeyMax1 && (*pyy)>=nKeyMin1) && 
                   ((*puu)<=nKeyMax2 && (*puu)>=nKeyMin2) &&
                   ((*pvv)<=nKeyMax3 && (*pvv)>=nKeyMin3))
                {
                    /* check if a color key is avialable around the pixel */
                    if(((*(puu-1)== 0 && *(pvv-1)== 0) || (*(puu+1)== 0 && *(pvv+1)== 0)) ||
                    
                       ((*(puu-(iWidth+iAlign))   == 0 && *(pvv-(iWidth+iAlign))   == 0) || 
                        (*(puu-(iWidth+iAlign)-1) == 0 && *(pvv-(iWidth+iAlign)-1) == 0) || 
                        (*(puu-(iWidth+iAlign)+1) == 0 && *(pvv-(iWidth+iAlign)+1) == 0))||
                        
                       ((*(puu+(iWidth+iAlign))   == 0 && *(pvv+(iWidth+iAlign))   == 0) || 
                        (*(puu+(iWidth+iAlign)-1) == 0 && *(pvv+(iWidth+iAlign)-1) == 0) || 
                        (*(puu+(iWidth+iAlign)+1) == 0 && *(pvv+(iWidth+iAlign)+1) == 0)))
                    {
                        *puOut = 0;                           /* set the U and V pixel to UV color Key */
                        *pvOut = 0;
                    }
                }
                puOut++; 
                pvOut++;
                pyy++; 
                puu++; 
                pvv++;
            }
            *puOut++ = *puu++;
            *pvOut++ = *pvv++;

            if(iAlign>1)
            {
                *puOut++ = *puu++;
                *pvOut++ = *pvv++;
            }
            pyy += 2;
        }
        memcpy(puOut,puu,iWidth+iAlign);        
        memcpy(pvOut,pvv,iWidth+iAlign);
    }
    uu = uuOut;
    vv = vvOut;

    pu1 = uu;
    pu2 = uu+iWidth+iAlign;
    pv1 = vv;
    pv2 = vv+iWidth+iAlign;
     
    for(hCpt=0;hCpt<iHeight;hCpt+=2)
    {
        for(wCpt=0;wCpt<iWidth;wCpt+=2)
        {
            *u++ = (OMX_U8)(((OMX_U32)(*pu1+2*(*(pu1+1))+*(pu1+2)+*pu2+2*(*(pu2+1))+*(pu2+2)))>>3);
            *v++ = (OMX_U8)(((OMX_U32)(*pv1+2*(*(pv1+1))+*(pv1+2)+*pv2+2*(*(pv2+1))+*(pv2+2)))>>3);

            *w    = 0;
            (*w) += (*(pu1  )!=0  || *(pv1  )!=0)?0:1;        
            (*w) += (*(pu1+1)!=0  || *(pv1+1)!=0)?0:2;
            (*w) += (*(pu1+2)!=0  || *(pv1+2)!=0)?0:1;
            (*w) += (*(pu2  )!=0  || *(pv2  )!=0)?0:1;
            (*w) += (*(pu2+1)!=0  || *(pv2+1)!=0)?0:2;
            (*w) += (*(pu2+2)!=0  || *(pv2+2)!=0)?0:1;
            
            w++;
            pu1 += 2; 
            pv1 += 2;
            pu2 += 2; 
            pv2 += 2; 
        }
        
        pu1 += iWidth+2*iAlign; pu2+=iWidth+2*iAlign;
        pv1 += iWidth+2*iAlign; pv2+=iWidth+2*iAlign;
        
    }
}

/* PRE PROSESSING OVERLAYING ALGORITHM WITHOUT CHROMINANCE ARTEFACT REDUCTION ALGORITH 
// The algorithm is the same one which it used above but we did't need to allocate a full frame buffer in 444 
// Only 2 UV 444 lines are mandatoried */
static void ConvertNoChromReduction(VPP_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_U8 *y, *u, *v, *w;                  
    OMX_U8 *uu, *vv;                        
    OMX_U8 *puu,*pvv;                       
    OMX_U8 *pv1, *pv2,*pu1, *pu2;           
    OMX_U8 nKeyMax1,nKeyMax2,nKeyMax3;       
    OMX_U8 nKeyMin1,nKeyMin2,nKeyMin3;      
    OMX_U8 nIncAlign;                        
    OMX_U8 nKeyErrorSize = KColorKeyTolerence; 
    OMX_U32 lCpt, hCpt, wCpt;

    y  = pComponentPrivate->overlay->iOvlyConvBufPtr + (4*(iWidth+iAlign));
    uu = pComponentPrivate->overlay->iOvlyConvBufPtr;  
    vv = pComponentPrivate->overlay->iOvlyConvBufPtr + (iWidth+iAlign)*2;
   
    u = (y+iWidth*iHeight);
    v = (u+(iWidth*iHeight)/4);
    w = (v+(iWidth*iHeight)/4);

    /* Compute color key acceptable range depending on nKeyErrorSize. */
    nKeyMax1 = ((iRKey+nKeyErrorSize/2)<255)?(iRKey+nKeyErrorSize/2):255;
    nKeyMax2 = ((iGKey+nKeyErrorSize/2)<255)?(iGKey+nKeyErrorSize/2):255;
    nKeyMax3 = ((iBKey+nKeyErrorSize/2)<255)?(iBKey+nKeyErrorSize/2):255;

    nKeyMin1 = ((nKeyErrorSize/2)<iRKey)?(iRKey-nKeyErrorSize/2):0;
    nKeyMin2 = ((nKeyErrorSize/2)<iGKey)?(iGKey-nKeyErrorSize/2):0;
    nKeyMin3 = ((nKeyErrorSize/2)<iBKey)?(iBKey-nKeyErrorSize/2):0;

    for(hCpt=0;hCpt<iHeight;hCpt+=2)
    {
        /* 2 lines calculation */
        puu = uu;
        pvv = vv;
        for (lCpt=0;lCpt<2;lCpt++)
        {
            nIncAlign = 0; 
            for (wCpt=0; wCpt<(iWidth+iAlign); wCpt++)
            {
                if( (*pComponentPrivate->overlay->iRBuff<=nKeyMax1 && 
                     *pComponentPrivate->overlay->iRBuff>=nKeyMin1) &&
                    (*pComponentPrivate->overlay->iGBuff<=nKeyMax2 && 
                     *pComponentPrivate->overlay->iGBuff>=nKeyMin2) &&
                    (*pComponentPrivate->overlay->iBBuff<=nKeyMax3 && 
                     *pComponentPrivate->overlay->iBBuff>=nKeyMin3) )
                {
                    *y     = 0;
                    *puu++ = 0;
                    *pvv++ = 0;
                }
                else
                {
                    *y   = (OMX_U8)((77*(OMX_S32)(*pComponentPrivate->overlay->iRBuff) + 
                                     150*(OMX_S32)(*pComponentPrivate->overlay->iGBuff) + 
                                     29*(OMX_S32)(*pComponentPrivate->overlay->iBBuff))>>8);
                    *puu = (OMX_U8)(((160*((OMX_S32)(*pComponentPrivate->overlay->iRBuff) - (OMX_S32)(*y)))>>8) + 128); 
                    *pvv = (OMX_U8)(((126*((OMX_S32)(*pComponentPrivate->overlay->iBBuff) - (OMX_S32)(*y)))>>8) + 128);

                    if(*y == 0)
                        (*y)++;                                  

                    if(*puu == 0 && *pvv == 0)                  
                        (*puu)++;

                    puu++;
                    pvv++;
                }

                if(wCpt>iWidth)
                    nIncAlign=0;
                
                y        += nIncAlign;
                pComponentPrivate->overlay->iRBuff   += 3*nIncAlign;
                pComponentPrivate->overlay->iGBuff   += 3*nIncAlign;
                pComponentPrivate->overlay->iBBuff   += 3*nIncAlign;
                nIncAlign = 1;
            }   
        }

        pu1 = uu;
        pu2 = uu+iWidth+iAlign;
        pv1 = vv;
        pv2 = vv+iWidth+iAlign;

        for (wCpt=0; wCpt < iWidth; wCpt += 2)
        {
            *u++ = (OMX_U8)(((OMX_S32)(*pu1+2*(*(pu1+1))+*(pu1+2)+*pu2+2*(*(pu2+1))+*(pu2+2)))>>3);
            *v++ = (OMX_U8)(((OMX_S32)(*pv1+2*(*(pv1+1))+*(pv1+2)+*pv2+2*(*(pv2+1))+*(pv2+2)))>>3);

            *w = 0;
            (*w) += (*(pu1  )!=0 || *(pv1 )!=0) ?0:1;           
            (*w) += (*(pu1+1)!=0 || *(pv1+1)!=0)?0:2;
            (*w) += (*(pu1+2)!=0 || *(pv1+2)!=0)?0:1;
            (*w) += (*(pu2  )!=0 || *(pv2  )!=0)?0:1;
            (*w) += (*(pu2+1)!=0 || *(pv2+1)!=0)?0:2;
            (*w) += (*(pu2+2)!=0 || *(pv2+2)!=0)?0:1;
            
            w++;
            pu1 += 2; 
            pv1 += 2; 
            pu2 += 2; 
            pv2 += 2; 
        }
    }
}

/*  Convert  buffer YUV420W planar to TI propietary file for overlaying post-processing 
//  The format is two lines of luminance followed with one line of interlaced Cb anc Cr value and followed by one Weight line in 16 dword size 
//  Y(k)   Y1     Y2     Y3     Y4     first Y line of image) 
//  Y(k+1) Y1     Y2     Y3     Y4     Y5(seconde Y line of image) 
//  C(k)   Cb1Cr1 Cb2Cr2 Cb3Cr3 Cb4Cr4 (one interlace line of Cb and Cr) 
//  W(k)   [0]W1  [0]W2  [0]W3  [0]W4  (One weight line in dword size) */
static void ConvertFormatFromPlanar(OMX_U8 *apInBufferYUV420W, OMX_U8 *apTIinternalFormat)
{
    OMX_S32    wCpt;
    OMX_S32    hCpt;
    OMX_S32    yCpt     = iHeight-1;
    OMX_U8  nUvalue  = 0;
    OMX_U8  nVvalue  = 0;
    OMX_U8  nWeight  = 0;
    OMX_U8* pYbuffer = apInBufferYUV420W;
    OMX_U8* pUbuffer = (pYbuffer+((OMX_S32)(iWidth)*iHeight));
    OMX_U8* pVbuffer = (pUbuffer+((OMX_S32)(iWidth)*iHeight/4));
    OMX_U8* pWbuffer = (pVbuffer+((OMX_S32)(iWidth)*iHeight/4));

    /* Perform copy of Y data with byte swapp for DSP DMA */
    for (hCpt=((iHeight)/2-1); hCpt>=0; hCpt--)
    {
        for(wCpt = 0; (OMX_U32)wCpt < iWidth; wCpt += 4)    
        {   
            *apTIinternalFormat++ = *(pYbuffer+1+(wCpt)+yCpt*iWidth);
            *apTIinternalFormat++ = *(pYbuffer+0+(wCpt)+yCpt*iWidth);
            *apTIinternalFormat++ = *(pYbuffer+3+(wCpt)+yCpt*iWidth);
            *apTIinternalFormat++ = *(pYbuffer+2+(wCpt)+yCpt*iWidth);
        }
        yCpt -= 1;
        for (wCpt = 0; (OMX_U32)wCpt < iWidth; wCpt += 4)    
        {   
            *apTIinternalFormat++ = *(pYbuffer+0+(wCpt)+yCpt*iWidth);
            *apTIinternalFormat++ = *(pYbuffer+1+(wCpt)+yCpt*iWidth);
            *apTIinternalFormat++ = *(pYbuffer+2+(wCpt)+yCpt*iWidth);
            *apTIinternalFormat++ = *(pYbuffer+3+(wCpt)+yCpt*iWidth);
        }
        yCpt -= 1;

        for (wCpt = 0; (OMX_U32)wCpt < iWidth/2; wCpt++)    
        {   
            nUvalue = *(pUbuffer+wCpt+hCpt*(iWidth/2));
            nVvalue = *(pVbuffer+wCpt+hCpt*(iWidth/2));
            if(nUvalue !=0 || nVvalue !=0)
            {
                nWeight  = *(pWbuffer+wCpt+hCpt*(iWidth/2));
                nUvalue -= (8-nWeight)<<4;
                nVvalue -= (8-nWeight)<<4;
            }
            *apTIinternalFormat++ = nVvalue;
            *apTIinternalFormat++ = nUvalue;
        }

        for (wCpt = 0; (OMX_U32)wCpt < iWidth/2; wCpt++)
        {
            *apTIinternalFormat++ = (OMX_U8)0;
            *apTIinternalFormat++ = *(pWbuffer+wCpt+hCpt*(iWidth/2));
        }
    }
}




