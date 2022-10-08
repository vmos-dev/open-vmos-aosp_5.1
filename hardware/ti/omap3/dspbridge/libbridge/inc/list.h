/*
 * dspbridge/mpu_api/inc/list.h
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


/*
 *  ======== list.h ========
 *  Purpose:
 *      Declarations of list management control structures and definitions
 *      of inline list management functions.
 *
 *  Public Functions:
 *      LST_Create
 *      LST_Delete
 *      LST_Exit
 *      LST_First
 *      LST_GetHead
 *      LST_InitElem
 *      LST_Init
 *      LST_InsertBefore
 *      LST_IsEmpty
 *      LST_Next
 *      LST_PutTail
 *      LST_RemoveElem
 *
 *  Notes:
 *
 *! Revision History
 *! ================
 *! 10-Aug-2000 ag:  Added LST_InsertBefore().
 *! 29-Oct-1999 kc:  Cleaned up for code review.
 *! 16-Aug-1997 cr:  added explicit identifiers.
 *! 10-Aug-1996 gp:  Acquired from SMM for WinSPOX v.1.1; renamed identifiers.
 *! 21-Oct-1994 dh4: Cleaned / commented for code review.
 *! 08-Jun-1994 dh4: Converted to SPM (added extern "C").
 */

#ifndef LIST_
#define LIST_

