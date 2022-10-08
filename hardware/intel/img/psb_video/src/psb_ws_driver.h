/*
 * Copyright (c) 2011 Intel Corporation. All Rights Reserved.
 *
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Binglin Chen <binglin.chen@intel.com>
 *
 */

#ifndef _PSB_WS_DRIVER_H_
#define _PSB_WS_DRIVER_H_

#include <linux/types.h>
#include <wsbm/wsbm_util.h>
#include <wsbm/wsbm_driver.h>
#ifdef ANDROID
#include <drm/ttm/ttm_placement.h>
#include <linux/psb_drm.h>
#else
#include <psb_drm.h>
#endif

struct _PsbDrmValidateNode {
    struct _ValidateNode base;
    struct psb_validate_arg val_arg;
};

extern struct _WsbmVNodeFuncs *psbVNodeFuncs(void);

static inline struct psb_validate_req *
psbValReq(struct _ValidateNode *node) {
    return &(containerOf(node, struct _PsbDrmValidateNode, base)->
             val_arg.d.req);
}


#endif  /* _PSB_WS_DRIVER_H_ */
