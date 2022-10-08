/*
* Copyright (C) 2011 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#ifndef _OPENGL_RENDERER_RENDER_API_H
#define _OPENGL_RENDERER_RENDER_API_H

/* This header and its declarations must be usable from C code.
 *
 * If RENDER_API_NO_PROTOTYPES is #defined before including this header, only
 * the interface function pointer types will be declared, not the prototypes.
 * This allows the client to use those names for its function pointer variables.
 *
 * All interfaces which can fail return an int, with zero indicating failure
 * and anything else indicating success.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include "render_api_platform_types.h"

#if defined(RENDER_API_NO_PROTOTYPES)
#define DECL(ret, name, args) \
	typedef ret (* name##Fn) args
#else
#define DECL(ret, name, args) \
	typedef ret (* name##Fn) args ; \
	ret name args
#endif

/* initLibrary - initialize the library and tries to load the corresponding
 *     GLES translator libraries. This function must be called before anything
 *     else to ensure that everything works. If it returns an error, then
 *     you cannot use the library at all (this can happen under certain
 *     environments where the desktop GL libraries are not available)
 */
DECL(int, initLibrary, (void));

/* list of constants to be passed to setStreamMode */
#define STREAM_MODE_DEFAULT   0
#define STREAM_MODE_TCP       1
#define STREAM_MODE_UNIX      2
#define STREAM_MODE_PIPE      3

/* Change the stream mode. This must be called before initOpenGLRenderer */
DECL(int, setStreamMode, (int mode));

/* initOpenGLRenderer - initialize the OpenGL renderer process.
 *
 * width and height are the framebuffer dimensions that will be reported to the
 * guest display driver.
 *
 * addr is a buffer of addrLen bytes that will receive the address that clients
 * should connect to. The interpretation depends on the transport:
 *   - TCP: The buffer contains the port number as a string. The server is
 *     listening only on the loopback address.
 *   - Win32 and UNIX named pipes: The buffer contains the full path clients
 *     should connect to.
 *
 * This function is *NOT* thread safe and should be called first
 * to initialize the renderer after initLibrary().
 */
DECL(int, initOpenGLRenderer, (int width, int height, char* addr, size_t addrLen));

/* getHardwareStrings - describe the GPU hardware and driver.
 *    The underlying GL's vendor/renderer/version strings are returned to the
 *    caller. The pointers become invalid after a call to stopOpenGLRenderer().
 */
DECL(void, getHardwareStrings, (const char** vendor, const char** renderer,
		const char** version));

/* A per-frame callback can be registered with setPostCallback(); to remove it
 * pass NULL for both parameters. While a callback is registered, the renderer
 * will call it just before each new frame is displayed, providing a copy of
 * the framebuffer contents.
 *
 * The callback will be called from one of the renderer's threads, so will
 * probably need synchronization on any data structures it modifies. The
 * pixels buffer may be overwritten as soon as the callback returns; if it
 * needs the pixels afterwards it must copy them.
 *
 * The pixels buffer is intentionally not const: the callback may modify the
 * data without copying to another buffer if it wants, e.g. in-place RGBA to
 * RGB conversion, or in-place y-inversion.
 *
 * Parameters are:
 *   context        The pointer optionally provided when the callback was
 *                  registered. The client can use this to pass whatever
 *                  information it wants to the callback.
 *   width, height  Dimensions of the image, in pixels. Rows are tightly
 *                  packed; there is no inter-row padding.
 *   ydir           Indicates row order: 1 means top-to-bottom order, -1 means
 *                  bottom-to-top order.
 *   format, type   Format and type GL enums, as used in glTexImage2D() or
 *                  glReadPixels(), describing the pixel format.
 *   pixels         The framebuffer image.
 *
 * In the first implementation, ydir is always -1 (bottom to top), format and
 * type are always GL_RGBA and GL_UNSIGNED_BYTE, and the width and height will
 * always be the same as the ones passed to initOpenGLRenderer().
 */
typedef void (*OnPostFn)(void* context, int width, int height, int ydir,
                         int format, int type, unsigned char* pixels);
DECL(void, setPostCallback, (OnPostFn onPost, void* onPostContext));

/* createOpenGLSubwindow -
 *     Create a native subwindow which is a child of 'window'
 *     to be used for framebuffer display.
 *     Framebuffer will not get displayed if a subwindow is not
 *     created.
 *     x,y,width,height are the dimensions of the rendering subwindow.
 *     zRot is the rotation to apply on the framebuffer display image.
 */
DECL(int, createOpenGLSubwindow, (FBNativeWindowType window,
		int x, int y, int width, int height, float zRot));

/* destroyOpenGLSubwindow -
 *   destroys the created native subwindow. Once destroyed,
 *   Framebuffer content will not be visible until a new
 *   subwindow will be created.
 */
DECL(int, destroyOpenGLSubwindow, (void));

/* setOpenGLDisplayRotation -
 *    set the framebuffer display image rotation in units
 *    of degrees around the z axis
 */
DECL(void, setOpenGLDisplayRotation, (float zRot));

/* repaintOpenGLDisplay -
 *    causes the OpenGL subwindow to get repainted with the
 *    latest framebuffer content.
 */
DECL(void, repaintOpenGLDisplay, (void));

/* stopOpenGLRenderer - stops the OpenGL renderer process.
 *     This functions is *NOT* thread safe and should be called
 *     only if previous initOpenGLRenderer has returned true.
 */
DECL(int, stopOpenGLRenderer, (void));

#ifdef __cplusplus
}
#endif

#endif
