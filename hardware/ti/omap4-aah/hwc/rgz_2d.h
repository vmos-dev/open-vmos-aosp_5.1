/*
 * Copyright (C) Texas Instruments - http://www.ti.com/
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
#ifndef __RGZ_2D__
#define __RGZ_2D__

#include <linux/bltsville.h>

/*
 * Maximum number of layers used to generate subregion rectangles in a
 * horizontal region.
 */
#define RGZ_MAXLAYERS 13

/*
 * Maximum number of layers the regionizer will accept as input. Account for an
 * additional 'background layer' to generate empty subregion rectangles.
 */
#define RGZ_INPUT_MAXLAYERS (RGZ_MAXLAYERS - 1)

/*
 * Regionizer data
 *
 * This is an oqaque structure passed in by the client
 */
struct rgz;
typedef struct rgz rgz_t;

/*
 * With an open framebuffer file descriptor get the geometry of
 * the device
 */
int rgz_get_screengeometry(int fd, struct bvsurfgeom *geom, int fmt);

/*
 * Regionizer input parameters
 */
struct rgz_in_hwc {
    int flags;
    int layerno;
    hwc_layer_1_t *layers;
    struct bvsurfgeom *dstgeom;
};

typedef struct rgz_in_params {
    int op; /* See RGZ_IN_* */
    union {
        struct rgz_in_hwc hwc;
    } data;
} rgz_in_params_t;

/*
 * Validate whether the HWC layers can be rendered
 *
 * Arguments (rgz_in_params_t):
 * op                RGZ_IN_HWCCHK
 * data.hwc.layers   HWC layer array
 * data.hwc.layerno  HWC layer array size
 *
 * Returns:
 * rv = RGZ_ALL, -1 failure
 */
#define RGZ_IN_HWCCHK 1

/*
 * Regionize the HWC layers
 *
 * This generates region data which can be used with regionizer
 * output function. This call will validate whether all or some of the
 * layers can be rendered.
 *
 * The caller must use rgz_release when done with the region data
 *
 * Arguments (rgz_in_params_t):
 * op                RGZ_IN_HWC
 * data.hwc.layers   HWC layer array
 * data.hwc.layerno  HWC layer array size
 *
 * Returns:
 * rv = RGZ_ALL, -1 failure
 */
#define RGZ_IN_HWC 2

int rgz_in(rgz_in_params_t *param, rgz_t *rgz);

/* This means all layers can be blitted */
#define RGZ_ALL 1

/*
 * Free regionizer resources
 */
void rgz_release(rgz_t *rgz);

/*
 * Regionizer output operations
 */
struct rgz_out_bvcmd {
    void *cmdp;
    int cmdlen;
    struct bvsurfgeom *dstgeom;
    int noblend;
    buffer_handle_t out_hndls[RGZ_INPUT_MAXLAYERS]; /* OUTPUT */
    int out_nhndls; /* OUTPUT */
    int out_blits; /* OUTPUT */
};

struct rgz_out_svg {
    int dispw;
    int disph;
    int htmlw;
    int htmlh;
};

struct rgz_out_bvdirect {
    struct bvbuffdesc *dstdesc;
    struct bvsurfgeom *dstgeom;
    int noblend;
};

typedef struct rgz_out_params {
    int op; /* See RGZ_OUT_* */
    union {
        struct rgz_out_bvcmd bvc;
        struct rgz_out_bvdirect bv;
        struct rgz_out_svg svg;
    } data;
} rgz_out_params_t;

/*
 * Regionizer output commands
 */

/*
 * Output SVG from regionizer
 *
 * rgz_out_params_t:
 *
 * op              RGZ_OUT_SVG
 * data.svg.dispw
 * data.svg.disph  Display width and height these values will be the
 *                 viewport dimensions i.e. the logical coordinate space
 *                 rather than the physical size
 * data.svg.htmlw
 * data.svg.htmlh  HTML output dimensions
 */
#define RGZ_OUT_SVG 0

/*
 * This commands generates bltsville command data structures for HWC which will
 * paint layer by layer
 *
 * rgz_out_params_t:
 *
 * op                   RGZ_OUT_BVCMD_PAINT
 * data.bvc.cmdp        Pointer to buffer with cmd data
 * data.bvc.cmdlen      length of cmdp
 * data.bvc.dstgeom     bltsville struct describing the destination geometry
 * data.bvc.noblend     Test option to disable blending
 * data.bvc.out_hndls   Array of buffer handles (OUTPUT)
 * data.bvc.out_nhndls  Number of buffer handles (OUTPUT)
 * data.bvc.out_blits   Number of blits (OUTPUT)
 */
