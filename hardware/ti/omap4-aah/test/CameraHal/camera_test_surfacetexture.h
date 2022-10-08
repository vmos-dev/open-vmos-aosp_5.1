#ifndef CAMERA_TEST_SURFACE_TEXTURE_H
#define CAMERA_TEST_SURFACE_TEXTURE_H

#include "camera_test.h"

#ifdef ANDROID_API_JB_OR_LATER
#include <gui/Surface.h>
#include <gui/GLConsumer.h>
#include <gui/SurfaceComposerClient.h>
#else
#include <surfaceflinger/Surface.h>
#include <surfaceflinger/ISurface.h>
#include <surfaceflinger/ISurfaceComposer.h>
#include <surfaceflinger/ISurfaceComposerClient.h>
#include <surfaceflinger/SurfaceComposerClient.h>
#endif

#ifdef ANDROID_API_JB_OR_LATER
#   define CAMHAL_LOGV            ALOGV
#   define CAMHAL_LOGE            ALOGE
#   define PRINTOVER(arg...)      ALOGD(#arg)
#   define LOG_FUNCTION_NAME      ALOGD("%d: %s() ENTER", __LINE__, __FUNCTION__);
#   define LOG_FUNCTION_NAME_EXIT ALOGD("%d: %s() EXIT", __LINE__, __FUNCTION__);
#else
#   define CAMHAL_LOGV            LOGV
#   define CAMHAL_LOGE            LOGE
#   define PRINTOVER(arg...)      LOGD(#arg)
#   define LOG_FUNCTION_NAME      LOGD("%d: %s() ENTER", __LINE__, __FUNCTION__);
#   define LOG_FUNCTION_NAME_EXIT LOGD("%d: %s() EXIT", __LINE__, __FUNCTION__);
#endif

using namespace android;

class FrameWaiter : public android::GLConsumer::FrameAvailableListener {
public:
    FrameWaiter():
            mPendingFrames(0) {
    }

    virtual ~FrameWaiter() {
        onFrameAvailable();
    }

    void waitForFrame() {
        Mutex::Autolock lock(mMutex);
        while (mPendingFrames == 0) {
            mCondition.wait(mMutex);
        }
        mPendingFrames--;
    }

    virtual void onFrameAvailable() {
        Mutex::Autolock lock(mMutex);
        mPendingFrames++;
        mCondition.signal();
    }

    int mPendingFrames;
    Mutex mMutex;
    Condition mCondition;
};

class GLSurface {
public:

    GLSurface():
            mEglDisplay(EGL_NO_DISPLAY),
            mEglSurface(EGL_NO_SURFACE),
            mEglContext(EGL_NO_CONTEXT) {
    }

    virtual ~GLSurface() {}

    void initialize(int display);
    void deinit();
    void loadShader(GLenum shaderType, const char* pSource, GLuint* outShader);
    void createProgram(const char* pVertexSource, const char* pFragmentSource,
            GLuint* outPgm);

private:
    EGLint const* getConfigAttribs();
    EGLint const* getContextAttribs();

protected:
    sp<SurfaceComposerClient> mComposerClient;
    sp<SurfaceControl> mSurfaceControl;

    EGLDisplay mEglDisplay;
    EGLSurface mEglSurface;
    EGLContext mEglContext;
    EGLConfig  mGlConfig;
};

class SurfaceTextureBase  {
public:
    virtual ~SurfaceTextureBase() {}

    void initialize(int tex_id, EGLenum tex_target = EGL_NONE);
    void deinit();
    void getId(const char **name);

    virtual sp<GLConsumer> getST();

protected:
    sp<GLConsumer> mST;
    sp<Surface> mSTC;
    sp<ANativeWindow> mANW;
    int mTexId;
};

class SurfaceTextureGL : public GLSurface, public SurfaceTextureBase {
public:
    virtual ~SurfaceTextureGL() {}

    void initialize(int display, int tex_id);
    void deinit();

