/*
 * dspbridge/mpu_api/inc/qosregistry.h
 *
 * DSP-BIOS Bridge driver support functions for TI OMAP processors.
 *
 * Copyright (C) 2007 Texas Instruments, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published 
 * by the Free Software Foundation version 2.1 of the License.
 *
 * This program is distributed .as is. WITHOUT ANY WARRANTY of any kind,
 * whether express or implied; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */


#ifndef __QOSTI_H__

#define __QOSTI_H__

#include <dbapi.h>

/*  ============================================================================

  name   TLoadMode



  desc   The node load mode for qos.

 ============================================================================ */

enum TLoadMode
{

	EStaticLoad,

	EDynamicLoad
};

/*  ============================================================================

  name    CQosTI



  desc    An example class that uses the DSP BIOS/Bridge interfaces.and

          demonstrates creating an xDAIS Socket Node on the DSP. It sends

          messages and data buffers to the DSP, and then receives the qosd

          data back from the DSP for display.



  ============================================================================

	RHwaOmap            iDsp ;

  ============================================================================

  name   TQosResourceID



  desc   List of available resource types

  ============================================================================
 */

typedef enum _QOSDATATYPE {

	QOSDataType_Memory_DynLoad = 0,

	QOSDataType_Memory_DynAlloc,

	QOSDataType_Memory_Scratch,

	QOSDataType_Processor_C55X,

	QOSDataType_Processor_C6X,

	QOSDataType_Peripheral_DMA,

	QOSDataType_Stream,

	QOSDataType_Component,

	QOSDataType_Registry,

	QOSDataType_DynDependentLibrary
} QOSDATATYPE;

#define QOS_USER_DATA_TYPE	0x80000000	/* Flag indicating a application-defined
										   data type ID */

/*  ============================================================================

  name   QOSDATA



  desc   Generic data for resource management is described by the following
  structure

  ============================================================================
*/
struct QOSDATA {

	ULONG Id;

	struct QOSDATA *Next;

	 ULONG(*TypeSpecific)(struct QOSDATA *DataObject, ULONG FunctionCode,
			 											ULONG Parameter1);	
	 /* ptr to type-specific func. */

	ULONG Size;		/* size of data plus this header */

	char Data[];

};

//  ============================================================================

//  name   QOSFNTYPESPECIFIC

//

//  desc   Pointer to type-specific function handler for the data object

//  ============================================================================

typedef ULONG(*QOSFNTYPESPECIFIC)(struct QOSDATA *DataObject,ULONG FunctionCode,
															ULONG Parameter1);

ULONG QOS_Memory_Scratch_FunctionHandler(struct QOSDATA *DataObject,
										ULONG FunctionCode, ULONG Parameter1);

ULONG QOS_Memory_DynAlloc_FunctionHandler(struct QOSDATA *DataObject,
										ULONG FunctionCode, ULONG Parameter1);

ULONG QOS_Memory_DynLoad_FunctionHandler(struct QOSDATA *DataObject,
										ULONG FunctionCode, ULONG Parameter1);

ULONG QOS_Processor_FunctionHandler(struct QOSDATA *DataObject,
										ULONG FunctionCode, ULONG Parameter1);

ULONG QOS_Resource_DefaultFunctionHandler(struct QOSDATA *DataObject,
										  ULONG FunctionCode, ULONG Parameter1);

ULONG QOS_Component_DefaultFunctionHandler(struct QOSDATA *DataObject,
										ULONG FunctionCode, ULONG Parameter1);

ULONG QOS_DynDependentLibrary_FunctionHandler(struct QOSDATA *DataObject,
									      ULONG FunctionCode, ULONG Parameter1);

ULONG QOS_Registry_FunctionHandler(struct QOSDATA *DataObject,
										ULONG FunctionCode, ULONG Parameter1);

/*  ============================================================================

  name   QOSREGISTRY



  desc   The QOSREGISTRY structure contains a list of all resources and
  components in the system


  ============================================================================
*/

struct QOSREGISTRY {

	struct QOSDATA data;

	struct QOSDATA *ResourceRegistry;

	struct QOSDATA *ComponentRegistry;

};

//  ============================================================================

//  name   QOSRESOURCE_MEMORY

//

//  desc   System memory resources are characterized by the following structure

