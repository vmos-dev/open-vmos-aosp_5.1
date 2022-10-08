/*
 * Copyright (C) 2013 The Android Open Source Project
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

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include <system/graphics.h>

#include "util.h"

void matrix_init_ortho(GLfloat *m, float w, float h) {
	m[0] = 2.0 / w;
	m[1] = 0.0;
	m[2] = 0.0;
	m[3] = -1.0;
	m[4] = 0.0;
	m[5] = 2.0 / h;
	m[6] = 0.0;
	m[7] = -1.0;
	m[8] = 0.0;
	m[9] = 0.0;
	m[10] -1.0;
	m[11] = 0.0;
	m[12] = 0.0;
	m[13] = 0.0;
	m[14] = 0.0;
	m[15] = 1.0;
}

static GLuint load_shader(GLenum shaderType, const char *src) {
	GLint status = 0, len = 0;
	GLuint shader;

	if (!(shader = glCreateShader(shaderType)))
		return 0;

	glShaderSource(shader, 1, &src, NULL);
	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

	if (status)
		return shader;

	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
	if (len) {
		char *msg = malloc(len);
		if (msg) {
			glGetShaderInfoLog(shader, len, NULL, msg);
			msg[len-1] = 0;
			fprintf(stderr, "error compiling shader:\n%s\n", msg);
			free(msg);
		}
	}
	glDeleteShader(shader);
	return 0;
}

GLuint load_program(const char *vert_src, const char *frag_src) {
	GLuint vert, frag, prog;
	GLint status = 0, len = 0;

	if (!(vert = load_shader(GL_VERTEX_SHADER, vert_src)))
		return 0;
	if (!(frag = load_shader(GL_FRAGMENT_SHADER, frag_src)))
		goto fail_frag;
	if (!(prog = glCreateProgram()))
		goto fail_prog;

	glAttachShader(prog, vert);
	glAttachShader(prog, frag);
	glLinkProgram(prog);

	glGetProgramiv(prog, GL_LINK_STATUS, &status);
	if (status)
		return prog;

	glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
	if (len) {
		char *buf = (char*) malloc(len);
		if (buf) {
			glGetProgramInfoLog(prog, len, NULL, buf);
			buf[len-1] = 0;
			fprintf(stderr, "error linking program:\n%s\n", buf);
			free(buf);
		}
	}
	glDeleteProgram(prog);
fail_prog:
	glDeleteShader(frag);
fail_frag:
	glDeleteShader(vert);
	return 0;
}

int select_config_for_window(EGLDisplay dpy, EGLint *attr,
	unsigned format, EGLConfig *config) {
	EGLint R,G,B,A,r,g,b,a;
	EGLint i, n, max;
	EGLConfig *cfg;

	switch (format) {
	case HAL_PIXEL_FORMAT_RGBA_8888:
	case HAL_PIXEL_FORMAT_BGRA_8888:
		R = G = B = A = 8;
		break;
	case HAL_PIXEL_FORMAT_RGB_565:
		R = 5; G = 6; B = 5; A = 0;
		break;
	default:
		fprintf(stderr, "unknown fb pixel format %d\n", format);
		return -1;
	}

	if (eglGetConfigs(dpy, NULL, 0, &max) == EGL_FALSE) {
		fprintf(stderr, "no EGL configurations available?!\n");
		return -1;
	}

	cfg = (EGLConfig*) malloc(sizeof(EGLConfig) * max);
	if (!cfg)
		return -1;

	if (eglChooseConfig(dpy, attr, cfg, max, &n) == EGL_FALSE) {
		fprintf(stderr, "eglChooseConfig failed\n");
		return -1;
	}

	for (i = 0; i < n; i++) {
		EGLint r,g,b,a;
		eglGetConfigAttrib(dpy, cfg[i], EGL_RED_SIZE,   &r);
		eglGetConfigAttrib(dpy, cfg[i], EGL_GREEN_SIZE, &g);
		eglGetConfigAttrib(dpy, cfg[i], EGL_BLUE_SIZE,  &b);
		eglGetConfigAttrib(dpy, cfg[i], EGL_ALPHA_SIZE, &a);
		if (r == R && g == G && b == B && a == A) {
			*config = cfg[i];
			free(cfg);
			return 0;
		}
	}

	fprintf(stderr, "cannot find matching config\n");
	free(cfg);
	return -1;
}

static struct CNativeWindow *_cnw = 0;

int egl_create(EGLDisplay *_display, EGLSurface *_surface, int *_w, int *_h) {
	EGLBoolean res;
	EGLConfig config = { 0 };
	EGLint context_attrs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
	EGLint config_attrs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_NONE };
	EGLint major, minor;
	EGLContext context;
	EGLSurface surface;
	EGLint w, h;
	EGLDisplay display;
	EGLNativeWindowType window;
	unsigned width, height, format;
	struct CNativeWindow *cnw;

	display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if (display == EGL_NO_DISPLAY)
		return -1;

	if (!(res = eglInitialize(display, &major, &minor)))
		return -1;

	fprintf(stderr, "egl version: %d.%d\n", major, minor);

	if ((cnw = cnw_create()) == 0)
		return -1;

	cnw_info(cnw, &width, &height, &format);
	window = (EGLNativeWindowType) cnw;

	if ((res = select_config_for_window(display, config_attrs, format, &config)))
		goto fail;

	surface = eglCreateWindowSurface(display, config, window, NULL);
	if (surface == EGL_NO_SURFACE)
		goto fail;

	context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attrs);
	if (context == EGL_NO_CONTEXT)
		goto fail;

	if (!(res = eglMakeCurrent(display, surface, surface, context)))
		goto fail;

	eglQuerySurface(display, surface, EGL_WIDTH, &w);
	eglQuerySurface(display, surface, EGL_HEIGHT, &h);

	fprintf(stderr, "window: %d x %d\n", w, h);

	*_display = display;
	*_surface = surface;
	*_w = w;
	*_h = h;

	_cnw = cnw;
	return 0;

fail:
	cnw_destroy(cnw);
	return -1;
}

void egl_destroy(EGLDisplay display, EGLSurface surface) {
	if (_cnw) {
		eglDestroySurface(display, surface);
		eglTerminate(display);
		cnw_destroy(_cnw);
		_cnw = 0;
	}
}
