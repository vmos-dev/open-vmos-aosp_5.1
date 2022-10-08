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

#ifndef AUDIO_ROUTE_H
#define AUDIO_ROUTE_H

#if defined(__cplusplus)
extern "C" {
#endif

/* Initialize and free the audio routes */
struct audio_route *audio_route_init(unsigned int card, const char *xml_path);
void audio_route_free(struct audio_route *ar);

/* Apply an audio route path by name */
int audio_route_apply_path(struct audio_route *ar, const char *name);

/* Apply and update mixer with audio route path by name */
int audio_route_apply_and_update_path(struct audio_route *ar, const char *name);

/* Reset an audio route path by name */
int audio_route_reset_path(struct audio_route *ar, const char *name);

/* Reset and update mixer with audio route path by name */
int audio_route_reset_and_update_path(struct audio_route *ar, const char *name);

/* Reset the audio routes back to the initial state */
void audio_route_reset(struct audio_route *ar);

/* Update the mixer with any changed values */
int audio_route_update_mixer(struct audio_route *ar);

#if defined(__cplusplus)
}  /* extern "C" */
#endif

#endif
