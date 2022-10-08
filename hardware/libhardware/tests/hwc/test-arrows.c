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

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include "util.h"

static const char gVertexShader[] =
	"attribute vec4 aPosition;\n"
	"uniform mat4 uTransform;\n"
	"varying vec4 vTexCoord;\n"
	"void main() {\n"
	"  gl_Position = aPosition * uTransform;\n"
	"  vTexCoord = aPosition * vec4(1.0/16.0,-1.0/16.0,0.0,0.0);\n"
	"}\n";

static const char gFragmentShader[] =
	"precision mediump float;\n"
	"uniform sampler2D uTexture;\n"
	"uniform float uAnim;\n"
	"varying vec4 vTexCoord;\n"
	"void main() {\n"
	"  vec2 tc = vec2(vTexCoord.x, uAnim + vTexCoord.y);\n"
	"  gl_FragColor = texture2D(uTexture, tc);\n"
	"}\n";

static GLuint pgm;
static GLint aPosition, uTransform, uTexture, uAnim;

static GLfloat vtx[2 * 3 * 2];
static GLfloat mtx[16];

//#define R (0xFF0000FF)
#define R (0xFF000000)
#define G (0xFF00FF00)
uint32_t t32[] = {
	R, R, R, R, R, R, R, G, G, R, R, R, R, R, R, R,
	R, R, R, R, R, R, G, G, G, G, R, R, R, R, R, R,
	R, R, R, R, R, G, G, G, G, G, G, R, R, R, R, R,
	R, R, R, R, G, G, G, G, G, G, G, G, R, R, R, R,
	R, R, R, G, G, G, G, G, G, G, G, G, G, R, R, R,
	R, R, G, G, G, G, G, G, G, G, G, G, G, G, R, R,
	R, R, G, G, G, G, G, G, G, G, G, G, G, G, R, R,
	R, R, R, R, R, R, G, G, G, G, R, R, R, R, R, R,
	R, R, R, R, R, R, G, G, G, G, R, R, R, R, R, R,
	R, R, R, R, R, R, G, G, G, G, R, R, R, R, R, R,
	R, R, R, R, R, R, G, G, G, G, R, R, R, R, R, R,
	R, R, R, R, R, R, G, G, G, G, R, R, R, R, R, R,
	R, R, R, R, R, R, G, G, G, G, R, R, R, R, R, R,
	R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R,
	R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R,
	R, R, R, R, R, R, R, R, R, R, R, R, R, R, R, R,
};
#undef R
#undef G

int prepare(int w, int h) {
	GLuint texid;

	int left = w / 4;
	int top = h / 4;
	int right = (w / 4) * 3;
	int bottom = (h / 4) * 3;

	vtx[0] = left;
	vtx[1] = top;
	vtx[2] = left;
	vtx[3] = bottom;
	vtx[4] = right;
	vtx[5] = bottom;

	vtx[6] = right;
	vtx[7] = bottom;
	vtx[8] = right;
	vtx[9] = top;
	vtx[10] = left;
	vtx[11] = top;

	matrix_init_ortho(mtx, w, h);

	pgm = load_program(gVertexShader, gFragmentShader);
	if (!pgm)
		return -1;

	aPosition = glGetAttribLocation(pgm, "aPosition");
	uTexture = glGetUniformLocation(pgm, "uTexture");
	uTransform = glGetUniformLocation(pgm, "uTransform");
	uAnim = glGetUniformLocation(pgm, "uAnim");

	glViewport(0, 0, w, h);

	glGenTextures(1, &texid);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texid);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glEnable(GL_TEXTURE_2D);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 16, 16, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, t32);

	return 0;
}

static float anim = 0.0;

void render() {
	anim += 0.1;
	if (anim >= 16.0) anim = 0.0;

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glUseProgram(pgm);
	glUniform1i(uTexture, 0);
	glUniform1f(uAnim, anim);
	glUniformMatrix4fv(uTransform, 1, 0, mtx);
	glVertexAttribPointer(aPosition, 2, GL_FLOAT, GL_FALSE, 0, vtx);
	glEnableVertexAttribArray(aPosition);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

int main(int argc, char **argv) {
	EGLDisplay display;
	EGLSurface surface;
	int w, h, count = 0;

	if (argc > 1)
		count = atoi(argv[1]);

	if (egl_create(&display, &surface, &w, &h))
		return -1;

	if (prepare(w, h))
		return -1;

	for (;;) {
		render();
		eglSwapBuffers(display, surface);
		if (count > 0)
			if (--count == 0)
				break;
	}

	egl_destroy(display, surface);
	return 0;
}
