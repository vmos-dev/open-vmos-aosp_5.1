/* Copyright (c) 2012, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef _OMX_JPEG_COMMON_H_
#define _OMX_JPEG_COMMON_H_
#include "OMX_Core.h"
#include "stdio.h"
#include "omx_debug.h"

#define INPUT_PORT 0
#define OUTPUT_PORT 1
#define CEILING16(X) (((X) + 0x000F) & 0xFFF0)
#define YUV_SIZER(W, H) ((W * H * 3) /2 )

#define DEFAULT_PICTURE_WIDTH 640
#define DEFAULT_PICTURE_HEIGHT 480

#define GET_COMP(X) ((OMX_PTR)((OMX_COMPONENTTYPE*)X)->pComponentPrivate)
#define PAD_TO_WORD(a)               (((a)+3)&~3)
#define OMX_JPEG_QUEUE_CAPACITY 100


#define OMX_MM_MALLOC(size) jpeg_malloc(size, __FILE__, __LINE__)
#define OMX_MM_ZERO(pointer, type) memset(pointer, 0 ,sizeof(type))
#define OMX_MM_FREE(pointer) {if (pointer) \
                               jpeg_free(pointer); \
                             else \
	                       OMX_DBG_INFO("%s:%d: null pointer", __FILE__, __LINE__); \
                             pointer = NULL; \
	                     }
#define ONERROR(X, handler) if(X) { \
        OMX_DBG_ERROR("Failure at  %s:%d", __FILE__, __LINE__); \
        handler; \
        return X; \
    }
#define ERROR_ENCODING(X, handler) if(X) { \
        OMX_DBG_ERROR("Failure at  %s:%d", __FILE__, __LINE__); \
        handler; \
        pthread_mutex_unlock(&comp->abort_mutex); \
        return X; \
    }
#define ONWARNING(X) if(X) {\
        OMX_DBG_ERROR("Warning: Failure at %s:%d", __FILE__, __LINE__); \
    }

#define MIN(a,b)  (((a) < (b)) ? (a) : (b))

/* These are custom events added as an extention to the OMX Events*/
typedef enum {
  OMX_EVENT_ETB_DONE = OMX_EventVendorStartUnused+1,
  OMX_EVENT_FTB_DONE,
  OMX_EVENT_THUMBNAIL_DROPPED,
  OMX_EVENT_JPEG_ERROR,
  OMX_EVENT_JPEG_ABORT,
  OMX_EVENT_MAIN_IMAGE,
  OMX_EVENT_THUMBNAIL_IMAGE,
  OMX_EVENT_DONE
}omx_jpeg_events;

/*This enum is consistent with the jpeg_color_format enum.
Please be careful while changing the order. It has to
match exactly*/
typedef enum {
  OMX_YCRCBLP_H2V2 = 0,
  OMX_YCBCRLP_H2V2 = 1,

  OMX_YCRCBLP_H2V1 = 2,
  OMX_YCBCRLP_H2V1 = 3,

  OMX_YCRCBLP_H1V2 = 4,
  OMX_YCBCRLP_H1V2 = 5,

  OMX_YCRCBLP_H1V1 = 6,
  OMX_YCBCRLP_H1V1 = 7,

  OMX_RGB565 = 8,
  OMX_RGB888 = 9,
  OMX_RGBa   = 10,

  OMX_JPEG_BITSTREAM_H2V2 = 12,
  OMX_JPEG_BITSTREAM_H2V1 = 14,
  OMX_JPEG_BITSTREAM_H1V2 = 16,
  OMX_JPEG_BITSTREAM_H1V1 = 18,

  OMX_JPEG_COLOR_FORMAT_MAX,

} omx_jpeg_color_format;

/*This enum is consistent with the jpege_preferences_t  and
jpegd_preferences_t enum.Please be careful while changing the order.
It has to match exactly*/
typedef enum {
  OMX_JPEG_PREF_HW_ACCELERATED_PREFERRED = 0,
  OMX_JPEG_PREF_HW_ACCELERATED_ONLY,
  OMX_JPEG_PREF_SOFTWARE_PREFERRED,
  OMX_JPEG_PREF_SOFTWARE_ONLY,
  OMX_JPEG_PREF_MAX,

} omx_jpeg_preference;

