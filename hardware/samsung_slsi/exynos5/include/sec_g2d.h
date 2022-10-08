/*
 * Copyright@ Samsung Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _SEC_G2D_DRIVER_H_
#define _SEC_G2D_DRIVER_H_
typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned long  u32;

#define SEC_G2D_DEV_NAME        "/dev/fimg2d"

#define G2D_IOCTL_MAGIC 'G'

#define G2D_BLIT                    _IO(G2D_IOCTL_MAGIC,0)
#define G2D_GET_VERSION             _IO(G2D_IOCTL_MAGIC,1)
#define G2D_GET_MEMORY              _IOR(G2D_IOCTL_MAGIC,2, unsigned int)
#define G2D_GET_MEMORY_SIZE         _IOR(G2D_IOCTL_MAGIC,3, unsigned int)
#define G2D_DMA_CACHE_CLEAN         _IOWR(G2D_IOCTL_MAGIC,4, struct g2d_dma_info)
#define G2D_DMA_CACHE_FLUSH         _IOWR(G2D_IOCTL_MAGIC,5, struct g2d_dma_info)
#define G2D_SYNC                    _IO(G2D_IOCTL_MAGIC,6)
#define G2D_RESET                   _IO(G2D_IOCTL_MAGIC,7)

#define G2D_MAX_WIDTH   (2048)
#define G2D_MAX_HEIGHT  (2048)

#define G2D_ALPHA_VALUE_MAX (255)

#define G2D_POLLING (1<<0)
#define G2D_INTERRUPT (0<<0)
#define G2D_CACHE_OP (1<<1)
#define G2D_NONE_INVALIDATE (0<<1)
#define G2D_HYBRID_MODE (1<<2)

typedef enum {
    G2D_ROT_0 = 0,
    G2D_ROT_90,
    G2D_ROT_180,
    G2D_ROT_270,
    G2D_ROT_X_FLIP,
    G2D_ROT_Y_FLIP
} G2D_ROT_DEG;

typedef enum {
    G2D_ALPHA_BLENDING_MIN    = 0,   // wholly transparent
    G2D_ALPHA_BLENDING_MAX    = 255, // 255
    G2D_ALPHA_BLENDING_OPAQUE = 256, // opaque
} G2D_ALPHA_BLENDING_MODE;

typedef enum {
    G2D_COLORKEY_NONE = 0,
    G2D_COLORKEY_SRC_ON,
    G2D_COLORKEY_DST_ON,
    G2D_COLORKEY_SRC_DST_ON,
} G2D_COLORKEY_MODE;

typedef enum {
    G2D_BLUE_SCREEN_NONE = 0,
    G2D_BLUE_SCREEN_TRANSPARENT,
    G2D_BLUE_SCREEN_WITH_COLOR,
} G2D_BLUE_SCREEN_MODE;

typedef enum {
    G2D_ROP_SRC = 0,
    G2D_ROP_DST,
    G2D_ROP_SRC_AND_DST,
    G2D_ROP_SRC_OR_DST,
    G2D_ROP_3RD_OPRND,
    G2D_ROP_SRC_AND_3RD_OPRND,
    G2D_ROP_SRC_OR_3RD_OPRND,
    G2D_ROP_SRC_XOR_3RD_OPRND,
    G2D_ROP_DST_OR_3RD,
} G2D_ROP_TYPE;

typedef enum {
    G2D_THIRD_OP_NONE = 0,
    G2D_THIRD_OP_PATTERN,
    G2D_THIRD_OP_FG,
    G2D_THIRD_OP_BG
} G2D_THIRD_OP_MODE;

typedef enum {
    G2D_BLACK = 0,
    G2D_RED,
    G2D_GREEN,
    G2D_BLUE,
    G2D_WHITE,
    G2D_YELLOW,
    G2D_CYAN,
    G2D_MAGENTA
} G2D_COLOR;

typedef enum {
    G2D_RGB_565 = ((0<<4)|2),

    G2D_ABGR_8888 = ((2<<4)|1),
    G2D_BGRA_8888 = ((3<<4)|1),
    G2D_ARGB_8888 = ((0<<4)|1),
    G2D_RGBA_8888 = ((1<<4)|1),

    G2D_XBGR_8888 = ((2<<4)|0),
    G2D_BGRX_8888 = ((3<<4)|0),
    G2D_XRGB_8888 = ((0<<4)|0),
    G2D_RGBX_8888 = ((1<<4)|0),

    G2D_ABGR_1555 = ((2<<4)|4),
    G2D_BGRA_5551 = ((3<<4)|4),
    G2D_ARGB_1555 = ((0<<4)|4),
    G2D_RGBA_5551 = ((1<<4)|4),

    G2D_XBGR_1555 = ((2<<4)|3),
    G2D_BGRX_5551 = ((3<<4)|3),
    G2D_XRGB_1555 = ((0<<4)|3),
    G2D_RGBX_5551 = ((1<<4)|3),

    G2D_ABGR_4444 = ((2<<4)|6),
    G2D_BGRA_4444 = ((3<<4)|6),
    G2D_ARGB_4444 = ((0<<4)|6),
    G2D_RGBA_4444 = ((1<<4)|6),

    G2D_XBGR_4444 = ((2<<4)|5),
    G2D_BGRX_4444 = ((3<<4)|5),
    G2D_XRGB_4444 = ((0<<4)|5),
    G2D_RGBX_4444 = ((1<<4)|5),

    G2D_PACKED_BGR_888 = ((2<<4)|7),
    G2D_PACKED_RGB_888 = ((0<<4)|7),

    G2D_MAX_COLOR_SPACE
} G2D_COLOR_SPACE;

typedef enum {
    G2D_Clear_Mode,    //!< [0, 0]
    G2D_Src_Mode,      //!< [Sa, Sc]
    G2D_Dst_Mode,      //!< [Da, Dc]
    G2D_SrcOver_Mode,  //!< [Sa + Da - Sa*Da, Rc = Sc + (1 - Sa)*Dc]
    G2D_DstOver_Mode,  //!< [Sa + Da - Sa*Da, Rc = Dc + (1 - Da)*Sc]
    G2D_SrcIn_Mode,    //!< [Sa * Da, Sc * Da]
    G2D_DstIn_Mode,    //!< [Sa * Da, Sa * Dc]
    G2D_SrcOut_Mode,   //!< [Sa * (1 - Da), Sc * (1 - Da)]
    G2D_DstOut_Mode,   //!< [Da * (1 - Sa), Dc * (1 - Sa)]
    G2D_SrcATop_Mode,  //!< [Da, Sc * Da + (1 - Sa) * Dc]
    G2D_DstATop_Mode,  //!< [Sa, Sa * Dc + Sc * (1 - Da)]
    G2D_Xor_Mode,      //!< [Sa + Da - 2 * Sa * Da, Sc * (1 - Da) + (1 - Sa) * Dc]

    // these modes are defined in the SVG Compositing standard
    // http://www.w3.org/TR/2009/WD-SVGCompositing-20090430/
    G2D_Plus_Mode,
    G2D_Multiply_Mode,
    G2D_Screen_Mode,
    G2D_Overlay_Mode,
    G2D_Darken_Mode,
    G2D_Lighten_Mode,
    G2D_ColorDodge_Mode,
    G2D_ColorBurn_Mode,
    G2D_HardLight_Mode,
    G2D_SoftLight_Mode,
    G2D_Difference_Mode,
    G2D_Exclusion_Mode,

    kLastMode = G2D_Exclusion_Mode
} G2D_PORTTERDUFF_MODE;

typedef enum {
       G2D_MEMORY_KERNEL,
       G2D_MEMORY_USER
} G2D_MEMORY_TYPE;

typedef struct {
    int    x;
    int    y;
    unsigned int    w;
    unsigned int    h;
    unsigned int    full_w;
    unsigned int    full_h;
    int             color_format;
    unsigned int    bytes_per_pixel;
    unsigned char * addr;
} g2d_rect;

typedef struct {
    unsigned int    rotate_val;
    unsigned int    alpha_val;

    unsigned int    blue_screen_mode;     //true : enable, false : disable
    unsigned int    color_key_val;        //screen color value
    unsigned int    color_switch_val;     //one color

    unsigned int    src_color;            // when set one color on SRC

    unsigned int    third_op_mode;
    unsigned int    rop_mode;
    unsigned int    mask_mode;
    unsigned int    render_mode;
    unsigned int    potterduff_mode;
        unsigned int    memory_type;
} g2d_flag;

typedef struct {
    unsigned int    t;
    unsigned int    b;
    unsigned int    l;
    unsigned int    r;
} g2d_clip;

typedef struct {
    g2d_rect src_rect;
    g2d_rect dst_rect;
    g2d_clip clip;
    g2d_flag flag;
} g2d_params;

struct g2d_dma_info {
    unsigned long addr;
    unsigned int  size;
};

typedef struct _sec_g2d_t {
    int dev_fd;
    g2d_params  params;
}sec_g2d_t;

typedef struct __s5p_rect {
    uint32_t x;
    uint32_t y;
    uint32_t w;
    uint32_t h;
} __s5p_rect;

typedef struct __s5p_img {
    uint32_t width;
    uint32_t height;
    uint32_t format;
    uint32_t offset;
    uint32_t base;
    int memory_id;
} __s5p_img;

#endif /*_SEC_G2D_DRIVER_H_*/
