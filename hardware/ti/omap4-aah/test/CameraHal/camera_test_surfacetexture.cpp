#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <semaphore.h>
#include <pthread.h>
#include <string.h>
#include <climits>
#include <math.h>

#include <gui/GLConsumer.h>
#include <gui/Surface.h>
#include <ui/GraphicBuffer.h>
#include <ui/GraphicBufferMapper.h>

#include <camera/Camera.h>
#include <camera/ICamera.h>
#include <media/mediarecorder.h>

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <cutils/properties.h>
#include <camera/CameraParameters.h>
#include <camera/ShotParameters.h>
#include <camera/CameraMetadata.h>
#include <system/audio.h>
#include <system/camera.h>

#include <cutils/memory.h>
#include <utils/Log.h>

#include <sys/wait.h>

#include "camera_test.h"
#include "camera_test_surfacetexture.h"

#define ASSERT(X) \
    do { \
       if(!(X)) { \
           printf("error: %s():%d", __FUNCTION__, __LINE__); \
           return; \
       } \
    } while(0);

#define ALIGN_DOWN(x, n) ((x) & (~((n) - 1)))
#define ALIGN_UP(x, n) ((((x) + (n) - 1)) & (~((n) - 1)))
#define ALIGN_WIDTH 32 // Should be 32...but the calculated dimension causes an ion crash
#define ALIGN_HEIGHT 2 // Should be 2...but the calculated dimension causes an ion crash

//temporarily define format here
#define HAL_PIXEL_FORMAT_TI_NV12 0x100
#define HAL_PIXEL_FORMAT_TI_Y8 0x103
#define HAL_PIXEL_FORMAT_TI_Y16 0x104
#define HAL_PIXEL_FORMAT_TI_UYVY 0x105

using namespace android;

static EGLint getSurfaceWidth() {
    return 512;
}

static EGLint getSurfaceHeight() {
    return 512;
}

static size_t calcBufSize(int format, int width, int height)
{
    int buf_size;

    switch (format) {
        case HAL_PIXEL_FORMAT_TI_NV12:
            buf_size = width * height * 3 /2;
            break;
        case HAL_PIXEL_FORMAT_TI_Y16:
        case HAL_PIXEL_FORMAT_TI_UYVY:
            buf_size = width * height * 2;
            break;
        // add more formats later
        default:
            buf_size = width * height * 3 /2;
            break;
    }

    return buf_size;
}

static unsigned int calcOffset(int format, unsigned int width, unsigned int top, unsigned int left)
{
    unsigned int bpp;

    switch (format) {
        case HAL_PIXEL_FORMAT_TI_NV12:
            bpp = 1;
            break;
        case HAL_PIXEL_FORMAT_TI_UYVY:
        case HAL_PIXEL_FORMAT_TI_Y16:
            bpp = 2;
            break;
        // add more formats later
        default:
            bpp = 1;
            break;
    }

    return top * width + left * bpp;
}

static int getHalPixFormat(const char *format)
{
    int pixformat = HAL_PIXEL_FORMAT_TI_NV12;
    if ( NULL != format ) {
        if ( strcmp(format, CameraParameters::PIXEL_FORMAT_BAYER_RGGB) == 0 ) {
            pixformat = HAL_PIXEL_FORMAT_TI_Y16;
        } else if ( strcmp(format, CameraParameters::PIXEL_FORMAT_YUV420SP) == 0 ) {
            pixformat = HAL_PIXEL_FORMAT_TI_NV12;
        } else if ( strcmp(format, CameraParameters::PIXEL_FORMAT_YUV422I) == 0 ) {
            pixformat = HAL_PIXEL_FORMAT_TI_UYVY;
        } else {
            pixformat = HAL_PIXEL_FORMAT_TI_NV12;
        }
    }

    return pixformat;
}

