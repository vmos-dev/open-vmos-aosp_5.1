/*
 * Copyright (C) 2010 The Android Open Source Project
 * Copyright (c) 2011-2012, The Linux Foundation. All rights reserved.
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

#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <gl2ext.h>
#include <OpenGLRenderer.h>
#include "tilerenderer.h"

namespace android {
ANDROID_SINGLETON_STATIC_INSTANCE(uirenderer::TileRenderer) ;
namespace uirenderer {

TileRenderer::TileRenderer() {
    mIsTiled = false;
}

TileRenderer::~TileRenderer() {
}

void TileRenderer::startTileRendering(OpenGLRenderer* renderer,
                                      int left, int top,
                                      int right, int bottom) {
    int width = 0;
    int height = 0;
    GLenum status = GL_NO_ERROR;

    if (renderer != NULL) {
        renderer->getViewport(width, height);
    }

    if (!left && !right && !top && !bottom) {
        left = 0;
        top = 0;
        right = width;
        bottom = height;
    }

    if (!left && !right && !top && !bottom) {
        //can't do tile rendering
        ALOGE("can't tile render; drity region, width, height not available");
        return;
    }

    int l = left, t = (height - bottom), w = (right - left), h = (bottom - top), preserve = 0;

    if (l < 0 || t < 0) {
        l = (l < 0) ? 0 : l;
        t = (t < 0) ? 0 : t;
        preserve = 1;
    }

    if (w > width || h > height) {
        w = (w > width) ? width : w;
        h = (h > height) ? height : h;
        preserve = 1;
    }

    //clear off all errors before tiling, if any
    while ((status = glGetError()) != GL_NO_ERROR);

    if (preserve)
        glStartTilingQCOM(l, t, w, h, GL_COLOR_BUFFER_BIT0_QCOM);
    else
        glStartTilingQCOM(l, t, w, h, GL_NONE);

    status = glGetError();
    if (status == GL_NO_ERROR)
        mIsTiled = true;
}

void TileRenderer::endTileRendering(OpenGLRenderer*) {
    if (!mIsTiled) {
        return;
    }
    glEndTilingQCOM(GL_COLOR_BUFFER_BIT0_QCOM);
    mIsTiled = false;
    GLenum status = GL_NO_ERROR;
    while ((status = glGetError()) != GL_NO_ERROR);
}

}; // namespace uirenderer
}; // namespace android
