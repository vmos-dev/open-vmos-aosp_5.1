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
 *
 * Authors:
 * Thomas Hellstrom <thomas-at-tungstengraphics-dot-com>
 */

#ifndef _WSBM_MM_H_
#define _WSBM_MM_H_

#include "wsbm_util.h"
struct _WsbmMM
{
    struct _WsbmListHead fl_entry;
    struct _WsbmListHead ml_entry;
};

struct _WsbmMMNode
{
    struct _WsbmListHead fl_entry;
    struct _WsbmListHead ml_entry;
    int free;
    unsigned long start;
    unsigned long size;
    struct _WsbmMM *mm;
};

extern struct _WsbmMMNode *wsbmMMSearchFree(const struct _WsbmMM *mm,
					    unsigned long size,
					    unsigned alignment,
					    int best_match);
extern struct _WsbmMMNode *wsbmMMGetBlock(struct _WsbmMMNode *parent,
					  unsigned long size,
					  unsigned alignment);
extern void wsbmMMPutBlock(struct _WsbmMMNode *cur);
extern void wsbmMMtakedown(struct _WsbmMM *mm);
extern int wsbmMMinit(struct _WsbmMM *mm, unsigned long start,
		      unsigned long size);
extern int wsbmMMclean(struct _WsbmMM *mm);
#endif