typedef enum {
    OMX_JPEG_MESSAGE_INITIAL=0,
    OMX_JPEG_MESSAGE_ETB,
    OMX_JPEG_MESSAGE_FTB,
    OMX_JPEG_MESSAGE_ETB_DONE,
    OMX_JPEG_MESSAGE_FTB_DONE,
    OMX_JPEG_MESSAGE_START_ENCODE,
    OMX_JPEG_MESSAGE_START_DECODE,
    OMX_JPEG_MESSAGE_CHANGE_STATE,
    OMX_JPEG_MESSAGE_FLUSH,
    OMX_JPEG_MESSAGE_STOP,
    OMX_JPEG_MESSAGE_FLUSH_COMPLETE,
    OMX_JPEG_MESSAGE_TRANSACT_COMPLETE,
    OMX_JPEG_MESSAGE_DEINIT,
    OMX_JPEG_MESSAGE_EVENT, //for event callback args contain event info
    OMX_JPEG_MESSAGE_DECODED_IMAGE,
    OMX_JPEG_MESSAGE_DECODE_DONE,

}omx_jpeg_message;

typedef enum omx_jpeg_image_type{
  OMX_JPEG =0,
  OMX_JPS,
  OMX_MPO,

}omx_jpeg_image_type;

//message arg
typedef union omx_jpeg_message_arg{
    void * pValue;
    int iValue;
}omx_jpeg_message_arg;

//message
typedef struct omx_jpeg_queue_item{
    omx_jpeg_message message;
    omx_jpeg_message_arg args[3];

}omx_jpeg_queue_item;


typedef enum {
    OMX_JPEG_QUEUE_COMMAND=0,
    OMX_JPEG_QUEUE_ETB,
    OMX_JPEG_QUEUE_FTB,
    OMX_JPEG_QUEUE_ABORT,
}omx_jpeg_queue_type;

typedef struct omx_jpeg_queue{
    omx_jpeg_queue_item container[OMX_JPEG_QUEUE_CAPACITY];
    int front;
    int back;
    int size;
    omx_jpeg_queue_type type;
}omx_jpeg_queue;

typedef struct omx_jpeg_message_queue{
    omx_jpeg_queue command;
    omx_jpeg_queue etb;
    omx_jpeg_queue ftb;
    omx_jpeg_queue abort;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    int messageCount;
    int initialized;
}omx_jpeg_message_queue;

int omx_jpeg_queue_insert(omx_jpeg_queue* queue, omx_jpeg_queue_item * item);
int omx_jpeg_queue_remove(omx_jpeg_queue * queue, omx_jpeg_queue_item* item);
void omx_jpeg_message_queue_init(omx_jpeg_message_queue * queue);
void omx_jpeg_queue_init(omx_jpeg_queue * queue);
int omx_jpeg_queue_flush(omx_jpeg_queue * queue);
//Common OMX Functions
OMX_ERRORTYPE
omx_component_image_init(OMX_IN OMX_HANDLETYPE hComp, OMX_IN OMX_STRING componentName);


OMX_ERRORTYPE
omx_component_image_get_version(OMX_IN OMX_HANDLETYPE               hComp,
                             OMX_OUT OMX_STRING          componentName,
                             OMX_OUT OMX_VERSIONTYPE* componentVersion,
                             OMX_OUT OMX_VERSIONTYPE*      specVersion,
                             OMX_OUT OMX_UUIDTYPE*       componentUUID);

OMX_ERRORTYPE
omx_component_image_send_command(OMX_IN OMX_HANDLETYPE hComp,
                              OMX_IN OMX_COMMANDTYPE  cmd,
                              OMX_IN OMX_U32       param1,
                              OMX_IN OMX_PTR      cmdData);