static int getUsageFromANW(int format)
{
    int usage = GRALLOC_USAGE_SW_READ_RARELY |
                GRALLOC_USAGE_SW_WRITE_NEVER;

    switch (format) {
        case HAL_PIXEL_FORMAT_TI_NV12:
        case HAL_PIXEL_FORMAT_TI_Y16:
            // This usage flag indicates to gralloc we want the
            // buffers to come from system heap
            usage |= GRALLOC_USAGE_PRIVATE_0;
            break;
        default:
            // No special flags needed
            break;
    }
    return usage;
}

static status_t writeCroppedNV12(unsigned int offset,
                                 unsigned int stride,
                                 unsigned int bufWidth,
                                 unsigned int bufHeight,
                                 const Rect &crop,
                                 int fd,
                                 unsigned char *buffer)
{
    unsigned char *luma = NULL, *chroma = NULL, *src = NULL;
    unsigned int uvoffset;
    int write_size;

    if (!buffer || !crop.isValid()) {
        return BAD_VALUE;
    }

    src = buffer;
    // offset to beginning of uv plane
    uvoffset =  stride * bufHeight;
    // offset to beginning of valid region of uv plane
    uvoffset += (offset - (offset % stride)) / 2 + (offset % stride);

    // start of valid luma region
    luma = src + offset;
    // start of valid chroma region
    chroma = src + uvoffset;

    // write luma line x line
    unsigned int height = crop.height();
    unsigned int width = crop.width();
    write_size = width;
    for (unsigned int i = 0; i < height; i++) {
        if (write_size != write(fd, luma, width)) {
            printf("Bad Write error (%d)%s\n",
                    errno, strerror(errno));
            return UNKNOWN_ERROR;
        }
        luma += stride;
    }

    // write chroma line x line
    height /= 2;
    write_size = width;
    for (unsigned int i = 0; i < height; i++) {
        if (write_size != write(fd, chroma, width)) {
            printf("Bad Write error (%d)%s\n",
                    errno, strerror(errno));
            return UNKNOWN_ERROR;
        }
        chroma += stride;
    }

    return NO_ERROR;
}

static status_t writeCroppedUYVY(unsigned int offset,
                                 unsigned int stride,
                                 unsigned int bufWidth,
                                 unsigned int bufHeight,
                                 const Rect &crop,
                                 int fd,
                                 unsigned char *buffer)
{
    unsigned char *src = NULL;
    int write_size;

    if (!buffer) {
        return BAD_VALUE;
    }

    src = buffer + offset;
    int height = crop.height();
    int width = crop.width();
    write_size = width*2;
    for (unsigned int i = 0; i < height; i++) {
        if (write_size != write(fd, src, width*2)) {
            printf("Bad Write error (%d)%s\n",
                    errno, strerror(errno));
            return UNKNOWN_ERROR;
        }
        src += stride*2;
    }

    return NO_ERROR;
}

static status_t copyCroppedNV12(unsigned int offset,
                                unsigned int strideSrc,
                                unsigned int strideDst,
                                unsigned int bufWidth,
                                unsigned int bufHeight,
                                const Rect &crop,
                                void *bufferSrc,
                                void *bufferDst)
{
    unsigned char *lumaSrc = NULL, *chromaSrc = NULL;
    unsigned char *lumaDst = NULL, *chromaDst = NULL;
    unsigned int uvoffset;
    int write_size;

    if (!bufferSrc || !bufferDst) {
        return BAD_VALUE;
    }

    uvoffset = strideSrc * crop.height();
    uvoffset += (offset - (offset % strideSrc)) / 2 + (offset % strideSrc);

    lumaSrc = static_cast<unsigned char *>(bufferSrc) + offset;
    chromaSrc = static_cast<unsigned char *>(bufferSrc) + uvoffset;

    int height = crop.height();
    int width = crop.width();

    uvoffset =  strideDst * height;

    lumaDst = static_cast<unsigned char *>(bufferDst);
    chromaDst = static_cast<unsigned char *>(bufferDst) + uvoffset;

    write_size = width;
    for (unsigned int i = 0; i < height; i++) {
        memcpy(lumaDst, lumaSrc, width);
        lumaSrc += strideSrc;
        lumaDst += strideDst;
    }

    height /= 2;
    write_size = width;
    for (unsigned int i = 0; i < height; i++) {
        memcpy(chromaDst, chromaSrc, width);
        chromaSrc += strideSrc;
        chromaDst += strideDst;
    }

    return NO_ERROR;
}

