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
 


#ifndef _DYNAMIC_LOADER_H_
#define _DYNAMIC_LOADER_H_
#include <stdarg.h>
#ifndef __KERNEL__
#include <stdint.h>
#else
#include <linux/types.h>
#endif

#ifdef __cplusplus
extern "C" {			/* C-only version */
#endif

/* Optional optimization defines */
#define OPT_ELIMINATE_EXTRA_DLOAD 1
/* #define OPT_ZERO_COPY_LOADER 1 */


/*
 * Dynamic Loader
 *
 * The function of the dynamic loader is to load a "module" containing 
 * instructions
 * for a "target" processor into that processor.  In the process it assigns 
 * memory
 * for the module, resolves symbol references made by the module, and remembers
 * symbols defined by the module.
 *
 * The dynamic loader is parameterized for a particular system by 4 classes
 * that supply
 * the module and system specific functions it requires
 */
	/* The read functions for the module image to be loaded */
	struct Dynamic_Loader_Stream;	
	/*typedef struct Dynamic_Loader_Stream Dynamic_Loader_Stream;*/

	/* This class defines "host" symbol and support functions */
	struct Dynamic_Loader_Sym;
	/*typedef struct Dynamic_Loader_Sym Dynamic_Loader_Sym;*/

	/* This class defines the allocator for "target" memory */
	struct Dynamic_Loader_Allocate;	
	/*typedef struct Dynamic_Loader_Allocate Dynamic_Loader_Allocate;*/

	/* This class defines the copy-into-target-memory functions */
	struct Dynamic_Loader_Initialize;	
	/*typedef struct Dynamic_Loader_Initialize Dynamic_Loader_Initialize;*/

/*
 * Option flags to modify the behavior of module loading
 */
#define DLOAD_INITBSS 0x1	/* initialize BSS sections to zero */
#define DLOAD_BIGEND 0x2	/* require big-endian load module */
#define DLOAD_LITTLE 0x4	/* require little-endian load module */

	typedef void *DLOAD_mhandle;	/* module handle for loaded modules */

/*****************************************************************************
 * Procedure Dynamic_Load_Module
 *
 * Parameters:
 *  module  The input stream that supplies the module image
 *  syms    Host-side symbol table and malloc/free functions
 *  alloc   Target-side memory allocation
 *  init    Target-side memory initialization, or NULL for symbol read only
 *  options Option flags DLOAD_*
 *  mhandle A module handle for use with Dynamic_Unload
 *
 * Effect:
 *  The module image is read using *module.  Target storage for the new image is
 * obtained from *alloc.  Symbols defined and referenced by the module are
 * managed using *syms.  The image is then relocated and references resolved
 * as necessary, and the resulting executable bits are placed into target memory
 * using *init.
 *
 * Returns:
 *  On a successful load, a module handle is placed in *mhandle, and zero is
 * returned.  On error, the number of errors detected is returned.  Individual
 * errors are reported during the load process using syms->Error_Report().
 *****************************************************************************/
	extern int Dynamic_Load_Module(
					/* the source for the module image*/
					struct Dynamic_Loader_Stream * module,	
					   /* host support for symbols and storage*/
				       struct Dynamic_Loader_Sym * syms,	
					   /* the target memory allocator*/
				       struct Dynamic_Loader_Allocate * alloc,	
					   /* the target memory initializer*/
				       struct Dynamic_Loader_Initialize * init,
				       unsigned options,	/* option flags*/
				       DLOAD_mhandle * mhandle	/* the returned module handle*/
	    );

#ifdef OPT_ELIMINATE_EXTRA_DLOAD
/*****************************************************************************
 * Procedure Dynamic_Open_Module
 *
 * Parameters:
 *  module  The input stream that supplies the module image
 *  syms    Host-side symbol table and malloc/free functions
 *  alloc   Target-side memory allocation
 *  init    Target-side memory initialization, or NULL for symbol read only
 *  options Option flags DLOAD_*
 *  mhandle A module handle for use with Dynamic_Unload
 *
 * Effect:
 *  The module image is read using *module.  Target storage for the new image is
 * obtained from *alloc.  Symbols defined and referenced by the module are
 * managed using *syms.  The image is then relocated and references resolved
 * as necessary, and the resulting executable bits are placed into target memory
 * using *init.
 *
 * Returns:
 *  On a successful load, a module handle is placed in *mhandle, and zero is
 * returned.  On error, the number of errors detected is returned.  Individual
 * errors are reported during the load process using syms->Error_Report().
 *****************************************************************************/
        extern int Dynamic_Open_Module(struct Dynamic_Loader_Stream * module,  // the source for the module image
                                       struct Dynamic_Loader_Sym * syms,       // host support for symbols and storage
                                       struct Dynamic_Loader_Allocate * alloc, // the target memory allocator
                                       struct Dynamic_Loader_Initialize * init,        // the target memory initializer
                                       unsigned options,        // option flags
                                       DLOAD_mhandle * mhandle  // the returned module handle
            );
#endif

/*****************************************************************************
 * Procedure Dynamic_Unload_Module
 *
 * Parameters:
 *  mhandle A module handle from Dynamic_Load_Module
 *  syms    Host-side symbol table and malloc/free functions
 *  alloc   Target-side memory allocation
 *
 * Effect:
 *  The module specified by mhandle is unloaded.  Unloading causes all
 * target memory to be deallocated, all symbols defined by the module to
 * be purged, and any host-side storage used by the dynamic loader for
 * this module to be released.
 *
 * Returns:
 *  Zero for success. On error, the number of errors detected is returned.
 * Individual errors are reported using syms->Error_Report().
 *****************************************************************************/
	extern int Dynamic_Unload_Module(DLOAD_mhandle mhandle,	/* the module
															   handle*/
					 /* host support for symbols and storage*/
					 struct Dynamic_Loader_Sym * syms,	
					 /* the target memory allocator*/
					 struct Dynamic_Loader_Allocate * alloc,	
					 /* the target memory initializer*/
					 struct Dynamic_Loader_Initialize * init	
	    );

/*****************************************************************************
 *****************************************************************************
 * A class used by the dynamic loader for input of the module image
 *****************************************************************************
 *****************************************************************************/
	struct Dynamic_Loader_Stream {
/* public: */
    /*************************************************************************
     * read_buffer
     *     
     * PARAMETERS :
     *  buffer  Pointer to the buffer to fill
     *  bufsiz  Amount of data desired in sizeof() units
     *
     * EFFECT :
     *  Reads the specified amount of data from the module input stream
     * into the specified buffer.  Returns the amount of data read in sizeof()
     * units (which if less than the specification, represents an error).
     *
     * NOTES:
     *  In release 1 increments the file position by the number of bytes read
     *
     *************************************************************************/
		int (*read_buffer) (struct Dynamic_Loader_Stream * thisptr,
				    void *buffer, unsigned bufsiz);

    /*************************************************************************
     * set_file_posn (release 1 only)
     *     
     * PARAMETERS :
     *  posn  Desired file position relative to start of file in sizeof() units.
     *
     * EFFECT :
     *  Adjusts the internal state of the stream object so that the next
     * read_buffer call will begin to read at the specified offset from
     * the beginning of the input module.  Returns 0 for success, non-zero
     * for failure.
     *
     *************************************************************************/
		int (*set_file_posn) (struct Dynamic_Loader_Stream * thisptr, 	
					unsigned int posn);	/* to be eliminated in release 2*/

	};

/*****************************************************************************
 *****************************************************************************
 * A class used by the dynamic loader for symbol table support and
 * miscellaneous host-side functions
 *****************************************************************************
 *****************************************************************************/
#ifndef __KERNEL__
	typedef uint32_t LDR_ADDR;
#else
	typedef u32 LDR_ADDR;
#endif

/*
 * the structure of a symbol known to the dynamic loader
 */
	struct dynload_symbol {
		LDR_ADDR value;
	} ;

	struct Dynamic_Loader_Sym {
/* public: */
    /*************************************************************************
     * Find_Matching_Symbol
     *     
     * PARAMETERS :
     *  name    The name of the desired symbol
     *
     * EFFECT :
     *  Locates a symbol matching the name specified.  A pointer to the
     * symbol is returned if it exists; 0 is returned if no such symbol is
     * found.
     *
     *************************************************************************/
		struct dynload_symbol *(*Find_Matching_Symbol) 
			(struct Dynamic_Loader_Sym *
							 thisptr,
							 const char *name);

    /*************************************************************************
     * Add_To_Symbol_Table
     *     
     * PARAMETERS :
     *  nname       Pointer to the name of the new symbol
     *  moduleid    An opaque module id assigned by the dynamic loader
     *
     * EFFECT :
     *  The new symbol is added to the table.  A pointer to the symbol is
     * returned, or NULL is returned for failure.
     *
     * NOTES:
     *  It is permissible for this function to return NULL; the effect is that
     * the named symbol will not be available to resolve references in
     * subsequent loads.  Returning NULL will not cause the current load
     * to fail.
     *************************************************************************/
		struct dynload_symbol *(*Add_To_Symbol_Table) 
						(struct Dynamic_Loader_Sym *
							thisptr,
							const char *nname,
							unsigned moduleid);

    /*************************************************************************
     * Purge_Symbol_Table
     *     
     * PARAMETERS :
     *  moduleid    An opaque module id assigned by the dynamic loader
     *
     * EFFECT :
     *  Each symbol in the symbol table whose moduleid matches the argument
     * is removed from the table.
     *************************************************************************/
		void (*Purge_Symbol_Table) (struct Dynamic_Loader_Sym * thisptr,
					    unsigned moduleid);

    /*************************************************************************
     * Allocate
     *     
     * PARAMETERS :
     *  memsiz  size of desired memory in sizeof() units
     *
     * EFFECT :
     *  Returns a pointer to some "host" memory for use by the dynamic
     * loader, or NULL for failure.
     * This function is serves as a replaceable form of "malloc" to
     * allow the user to configure the memory usage of the dynamic loader.
     *************************************************************************/
		void *(*Allocate) (struct Dynamic_Loader_Sym * thisptr,
				   unsigned memsiz);

    /*************************************************************************
     * Deallocate
     *     
     * PARAMETERS :
     *  memptr  pointer to previously allocated memory
     *
     * EFFECT :
     *  Releases the previously allocated "host" memory.
     *************************************************************************/
		void (*Deallocate) (struct Dynamic_Loader_Sym * thisptr, void *memptr);

    /*************************************************************************
     * Error_Report
     *     
     * PARAMETERS :
     *  errstr  pointer to an error string
     *  args    additional arguments
     *
     * EFFECT :
     *  This function provides an error reporting interface for the dynamic
     * loader.  The error string and arguments are designed as for the
     * library function vprintf.
     *************************************************************************/
		void (*Error_Report) (struct Dynamic_Loader_Sym * thisptr,
				      const char *errstr, va_list args);

	};			/* class Dynamic_Loader_Sym */

/*****************************************************************************
 *****************************************************************************
 * A class used by the dynamic loader to allocate and deallocate target memory.
 *****************************************************************************
 *****************************************************************************/

	struct LDR_SECTION_INFO {
		/* Name of the memory section assigned at build time */
		const char *name;	
		LDR_ADDR run_addr;	/* execution address of the section */
		LDR_ADDR load_addr;	/* load address of the section */
		LDR_ADDR size;	/* size of the section in addressable units */
/* #ifndef _BIG_ENDIAN *//* _BIG_ENDIAN Not defined by bridge driver */
#ifdef __KERNEL__
		u16 page;	/* memory page or view */
		u16 type;	/* one of the section types below */
#else
		uint16_t page;	/* memory page or view */
		uint16_t type;	/* one of the section types below */
#endif
/*#else
#ifdef __KERNEL__
		u16 type;*/	/* one of the section types below */
/*		u16 page;*/	/* memory page or view */
/*#else
		uint16_t type;*//* one of the section types below */
/*		uint16_t page;*//* memory page or view */
/*#endif
#endif*//* _BIG_ENDIAN Not defined by bridge driver */
		/* a context field for use by Dynamic_Loader_Allocate;
					   ignored but maintained by the dynamic loader */
#ifdef __KERNEL__
		u32 context;
#else
		uintptr_t context;
#endif
	} ;

/* use this macro to extract type of section from LDR_SECTION_INFO.type field */
#define DLOAD_SECTION_TYPE(typeinfo) (typeinfo & 0xF)

/* type of section to be allocated */
#define DLOAD_TEXT 0
#define DLOAD_DATA 1
#define DLOAD_BSS 2
	/* internal use only, run-time cinit will be of type DLOAD_DATA */
#define DLOAD_CINIT 3		

	struct Dynamic_Loader_Allocate {
/* public: */

    /*************************************************************************
    * Function allocate
    *
    * Parameters:
    *   info        A pointer to an information block for the section
    *   align       The alignment of the storage in target AUs
    *
    * Effect:
    *   Allocates target memory for the specified section and fills in the
    * load_addr and run_addr fields of the section info structure. Returns TRUE
    * for success, FALSE for failure.
    *
    * Notes:
    *   Frequently load_addr and run_addr are the same, but if they are not
    * load_addr is used with Dynamic_Loader_Initialize, and run_addr is
    * used for almost all relocations.  This function should always initialize
    * both fields.
    *************************************************************************/
		int (*Allocate) (struct Dynamic_Loader_Allocate * thisptr,
				 struct LDR_SECTION_INFO * info, unsigned align);

    /*************************************************************************
    * Function deallocate
    *
    * Parameters:
    *   info        A pointer to an information block for the section
    *
    * Effect:
    *   Releases the target memory previously allocated.
    *
    * Notes:
    * The content of the info->name field is undefined on call to this function.
    *************************************************************************/
		void (*Deallocate) (struct Dynamic_Loader_Allocate * thisptr,
				    struct LDR_SECTION_INFO * info);

	};			/* class Dynamic_Loader_Allocate */

/*****************************************************************************
 *****************************************************************************
 * A class used by the dynamic loader to load data into a target.  This class
 * provides the interface-specific functions needed to load data.
 *****************************************************************************
 *****************************************************************************/

	struct Dynamic_Loader_Initialize {
/* public: */
    /*************************************************************************
    * Function connect
    *
    * Parameters:
    *   none
    *
    * Effect:
    *   Connect to the initialization interface. Returns TRUE for success,
    * FALSE for failure.
    *
    * Notes:
    *   This function is called prior to use of any other functions in
    * this interface.
    *************************************************************************/
		int (*connect) (struct Dynamic_Loader_Initialize * thisptr);

    /*************************************************************************
    * Function readmem
    *
    * Parameters:
    *   bufr        Pointer to a word-aligned buffer for the result
    *   locn        Target address of first data element
    *   info        Section info for the section in which the address resides
    *   bytsiz      Size of the data to be read in sizeof() units
    *
    * Effect:
    *   Fills the specified buffer with data from the target.  Returns TRUE for
    * success, FALSE for failure.
    *************************************************************************/
		int (*readmem) (struct Dynamic_Loader_Initialize * thisptr, void *bufr,
				LDR_ADDR locn, struct LDR_SECTION_INFO * info,
				unsigned bytsiz);

    /*************************************************************************
    * Function writemem
    *
    * Parameters:
    *   bufr        Pointer to a word-aligned buffer of data
    *   locn        Target address of first data element to be written
    *   info        Section info for the section in which the address resides
    *   bytsiz      Size of the data to be written in sizeof() units
    *
    * Effect:
    *   Writes the specified buffer to the target.  Returns TRUE for success,
    * FALSE for failure.
    *************************************************************************/
		int (*writemem) (struct Dynamic_Loader_Initialize * thisptr,
				 void *bufr, LDR_ADDR locn,
				 struct LDR_SECTION_INFO * info, unsigned bytsiz);

    /*************************************************************************
    * Function fillmem
    *
    * Parameters:
    *   locn        Target address of first data element to be written
    *   info        Section info for the section in which the address resides
    *   bytsiz      Size of the data to be written in sizeof() units
    *   val         Value to be written in each byte
    * Effect:
    *   Fills the specified area of target memory.  Returns TRUE for success,
    * FALSE for failure.
    *************************************************************************/
		int (*fillmem) (struct Dynamic_Loader_Initialize * thisptr,
				LDR_ADDR locn, struct LDR_SECTION_INFO * info,
				unsigned bytsiz, unsigned val);

    /*************************************************************************
    * Function execute
    *
    * Parameters:
    *   start       Starting address
    *
    * Effect:
    *   The target code at the specified starting address is executed.
    *
    * Notes:
    *   This function is called at the end of the dynamic load process
    * if the input module has specified a starting address.
    *************************************************************************/
		int (*execute) (struct Dynamic_Loader_Initialize * thisptr,
				LDR_ADDR start);

    /*************************************************************************
    * Function release
    *
    * Parameters:
    *   none
    *
    * Effect:
    *   Releases the connection to the load interface.
    *
    * Notes:
    *   This function is called at the end of the dynamic load process.
    *************************************************************************/
		void (*release) (struct Dynamic_Loader_Initialize * thisptr);

	};			/* class Dynamic_Loader_Initialize */
#ifdef __cplusplus
}
#endif
#endif				/* _DYNAMIC_LOADER_H_ */

