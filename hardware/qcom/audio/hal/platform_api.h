/*
 * Copyright (C) 2013-2014 The Android Open Source Project
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

#ifndef AUDIO_PLATFORM_API_H
#define AUDIO_PLATFORM_API_H

void *platform_init(struct audio_device *adev);
void platform_deinit(void *platform);
const char *platform_get_snd_device_name(snd_device_t snd_device);
void platform_add_backend_name(void *platform, char *mixer_path,
                                                    snd_device_t snd_device);
int platform_get_pcm_device_id(audio_usecase_t usecase, int device_type);
int platform_get_snd_device_index(char *snd_device_index_name);
int platform_set_snd_device_acdb_id(snd_device_t snd_device, unsigned int acdb_id);
int platform_send_audio_calibration(void *platform, snd_device_t snd_device);
int platform_switch_voice_call_device_pre(void *platform);
int platform_switch_voice_call_enable_device_config(void *platform,
                                                    snd_device_t out_snd_device,
                                                    snd_device_t in_snd_device);
int platform_switch_voice_call_device_post(void *platform,
                                           snd_device_t out_snd_device,
                                           snd_device_t in_snd_device);
int platform_switch_voice_call_usecase_route_post(void *platform,
                                                  snd_device_t out_snd_device,
                                                  snd_device_t in_snd_device);
int platform_start_voice_call(void *platform, uint32_t vsid);
int platform_stop_voice_call(void *platform, uint32_t vsid);
int platform_set_voice_volume(void *platform, int volume);
int platform_set_mic_mute(void *platform, bool state);
int platform_get_sample_rate(void *platform, uint32_t *rate);
int platform_set_device_mute(void *platform, bool state, char *dir);
snd_device_t platform_get_output_snd_device(void *platform, audio_devices_t devices);
snd_device_t platform_get_input_snd_device(void *platform, audio_devices_t out_device);
int platform_set_hdmi_channels(void *platform, int channel_count);
int platform_edid_get_max_channels(void *platform);

/* returns the latency for a usecase in Us */
int64_t platform_render_latency(audio_usecase_t usecase);

int platform_set_incall_recording_session_id(void *platform,
                                             uint32_t session_id, int rec_mode);
int platform_stop_incall_recording_usecase(void *platform);
int platform_start_incall_music_usecase(void *platform);
int platform_stop_incall_music_usecase(void *platform);

int platform_set_snd_device_backend(snd_device_t snd_device, const char * backend);

/* From platform_info_parser.c */
int platform_info_init(void);

int platform_get_usecase_index(const char * usecase);
int platform_set_usecase_pcm_id(audio_usecase_t usecase, int32_t type, int32_t pcm_id);
void platform_set_echo_reference(struct audio_device *adev, bool enable, audio_devices_t out_device);

#endif // AUDIO_PLATFORM_API_H