static status_t copyCroppedPacked16(unsigned int offset,
                                    unsigned int stride,
                                    unsigned int bufWidth,
                                    unsigned int bufHeight,
                                    const Rect &crop,
                                    void *bufferSrc,
                                    void *bufferDst)
{
    unsigned char *src = NULL, *dst = NULL;

    if (!bufferSrc || !bufferDst) {
        return BAD_VALUE;
    }

    src = static_cast<unsigned char *>(bufferSrc) + offset;
    dst = static_cast<unsigned char *>(bufferDst);

    int height = crop.height();
    int width = crop.width();
    for (unsigned int i = 0; i < height; i++) {
        memcpy(dst, src, width*2);
        src += stride*2;
        dst += width*2;
    }

    return NO_ERROR;
}

void GLSurface::initialize(int display) {
    mEglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    ASSERT(EGL_SUCCESS == eglGetError());
    ASSERT(EGL_NO_DISPLAY != mEglDisplay);

    EGLint majorVersion;
    EGLint minorVersion;
    ASSERT(eglInitialize(mEglDisplay, &majorVersion, &minorVersion));
    ASSERT(EGL_SUCCESS == eglGetError());

    EGLint numConfigs = 0;
    ASSERT(eglChooseConfig(mEglDisplay, getConfigAttribs(), &mGlConfig,
                1, &numConfigs));
    ASSERT(EGL_SUCCESS == eglGetError());

    if (display) {
        mComposerClient = new SurfaceComposerClient;
        ASSERT(NO_ERROR == mComposerClient->initCheck());
        mSurfaceControl = mComposerClient->createSurface(
                String8("Test Surface"), 0,
                800, 480, HAL_PIXEL_FORMAT_YCrCb_420_SP, 0);

        ASSERT(mSurfaceControl != NULL);
        ASSERT(mSurfaceControl->isValid());

        SurfaceComposerClient::openGlobalTransaction();
        ASSERT(NO_ERROR == mSurfaceControl->setLayer(0x7FFFFFFF));
        ASSERT(NO_ERROR == mSurfaceControl->show());
        SurfaceComposerClient::closeGlobalTransaction();

        sp<ANativeWindow> window = mSurfaceControl->getSurface();
        mEglSurface = eglCreateWindowSurface(mEglDisplay, mGlConfig,
                window.get(), NULL);
    } else {
        EGLint pbufferAttribs[] = {
            EGL_WIDTH, getSurfaceWidth(),
            EGL_HEIGHT, getSurfaceHeight(),
            EGL_NONE };
        mEglSurface = eglCreatePbufferSurface(mEglDisplay, mGlConfig,
                pbufferAttribs);
    }
    ASSERT(EGL_SUCCESS == eglGetError());
    ASSERT(EGL_NO_SURFACE != mEglSurface);

    mEglContext = eglCreateContext(mEglDisplay, mGlConfig, EGL_NO_CONTEXT,
            getContextAttribs());
    ASSERT(EGL_SUCCESS == eglGetError());
    ASSERT(EGL_NO_CONTEXT != mEglContext);

    ASSERT(eglMakeCurrent(mEglDisplay, mEglSurface, mEglSurface,
            mEglContext));
    ASSERT(EGL_SUCCESS == eglGetError());

    EGLint w, h;
    ASSERT(eglQuerySurface(mEglDisplay, mEglSurface, EGL_WIDTH, &w));
    ASSERT(EGL_SUCCESS == eglGetError());
    ASSERT(eglQuerySurface(mEglDisplay, mEglSurface, EGL_HEIGHT, &h));
    ASSERT(EGL_SUCCESS == eglGetError());

    glViewport(0, 0, w, h);
    ASSERT(GLenum(GL_NO_ERROR) == glGetError());
}