//  ============================================================================

struct QOSRESOURCE_MEMORY {

	struct QOSDATA data;

	UINT align;		/* alignment of memory heap */

	UINT heapId;		/* resource heap ID */

	UINT size;		/* size of memory heap */

	UINT type;		/* type of memory: prefer/require/scratch/persist */

	UINT allocated;		/* size of heap in use (not free) */

	UINT largestfree;	/* size of largest contiguous free block */

	UINT group;		/* scratch group ID (only for scratch memory) */

};

/*  ============================================================================

  name   QOSRESOURCE_PROCESSOR



  desc   Each processor is described by its processor type, hardware
  attributes, and available processing cycles (MIPS). 

  ============================================================================*/

struct QOSRESOURCE_PROCESSOR {

	struct QOSDATA data;

	UINT MaxMips;		/* max cpu cycles required for component */

	UINT TypicalMips;	/* typical cpu cycles required */

	UINT MaxCycles;		/* max cpu cycles for single iteration */

	UINT TypicalCycles;	/* typical cpu cycles for single iteration */

	UINT Utilization;	/* percentage of time cpu is idle */

	UINT currentLoad;

	UINT predLoad;

	UINT currDspFreq;

	UINT predictedFreq;

};
	

/*  ============================================================================

  name   QOSRESOURCE_STREAM



  desc   Bridge Streams are introduced as a QoS resource structure

  ============================================================================
*/

struct QOSRESOURCE_STREAM {

	struct QOSDATA data;

	struct DSP_STRMATTR Attrs;	/* Stream attributes for this stream */

	UINT Direction;		/* DSP_TONODE or DSP_FROMNODE */

};

 struct QOSDYNDEPLIB {

	struct QOSDATA data;

	struct DSP_UUID depLibUuid;	/* UUID of Dynamic Dependent Library */

	const CHAR *depLibPath;	/* Path to Dynamic Dependent Library */

};

/*  ============================================================================

  name   QOSCOMPONENT



  desc   Bridge Streams are introduced as a QoS resource structure

  ============================================================================
*/  

struct QOSCOMPONENT {

	struct QOSDATA data;

	UINT InUse;		/* Count of instances of this component in use */

	UINT aTaskId;

	UINT VariantID;

	UINT InterfaceID;

	struct DSP_UUID NodeUuid;

	PVOID dynNodePath;

	struct QOSDATA *resourceList;

	struct QOSDYNDEPLIB *dynDepLibList;

};

/*  ============================================================================

  name   Registry-specific QOS_FN_xxx definitions



  desc   These are defines for the registry-specific function codes

  ============================================================================
 */

#define QOS_FN_GetNumDynAllocMemHeaps 		1

#define QOS_FN_HasAvailableResource			2

/*  ============================================================================

  name   Resource-specific QOS_FN_xxx definitions



  desc   These are defines for the resource-specific function codes

  ============================================================================
*/

#define QOS_FN_ResourceIsAvailable	1

#define QOS_FN_ResourceUpdateInfo	2

//  ============================================================================

/*  name        DSPRegistry_Create



	Implementation

		Creates empty Registry, then adds all the default system resources

	Parameters

		none

	Return

		QOSREGISTRY*	ptr to new system registry

		NULL			Failure (out of memory)

	Requirement Coverage

		This method addresses requirement(s):  SR10085

*/

struct QOSREGISTRY *DSPRegistry_Create();

/*  ============================================================================

  name        DSPRegistry_Delete



	Implementation

		Deletes Registry and cleans up QoS Gateway & Registry objects that it
		owns.

	Parameters

		registry		ptr to previously created registry

	Return

		none

	Requirement Coverage

		This method addresses requirement(s):  SR10085

*/

void DSPRegistry_Delete(struct QOSREGISTRY *registry);

/*  ============================================================================

  name        DSPRegistry_Find



	Implementation

		Finds resource(s) or component(s) that match the given Id. For
		resources, each matching

		resource's TypeSpecific function is called with the function 
		ID QOS_FN_ResourceUpdateInfo to 

		ensure that all resources have current data in their structures.

	Parameters	

		Id	requested Id

		registry		system registry

		ResultList		ptr to results array

		Size			ptr to ULONG number of entries available in array

	Return

		DSP_OK			successful

		DSP_ESIZE		block for results is too small

		DSP_ENOTFOUND	item not found

	Requirement Coverage

		This method addresses requirement(s):  SR10008

*/

