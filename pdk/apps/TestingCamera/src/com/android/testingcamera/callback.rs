#pragma version(1)
#pragma rs java_package_name(com.android.testingcamera)
#pragma rs_fp_relaxed

uchar *yuv_in;

// Input globals
uint32_t yuv_height;
uint32_t yuv_width;
uint32_t out_width;
uint32_t out_height;
// Derived globals
uint32_t y_stride;
uint32_t uv_stride;
uint32_t u_start;
uint32_t v_start;
float x_scale;
float y_scale;

static const float CLAMP_MIN = 0;
static const float CLAMP_MAX = 255;

/**
 * JFIF standard YCbCr <-> RGB conversion matrix,
 * column-major order.
 */
static const float YUV2RGB[] = {
    1.0f, 1.0f, 1.0f, 0.0f,
    0.0f, -0.34414f, 1.772f, 0.0f,
    1.402f, -0.71414f, 0.0f, 0.0f,
    -0.701f, 0.529f, -0.886f, 1.0f
};
rs_matrix4x4 yuv2rgb_matrix;

enum ImageFormat {
    NV16 = 16,
    NV21 = 17,
    RGB_565 = 4,
    UNKNOWN = 0,
    YUY2 = 20,
    YV12 = 0x32315659
};

// Must be called before using any conversion methods
void init_convert(uint32_t yw, uint32_t yh, uint32_t format,
        uint32_t ow, uint32_t oh) {
    yuv_height = yh;
    yuv_width = yw;
    out_width = ow;
    out_height = oh;
    rsMatrixLoad(&yuv2rgb_matrix, YUV2RGB);

    x_scale = (float)yuv_width / out_width;
    y_scale = (float)yuv_height / out_height;

    switch (format) {
    case NV16:
    case NV21:
        y_stride = yuv_width;
        uv_stride = yuv_width;
        v_start = y_stride * yuv_height;
        u_start = v_start + 1;
        break;
    case YV12:
        // Minimum align-16 stride
        y_stride = (yuv_width + 0xF) & ~0xF;
        uv_stride = (y_stride / 2 + 0xF) & ~0xF;
        v_start = y_stride * yuv_height;
        u_start = v_start + uv_stride * (yuv_height / 2);
        break;
    case YUY2:
        y_stride = yuv_width * 2;
        uv_stride = y_stride;
        u_start = 1;
        v_start = 3;
        break;
    case RGB_565:
    case UNKNOWN:
    default:
        y_stride = yuv_width;
        uv_stride = yuv_width;
        v_start = 0;
        u_start = 0;
    }
}

// Multiply by color matrix and clamp to range [0, 255]
static inline uchar4 multiply_and_clamp(const rs_matrix4x4* mat, uchar4 input) {
    float4 intermediate = convert_float4(input);
    intermediate = rsMatrixMultiply(mat, intermediate);
    intermediate = clamp(intermediate, CLAMP_MIN, CLAMP_MAX);
    return convert_uchar4(intermediate);
}

// Makes up a conversion for unknown YUV types to try to display something
// Asssumes that there's at least 1bpp in input YUV data
uchar4 __attribute__((kernel)) convert_unknown(uint32_t x, uint32_t y) {
    uint32_t x_scaled = x * x_scale;
    uint32_t y_scaled = y * y_scale;

    uchar4 out;
    out.r = yuv_in[y_stride * y_scaled + x_scaled];
    out.g = 128;
    out.b = 128;
    out.a = 255; // For affine transform later

    // Apply yuv->rgb color transform
    return multiply_and_clamp(&yuv2rgb_matrix, out);
}

// Converts semiplanar YVU to interleaved YUV, nearest neighbor
uchar4 __attribute__((kernel)) convert_semiplanar(uint32_t x, uint32_t y) {
    uint32_t x_scaled = x * x_scale;
    uint32_t y_scaled = y * y_scale;

    uint32_t uv_row = y_scaled / 2; // truncation is important here
    uint32_t uv_col = x_scaled & ~0x1;
    uint32_t vu_pixel = uv_row * uv_stride + uv_col;

    uchar4 out;
    out.r = yuv_in[y_stride * y_scaled + x_scaled];
    out.g = yuv_in[u_start + vu_pixel];
    out.b = yuv_in[v_start + vu_pixel];
    out.a = 255; // For affine transform later

    // Apply yuv->rgb color transform
    return multiply_and_clamp(&yuv2rgb_matrix, out);
}

// Converts planar YVU to interleaved YUV, nearest neighbor
uchar4 __attribute__((kernel)) convert_planar(uint32_t x, uint32_t y) {
    uint32_t x_scaled = x * x_scale;
    uint32_t y_scaled = y * y_scale;

    uint32_t uv_row = y_scaled / 2; // truncation is important here
    uint32_t vu_pixel = uv_stride * uv_row + x_scaled / 2;

    uchar4 out;
    out.r = yuv_in[y_stride * y_scaled + x_scaled];
    out.g = yuv_in[u_start + vu_pixel];
    out.b = yuv_in[v_start + vu_pixel];
    out.a = 255; // For affine transform later

    // Apply yuv->rgb color transform
    return multiply_and_clamp(&yuv2rgb_matrix, out);
}

// Converts interleaved 4:2:2 YUV to interleaved YUV, nearest neighbor
uchar4 __attribute__((kernel)) convert_interleaved(uint32_t x, uint32_t y) {
    uint32_t x_scaled = x * x_scale;
    uint32_t y_scaled = y * y_scale;

    uint32_t uv_col = 2 * (x_scaled & ~0x1);
    uint32_t vu_pixel = y_stride * y_scaled + uv_col;

    uchar4 out;
    out.r = yuv_in[y_stride * y_scaled + x_scaled * 2];
    out.g = yuv_in[u_start + vu_pixel];
    out.b = yuv_in[v_start + vu_pixel];
    out.a = 255; // For affine transform later

    // Apply yuv->rgb color transform
    return multiply_and_clamp(&yuv2rgb_matrix, out);
}