void GLSurface::deinit() {
    if (mComposerClient != NULL) {
        mComposerClient->dispose();
    }

    if (mEglContext != EGL_NO_CONTEXT) {
        eglDestroyContext(mEglDisplay, mEglContext);
    }

    if (mEglSurface != EGL_NO_SURFACE) {
        eglDestroySurface(mEglDisplay, mEglSurface);
    }
    if (mEglDisplay != EGL_NO_DISPLAY) {
        eglMakeCurrent(mEglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE,
               EGL_NO_CONTEXT);
        eglTerminate(mEglDisplay);
    }
    ASSERT(EGL_SUCCESS == eglGetError());
}

EGLint const* GLSurface::getConfigAttribs() {
    static EGLint sDefaultConfigAttribs[] = {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 16,
        EGL_STENCIL_SIZE, 8,
        EGL_NONE };

    return sDefaultConfigAttribs;
}

EGLint const* GLSurface::getContextAttribs() {
    static EGLint sDefaultContextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE };

    return sDefaultContextAttribs;
}

void GLSurface::loadShader(GLenum shaderType, const char* pSource, GLuint* outShader) {
    GLuint shader = glCreateShader(shaderType);
    ASSERT(GLenum(GL_NO_ERROR) == glGetError());
    if (shader) {
        glShaderSource(shader, 1, &pSource, NULL);
        ASSERT(GLenum(GL_NO_ERROR) == glGetError());
        glCompileShader(shader);
        ASSERT(GLenum(GL_NO_ERROR) == glGetError());
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        ASSERT(GLenum(GL_NO_ERROR) == glGetError());
        if (!compiled) {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            ASSERT(GLenum(GL_NO_ERROR) == glGetError());
            if (infoLen) {
                char* buf = (char*) malloc(infoLen);
                if (buf) {
                    glGetShaderInfoLog(shader, infoLen, NULL, buf);
                    printf("Shader compile log:\n%s\n", buf);
                    free(buf);
                }
            } else {
                char* buf = (char*) malloc(0x1000);
                if (buf) {
                    glGetShaderInfoLog(shader, 0x1000, NULL, buf);
                    printf("Shader compile log:\n%s\n", buf);
                    free(buf);
                }
            }
            glDeleteShader(shader);
            shader = 0;
        }
    }
    ASSERT(shader != 0);
    *outShader = shader;
}

void GLSurface::createProgram(const char* pVertexSource, const char* pFragmentSource,
            GLuint* outPgm) {
    GLuint vertexShader, fragmentShader;
    {
        loadShader(GL_VERTEX_SHADER, pVertexSource, &vertexShader);
    }
    {
        loadShader(GL_FRAGMENT_SHADER, pFragmentSource, &fragmentShader);
    }

    GLuint program = glCreateProgram();
    ASSERT(GLenum(GL_NO_ERROR) == glGetError());
    if (program) {
        glAttachShader(program, vertexShader);
        ASSERT(GLenum(GL_NO_ERROR) == glGetError());
        glAttachShader(program, fragmentShader);
        ASSERT(GLenum(GL_NO_ERROR) == glGetError());
        glLinkProgram(program);
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            GLint bufLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
            if (bufLength) {
                char* buf = (char*) malloc(bufLength);
                if (buf) {
                    glGetProgramInfoLog(program, bufLength, NULL, buf);
                    printf("Program link log:\n%s\n", buf);
                    free(buf);
                }
            }
            glDeleteProgram(program);
            program = 0;
        }
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    ASSERT(program != 0);
    *outPgm = program;
}

// GLConsumer specific
sp<GLConsumer> SurfaceTextureBase::getST() {
     return mST;
}

