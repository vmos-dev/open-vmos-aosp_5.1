/*
 * Copyright (c) 2010, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 *  @file  profile.c
 *         This file contains methods to profile DOMX
 *
 *  @path ...\hardware\ti\domx\domx\profiling\inc
 *
 *  @rev 1.0
 */

/******************************************************************
 *   INCLUDE FILES
 ******************************************************************/
/* ----- system and platform files ----------------------------*/
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _Android
#include <cutils/properties.h>
#endif

#include <OMX_Types.h>
#include <OMX_Component.h>

/*-------program files ----------------------------------------*/
#include "omx_rpc_utils.h"
#include "omx_proxy_common.h"
#include "profile.h"


/******************************************************************
 *   DEFINES - CONSTANTS
 ******************************************************************/
/* Events that can be dynamically enabled */
enum KPI_STATUS {
	KPI_BUFFER_EVENTS = 1
};

/* OMX buffer events per component */
typedef struct {
	OMX_HANDLETYPE hComponent;
	OMX_U32 count_ftb;
	OMX_U32 count_fbd;
	OMX_U32 count_etb;
	OMX_U32 count_ebd;
	char name[50];
} kpi_omx_component;

/* we trace up to MAX_OMX_COMP components */
#define MAX_OMX_COMP 8


/***************************************************************
 * kpi_omx_monitor
 * -------------------------------------------------------------
 * Contains up to 8 components data
 *
 ***************************************************************/
kpi_omx_component kpi_omx_monitor[MAX_OMX_COMP]; /* we trace up to MAX_OMX_COMP components */
OMX_U32 kpi_omx_monitor_cnt = 0; /* no component yet */
unsigned int kpi_status = 0;


/* ===========================================================================*/
/**
 * @name KPI_GetTime()
 * @brief Compute time since boot to timestamp events
 * @param void
 * @return OMX_U64 = time since boot in us
 * @sa TBD
 *
 */
/* ===========================================================================*/
OMX_U64 KPI_GetTime(void)
{
	struct timespec tp;

	clock_gettime(CLOCK_MONOTONIC, &tp);
	return (tp.tv_sec * 1000000 + tp.tv_nsec / 1000);
}

/* ===========================================================================*/
/**
 * @name KPI_OmxCompKpiUpdateStatus()
 * @brief Update dynamic activation of traces
 * @param void
 * @return void
 * @sa TBD
 *
 */
/* ===========================================================================*/
void KPI_OmxCompKpiUpdateStatus(void)
{
        char *val = getenv("DEBUG_DOMX_KPI_STATUS");

        if (val)
        {
                kpi_status = strtol(val, NULL, 0);
        }
#ifdef _Android
        else
        {
                char value[PROPERTY_VALUE_MAX];
                int val;

                property_get("debug.domx.kpi_status", value, "0");
                val = atoi(value);
                if (val >= 0)
                        kpi_status = val;
        }
#endif
}

/* ===========================================================================*/
/**
 * @name KPI_OmxCompInit()
 * @brief Prepare monitoring structure for new component starting
 * @param void
 * @return void
 * @sa TBD
 *
 */
/* ===========================================================================*/
void KPI_OmxCompInit(OMX_HANDLETYPE hComponent)
{
	OMX_VERSIONTYPE nVersionComp;
	OMX_VERSIONTYPE nVersionSpec;
	OMX_UUIDTYPE    compUUID;
	char compName[OMX_MAX_STRINGNAME_SIZE];
	char* p;
	OMX_U32 omx_cnt;
	struct timespec tp;

	/* Check if some profiling events have been enabled/disabled */
	KPI_OmxCompKpiUpdateStatus();

	if ( !(kpi_status & KPI_BUFFER_EVENTS) )
		return;

	/* First init: clear kpi_omx_monitor components */
	if( kpi_omx_monitor_cnt == 0) {
		for (omx_cnt = 0; omx_cnt < MAX_OMX_COMP; omx_cnt++) {
			/*clear handler registry */
			kpi_omx_monitor[omx_cnt].hComponent = 0;
		}
	}

	/* find an empty monitoring structure */
	for( omx_cnt = 0; omx_cnt < MAX_OMX_COMP;  omx_cnt++ ) {
		if( kpi_omx_monitor[omx_cnt].hComponent == 0 ) break;
	}

	/* too omany components started, do not monitor */
	if( omx_cnt >= MAX_OMX_COMP) return;

	/* current comp num and update */
	kpi_omx_monitor_cnt++;

	/* register the component handle */
	kpi_omx_monitor[omx_cnt].hComponent = hComponent;

	/* reset event counts */
	kpi_omx_monitor[omx_cnt].count_ftb = 0;
	kpi_omx_monitor[omx_cnt].count_fbd = 0;
	kpi_omx_monitor[omx_cnt].count_etb = 0;
	kpi_omx_monitor[omx_cnt].count_ebd = 0;

	/* register the component name */
	((OMX_COMPONENTTYPE*) hComponent)->GetComponentVersion(hComponent, compName, &nVersionComp, &nVersionSpec, &compUUID);

	/* get the end of the string compName... */
	p = compName + strlen( compName ) - 1;
	while( (*p != '.' ) && (p != compName) ) p--;
	strncpy(kpi_omx_monitor[omx_cnt].name, p + 1, 6);

	/* trace component init */
	DOMX_PROF("<KPI> OMX %-6s Init %-8lld", kpi_omx_monitor[omx_cnt].name, KPI_GetTime());

	return;
}