DSP_STATUS DSPRegistry_Find(UINT Id, struct QOSREGISTRY *registry,
									struct QOSDATA **ResultList, ULONG *Size);

/*  ============================================================================

  name        DSPRegistry_Add



	Implementation

		Add given resource or component to the list

	Parameters

		listhead		system registry (in the case of adding resources or
		components to the system)

						or component (in the case of adding required resources
						to a component)

		entry			entry to add in list

	Return

		DSP_STATUS		Error code or DSP_SOK for success

	Requirement Coverage

		This method addresses requirement(s):  SR10085

*/

DSP_STATUS DSPRegistry_Add(struct QOSDATA *listhead, struct QOSDATA *entry);

/* ============================================================================

  name        DSPRegistry_Remove



	Implementation

		Removes given resource or component from the list

	Parameters

		listhead		system registry (in the case of removing resources or
		components from the system)

						or component (in the case of removing required
						resources from a component)

		entry			resource or component to remove

	Return

		DSP_STATUS	Error code or DSP_SOK for success

	Requirement Coverage

		This method addresses requirement(s):  SR10085

*/

DSP_STATUS DSPRegistry_Remove(struct QOSDATA *listhead, struct QOSDATA *entry);

/*  ============================================================================

  name        DSPQos_TypeSpecific



	Implementation

		Calls the type-specific function defined for this data type. 
		Internally, this is implemented 

		as a call to the QOSDATA structure's TypeSpecific() function. 

	Parameters	

		DataObject		Far pointer to the structure for the data object

		FunctionCode	Type-specific function code

		Parameter1		Function-specific parameter

	Return

		ULONG			Function-specific return code.

	Requirement Coverage	

		This method addresses requirement(s):  SR10085, SR10008

*/

ULONG DSPQos_TypeSpecific(struct QOSDATA *DataObject, ULONG FunctionCode,
															ULONG Parameter1);

/*  ============================================================================

  name        DSPComponent_Register



	Implementation

		Informs Registry that the given component is using system resources.
		Internally, this 

		increments the InUse field of the QOSCOMPONENT structure.

	Parameters

		registry		system registry

		comp			component using system resources

	Return

		DSP_STATUS		Error code or DSP_SOK for success

	Requirement Coverage

		This method addresses requirement(s):  SR10085

*/

DSP_STATUS DSPComponent_Register(struct QOSREGISTRY *registry,
													struct QOSCOMPONENT *comp);

/*  ============================================================================

  name        DSPComponent_Unregister



	Implementation

		Informs Registry that component is no longer using system resources.
		Internally, this 

		decrements the InUse field of the QOSCOMPONENT structure.

	Parameters

		registry		system registry

		comp			component releasing system resources

	Return

		DSP_STATUS		Error code or DSP_SOK for success

	Requirement Coverage

		This method addresses requirement(s):  SR10085

*/

DSP_STATUS DSPComponent_Unregister(struct QOSREGISTRY *registry,
													struct QOSCOMPONENT *comp);

/*  ============================================================================

  name        DSPData_Create



	Implementation

		Allocates and initializes a QOSDATA structure.

	Parameters

		id				type of data

	Return

		QOSDATA *		ptr to data structure or NULL for failure

	Requirement Coverage

		This method addresses requirement(s):

*/

struct QOSDATA *DSPData_Create(ULONG id);

/*  ============================================================================

  name        DSPData_Delete



	Implementation

		Deletes a QOSDATA structure, recursively deleting any associated lists.

	Parameters

		data			ptr to data structure to delete

	Return

		DSP_STATUS		Error code or DSP_SOK for success

	Requirement Coverage

		This method addresses requirement(s):

*/

DSP_STATUS DSPData_Delete(struct QOSDATA *data);

/*  ============================================================================

  name        DSPData_IsResource



	Implementation

		Determines whether a QOSDATA structure Id is a "resource" type ID.

	Parameters

		Id				type ID to check

	Return

		bool			TRUE for resources, FALSE otherwise.

	Requirement Coverage

		This method addresses requirement(s):

*/

bool DSPData_IsResource(ULONG Id);

#endif