void SurfaceTextureBase::initialize(int tex_id, EGLenum tex_target) {
    mTexId = tex_id;
    sp<BufferQueue> bq = new BufferQueue();
    mST = new GLConsumer(bq, tex_id, tex_target);
    mSTC = new Surface(mST);
    mANW = mSTC;
}

void SurfaceTextureBase::deinit() {
    mANW.clear();
    mSTC.clear();

    mST->abandon();
    mST.clear();
}

void SurfaceTextureBase::getId(const char **name) {
    sp<ANativeWindow> windowTapOut = mSTC;

    *name = NULL;
    if (windowTapOut.get()) {
        windowTapOut->perform(windowTapOut.get(), NATIVE_WINDOW_GET_ID, name);
    }

    windowTapOut.clear();
}

// GLConsumer with GL specific

void SurfaceTextureGL::initialize(int display, int tex_id) {
    GLSurface::initialize(display);
    SurfaceTextureBase::initialize(tex_id, GL_TEXTURE_EXTERNAL_OES);

    const char vsrc[] =
        "attribute vec4 vPosition;\n"
        "varying vec2 texCoords;\n"
        "uniform mat4 texMatrix;\n"
        "void main() {\n"
        "  vec2 vTexCoords = 0.5 * (vPosition.xy + vec2(1.0, 1.0));\n"
        "  texCoords = (texMatrix * vec4(vTexCoords, 0.0, 1.0)).xy;\n"
        "  gl_Position = vPosition;\n"
        "}\n";

    const char fsrc[] =
        "#extension GL_OES_EGL_image_external : require\n"
        "precision mediump float;\n"
        "uniform samplerExternalOES texSampler;\n"
        "varying vec2 texCoords;\n"
        "void main() {\n"
        "  gl_FragColor = texture2D(texSampler, texCoords);\n"
        "}\n";

    {
        createProgram(vsrc, fsrc, &mPgm);
    }

    mPositionHandle = glGetAttribLocation(mPgm, "vPosition");
    ASSERT(GLenum(GL_NO_ERROR) == glGetError());
    ASSERT(-1 != mPositionHandle);
    mTexSamplerHandle = glGetUniformLocation(mPgm, "texSampler");
    ASSERT(GLenum(GL_NO_ERROR) == glGetError());
    ASSERT(-1 != mTexSamplerHandle);
    mTexMatrixHandle = glGetUniformLocation(mPgm, "texMatrix");
    ASSERT(GLenum(GL_NO_ERROR) == glGetError());
    ASSERT(-1 != mTexMatrixHandle);
}

void SurfaceTextureGL::deinit() {
    SurfaceTextureBase::deinit();
    GLSurface::deinit();
}

// drawTexture draws the GLConsumer over the entire GL viewport.
void SurfaceTextureGL::drawTexture() {
    const GLfloat triangleVertices[] = {
        -1.0f, 1.0f,
        -1.0f, -1.0f,
        1.0f, -1.0f,
        1.0f, 1.0f,
    };

    glVertexAttribPointer(mPositionHandle, 2, GL_FLOAT, GL_FALSE, 0,
            triangleVertices);
    ASSERT(GLenum(GL_NO_ERROR) == glGetError());
    glEnableVertexAttribArray(mPositionHandle);
    ASSERT(GLenum(GL_NO_ERROR) == glGetError());

    glUseProgram(mPgm);
    glUniform1i(mTexSamplerHandle, 0);
    ASSERT(GLenum(GL_NO_ERROR) == glGetError());
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, mTexId);
    ASSERT(GLenum(GL_NO_ERROR) == glGetError());

    // XXX: These calls are not needed for GL_TEXTURE_EXTERNAL_OES as
    // they're setting the defautls for that target, but when hacking things
    // to use GL_TEXTURE_2D they are needed to achieve the same behavior.
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER,
            GL_LINEAR);
    ASSERT(GLenum(GL_NO_ERROR) == glGetError());
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER,
            GL_LINEAR);
    ASSERT(GLenum(GL_NO_ERROR) == glGetError());
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S,
            GL_CLAMP_TO_EDGE);
    ASSERT(GLenum(GL_NO_ERROR) == glGetError());
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T,
            GL_CLAMP_TO_EDGE);
    ASSERT(GLenum(GL_NO_ERROR) == glGetError());

    GLfloat texMatrix[16];
    mST->getTransformMatrix(texMatrix);
    glUniformMatrix4fv(mTexMatrixHandle, 1, GL_FALSE, texMatrix);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    ASSERT(GLenum(GL_NO_ERROR) == glGetError());

    eglSwapBuffers(mEglDisplay, mEglSurface);
}