    // drawTexture draws the GLConsumer over the entire GL viewport.
    void drawTexture();

private:
    GLuint mPgm;
    GLint mPositionHandle;
    GLint mTexSamplerHandle;
    GLint mTexMatrixHandle;
};

class ST_BufferSourceThread : public BufferSourceThread {
public:
    ST_BufferSourceThread(int tex_id, sp<Camera> camera) : BufferSourceThread(camera) {
        mSurfaceTextureBase = new SurfaceTextureBase();
        mSurfaceTextureBase->initialize(tex_id);
        mSurfaceTexture = mSurfaceTextureBase->getST();
        mSurfaceTexture->setSynchronousMode(true);
        mFW = new FrameWaiter();
        mSurfaceTexture->setFrameAvailableListener(mFW);
#ifndef ANDROID_API_JB_OR_LATER
        mCamera->setBufferSource(NULL, mSurfaceTexture);
#endif
    }
    virtual ~ST_BufferSourceThread() {
#ifndef ANDROID_API_JB_OR_LATER
        mCamera->releaseBufferSource(NULL, mSurfaceTexture);
#endif
        mSurfaceTextureBase->deinit();
        delete mSurfaceTextureBase;
    }

    virtual bool threadLoop() {
        sp<GraphicBuffer> graphic_buffer;

        mFW->waitForFrame();
        if (!mDestroying) {
            float mtx[16] = {0.0};
            mSurfaceTexture->updateTexImage();
            printf("=== Metadata for buffer %d ===\n", mCounter);
#ifndef ANDROID_API_JB_OR_LATER
            showMetadata(mSurfaceTexture->getMetadata());
#endif
            printf("\n");
            graphic_buffer = mSurfaceTexture->getCurrentBuffer();
            mSurfaceTexture->getTransformMatrix(mtx);
            Rect crop = getCrop(graphic_buffer, mtx);

            mDeferThread->add(graphic_buffer, crop, mCounter++);
            restartCapture();
            return true;
        }
        return false;
    }

    virtual void requestExit() {
        Thread::requestExit();

        mDestroying = true;
        mFW->onFrameAvailable();
    }

    virtual void setBuffer(android::ShotParameters &params) {
        {
            const char* id = NULL;

            mSurfaceTextureBase->getId(&id);

            if (id) {
                params.set(KEY_TAP_OUT_SURFACES, id);
            } else {
                params.remove(KEY_TAP_OUT_SURFACES);
            }
        }
    }

private:
    SurfaceTextureBase *mSurfaceTextureBase;
    sp<GLConsumer> mSurfaceTexture;
    sp<FrameWaiter> mFW;
};

class ST_BufferSourceInput : public BufferSourceInput {
public:
    ST_BufferSourceInput(int tex_id, sp<Camera> camera) :
                 BufferSourceInput(camera), mTexId(tex_id) {
        mSurfaceTexture = new SurfaceTextureBase();
        sp<GLConsumer> surface_texture;
        mSurfaceTexture->initialize(mTexId);
        surface_texture = mSurfaceTexture->getST();
        surface_texture->setSynchronousMode(true);

        mWindowTapIn = new Surface(surface_texture);
#ifndef ANDROID_API_JB_OR_LATER
        mCamera->setBufferSource(mSurfaceTexture->getST(), NULL);
#else
        mCamera->setBufferSource(mSurfaceTexture->getST()->getBufferQueue(), NULL);
#endif
    }
    virtual ~ST_BufferSourceInput() {
#ifndef ANDROID_API_JB_OR_LATER
        mCamera->releaseBufferSource(mSurfaceTexture->getST(), NULL);
#else
        mCamera->releaseBufferSource(mSurfaceTexture->getST()->getBufferQueue(), NULL);
#endif
        delete mSurfaceTexture;
    }

    virtual void setInput(buffer_info_t bufinfo, const char *format) {
        android::ShotParameters params;
        mSurfaceTexture->getST()->setDefaultBufferSize(bufinfo.width, bufinfo.height);
        BufferSourceInput::setInput(bufinfo, format, params);
    }

private:
    int mTexId;
    SurfaceTextureBase *mSurfaceTexture;
};

#endif
