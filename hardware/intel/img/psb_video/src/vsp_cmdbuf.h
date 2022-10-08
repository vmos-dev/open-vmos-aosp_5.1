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

#ifndef _VSP_CMDBUF_H_
#define _VSP_CMDBUF_H_

#include <wsbm/wsbm_manager.h>

#include "psb_drv_video.h"
#include "psb_buffer.h"

struct vsp_cmdbuf_s {
	struct psb_buffer_s buf;
	unsigned int size;

	/* Relocation records */
	unsigned char *reloc_base;
	struct drm_psb_reloc *reloc_idx;

	/* CMD stream data */
	int cmd_count;
	unsigned char *cmd_base;
	unsigned char *cmd_start;
	unsigned int *cmd_idx;

	int param_mem_loc;

	/* Referenced buffers */
	psb_buffer_p *buffer_refs;
	int buffer_refs_count;
	int buffer_refs_allocated;

	struct psb_buffer_s param_mem;
	unsigned char *param_mem_p;
	unsigned char *pic_param_p;
	unsigned char *seq_param_p;
	unsigned char *end_param_p;
	unsigned char *pipeline_param_p;
	unsigned char *denoise_param_p;
	unsigned char *enhancer_param_p;
	unsigned char *sharpen_param_p;
	unsigned char *frc_param_p;
	unsigned char *ref_param_p;
	unsigned char *compose_param_p;
};

typedef struct vsp_cmdbuf_s *vsp_cmdbuf_p;

#define VSP_RELOC_CMDBUF(dest, offset, buf) vsp_cmdbuf_add_relocation(cmdbuf, (uint32_t*)(dest), buf, offset, 0XFFFFFFFF, 0, 0, 0, (uint32_t *)cmdbuf->cmd_start)

/**
 * VSP command:
 * context
 * type
 * buffer
 * size
 * buffer_id
 * irq
 * reserved6
 * reserved7
 */
/* operation number is inserted by DRM */
#define vsp_cmdbuf_insert_command(cmdbuf,context_id, ref_buf,type,offset,size)	\
	do { *cmdbuf->cmd_idx++ = context_id; *cmdbuf->cmd_idx++ = type;\
	     VSP_RELOC_CMDBUF(cmdbuf->cmd_idx++, offset, ref_buf);\
	     *cmdbuf->cmd_idx++ = size; *cmdbuf->cmd_idx++ = 0;\
	     *cmdbuf->cmd_idx++ = 0; *cmdbuf->cmd_idx++ = 0; \
	     *cmdbuf->cmd_idx++ = wsbmKBufHandle(wsbmKBuf((ref_buf)->drm_buf));} while(0)


#define vsp_cmdbuf_reloc_pic_param(pic_param_dest,offset,ref_buf, dst_buf_loc, pic_param_buf_start)	\
	do { vsp_cmdbuf_add_relocation(cmdbuf, (uint32_t*)(pic_param_dest), ref_buf, offset, 0XFFFFFFFF, 0, 0, dst_buf_loc,(uint32_t *)pic_param_buf_start); } while(0)
#define vsp_cmdbuf_fence_pic_param(cmdbuf, pic_param_handler) \
	do { *cmdbuf->cmd_idx++ = 0; *cmdbuf->cmd_idx++ = VspFencePictureParamCommand; *cmdbuf->cmd_idx++ = pic_param_handler; *cmdbuf->cmd_idx++ = 0; *cmdbuf->cmd_idx++ = 0; *cmdbuf->cmd_idx++ = 0; *cmdbuf->cmd_idx++ = 0; *cmdbuf->cmd_idx++ = 0;} while(0)

#define vsp_cmdbuf_vpp_context(cmdbuf, type, buffer, size) \
	do { *cmdbuf->cmd_idx++ = VSP_API_GENERIC_CONTEXT_ID; \
	     *cmdbuf->cmd_idx++ = type; \
	     *cmdbuf->cmd_idx++ = buffer; \
	     *cmdbuf->cmd_idx++ = size; \
	     *cmdbuf->cmd_idx++ = 0; \
	     *cmdbuf->cmd_idx++ = 0; \
	     *cmdbuf->cmd_idx++ = 0; *cmdbuf->cmd_idx++ = 0;} while(0)

#define vsp_cmdbuf_fence_compose_param(cmdbuf, pic_param_handler) \
	do { \
		*cmdbuf->cmd_idx++ = 0; \
		*cmdbuf->cmd_idx++ = VspFenceComposeCommand; \
		*cmdbuf->cmd_idx++ = pic_param_handler; \
		*cmdbuf->cmd_idx++ = 0; \
		*cmdbuf->cmd_idx++ = 0; \
		*cmdbuf->cmd_idx++ = 0; \
		*cmdbuf->cmd_idx++ = 0; \
		*cmdbuf->cmd_idx++ = 0; \
	} while(0)

#define vsp_cmdbuf_compose_end(cmdbuf) \
	do { *cmdbuf->cmd_idx++ = 0; \
	     *cmdbuf->cmd_idx++ = VssWiDi_ComposeEndOfSequenceCommand; \
	     *cmdbuf->cmd_idx++ = 0; \
	     *cmdbuf->cmd_idx++ = 0; \
	     *cmdbuf->cmd_idx++ = 0; \
	     *cmdbuf->cmd_idx++ = 0; \
	     *cmdbuf->cmd_idx++ = 0; \
	     *cmdbuf->cmd_idx++ = 0;} while(0)

/*
 * Create command buffer
 */
VAStatus vsp_cmdbuf_create(object_context_p obj_context,
                           psb_driver_data_p driver_data,
                           vsp_cmdbuf_p cmdbuf
	);

/*
 * Destroy buffer
 */
void vsp_cmdbuf_destroy(vsp_cmdbuf_p cmdbuf);

/*
 * Reset buffer & map
 *
 * Returns 0 on success
 */
int vsp_cmdbuf_reset(vsp_cmdbuf_p cmdbuf);

/*
 * Unmap buffer
 *
 * Returns 0 on success
 */
int vsp_cmdbuf_unmap(vsp_cmdbuf_p cmdbuf);

/*
 * Advances "obj_context" to the next cmdbuf
 *
 * Returns 0 on success
 */
int vsp_context_get_next_cmdbuf(object_context_p obj_context);

/*
 * Submits the current cmdbuf
 *
 * Returns 0 on success
 */
int vsp_context_submit_cmdbuf(object_context_p obj_context);

/*
 * Flushes the pending cmdbuf
 *
 * Return 0 on success
 */
int vsp_context_flush_cmdbuf(object_context_p obj_context);

void vsp_cmdbuf_add_relocation(vsp_cmdbuf_p cmdbuf,
                               uint32_t *addr_in_dst_buffer,
                               psb_buffer_p ref_buffer,
                               uint32_t buf_offset,
                               uint32_t mask,
                               uint32_t background,
                               uint32_t align_shift,
                               uint32_t dst_buffer,
                               uint32_t *start_of_dst_buffer);
int vsp_cmdbuf_buffer_ref(vsp_cmdbuf_p cmdbuf, psb_buffer_p buf);

#endif /* _VSP_CMDBUF_H_ */