// buffer source stuff
void BufferSourceThread::handleBuffer(sp<GraphicBuffer> &graphic_buffer, uint8_t *buffer,
                                        unsigned int count, const Rect &crop) {
    int size;
    buffer_info_t info;
    unsigned int offset = 0;
    int fd = -1;
    char fn[256];

    if (!graphic_buffer.get()) {
        printf("Invalid graphic_buffer!\n");
        return;
    }

    size = calcBufSize((int)graphic_buffer->getPixelFormat(),
                              graphic_buffer->getWidth(),
                              graphic_buffer->getHeight());
    if (size <= 0) {
        printf("Can't get size!\n");
        return;
    }

    if (!buffer) {
        printf("Invalid mapped buffer!\n");
        return;
    }

    info.size = size;
    info.width = graphic_buffer->getWidth();
    info.height = graphic_buffer->getHeight();
    info.format = graphic_buffer->getPixelFormat();
    info.buf = graphic_buffer;
    info.crop = crop;

    {
        Mutex::Autolock lock(mReturnedBuffersMutex);
        if (mReturnedBuffers.size() >= kReturnedBuffersMaxCapacity) mReturnedBuffers.removeAt(0);
    }

    // re-calculate size and offset
    size = calcBufSize((int) graphic_buffer->getPixelFormat(), crop.width(), crop.height());
    offset = calcOffset((int) graphic_buffer->getPixelFormat(), info.width, crop.top, crop.left);

    // Do not write buffer to file if we are streaming capture
    // It adds too much latency
    if (!mRestartCapture) {
        fn[0] = 0;
        sprintf(fn, "/sdcard/img%03d.raw", count);
        fd = open(fn, O_CREAT | O_WRONLY | O_TRUNC, 0777);
        if (fd >= 0) {
            if (HAL_PIXEL_FORMAT_TI_NV12 == info.format) {
                writeCroppedNV12(offset, info.width, info.width, info.height,
                                 crop, fd, buffer);
            } else if (HAL_PIXEL_FORMAT_TI_UYVY == info.format) {
                writeCroppedUYVY(offset, info.width, info.width, info.height,
                                 crop, fd, buffer);
            } else if (size != write(fd, buffer + offset, size)) {
                printf("Bad Write int a %s error (%d)%s\n", fn, errno, strerror(errno));
            }
            printf("%s: buffer=%08X, size=%d stored at %s\n"
                   "\tRect: top[%d] left[%d] right[%d] bottom[%d] width[%d] height[%d] offset[%d] stride[%d]\n",
                        __FUNCTION__, (int)buffer, size, fn,
                        crop.top, crop.left, crop.right, crop.bottom,
                        crop.width(), crop.height(),
                        offset, info.width);
            close(fd);
        } else {
            printf("error opening or creating %s\n", fn);
        }
    }
}

