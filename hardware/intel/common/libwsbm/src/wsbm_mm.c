/**************************************************************************
 *
 * Copyright 2006-2008 Tungsten Graphics, Inc., Cedar Park, TX., USA.
 * All Rights Reserved.
 * Copyright 2009 VMware, Inc., Palo Alto, CA., USA.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

/*
 * Generic simple memory manager implementation. Intended to be used as a base
 * class implementation for more advanced memory managers.
 *
 * Note that the algorithm used is quite simple and there might be substantial
 * performance gains if a smarter free list is implemented. Currently it is just an
 * unordered stack of free regions. This could easily be improved if an RB-tree
 * is used instead. At least if we expect heavy fragmentation.
 * Note that this implementation is more or less identical to the drm core manager
 * in the linux kernel.
 *
 * Authors:
 * Thomas Hellström <thomas-at-tungstengraphics-dot-com>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "wsbm_mm.h"
#include <errno.h>
#include <stdlib.h>

unsigned long
wsbmMMTailSpace(struct _WsbmMM *mm)
{
    struct _WsbmListHead *tail_node;
    struct _WsbmMMNode *entry;

    tail_node = mm->ml_entry.prev;
    entry = WSBMLISTENTRY(tail_node, struct _WsbmMMNode, ml_entry);

    if (!entry->free)
	return 0;

    return entry->size;
}

int
wsbmMMRemoveSpaceFromTail(struct _WsbmMM *mm, unsigned long size)
{
    struct _WsbmListHead *tail_node;
    struct _WsbmMMNode *entry;

    tail_node = mm->ml_entry.prev;
    entry = WSBMLISTENTRY(tail_node, struct _WsbmMMNode, ml_entry);

    if (!entry->free)
	return -ENOMEM;

    if (entry->size <= size)
	return -ENOMEM;

    entry->size -= size;
    return 0;
}

static int
wsbmMMCreateTailNode(struct _WsbmMM *mm,
		     unsigned long start, unsigned long size)
{
    struct _WsbmMMNode *child;

    child = (struct _WsbmMMNode *)malloc(sizeof(*child));
    if (!child)
	return -ENOMEM;

    child->free = 1;
    child->size = size;
    child->start = start;
    child->mm = mm;

    WSBMLISTADDTAIL(&child->ml_entry, &mm->ml_entry);
    WSBMLISTADDTAIL(&child->fl_entry, &mm->fl_entry);

    return 0;
}

static struct _WsbmMMNode *
wsbmMMSplitAtStart(struct _WsbmMMNode *parent, unsigned long size)
{
    struct _WsbmMMNode *child;

    child = (struct _WsbmMMNode *)malloc(sizeof(*child));
    if (!child)
	return NULL;

    WSBMINITLISTHEAD(&child->fl_entry);

    child->free = 0;
    child->size = size;
    child->start = parent->start;
    child->mm = parent->mm;

    WSBMLISTADDTAIL(&child->ml_entry, &parent->ml_entry);
    WSBMINITLISTHEAD(&child->fl_entry);

    parent->size -= size;
    parent->start += size;
    return child;
}

struct _WsbmMMNode *
wsbmMMGetBlock(struct _WsbmMMNode *parent,
	       unsigned long size, unsigned alignment)
{

    struct _WsbmMMNode *align_splitoff = NULL;
    struct _WsbmMMNode *child;
    unsigned tmp = 0;

    if (alignment)
	tmp = parent->start % alignment;

    if (tmp) {
	align_splitoff = wsbmMMSplitAtStart(parent, alignment - tmp);
	if (!align_splitoff)
	    return NULL;
    }

    if (parent->size == size) {
	WSBMLISTDELINIT(&parent->fl_entry);
	parent->free = 0;
	return parent;
    } else {
	child = wsbmMMSplitAtStart(parent, size);
    }

    if (align_splitoff)
	wsbmMMPutBlock(align_splitoff);

    return child;
}

/*
 * Put a block. Merge with the previous and / or next block if they are free.
 * Otherwise add to the free stack.
 */

void
wsbmMMPutBlock(struct _WsbmMMNode *cur)
{

    struct _WsbmMM *mm = cur->mm;
    struct _WsbmListHead *cur_head = &cur->ml_entry;
    struct _WsbmListHead *root_head = &mm->ml_entry;
    struct _WsbmMMNode *prev_node = NULL;
    struct _WsbmMMNode *next_node;

    int merged = 0;

    if (cur_head->prev != root_head) {
	prev_node =
	    WSBMLISTENTRY(cur_head->prev, struct _WsbmMMNode, ml_entry);
	if (prev_node->free) {
	    prev_node->size += cur->size;
	    merged = 1;
	}
    }
    if (cur_head->next != root_head) {
	next_node =
	    WSBMLISTENTRY(cur_head->next, struct _WsbmMMNode, ml_entry);
	if (next_node->free) {
	    if (merged) {
		prev_node->size += next_node->size;
		WSBMLISTDEL(&next_node->ml_entry);
		WSBMLISTDEL(&next_node->fl_entry);
		free(next_node);
	    } else {
		next_node->size += cur->size;
		next_node->start = cur->start;
		merged = 1;
	    }
	}
    }
    if (!merged) {
	cur->free = 1;
	WSBMLISTADD(&cur->fl_entry, &mm->fl_entry);
    } else {
	WSBMLISTDEL(&cur->ml_entry);
	free(cur);
    }
}

struct _WsbmMMNode *
wsbmMMSearchFree(const struct _WsbmMM *mm,
		 unsigned long size, unsigned alignment, int best_match)
{
    struct _WsbmListHead *list;
    const struct _WsbmListHead *free_stack = &mm->fl_entry;
    struct _WsbmMMNode *entry;
    struct _WsbmMMNode *best;
    unsigned long best_size;
    unsigned wasted;

    best = NULL;
    best_size = ~0UL;

    WSBMLISTFOREACH(list, free_stack) {
	entry = WSBMLISTENTRY(list, struct _WsbmMMNode, fl_entry);

	wasted = 0;

	if (entry->size < size)
	    continue;

	if (alignment) {
	    register unsigned tmp = entry->start % alignment;

	    if (tmp)
		wasted += alignment - tmp;
	}

	if (entry->size >= size + wasted) {
	    if (!best_match)
		return entry;
	    if (size < best_size) {
		best = entry;
		best_size = entry->size;
	    }
	}
    }

    return best;
}

int
wsbmMMclean(struct _WsbmMM *mm)
{
    struct _WsbmListHead *head = &mm->ml_entry;

    return (head->next->next == head);
}

int
wsbmMMinit(struct _WsbmMM *mm, unsigned long start, unsigned long size)
{
    WSBMINITLISTHEAD(&mm->ml_entry);
    WSBMINITLISTHEAD(&mm->fl_entry);

    return wsbmMMCreateTailNode(mm, start, size);
}

void
wsbmMMtakedown(struct _WsbmMM *mm)
{
    struct _WsbmListHead *bnode = mm->fl_entry.next;
    struct _WsbmMMNode *entry;

    entry = WSBMLISTENTRY(bnode, struct _WsbmMMNode, fl_entry);

    if (entry->ml_entry.next != &mm->ml_entry ||
	entry->fl_entry.next != &mm->fl_entry) {
	return;
    }

    WSBMLISTDEL(&entry->fl_entry);
    WSBMLISTDEL(&entry->ml_entry);
    free(entry);
}
