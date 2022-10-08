/*
 * Copyright (C) 2014 The Android Open Source Project
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

#ifndef AUDIO_EXTN_H
#define AUDIO_EXTN_H

#include <cutils/str_parms.h>

void *audio_extn_extspk_init(struct audio_device *adev);
void audio_extn_extspk_deinit(void *extn);
void audio_extn_extspk_update(void* extn);
void audio_extn_extspk_set_mode(void* extn, audio_mode_t mode);
void audio_extn_extspk_set_voice_vol(void* extn, float vol);

#ifndef HFP_ENABLED
#define audio_extn_hfp_is_active(adev)                  (0)
#define audio_extn_hfp_get_usecase()                    (-1)
#define audio_extn_hfp_set_parameters(adev, params)     (0)
#else
bool audio_extn_hfp_is_active(struct audio_device *adev);

audio_usecase_t audio_extn_hfp_get_usecase();

void audio_extn_hfp_set_parameters(struct audio_device *adev,
                                    struct str_parms *parms);
#endif

#endif /* AUDIO_EXTN_H */
