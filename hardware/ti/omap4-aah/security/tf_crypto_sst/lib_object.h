/**
 * Copyright(c) 2011 Trusted Logic.   All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name Trusted Logic nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef   __LIB_OBJECT_H__
#define   __LIB_OBJECT_H__

#include "s_type.h"

typedef struct
{
   /* Type of storage: See S_STORAGE_TYPE_XXX */
   uint32_t nStorageType;

   /* Login type of the client: See S_LOGIN_XXX */
   uint32_t nLoginType;

   /* Identifier of the client (secure or non-secure client) */
   S_UUID sClientUUID;
}
S_STORAGE_NAME;


/**
 * This library defines three types of objects and keys:
 * - objects identified by a 16-bit handle
 * - objects identified by a S_STORAGE_NAME
 * - objects identified by a filename, which is a variable-size up-to-64 bytes byte array
 * - unindexed objects
 **/

/* -------------------------------------------------------------------------
   Useful macro to get a structure from a pointer to one of its fields.

   Typical usage:
   typedef struct
   {
      LIB_OBJECT_NODE_HANDLE16  sNodeInHandleTable;
      LIB_OBJECT_NODE_UNINDEXED sNodeInList;
   }
   CONTEXT;

   LIB_OBJECT_CONTAINER_OF(libObjectUnindexedNext(pList, pObject), CONTEXT, sNodeInList)


   -------------------------------------------------------------------------*/
#define LIB_OBJECT_CONTAINER_OF(ptr, type, member) (((type*)(((char*)(ptr)) - offsetof(type, member))))


/* -------------------------------------------------------------------------
   Table of objects indexed by 16-bit handles
   -------------------------------------------------------------------------*/

#define LIB_OBJECT_HANDLE16_MAX ((uint16_t)0xFFFF)

/**
 * NODE of an object in a table indexed by 16-bit handles
 **/
typedef struct
{
   /* Implementation-defined fields */
   uint32_t _l[2];

   /* Public field */
   uint16_t   nHandle;
}
LIB_OBJECT_NODE_HANDLE16;

/**
 * A table of objects indexed by 16-bit handles
 **/
typedef struct
{
   LIB_OBJECT_NODE_HANDLE16* pRoot;
}
LIB_OBJECT_TABLE_HANDLE16;

/**
 * Add an object in a handle table. This function also
 * assigns a new handle value to the object. The handle
 * is guaranteed to be unique among all the objects
 * in the table and non-zero.
 *
 * Returns false if the maximum number of handles has been reached
 *    (i.e., there are already 65535 objects in the table)
 **/
bool libObjectHandle16Add(
               LIB_OBJECT_TABLE_HANDLE16* pTable,
               LIB_OBJECT_NODE_HANDLE16* pObject);

/**
 * Search an object by its handle. Return NULL if the
 * object is not found
 **/
LIB_OBJECT_NODE_HANDLE16* libObjectHandle16Search(
               LIB_OBJECT_TABLE_HANDLE16* pTable,
               uint32_t nHandle);

/**
 * Remove an object from a handle table.
 *
 * The object must be part of the table
 **/
void libObjectHandle16Remove(
               LIB_OBJECT_TABLE_HANDLE16* pTable,
               LIB_OBJECT_NODE_HANDLE16* pObject);

/**
 * Remove one object from the table. This is useful when
 * you want to destroy all the objects in the table.
 *
 * Returns NULL if the table is empty
 **/
LIB_OBJECT_NODE_HANDLE16* libObjectHandle16RemoveOne(
               LIB_OBJECT_TABLE_HANDLE16* pTable);

/**
 * Get the object following pObject in the handle table.
 * If pObject is NULL, return the first object in the list
 * Return NULL if the object is the last in the table
 **/
LIB_OBJECT_NODE_HANDLE16* libObjectHandle16Next(
               LIB_OBJECT_TABLE_HANDLE16* pTable,
               LIB_OBJECT_NODE_HANDLE16* pObject);

/* -------------------------------------------------------------------------
   Table of objects indexed by storage name
   -------------------------------------------------------------------------*/

/**
 * NODE of an object in a table indexed by storage name
 **/
typedef struct
{
   /* Implementation-defined fields */
   uint32_t _l[2];

   /* Public fields */
   S_STORAGE_NAME sStorageName;
}
LIB_OBJECT_NODE_STORAGE_NAME;

/**
 * A table of objects indexed by storage name
 **/
typedef struct
{
   LIB_OBJECT_NODE_STORAGE_NAME* pRoot;
}
LIB_OBJECT_TABLE_STORAGE_NAME;

/**
 * Add an object in a storage name table.
 *
 * The object must not be part of the table yet. The caller
 * must have set pObject->sStorageName with the storage name
 **/
void libObjectStorageNameAdd(
               LIB_OBJECT_TABLE_STORAGE_NAME* pTable,
               LIB_OBJECT_NODE_STORAGE_NAME* pObject);

/**
 * Search an object by its storage name. Return NULL if the
 * object is not found
 **/