OMX_ERRORTYPE
omx_component_image_get_parameter(OMX_IN OMX_HANDLETYPE     hComp,
                               OMX_IN OMX_INDEXTYPE paramIndex,
                               OMX_INOUT OMX_PTR     paramData);

OMX_ERRORTYPE
omx_component_image_set_parameter(OMX_IN OMX_HANDLETYPE     hComp,
                               OMX_IN OMX_INDEXTYPE paramIndex,
                               OMX_IN OMX_PTR        paramData);

OMX_ERRORTYPE
omx_component_image_get_config(OMX_IN OMX_HANDLETYPE      hComp,
          OMX_IN OMX_INDEXTYPE configIndex,
          OMX_INOUT OMX_PTR     configData);

OMX_ERRORTYPE
omx_component_image_set_config(OMX_IN OMX_HANDLETYPE      hComp,
                            OMX_IN OMX_INDEXTYPE configIndex,
                            OMX_IN OMX_PTR        configData);

OMX_ERRORTYPE
omx_component_image_get_extension_index(OMX_IN OMX_HANDLETYPE      hComp,
                                     OMX_IN OMX_STRING      paramName,
                                     OMX_OUT OMX_INDEXTYPE* indexType);

OMX_ERRORTYPE
omx_component_image_get_state(OMX_IN OMX_HANDLETYPE  hComp,
                           OMX_OUT OMX_STATETYPE* state);

OMX_ERRORTYPE
omx_component_image_tunnel_request(OMX_IN OMX_HANDLETYPE                hComp,
                                OMX_IN OMX_U32                        port,
                                OMX_IN OMX_HANDLETYPE        peerComponent,
                                OMX_IN OMX_U32                    peerPort,
                                OMX_INOUT OMX_TUNNELSETUPTYPE* tunnelSetup);

OMX_ERRORTYPE
omx_component_image_use_buffer(OMX_IN OMX_HANDLETYPE                hComp,
                            OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
                            OMX_IN OMX_U32                        port,
                            OMX_IN OMX_PTR                     appData,
                            OMX_IN OMX_U32                       bytes,
                            OMX_IN OMX_U8*                      buffer);


OMX_ERRORTYPE
omx_component_image_allocate_buffer(OMX_IN OMX_HANDLETYPE                hComp,
                                 OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
                                 OMX_IN OMX_U32                        port,
                                 OMX_IN OMX_PTR                     appData,
                                 OMX_IN OMX_U32                       bytes);

OMX_ERRORTYPE
omx_component_image_free_buffer(OMX_IN OMX_HANDLETYPE         hComp,
                             OMX_IN OMX_U32                 port,
                             OMX_IN OMX_BUFFERHEADERTYPE* buffer);

OMX_ERRORTYPE
omx_component_image_empty_this_buffer(OMX_IN OMX_HANDLETYPE         hComp,
                                   OMX_IN OMX_BUFFERHEADERTYPE* buffer);

OMX_ERRORTYPE
omx_component_image_fill_this_buffer(OMX_IN OMX_HANDLETYPE         hComp,
                                  OMX_IN OMX_BUFFERHEADERTYPE* buffer);

OMX_ERRORTYPE
omx_component_image_set_callbacks(OMX_IN OMX_HANDLETYPE        hComp,
                               OMX_IN OMX_CALLBACKTYPE* callbacks,
                               OMX_IN OMX_PTR             appData);

OMX_ERRORTYPE
omx_component_image_deinit(OMX_IN OMX_HANDLETYPE hComp);

OMX_ERRORTYPE
omx_component_image_use_EGL_image(OMX_IN OMX_HANDLETYPE                hComp,
                               OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
                               OMX_IN OMX_U32                        port,
                               OMX_IN OMX_PTR                     appData,
                               OMX_IN void*                      eglImage);

OMX_ERRORTYPE
omx_component_image_role_enum(OMX_IN OMX_HANDLETYPE hComp,
                           OMX_OUT OMX_U8*        role,
                           OMX_IN OMX_U32        index);

#endif