/* ===========================================================================*/
/**
 * @name KPI_OmxCompDeinit()
 * @brief Reset monitoring structure for component stopping
 * @param void
 * @return void
 * @sa TBD
 *
 */
/* ===========================================================================*/
void KPI_OmxCompDeinit( OMX_HANDLETYPE hComponent)
{
	OMX_U32 omx_cnt;

	if ( !(kpi_status & KPI_BUFFER_EVENTS) )
		return;

	if( kpi_omx_monitor_cnt == 0) return;

	/* identify the component from the registry */
	for( omx_cnt = 0; omx_cnt < MAX_OMX_COMP;  omx_cnt++ ) {
		if( kpi_omx_monitor[omx_cnt].hComponent == hComponent ) break;
	}

	/* trace component init */
	DOMX_PROF( "<KPI> OMX %-6s Deinit %-8lld", kpi_omx_monitor[omx_cnt].name, KPI_GetTime());

	/* unregister the component */
	kpi_omx_monitor[omx_cnt].hComponent = 0;

	kpi_omx_monitor_cnt--;

	return;
}

/* ===========================================================================*/
/**
 * @name KPI_OmxCompBufferEvent()
 * @brief Trace FTB/ETB/FBD/EBD events
 * @param void
 * @return void
 * @sa TBD
 *
 */
/* ===========================================================================*/
void KPI_OmxCompBufferEvent(enum KPI_BUFFER_EVENT event, OMX_HANDLETYPE hComponent, PROXY_BUFFER_INFO* pBuffer)
{
        OMX_U32 omx_cnt;

	if ( !(kpi_status & KPI_BUFFER_EVENTS) )
		return;

        if (kpi_omx_monitor_cnt == 0) return;

        /* identify the component from the registry */
        for (omx_cnt = 0; omx_cnt < MAX_OMX_COMP;  omx_cnt++) {
                if( kpi_omx_monitor[omx_cnt].hComponent == hComponent ) break;
        }

        /* Update counts and trace the event */
        if( omx_cnt < MAX_OMX_COMP ) {
                /* trace the event, we trace remote address to correlate to Ducati trace */
		switch(event) {
			case KPI_BUFFER_ETB:
				DOMX_PROF("ETB %-6s %-4u %-8lld x%-8x", kpi_omx_monitor[omx_cnt].name, \
					(unsigned int)++kpi_omx_monitor[omx_cnt].count_etb, KPI_GetTime(), (unsigned int)pBuffer->pBufHeaderRemote);
			break;
			case KPI_BUFFER_FTB:
				DOMX_PROF("FTB %-6s %-4u %-8lld x%-8x", kpi_omx_monitor[omx_cnt].name, \
					(unsigned int)++kpi_omx_monitor[omx_cnt].count_ftb, KPI_GetTime(), (unsigned int)pBuffer->pBufHeaderRemote);
			break;
			case KPI_BUFFER_EBD:
				DOMX_PROF("EBD %-6s %-4u %-8lld x%-8x", kpi_omx_monitor[omx_cnt].name, \
					(unsigned int)++kpi_omx_monitor[omx_cnt].count_ebd, KPI_GetTime(), (unsigned int)pBuffer->pBufHeaderRemote);
			break;
			/* we add timestamp metadata because this is a unique identifier of buffer among all SW layers */
			case KPI_BUFFER_FBD:
		                DOMX_PROF("FBD %-6s %-4u %-8lld x%-8x %lld", kpi_omx_monitor[omx_cnt].name, \
					(unsigned int)++kpi_omx_monitor[omx_cnt].count_fbd, KPI_GetTime(), (unsigned int)pBuffer->pBufHeaderRemote, pBuffer->pBufHeader->nTimeStamp);
			break;

		}
        }

        return;
}