#ifdef __cplusplus
extern "C" {
#endif

#include <dspapi.h>

#define LST_IsEmpty(l)      (((l)->head.next == &(l)->head))

	struct LST_ELEM {
		struct LST_ELEM *next;
		struct LST_ELEM *prev;
		struct LST_ELEM *self;
	} ;

	/*typedef LST_ELEM *LST_PELEM;*/

	struct LST_LIST {
		struct LST_ELEM head;
	} ;

	/*typedef LST_LIST *LST_PLIST;*/

/*
 *  ======== LST_Create ========
 *  Purpose:
 *      Allocates and initializes a circular list.
 *  Details:
 *      Uses portable MEM_Calloc() function to allocate a list containing
 *      a single element and initializes that element to indicate that it
 *      is the "end of the list" (i.e., the list is empty).
 *      An empty list is indicated by the "next" pointer in the element
 *      at the head of the list pointing to the head of the list, itself.
 *  Parameters:
 *  Returns:
 *      Pointer to beginning of created list (success)
 *      NULL --> Allocation failed
 *  Requires:
 *      LST initialized.
 *  Ensures:
 *  Notes:
 *      The created list contains a single element.  This element is the
 *      "empty" element, because its "next" and "prev" pointers point at
 *      the same location (the element itself).
 */
	extern struct LST_LIST* LST_Create();

/*
 *  ======== LST_Delete ========
 *  Purpose:
 *      Removes a list by freeing its control structure's memory space.
 *  Details:
 *      Uses portable MEM_Free() function to deallocate the memory
 *      block pointed at by the input parameter.
 *  Parameters:
 *      pList:  Pointer to list control structure of list to be deleted
 *  Returns:
 *      Void
 *  Requires:
 *      - LST initialized.
 *      - pList != NULL.
 *  Ensures:
 *  Notes:
 *      Must ONLY be used for empty lists, because it does not walk the
 *      chain of list elements.  Calling this function on a non-empty list
 *      will cause a memory leak.
 */
	extern VOID LST_Delete(IN struct LST_LIST* pList);

/*
 *  ======== LST_Exit ========
 *  Purpose:
 *      Discontinue usage of module; free resources when reference count 
 *      reaches 0.
 *  Parameters:
 *  Returns:
 *  Requires:
 *      LST initialized.
 *  Ensures:
 *      Resources used by module are freed when cRef reaches zero.
 */
	extern VOID LST_Exit();

/*
 *  ======== LST_First ========
 *  Purpose:
 *      Returns a pointer to the first element of the list, or NULL if the list
 *      is empty.
 *  Parameters:
 *      pList:  Pointer to list control structure.
 *  Returns:
 *      Pointer to first list element, or NULL.
 *  Requires:
 *      - LST initialized.
 *      - pList != NULL.
 *  Ensures:
 */
	extern struct LST_ELEM* LST_First(IN struct LST_LIST* pList);

/*
 *  ======== LST_GetHead ========
 *  Purpose:
 *      Pops the head off the list and returns a pointer to it.
 *  Details:
 *      If the list is empty, returns NULL.
 *      Else, removes the element at the head of the list, making the next
 *      element the head of the list.
 *      The head is removed by making the tail element of the list point its
 *      "next" pointer at the next element after the head, and by making the
 *      "prev" pointer of the next element after the head point at the tail
 *      element.  So the next element after the head becomes the new head of
 *      the list.
 *  Parameters:
 *      pList:  Pointer to list control structure of list whose head
 *              element is to be removed
 *  Returns:
 *      Pointer to element that was at the head of the list (success)
 *      NULL          No elements in list
 *  Requires:
 *      - head.self must be correctly set to &head.
 *      - LST initialized.
 *      - pList != NULL.
 *  Ensures:
 *  Notes:
 *      Because the tail of the list points forward (its "next" pointer) to
 *      the head of the list, and the head of the list points backward (its
 *      "prev" pointer) to the tail of the list, this list is circular.
 */
	extern struct LST_ELEM* LST_GetHead(IN struct LST_LIST* pList);

/*
 *  ======== LST_Init ========
 *  Purpose:
 *      Initializes private state of LST module.
 *  Parameters:
 *  Returns:
 *      TRUE if initialized; FALSE otherwise.
 *  Requires:
 *  Ensures:
 *      LST initialized.
 */
	extern bool LST_Init();

/*
 *  ======== LST_InitElem ========
 *  Purpose:
 *      Initializes a list element to default (cleared) values
 *  Details:
 *  Parameters:
 *      pElem:  Pointer to list element to be reset
 *  Returns:
 *  Requires:
 *      LST initialized.
 *  Ensures:
 *  Notes:
 *      This function must not be called to "reset" an element in the middle
 *      of a list chain -- that would break the chain.
 *
 */
	extern VOID LST_InitElem(IN struct LST_ELEM* pListElem);

/*
 *  ======== LST_InsertBefore ========
 *  Purpose:
 *     Insert the element before the existing element.
 *  Parameters:
 *      pList:          Pointer to list control structure.
 *      pElem:          Pointer to element in list to insert.
 *      pElemExisting:  Pointer to existing list element.
 *  Returns:
 *  Requires:
 *      - LST initialized.
 *      - pList != NULL.
 *      - pElem != NULL.
 *      - pElemExisting != NULL.
 *  Ensures:
 */
	extern VOID LST_InsertBefore(IN struct LST_LIST* pList,
				     IN struct LST_ELEM* pElem,
				     IN struct LST_ELEM* pElemExisting);

/*
 *  ======== LST_Next ========
 *  Purpose:
 *      Returns a pointer to the next element of the list, or NULL if the next
 *      element is the head of the list or the list is empty.
 *  Parameters:
 *      pList:      Pointer to list control structure.
 *      pCurElem:   Pointer to element in list to remove.
 *  Returns:
 *      Pointer to list element, or NULL.
 *  Requires:
 *      - LST initialized.
 *      - pList != NULL.
 *      - pCurElem != NULL.
 *  Ensures:
 */
	extern struct LST_ELEM* LST_Next(IN struct LST_LIST* pList, 
											IN struct LST_ELEM* pCurElem);

/*
 *  ======== LST_PutTail ========
 *  Purpose:
 *      Adds the specified element to the tail of the list
 *  Details:
 *      Sets new element's "prev" pointer to the address previously held by
 *      the head element's prev pointer.  This is the previous tail member of
 *      the list.
 *      Sets the new head's prev pointer to the address of the element.
 *      Sets next pointer of the previous tail member of the list to point to
 *      the new element (rather than the head, which it had been pointing at).
 *      Sets new element's next pointer to the address of the head element.
 *      Sets head's prev pointer to the address of the new element.
 *  Parameters:
 *      pList:  Pointer to list control structure to which *pElem will be
 *              added
 *      pElem:  Pointer to list element to be added
 *  Returns:
 *      Void
 *  Requires:
 *      *pElem and *pList must both exist.
 *      pElem->self = pElem before pElem is passed to this function.
 *      LST initialized.
 *  Ensures:
 *  Notes:
 *      Because the tail is always "just before" the head of the list (the
 *      tail's "next" pointer points at the head of the list, and the head's
 *      "prev" pointer points at the tail of the list), the list is circular.
 *  Warning: if pElem->self is not set beforehand, LST_GetHead() will
 *      return an erroneous pointer when it is called for this element.
 */
	extern VOID LST_PutTail(IN struct LST_LIST* pList, 
										IN struct LST_ELEM* pListElem);

/*
 *  ======== LST_RemoveElem ========
 *  Purpose:
 *      Removes (unlinks) the given element from the list, if the list is not
 *      empty.  Does not free the list element.
 *  Parameters:
 *      pList:      Pointer to list control structure.
 *      pCurElem:   Pointer to element in list to remove.
 *  Returns:
 *  Requires:
 *      - LST initialized.
 *      - pList != NULL.
 *      - pCurElem != NULL.
 *  Ensures:
 */
extern VOID LST_RemoveElem(IN struct LST_LIST* pList,
											IN struct LST_ELEM* pCurElem);

#ifdef __cplusplus
}
#endif
#endif				/* LIST_ */
