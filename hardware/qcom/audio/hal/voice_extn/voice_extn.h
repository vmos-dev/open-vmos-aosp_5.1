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

#ifndef VOICE_EXTN_H
#define VOICE_EXTN_H

#ifdef MULTI_VOICE_SESSION_ENABLED
int voice_extn_start_call(struct audio_device *adev);
int voice_extn_stop_call(struct audio_device *adev);
int voice_extn_get_session_from_use_case(struct audio_device *adev,
                                         const audio_usecase_t usecase_id,
                                         struct voice_session **session);
void voice_extn_init(struct audio_device *adev);
int voice_extn_set_parameters(struct audio_device *adev,
                              struct str_parms *parms);
void voice_extn_get_parameters(const struct audio_device *adev,
                               struct str_parms *query,
                               struct str_parms *reply);
int voice_extn_is_in_call_rec_stream(struct stream_in *in, bool *in_call_rec);
int voice_extn_get_active_session_id(struct audio_device *adev,
                                     uint32_t *session_id);
int voice_extn_is_call_state_active(struct audio_device *adev,
                                    bool *is_call_active);
#else
static int voice_extn_start_call(struct audio_device *adev __unused)
{
    return -ENOSYS;
}

static int voice_extn_stop_call(struct audio_device *adev __unused)
{
    return -ENOSYS;
}

static int voice_extn_get_session_from_use_case(struct audio_device *adev __unused,
                                                const audio_usecase_t usecase_id __unused,
                                                struct voice_session **session __unused)
{
    return -ENOSYS;
}

static void voice_extn_init(struct audio_device *adev __unused)
{
}

static int voice_extn_set_parameters(struct audio_device *adev __unused,
                                     struct str_parms *parms __unused)
{
    return -ENOSYS;
}

static void voice_extn_get_parameters(const struct audio_device *adev __unused,
                                      struct str_parms *query __unused,
                                      struct str_parms *reply __unused)
{
}

static int voice_extn_is_call_state_active(struct audio_device *adev __unused,
                                           bool *is_call_active __unused)
{
    return -ENOSYS;
}

static int voice_extn_is_in_call_rec_stream(struct stream_in *in __unused, bool *in_call_rec __unused)
{
    return -ENOSYS;
}

static int voice_extn_get_active_session_id(struct audio_device *adev __unused,
                                            uint32_t *session_id __unused)
{
    return -ENOSYS;
}

#endif

#ifdef INCALL_MUSIC_ENABLED
int voice_extn_check_and_set_incall_music_usecase(struct audio_device *adev,
                                                  struct stream_out *out);
#else
static int voice_extn_check_and_set_incall_music_usecase(struct audio_device *adev __unused,
                                                         struct stream_out *out __unused)
{
    return -ENOSYS;
}
#endif

#endif //VOICE_EXTN_H
