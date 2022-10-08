/*
 * Copyright (c) 2011 Intel Corporation. All Rights Reserved.
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

#include "psb_def.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <linux/types.h>
#include <stdlib.h>
#include <string.h>
#include "psb_ws_driver.h"

static struct _ValidateNode *
psb_alloc(struct _WsbmVNodeFuncs *func, int type_id) {
    if (type_id == 0) {
        struct _PsbDrmValidateNode *vNode = malloc(sizeof(*vNode));

        if (vNode == NULL) return NULL;

        vNode->base.func = func;
        vNode->base.type_id = 0;
        return &vNode->base;
    } else {
        struct _ValidateNode *node = malloc(sizeof(*node));

        if (node == NULL) return NULL;

        node->func = func;
        node->type_id = 1;
        return node;
    }
}

/*
 * Free an allocated validate list node.
 */

static void
psb_free(struct _ValidateNode *node)
{
    if (node->type_id == 0)
        free(containerOf(node, struct _PsbDrmValidateNode, base));

    else
        free(node);
}

/*
 * Clear the private part of the validate list node. This happens when
 * the list node is newly allocated or is being reused. Since we only have
 * a private part when node->type_id == 0 we only care to clear in that
 * case. We want to clear the drm ioctl argument.
 */

static void
psb_clear(struct _ValidateNode *node)
{
    if (node->type_id == 0) {
        struct _PsbDrmValidateNode *vNode =
            containerOf(node, struct _PsbDrmValidateNode, base);

        memset(&vNode->val_arg.d.req, 0, sizeof(vNode->val_arg.d.req));
    }
}

static struct _WsbmVNodeFuncs psbVNode = {
    .alloc = psb_alloc,
    .free = psb_free,
    .clear = psb_clear,
};

struct _WsbmVNodeFuncs *
psbVNodeFuncs(void) {
    return &psbVNode;
}
