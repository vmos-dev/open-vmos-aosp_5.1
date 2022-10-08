/*
 * Copyright (C) 2007 The Android Open Source Project
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

#ifndef _MINUI_H_
#define _MINUI_H_

int gr_init(void);
void gr_exit(void);

int gr_fb_width(void);
int gr_fb_height(void);
void gr_flip(void);

void gr_color(unsigned char r, unsigned char g, unsigned char b);
void gr_fill(int x, int y, int w, int h);
int gr_text(int x, int y, const char *s);
int gr_measure(const char *s);


typedef struct event event;

struct event
{
    unsigned type;
    unsigned code;
    unsigned value;
};
    
int ev_init(void);
void ev_exit(void);

int ev_get(event *ev, unsigned dont_wait);

#define TYPE_KEY 1

#define KEY_UP      103
#define KEY_DOWN    108
#define KEY_LEFT    105
#define KEY_RIGHT   106
#define KEY_CENTER  232
#define KEY_ENTER   28

#endif
