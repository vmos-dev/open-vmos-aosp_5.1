/**
 * @defgroup MCLF   MobiCore Load Format
 *
 * @defgroup MCLF_VER    MCLF Versions
 * @ingroup MCLF
 *
 * @addtogroup MCLF
 * @{
 *
 * MobiCore Load Format declarations.
 *
 * Holds the definitions for the layout of MobiCore Trustlet Blob.
 * <!-- Copyright Giesecke & Devrient GmbH 2009-2012 -->
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef MCLOADFORMAT_H_
#define MCLOADFORMAT_H_

#include "mcUuid.h"
#include "mcDriverId.h"

#define MCLF_VERSION_MAJOR   2
#define MCLF_VERSION_MINOR   1

#define MC_SERVICE_HEADER_MAGIC_BE         ((uint32_t)('M'|('C'<<8)|('L'<<16)|('F'<<24))) /**< "MCLF" in big endian integer representation */
#define MC_SERVICE_HEADER_MAGIC_LE         ((uint32_t)(('M'<<24)|('C'<<16)|('L'<<8)|'F')) /**< "MCLF" in little endian integer representation */
#define MC_SERVICE_HEADER_MAGIC_STR         "MCLF"                                        /**< "MCLF" as string */

/** @name MCLF flags */
/*@{*/
#define MC_SERVICE_HEADER_FLAGS_PERMANENT               (1U << 0) /**< Loaded service cannot be unloaded from MobiCore. */
#define MC_SERVICE_HEADER_FLAGS_NO_CONTROL_INTERFACE    (1U << 1) /**< Service has no WSM control interface. */
#define MC_SERVICE_HEADER_FLAGS_DEBUGGABLE              (1U << 2) /**< Service can be debugged. */
/*@}*/

#if !defined(ADDR_T_DEFINED)
#define ADDR_T_DEFINED
typedef void*    addr_t;                /**< an address, can be physical or virtual */
#endif // !defined(ADDR_T_DEFINED)

/** Service type.
 * The service type defines the type of executable.
 */
typedef enum {
    SERVICE_TYPE_ILLEGAL    = 0,        /**< Service type is invalid. */
    SERVICE_TYPE_DRIVER     = 1,        /**< Service is a driver. */
    SERVICE_TYPE_SP_TRUSTLET   = 2,     /**< Service is a Trustlet. */
    SERVICE_TYPE_SYSTEM_TRUSTLET = 3    /**< Service is a system Trustlet. */
} serviceType_t;

/**
 * Memory types.
 */
typedef enum {
    MCLF_MEM_TYPE_INTERNAL_PREFERRED = 0, /**< If available use internal memory; otherwise external memory. */
    MCLF_MEM_TYPE_INTERNAL = 1, /**< Internal memory must be used for executing the service. */
    MCLF_MEM_TYPE_EXTERNAL = 2, /**< External memory must be used for executing the service. */
} memType_t;

/**
 * Descriptor for a memory segment.
 */
typedef struct {
    addr_t      start;  /**< Virtual start address. */
    uint32_t    len;    /**< Length of the segment in bytes. */
} segmentDescriptor_t, *segmentDescriptor_ptr;

/**
 * MCLF intro for data structure identification.
 * Must be the first element of a valid MCLF file.
 */
typedef struct {
    uint32_t        magic;      /**< Header magic value ASCII "MCLF". */
    uint32_t        version;    /**< Version of the MCLF header structure. */
} mclfIntro_t, *mclfIntro_ptr;

/** @} */


// Version 2 /////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @defgroup MCLF_VER_V2   MCLF Version 2
 * @ingroup MCLF_VER
 *
 * @addtogroup MCLF_VER_V2
 * @{
 */

/**
 * Version 2 MCLF header.
 */
typedef struct {
    mclfIntro_t             intro;           /**< MCLF header start with the mandatory intro. */
    uint32_t                flags;           /**< Service flags. */
    memType_t               memType;         /**< Type of memory the service must be executed from. */
    serviceType_t           serviceType;     /**< Type of service. */

    uint32_t                numInstances;    /**< Number of instances which can be run simultaneously. */
    mcUuid_t                uuid;            /**< Loadable service unique identifier (UUID). */
    mcDriverId_t            driverId;        /**< If the serviceType is SERVICE_TYPE_DRIVER the Driver ID is used. */
    uint32_t                numThreads;      /**<
                                              * <pre>
                                              * <br>Number of threads (N) in a service depending on service type.<br>
                                              *
                                              *   SERVICE_TYPE_SP_TRUSTLET: N = 1
                                              *   SERVICE_TYPE_SYSTEM_TRUSTLET: N = 1
                                              *   SERVICE_TYPE_DRIVER: N >= 1
                                              * </pre>
                                              */
    segmentDescriptor_t     text;           /**< Virtual text segment. */
    segmentDescriptor_t     data;           /**< Virtual data segment. */
    uint32_t                bssLen;         /**< Length of the BSS segment in bytes. MUST be at least 8 byte. */
    addr_t                  entry;          /**< Virtual start address of service code. */
    uint32_t                serviceVersion; /**< Version of the interface the driver exports. */
} mclfHeaderV2_t, *mclfHeaderV2_ptr;
/** @} */


/**
 * Version 2 MCLF text segment header.
 * Required to be present in MobiCore 1.2 components at address (0x1080).
 * This extension is initialized already at trustlet compile time,
 * but may be modified later by configuration tools and by MobiCore at load time.
 */
typedef struct {
    uint32_t                version;        /**< Version of the TextHeader structure. */
    uint32_t                textHeaderLen;  /**< Size of this structure (fixed at compile time) */
    uint32_t                requiredFeat;   /**< Flags to indicate features that Mobicore must understand/interprete when loading.
                                                 Initial value set at compile time.
                                                 Required always. */
    addr_t                  mcLibEntry;     /**< Address for McLib entry.
                                                 Mobicore sets at load time for trustlets / drivers.
                                                 Required always. */
    segmentDescriptor_t     mcLibData;      /**< Segment for McLib data.
                                                 Set at compile time.
                                                 Required always. */
    addr_t                  mcLibBase;      /**< McLib base address.
                                                 Mobicore sets at load time for trustlets / drivers.
                                                 Required always. */
} mclfTextHeader_t, *mclfTextHeader_ptr;

// Version 2 ///////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @addtogroup MCLF
 * @{
 */

/** MCLF header */
typedef union {
    mclfIntro_t    intro;           /**< Intro for data structure identification. */
    mclfHeaderV2_t mclfHeaderV2;    /**< Version 2 header */
} mclfHeader_t, *mclfHeader_ptr;

#endif /* MCLOADFORMAT_H_ */

/** @} */