LIB_OBJECT_NODE_STORAGE_NAME* libObjectStorageNameSearch(
               LIB_OBJECT_TABLE_STORAGE_NAME* pTable,
               S_STORAGE_NAME* pStorageName);

/**
 * Remove an object from a storage name table.
 *
 * The object must be part of the table
 **/
void libObjectStorageNameRemove(
               LIB_OBJECT_TABLE_STORAGE_NAME* pTable,
               LIB_OBJECT_NODE_STORAGE_NAME* pObject);

/**
 * Remove one object from the table. This is useful when
 * you want to destroy all the objects in the table
 * Returns NULL if the table is empty
 **/
LIB_OBJECT_NODE_STORAGE_NAME* libObjectStorageNameRemoveOne(
               LIB_OBJECT_TABLE_STORAGE_NAME* pTable);

/**
 * Get the object following pObject in the storage name table.
 * If pObject is NULL, return the first object
 * Return NULL if the object is the last in the table
 **/
LIB_OBJECT_NODE_STORAGE_NAME* libObjectStorageNameNext(
               LIB_OBJECT_TABLE_STORAGE_NAME* pTable,
               LIB_OBJECT_NODE_STORAGE_NAME* pObject);

/* -------------------------------------------------------------------------
   Table of objects indexed by filenames
   -------------------------------------------------------------------------*/

/**
 * NODE of an object in a table indexed by filenames (varsize up-to-64 bytes)
 **/
typedef struct
{
   /* Implementation-defined fields */
   uint32_t _l[2];

   /* Public fields */
   uint8_t  sFilename[64];
   uint8_t  nFilenameLength;
}
LIB_OBJECT_NODE_FILENAME;

/**
 * A table of objects indexed by filenames
 **/
typedef struct
{
   LIB_OBJECT_NODE_FILENAME* pRoot;
}
LIB_OBJECT_TABLE_FILENAME;

/**
 * Add an object in a filename table.
 *
 * The object must not be part of the table yet. The caller
 * must have set pObject->sFilename and pObject->nFilenameLength
 * with the object filename
 **/
void libObjectFilenameAdd(
               LIB_OBJECT_TABLE_FILENAME* pTable,
               LIB_OBJECT_NODE_FILENAME* pObject);

/**
 * Search an object by its filename. Return NULL if the
 * object is not found
 **/
LIB_OBJECT_NODE_FILENAME* libObjectFilenameSearch(
               LIB_OBJECT_TABLE_FILENAME* pTable,
               uint8_t* pFilename,
               uint32_t  nFilenameLength);

/**
 * Remove an object from a filename table.
 *
 * The object must be part of the table
 **/
void libObjectFilenameRemove(
               LIB_OBJECT_TABLE_FILENAME* pTable,
               LIB_OBJECT_NODE_FILENAME* pObject);

/**
 * Remove one element from the table and return it. This is useful when
 * you want to destroy all the objects in the table
 * Returns NULL if the table is empty
 **/
LIB_OBJECT_NODE_FILENAME* libObjectFilenameRemoveOne(
               LIB_OBJECT_TABLE_FILENAME* pTable);

/**
 * Get the object following pObject in the filename table.
 * If pObject is NULL, return the first object
 * Return NULL if the object is the last in the table
 **/
LIB_OBJECT_NODE_FILENAME* libObjectFilenameNext(
               LIB_OBJECT_TABLE_FILENAME* pTable,
               LIB_OBJECT_NODE_FILENAME* pObject);

/* -------------------------------------------------------------------------
   Unindexed table of objects
   -------------------------------------------------------------------------*/
/**
 * NODE of an unindexed object
 **/
typedef struct
{
   /* Implementation-defined fields */
   uint32_t _l[2];
}
LIB_OBJECT_NODE_UNINDEXED;

/**
 * A table of unindexed objects
 **/
typedef struct
{
   LIB_OBJECT_NODE_UNINDEXED* pRoot;
}
LIB_OBJECT_TABLE_UNINDEXED;


/**
 * Add an object in an unindexed table. The object must not be part of the table yet.
 **/
void libObjectUnindexedAdd(
         LIB_OBJECT_TABLE_UNINDEXED* pTable,
         LIB_OBJECT_NODE_UNINDEXED* pObject);

/**
 * Remove an object from an unindexed table. The object must be part of the table.
 **/
void libObjectUnindexedRemove(
         LIB_OBJECT_TABLE_UNINDEXED* pTable,
         LIB_OBJECT_NODE_UNINDEXED* pObject);

/**
 * Remove one object in the table. This is useful when you want to destroy all objects
 * in the table.
 * Returns NULL if the table is empty
 **/
LIB_OBJECT_NODE_UNINDEXED* libObjectUnindexedRemoveOne(LIB_OBJECT_TABLE_UNINDEXED* pTable);

/**
 * Get the object following pObject in the table.
 * If pObject is NULL, return the first object
 * Return NULL if the object is the last in the table
 **/
LIB_OBJECT_NODE_UNINDEXED* libObjectUnindexedNext(
               LIB_OBJECT_TABLE_UNINDEXED* pTable,
               LIB_OBJECT_NODE_UNINDEXED* pObject);

#endif /* __LIB_OBJECT_H__ */