Rect BufferSourceThread::getCrop(sp<GraphicBuffer> &graphic_buffer, const float *mtx) {
    Rect crop(graphic_buffer->getWidth(), graphic_buffer->getHeight());

    // calculate crop rectangle from tranformation matrix
    float sx, sy, tx, ty, h, w;
    unsigned int rect_x, rect_y;
    /*   sx, 0, 0, 0,
         0, sy, 0, 0,
         0, 0, 1, 0,
         tx, ty, 0, 1 */

    sx = mtx[0];
    sy = mtx[5];
    tx = mtx[12];
    ty = mtx[13];
    w = float(graphic_buffer->getWidth());
    h = float(graphic_buffer->getHeight());

    unsigned int bottom = (unsigned int)(h - (ty * h + 1));
    unsigned int left = (unsigned int)(tx * w -1);
    rect_y = (unsigned int)(fabsf(sy) * h);
    rect_x = (unsigned int)(fabsf(sx) * w);

    // handle v-flip
    if (sy < 0.0f) {
        bottom = h - bottom;
    }

    // handle h-flip
    if (sx < 0.0f) {
        left = w - left;
    }

    unsigned int top = bottom - rect_y;
    unsigned int right = left + rect_x;

    Rect updatedCrop(left, top, right, bottom);
    if (updatedCrop.isValid()) {
        crop = updatedCrop;
    } else {
        printf("Crop for buffer %d is not valid: "
               "left=%u, top=%u, right=%u, bottom=%u. "
               "Will use default.\n",
               mCounter,
               left, top, right, bottom);
    }

    return crop;
}

void BufferSourceInput::setInput(buffer_info_t bufinfo, const char *format, ShotParameters &params) {
    ANativeWindowBuffer* anb;
    GraphicBufferMapper &mapper = GraphicBufferMapper::get();
    int pixformat = HAL_PIXEL_FORMAT_TI_NV12;
    size_t tapInMinUndequeued = 0;

    int aligned_width, aligned_height;

    pixformat = bufinfo.format;

    // Aligning is not needed for Bayer
    if ( ( pixformat == HAL_PIXEL_FORMAT_TI_Y16 ) ||
         ( pixformat == HAL_PIXEL_FORMAT_TI_UYVY ) ) {
        aligned_width = bufinfo.crop.right - bufinfo.crop.left;
    } else {
        aligned_width = ALIGN_UP(bufinfo.crop.right - bufinfo.crop.left, ALIGN_WIDTH);
    }
    aligned_height = bufinfo.crop.bottom - bufinfo.crop.top;
    printf("aligned width: %d height: %d \n", aligned_width, aligned_height);

    if (mWindowTapIn.get() == 0) {
        return;
    }

    native_window_set_usage(mWindowTapIn.get(),
                            getUsageFromANW(pixformat));
    mWindowTapIn->perform(mWindowTapIn.get(),
                          NATIVE_WINDOW_MIN_UNDEQUEUED_BUFFERS,
                          &tapInMinUndequeued);;
    native_window_set_buffer_count(mWindowTapIn.get(), tapInMinUndequeued);
    native_window_set_buffers_geometry(mWindowTapIn.get(),
                  aligned_width, aligned_height, bufinfo.format);

    // if buffer dimensions are the same as the aligned dimensions, then we can
    // queue the buffer directly to tapin surface. if the dimensions are different
    // then the aligned ones, then we have to copy the buffer into our own buffer
    // to make sure the stride of the buffer is correct
    if ((aligned_width != bufinfo.width) || (aligned_height != bufinfo.height) ||
        ( pixformat == HAL_PIXEL_FORMAT_TI_Y16 ) ||
        ( pixformat == HAL_PIXEL_FORMAT_TI_UYVY) ) {
        void *dest[3] = { 0 };
        void *src[3] = { 0 };
        Rect bounds(aligned_width, aligned_height);

        mWindowTapIn->dequeueBuffer(mWindowTapIn.get(), &anb);
        mapper.lock(anb->handle, GRALLOC_USAGE_SW_WRITE_OFTEN, bounds, dest);
        // copy buffer to input buffer if available
        if (bufinfo.buf.get()) {
            bufinfo.buf->lock(GRALLOC_USAGE_SW_READ_OFTEN, src);
        }
        if (src[0]) {
            switch (pixformat) {
                case HAL_PIXEL_FORMAT_TI_Y16:
                case HAL_PIXEL_FORMAT_TI_UYVY:
                    copyCroppedPacked16(bufinfo.offset,
                                        bufinfo.width,
                                        bufinfo.width,
                                        bufinfo.height,
                                        bufinfo.crop,
                                        src[0],
                                        dest[0]);
                    break;
                case HAL_PIXEL_FORMAT_TI_NV12:
                    copyCroppedNV12(bufinfo.offset,
                                    bufinfo.width,
                                    aligned_width,
                                    bufinfo.width,
                                    bufinfo.height,
                                    bufinfo.crop,
                                    src[0],
                                    dest[0]);
                    break;
                default:
                    printf("Pixel format 0x%x not supported\n", pixformat);
                    exit(1);
                    break;
            }
        }
        if (bufinfo.buf.get()) {
            bufinfo.buf->unlock();
        }

        mapper.unlock(anb->handle);
    } else {
        mWindowTapIn->perform(mWindowTapIn.get(), NATIVE_WINDOW_ADD_BUFFER_SLOT, &bufinfo.buf);
        anb = bufinfo.buf->getNativeBuffer();
    }

    mWindowTapIn->queueBuffer(mWindowTapIn.get(), anb);

    {
        sp<ANativeWindow> windowTapIn = mWindowTapIn;
        const char* id = NULL;

        if (windowTapIn.get()) {
            windowTapIn->perform(windowTapIn.get(), NATIVE_WINDOW_GET_ID, &id);
        }

        if (id) {
            params.set(KEY_TAP_IN_SURFACE, id);
        } else {
            params.remove(KEY_TAP_IN_SURFACE);
        }

        windowTapIn.clear();
    }
}