#define RGZ_OUT_BVCMD_PAINT 1

/*
 * This commands generates bltsville command data structures for HWC which will
 * render via regions. This will involve a complete redraw of the screen.
 *
 * See RGZ_OUT_BVCMD_PAINT
 */
#define RGZ_OUT_BVCMD_REGION 2

/*
 * Perform actual blits painting each layer from back to front - this is a test
 * command
 *
 * rgz_out_params_t:
 *
 * op                  RGZ_OUT_BVDIRECT_PAINT
 * data.bv.dstdesc     bltsville struct describing the destination buffer
 * data.bv.dstgeom     bltsville struct describing the destination geometry
 * data.bv.list        List of HWC layers to blit, only HWC_OVERLAY layers
 *                     will be rendered
 * data.bv.noblend     Test option to disable blending
 */
#define RGZ_OUT_BVDIRECT_PAINT 3
/*
 * Perform actual blits where each blit is a subregion - this is a test mode
 */
#define RGZ_OUT_BVDIRECT_REGION 5

int rgz_out(rgz_t *rgz, rgz_out_params_t* params);

/*
 * Produce instrumented logging of layer data
 */
void rgz_profile_hwc(hwc_display_contents_1_t* list, int dispw, int disph);

/*
 * ----------------------------------
 * IMPLEMENTATION DETAILS FOLLOW HERE
 * ----------------------------------
 */

/*
 * Regionizer blit data structures
 */
typedef struct blit_rect {
    int left, top, right, bottom;
} blit_rect_t;

/*
 * A hregion is a horizontal area generated from the intersection of layers
 * for a given composition.
 *
 * ----------------------------------------
 * | layer 0                              |
 * |           xxxxxxxxxxxxxxxxxx         |
 * |           x layer 1        x         |
 * |           x                x         |
 * |           x        xxxxxxxxxxxxxxxxxxx
 * |           x        x layer 2         x
 * |           x        x                 x
 * |           xxxxxxxxxx                 x
 * |                    x                 x
 * |                    x                 x
 * ---------------------xxxxxxxxxxxxxxxxxxx
 *
 * This can be broken up into a number of horizontal regions:
 *
 * ----------------------------------------
 * | H1                                l0 |
 * |-----------xxxxxxxxxxxxxxxxxx---------|
 * | H2        x                x         |
 * |        l0 x            l01 x      l0 |
 * |-----------x--------xxxxxxxxxxxxxxxxxxx
 * | H3        x        x       x         x
 * |        l0 x    l01 x  l012 x     l02 x
 * |-----------xxxxxxxxxxxxxxxxxx---------x
 * | H4                 x                 x
 * |                 l0 x             l02 x
 * ---------------------xxxxxxxxxxxxxxxxxxx
 *
 * Each hregion is just an array of rectangles. By accounting for the layers
 * at different z-order, and hregion becomes a multi-dimensional array e.g. in
 * the diagram above H4 has 2 sub-regions, layer 0 intersects with the first
 * region and layers 0 and 2 intersect with the second region.
 */
#define RGZ_SUBREGIONMAX ((RGZ_MAXLAYERS << 1) - 1)
#define RGZ_MAX_BLITS (RGZ_SUBREGIONMAX * RGZ_SUBREGIONMAX)

typedef struct rgz_layer {
    hwc_layer_1_t *hwc_layer;
    int buffidx;
    int dirty_count;
    void* dirty_hndl;
} rgz_layer_t;

typedef struct blit_hregion {
    blit_rect_t rect;
    rgz_layer_t *rgz_layers[RGZ_MAXLAYERS];
    int nlayers;
    int nsubregions;
    blit_rect_t blitrects[RGZ_MAXLAYERS][RGZ_SUBREGIONMAX]; /* z-order | rectangle */
} blit_hregion_t;

enum { RGZ_STATE_INIT = 1, RGZ_REGION_DATA = 2} ;

struct rgz {
    /* All fields here are opaque to the caller */
    blit_hregion_t *hregions;
    int nhregions;
    int state;
    unsigned int rgz_layerno;
    rgz_layer_t rgz_layers[RGZ_MAXLAYERS];
};

#endif /* __RGZ_2D__ */