void BufferSourceThread::showMetadata(sp<IMemory> data) {
    static nsecs_t prevTime = 0;
    nsecs_t currTime = 0;

    ssize_t offset;
    size_t size;

    if ( NULL == data.get() ) {
        printf("No Metadata!");
        return;
    }

    sp<IMemoryHeap> heap = data->getMemory(&offset, &size);
    camera_metadata_t * meta = static_cast<camera_metadata_t *> (heap->base());

    printf("         frame nmber: %d\n", meta->frame_number);
    printf("         shot number: %d\n", meta->shot_number);
    printf("         analog gain: %d req: %d range: %d~%d dev: %d err: %d\n",
           meta->analog_gain,
           meta->analog_gain_req,
           meta->analog_gain_min,
           meta->analog_gain_max,
           meta->analog_gain_dev,
           meta->analog_gain_error);
    printf("       exposure time: %d req: %d range: %d~%d dev: %d err: %d\n",
           meta->exposure_time,
           meta->exposure_time_req,
           meta->exposure_time_min,
           meta->exposure_time_max,
           meta->exposure_time_dev,
           meta->exposure_time_error);
    printf("     EV compensation: req: %d dev: %d\n",
           meta->exposure_compensation_req,
           meta->exposure_dev);
    printf("            awb gain: %d\n", meta->analog_gain);
    printf("         awb offsets: %d\n", meta->offset_b);
    printf("     awb temperature: %d\n", meta->awb_temp);

    printf("   LSC table applied: %d\n", meta->lsc_table_applied);
    if ( meta->lsc_table_applied ) {
        uint8_t *lscTable = (uint8_t *)meta + meta->lsc_table_offset;
        printf("LSC Table Size:%d Data[0:7]: %d:%d:%d:%d:%d:%d:%d:%d\n",
                meta->lsc_table_size,
                lscTable[0],
                lscTable[1],
                lscTable[2],
                lscTable[3],
                lscTable[4],
                lscTable[5],
                lscTable[6],
                lscTable[7]);
    }

    printf("    Faces detected: %d\n", meta->number_of_faces);

    currTime = meta->timestamp;
    printf("      timestamp (ns): %llu\n", currTime);
    if (prevTime) printf("inter-shot time (ms): %llu\n", (currTime - prevTime) / 1000000l);
    prevTime = currTime;
}
