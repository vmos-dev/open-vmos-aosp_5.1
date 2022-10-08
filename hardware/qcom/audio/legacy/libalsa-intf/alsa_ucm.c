/*
 * Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Code Aurora Forum, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#define LOG_TAG "alsa_ucm"
//#define LOG_NDDEBUG 0

#ifdef ANDROID
/* definitions for Android logging */
#include <utils/Log.h>
#include <cutils/properties.h>
#else /* ANDROID */
#include <math.h>
#define strlcat g_strlcat
#define strlcpy g_strlcpy
#define ALOGI(...)      fprintf(stdout, __VA_ARGS__)
#define ALOGE(...)      fprintf(stderr, __VA_ARGS__)
#define ALOGV(...)      fprintf(stderr, __VA_ARGS__)
#define ALOGD(...)      fprintf(stderr, __VA_ARGS__)
#endif /* ANDROID */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <stdint.h>
#include <dlfcn.h>

#include <linux/ioctl.h>
#include "msm8960_use_cases.h"
#if defined(QC_PROP)
    static void (*acdb_send_audio_cal)(int,int);
    static void (*acdb_send_voice_cal)(int,int);
#endif
#define PARSE_DEBUG 0

/**
 * Create an identifier
 * fmt - sprintf like format,
 * ... - Optional arguments
 * returns - string allocated or NULL on error
 */
char *snd_use_case_identifier(const char *fmt, ...)
{
    ALOGE("API not implemented for now, to be updated if required");
    return NULL;
}

/**
 * Free a list
 * list - list to free
 * items -  Count of strings
 * Return Zero on success, otherwise a negative error code
 */
int snd_use_case_free_list(const char *list[], int items)
{
    /* list points to UCM internal static tables,
     * hence there is no need to do a free call
     * just set the list to NULL and return */
    list = NULL;
    return 0;
}

/**
 * Obtain a list of entries
 * uc_mgr - UCM structure pointer or  NULL for card list
 * identifier - NULL for card list
 * list - Returns allocated list
 * returns Number of list entries on success, otherwise a negative error code
 */
int snd_use_case_get_list(snd_use_case_mgr_t *uc_mgr,
                          const char *identifier,
                          const char **list[])
{
    use_case_verb_t *verb_list;
    int verb_index, list_size, index = 0;

    if (identifier == NULL) {
        *list = card_list;
        return ((int)MAX_NUM_CARDS);
    }

    pthread_mutex_lock(&uc_mgr->card_ctxt_ptr->card_lock);
    if ((uc_mgr->snd_card_index >= (int)MAX_NUM_CARDS) ||
        (uc_mgr->snd_card_index < 0) || (uc_mgr->card_ctxt_ptr == NULL)) {
        ALOGE("snd_use_case_get_list(): failed, invalid arguments");
        pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
        return -EINVAL;
    }

    if (!strncmp(identifier, "_verbs", 6)) {
        while(strncmp(uc_mgr->card_ctxt_ptr->verb_list[index],
            SND_UCM_END_OF_LIST, strlen(SND_UCM_END_OF_LIST))) {
            ALOGV("Index:%d Verb:%s", index,
                 uc_mgr->card_ctxt_ptr->verb_list[index]);
            index++;
        }
        *list = (char ***)uc_mgr->card_ctxt_ptr->verb_list;
        pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
        return index;
    } else  if (!strncmp(identifier, "_devices", 8)) {
        if (!strncmp(uc_mgr->card_ctxt_ptr->current_verb,
            SND_USE_CASE_VERB_INACTIVE, strlen(SND_USE_CASE_VERB_INACTIVE))) {
            ALOGE("Use case verb name not set, invalid current verb");
            pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
            return -EINVAL;
        }
        verb_list = uc_mgr->card_ctxt_ptr->use_case_verb_list;
        while(strncmp(uc_mgr->card_ctxt_ptr->current_verb,
            verb_list[index].use_case_name,
            (strlen(verb_list[index].use_case_name)+1))) {
            index++;
        }
        verb_index = index;
        index = 0;
        while(strncmp(verb_list[verb_index].device_list[index],
            SND_UCM_END_OF_LIST, strlen(SND_UCM_END_OF_LIST))) {
            ALOGV("Index:%d Device:%s", index,
                 verb_list[verb_index].device_list[index]);
            index++;
        }
        *list = verb_list[verb_index].device_list;
        pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
        return index;
    } else  if (!strncmp(identifier, "_modifiers", 10)) {
        if (!strncmp(uc_mgr->card_ctxt_ptr->current_verb,
                    SND_USE_CASE_VERB_INACTIVE, MAX_STR_LEN)) {
            ALOGE("Use case verb name not set, invalid current verb");
            pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
            return -EINVAL;
        }
        verb_list = uc_mgr->card_ctxt_ptr->use_case_verb_list;
        while(strncmp(uc_mgr->card_ctxt_ptr->current_verb,
            verb_list[index].use_case_name,
            (strlen(verb_list[index].use_case_name)+1))) {
            index++;
        }
        verb_index = index;
        index = 0;
        while(strncmp(verb_list[verb_index].modifier_list[index],
                    SND_UCM_END_OF_LIST, strlen(SND_UCM_END_OF_LIST))) {
            ALOGV("Index:%d Modifier:%s", index,
                 verb_list[verb_index].modifier_list[index]);
            index++;
        }
        *list = verb_list[verb_index].modifier_list;
        pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
        return index;
    } else  if (!strncmp(identifier, "_enadevs", 8)) {
        if (uc_mgr->device_list_count) {
            for (index = 0; index < uc_mgr->device_list_count; index++) {
                free(uc_mgr->current_device_list[index]);
                uc_mgr->current_device_list[index] = NULL;
            }
            free(uc_mgr->current_device_list);
            uc_mgr->current_device_list = NULL;
            uc_mgr->device_list_count = 0;
        }
        list_size =
            snd_ucm_get_size_of_list(uc_mgr->card_ctxt_ptr->dev_list_head);
        uc_mgr->device_list_count = list_size;
    if (list_size > 0) {
            uc_mgr->current_device_list =
                (char **)malloc(sizeof(char *)*list_size);
            if (uc_mgr->current_device_list == NULL) {
                *list = NULL;
                pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
                return -ENOMEM;
            }
            for (index = 0; index < list_size; index++) {
                uc_mgr->current_device_list[index] =
                snd_ucm_get_value_at_index(uc_mgr->card_ctxt_ptr->dev_list_head,
                index);
            }
        }
        *list = (const char **)uc_mgr->current_device_list;
        pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
        return (list_size);
    } else  if (!strncmp(identifier, "_enamods", 8)) {
        if (uc_mgr->modifier_list_count) {
            for (index = 0; index < uc_mgr->modifier_list_count; index++) {
                free(uc_mgr->current_modifier_list[index]);
                uc_mgr->current_modifier_list[index] = NULL;
            }
            free(uc_mgr->current_modifier_list);
            uc_mgr->current_modifier_list = NULL;
            uc_mgr->modifier_list_count = 0;
        }
        list_size =
            snd_ucm_get_size_of_list(uc_mgr->card_ctxt_ptr->mod_list_head);
        uc_mgr->modifier_list_count = list_size;
    if (list_size > 0) {
            uc_mgr->current_modifier_list =
                (char **)malloc(sizeof(char *) * list_size);
            if (uc_mgr->current_modifier_list == NULL) {
                *list = NULL;
                pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
                return -ENOMEM;
            }
            for (index = 0; index < list_size; index++) {
                uc_mgr->current_modifier_list[index] =
                snd_ucm_get_value_at_index(uc_mgr->card_ctxt_ptr->mod_list_head,
                index);
            }
        }
        *list = (const char **)uc_mgr->current_modifier_list;
        pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
        return (list_size);
    } else {
        ALOGE("Invalid identifier: %s", identifier);
        pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
        return -EINVAL;
    }
}


/**
 * Get current value of the identifier
 * identifier - NULL for current card
 *        _verb
 *        <Name>/<_device/_modifier>
 *    Name -    PlaybackPCM
 *        CapturePCM
 *        PlaybackCTL
 *        CaptureCTL
 * value - Value pointer
 * returns Zero if success, otherwise a negative error code
 */
int snd_use_case_get(snd_use_case_mgr_t *uc_mgr,
                     const char *identifier,
                     const char **value)
{
    card_mctrl_t *ctrl_list;
    use_case_verb_t *verb_list;
    char ident[MAX_STR_LEN], *ident1, *ident2, *temp_ptr;
    int index, verb_index = 0, ret = 0;

    pthread_mutex_lock(&uc_mgr->card_ctxt_ptr->card_lock);
    if ((uc_mgr->snd_card_index >= (int)MAX_NUM_CARDS) ||
        (uc_mgr->snd_card_index < 0) || (uc_mgr->card_ctxt_ptr == NULL)) {
        ALOGE("snd_use_case_get(): failed, invalid arguments");
        pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
        return -EINVAL;
    }

    if (identifier == NULL) {
        if (uc_mgr->card_ctxt_ptr->card_name != NULL) {
            *value = strdup(uc_mgr->card_ctxt_ptr->card_name);
        } else {
            *value = NULL;
        }
        pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
        return 0;
    }

    if (!strncmp(identifier, "_verb", 5)) {
        if (uc_mgr->card_ctxt_ptr->current_verb != NULL) {
            *value = strdup(uc_mgr->card_ctxt_ptr->current_verb);
        } else {
            *value = NULL;
        }
        pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
        return 0;
    }

    strlcpy(ident, identifier, sizeof(ident));
    if(!(ident1 = strtok_r(ident, "/", &temp_ptr))) {
        ALOGE("No valid identifier found: %s", ident);
        ret = -EINVAL;
    } else {
        if ((!strncmp(ident1, "PlaybackPCM", 11)) ||
            (!strncmp(ident1, "CapturePCM", 10))) {
            ident2 = strtok_r(NULL, "/", &temp_ptr);
            index = 0;
            if (ident2 != NULL) {
                verb_index = uc_mgr->card_ctxt_ptr->current_verb_index;
                verb_list = uc_mgr->card_ctxt_ptr->use_case_verb_list;
                if((get_usecase_type(uc_mgr, ident2)) == CTRL_LIST_VERB) {
                    ctrl_list = verb_list[verb_index].verb_ctrls;
                } else {
                    ctrl_list = verb_list[verb_index].mod_ctrls;
                }
                if((verb_index < 0) ||
                    (!strncmp(uc_mgr->card_ctxt_ptr->current_verb,
                    SND_UCM_END_OF_LIST, 3)) || (ctrl_list == NULL)) {
                    ALOGE("Invalid current verb value: %s - %d",
                         uc_mgr->card_ctxt_ptr->current_verb, verb_index);
                    pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
                    return -EINVAL;
                }
                while(strncmp(ctrl_list[index].case_name, ident2,
                    (strlen(ident2)+1))) {
                    if (!strncmp(ctrl_list[index].case_name,
                        SND_UCM_END_OF_LIST, strlen(SND_UCM_END_OF_LIST))){
                        *value = NULL;
                        ret = -EINVAL;
                        break;
                    } else {
                        index++;
                    }
                }
            } else {
                ret = -EINVAL;
            }
            if (ret < 0) {
                ALOGE("No valid device/modifier found with given identifier: %s",
                     ident2);
            } else {
                if(!strncmp(ident1, "PlaybackPCM", 11)) {
                    if (ctrl_list[index].playback_dev_name) {
                        *value = strdup(ctrl_list[index].playback_dev_name);
                    } else {
                        *value = NULL;
                        ret = -ENODEV;
                    }
                } else if(!strncmp(ident1, "CapturePCM", 10)) {
                    if (ctrl_list[index].capture_dev_name) {
                        *value = strdup(ctrl_list[index].capture_dev_name);
                    } else {
                        *value = NULL;
                        ret = -ENODEV;
                    }
                } else {
                    ALOGE("No valid device name exists for given identifier: %s",
                         ident2);
                    *value = NULL;
                    ret = -ENODEV;
                }
            }
        } else if ((!strncmp(ident1, "PlaybackCTL", 11)) ||
                   (!strncmp(ident1, "CaptureCTL", 10))) {
            if(uc_mgr->card_ctxt_ptr->control_device != NULL) {
                *value = strdup(uc_mgr->card_ctxt_ptr->control_device);
            } else {
                ALOGE("No valid control device found");
                *value = NULL;
                ret = -ENODEV;
            }
        } else if (!strncmp(ident1, "ACDBID", 11)) {
            ident2 = strtok_r(NULL, "/", &temp_ptr);
            index = 0; verb_index = 0;
            verb_list = uc_mgr->card_ctxt_ptr->use_case_verb_list;
            if((verb_index < 0) ||
               (!strncmp(uc_mgr->card_ctxt_ptr->current_verb,
                SND_UCM_END_OF_LIST, 3)) ||
                (verb_list[verb_index].verb_ctrls == NULL)) {
                ALOGE("Invalid current verb value: %s - %d",
                     uc_mgr->card_ctxt_ptr->current_verb, verb_index);
                pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
                return -EINVAL;
            }
            ctrl_list = verb_list[verb_index].device_ctrls;
            if (ident2 != NULL) {
                while(strncmp(ctrl_list[index].case_name, ident2,
                    MAX_LEN(ctrl_list[index].case_name,ident2))) {
                    if (!strncmp(ctrl_list[index].case_name, SND_UCM_END_OF_LIST,
                        strlen(SND_UCM_END_OF_LIST))){
                        ret = -EINVAL;
                        break;
                    } else {
                        index++;
                    }
                }
            }
            if (ret < 0) {
                ALOGE("No valid device/modifier found with given identifier: %s",
                      ident2);
            } else {
                if (verb_list[verb_index].device_ctrls[index].acdb_id) {
                    ret = verb_list[verb_index].device_ctrls[index].acdb_id;
                } else {
                    ret = -ENODEV;
                }
            }
        } else if (!strncmp(ident1, "EffectsMixerCTL", 11)) {
            ident2 = strtok_r(NULL, "/", &temp_ptr);
            index = 0; verb_index = 0;
            verb_list = uc_mgr->card_ctxt_ptr->use_case_verb_list;
            if((verb_index < 0) ||
               (!strncmp(uc_mgr->card_ctxt_ptr->current_verb,
                SND_UCM_END_OF_LIST, 3)) ||
                (verb_list[verb_index].verb_ctrls == NULL)) {
                ALOGE("Invalid current verb value: %s - %d",
                     uc_mgr->card_ctxt_ptr->current_verb, verb_index);
                pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
                return -EINVAL;
            }
            ctrl_list = verb_list[verb_index].device_ctrls;
            if (ident2 != NULL) {
                while(strncmp(ctrl_list[index].case_name, ident2, strlen(ident2)+1)) {
                    if (!strncmp(ctrl_list[index].case_name, SND_UCM_END_OF_LIST,
                        strlen(SND_UCM_END_OF_LIST))){
                        ret = -EINVAL;
                        break;
                    } else {
                        index++;
                    }
                }
            }
            if (ret < 0) {
                ALOGE("No valid device/modifier found with given identifier: %s",
                      ident2);
            } else {
                if (verb_list[verb_index].device_ctrls[index].effects_mixer_ctl) {
                    *value = strdup(verb_list[verb_index].device_ctrls[index].effects_mixer_ctl);
                } else {
                    *value = NULL;
                    ret = -ENODEV;
                }
            }
        } else {
            ALOGE("Unsupported identifier value: %s", ident1);
            *value = NULL;
            ret = -EINVAL;
        }
    }
    pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
    return ret;
}

/**
 * Get current status
 * uc_mgr - UCM structure
 * identifier - _devstatus/<device>,
        _modstatus/<modifier>
 * value - result
 * returns 0 on success, otherwise a negative error code
 */
int snd_use_case_geti(snd_use_case_mgr_t *uc_mgr,
              const char *identifier,
              long *value)
{
    char ident[MAX_STR_LEN], *ident1, *ident2, *ident_value, *temp_ptr;
    int index, list_size, ret = -EINVAL;

    pthread_mutex_lock(&uc_mgr->card_ctxt_ptr->card_lock);
    if ((uc_mgr->snd_card_index >= (int)MAX_NUM_CARDS) ||
        (uc_mgr->snd_card_index < 0) || (uc_mgr->card_ctxt_ptr == NULL)) {
        ALOGE("snd_use_case_geti(): failed, invalid arguments");
        pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
        return -EINVAL;
    }

    *value = 0;
    strlcpy(ident, identifier, sizeof(ident));
    if(!(ident1 = strtok_r(ident, "/", &temp_ptr))) {
        ALOGE("No valid identifier found: %s", ident);
        ret = -EINVAL;
    } else {
        if (!strncmp(ident1, "_devstatus", 10)) {
            ident2 = strtok_r(NULL, "/", &temp_ptr);
            if (ident2 != NULL) {
                list_size =
                snd_ucm_get_size_of_list(uc_mgr->card_ctxt_ptr->dev_list_head);
                for (index = 0; index < list_size; index++) {
                if ((ident_value =
                snd_ucm_get_value_at_index(uc_mgr->card_ctxt_ptr->dev_list_head,
                index))) {
                        if (!strncmp(ident2, ident_value,
                            (strlen(ident_value)+1))) {
                            *value = 1;
                            free(ident_value);
                            ident_value = NULL;
                            break;
                        } else {
                            free(ident_value);
                            ident_value = NULL;
                        }
                    }
                }
                ret = 0;
            }
        } else if (!strncmp(ident1, "_modstatus", 10)) {
            ident2 = strtok_r(NULL, "/", &temp_ptr);
            if (ident2 != NULL) {
                list_size =
                snd_ucm_get_size_of_list(uc_mgr->card_ctxt_ptr->mod_list_head);
                for (index = 0; index < list_size; index++) {
                if((ident_value =
                snd_ucm_get_value_at_index(uc_mgr->card_ctxt_ptr->mod_list_head,
                index))) {
                        if (!strncmp(ident2, ident_value,
                            (strlen(ident_value)+1))) {
                            *value = 1;
                            free(ident_value);
                            ident_value = NULL;
                            break;
                        } else {
                            free(ident_value);
                            ident_value = NULL;
                        }
                    }
                }
                ret = 0;
            }
        } else {
            ALOGE("Unknown identifier: %s", ident1);
        }
    }
    pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
    return ret;
}

static int check_devices_for_voice_call(snd_use_case_mgr_t *uc_mgr,
const char *use_case)
{
    struct snd_ucm_ident_node *dev_node = NULL;
    int index = 0, list_size = 0, rx_dev_status = 0, tx_dev_status = 0;

    if ((!strncmp(use_case, SND_USE_CASE_VERB_VOICECALL,
        strlen(SND_USE_CASE_VERB_VOICECALL))) ||
        (!strncmp(use_case, SND_USE_CASE_VERB_IP_VOICECALL,
        strlen(SND_USE_CASE_VERB_IP_VOICECALL))) ||
        (!strncmp(use_case, SND_USE_CASE_MOD_PLAY_VOICE,
        strlen(SND_USE_CASE_MOD_PLAY_VOICE))) ||
        (!strncmp(use_case, SND_USE_CASE_MOD_PLAY_VOIP,
        strlen(SND_USE_CASE_MOD_PLAY_VOIP)))) {
        ALOGV("check_devices_for_voice_call(): voice cap detected\n");
        list_size =
        snd_ucm_get_size_of_list(uc_mgr->card_ctxt_ptr->dev_list_head);
        for (index = 0; index < list_size; index++) {
            if ((dev_node =
                snd_ucm_get_device_node(uc_mgr->card_ctxt_ptr->dev_list_head,
                index))) {
                if (dev_node->capability == CAP_RX && dev_node->active == 1) {
                    rx_dev_status = 1;
                } else if (dev_node->capability == CAP_TX && dev_node->active == 1) {
                    tx_dev_status = 1;
                }
            }
        }
        if (rx_dev_status == 1 && tx_dev_status == 1) {
            ALOGV("check_devices_for_voice_call(): Rx and Tx devices enabled\n");
            return 0;
        } else {
            ALOGV("check_devices_for_voice_call(): Rx/Tx dev not enabled: \
                  %d,%d\n", rx_dev_status, tx_dev_status);
            return 1;
        }
    }
    return 0;
}

static int snd_use_case_apply_voice_acdb(snd_use_case_mgr_t *uc_mgr,
int use_case_index)
{
    card_mctrl_t *ctrl_list;
    int list_size, index, verb_index, ret = 0, voice_acdb = 0, rx_id, tx_id;
    char *ident_value = NULL;

    /* Check if voice call use case/modifier exists */
    if ((!strncmp(uc_mgr->card_ctxt_ptr->current_verb,
        SND_USE_CASE_VERB_VOICECALL, strlen(SND_USE_CASE_VERB_VOICECALL))) ||
        (!strncmp(uc_mgr->card_ctxt_ptr->current_verb,
        SND_USE_CASE_VERB_IP_VOICECALL,
        strlen(SND_USE_CASE_VERB_IP_VOICECALL)))) {
        voice_acdb = 1;
    }
    if (voice_acdb != 1) {
        list_size =
        snd_ucm_get_size_of_list(uc_mgr->card_ctxt_ptr->mod_list_head);
        for (index = 0; index < list_size; index++) {
            if ((ident_value =
                snd_ucm_get_value_at_index(uc_mgr->card_ctxt_ptr->mod_list_head,
                index))) {
                if ((!strncmp(ident_value, SND_USE_CASE_MOD_PLAY_VOICE,
                    strlen(SND_USE_CASE_MOD_PLAY_VOICE))) ||
                    (!strncmp(ident_value, SND_USE_CASE_MOD_PLAY_VOIP,
                    strlen(SND_USE_CASE_MOD_PLAY_VOIP)))) {
                    voice_acdb = 1;
                    free(ident_value);
                    ident_value = NULL;
                    break;
                }
                free(ident_value);
                ident_value = NULL;
            }
        }
    }

    verb_index = uc_mgr->card_ctxt_ptr->current_verb_index;
    if((verb_index < 0) ||
        (!strncmp(uc_mgr->card_ctxt_ptr->current_verb,
        SND_UCM_END_OF_LIST, 3))) {
        ALOGE("Invalid current verb value: %s - %d",
            uc_mgr->card_ctxt_ptr->current_verb, verb_index);
        return -EINVAL;
    }
    if (voice_acdb == 1) {
        ctrl_list =
        uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].device_ctrls;
        list_size =
        snd_ucm_get_size_of_list(uc_mgr->card_ctxt_ptr->dev_list_head);
        for (index = 0; index < list_size; index++) {
            if ((ident_value =
                snd_ucm_get_value_at_index(uc_mgr->card_ctxt_ptr->dev_list_head,
                index))) {
                if (strncmp(ident_value, ctrl_list[use_case_index].case_name,
                    (strlen(ctrl_list[use_case_index].case_name)+1))) {
                    break;
                }
                free(ident_value);
                ident_value = NULL;
            }
        }
        index = 0;
        if (ident_value != NULL) {
            while(strncmp(ctrl_list[index].case_name, ident_value,
                  (strlen(ident_value)+1))) {
                if (!strncmp(ctrl_list[index].case_name, SND_UCM_END_OF_LIST,
                    strlen(SND_UCM_END_OF_LIST))) {
                    ret = -EINVAL;
                    break;
                }
                index++;
            }
            if (ret < 0) {
                ALOGE("No valid device found: %s",ident_value);
            } else {
                if (ctrl_list[use_case_index].capability == CAP_RX) {
                    rx_id = ctrl_list[use_case_index].acdb_id;
                    tx_id = ctrl_list[index].acdb_id;
                } else {
                    rx_id = ctrl_list[index].acdb_id;
                    tx_id = ctrl_list[use_case_index].acdb_id;
                }
                if(((rx_id == DEVICE_SPEAKER_MONO_RX_ACDB_ID)||(rx_id == DEVICE_SPEAKER_STEREO_RX_ACDB_ID))
                    && tx_id == DEVICE_HANDSET_TX_ACDB_ID) {
                    tx_id = DEVICE_SPEAKER_TX_ACDB_ID;
                } else if (((rx_id == DEVICE_SPEAKER_MONO_RX_ACDB_ID )||(rx_id == DEVICE_SPEAKER_STEREO_RX_ACDB_ID))
                    && tx_id == DEVICE_HANDSET_TX_FV5_ACDB_ID) {
                    tx_id = DEVICE_SPEAKER_TX_FV5_ACDB_ID;
                }

                if ((rx_id != uc_mgr->current_rx_device) ||
                    (tx_id != uc_mgr->current_tx_device)) {
                    uc_mgr->current_rx_device = rx_id;
                    uc_mgr->current_tx_device = tx_id;
                    ALOGD("Voice acdb: rx id %d tx id %d",
                          uc_mgr->current_rx_device,
                          uc_mgr->current_tx_device);
                    if (uc_mgr->acdb_handle && !uc_mgr->isFusion3Platform) {
                        acdb_send_voice_cal = dlsym(uc_mgr->acdb_handle,"acdb_loader_send_voice_cal");
                        if (acdb_send_voice_cal == NULL) {
                            ALOGE("ucm: dlsym: Error:%s Loading acdb_loader_send_voice_cal", dlerror());
                        }else {
                            acdb_send_voice_cal(uc_mgr->current_rx_device,
                                       uc_mgr->current_tx_device);
                        }
                   }
                } else {
                    ALOGV("Voice acdb: Required acdb already pushed \
                         rx id %d tx id %d", uc_mgr->current_rx_device,
                         uc_mgr->current_tx_device);
                }
            }
            free(ident_value);
            ident_value = NULL;
        }
    } else {
        ALOGV("No voice use case found");
        uc_mgr->current_rx_device = -1; uc_mgr->current_tx_device = -1;
        ret = -ENODEV;
    }
    return ret;
}

int get_use_case_index(snd_use_case_mgr_t *uc_mgr, const char *use_case,
int ctrl_list_type)
{
    use_case_verb_t *verb_list;
    card_mctrl_t *ctrl_list;
    int ret = 0, index = 0, verb_index;

    verb_list = uc_mgr->card_ctxt_ptr->use_case_verb_list;
    verb_index = uc_mgr->card_ctxt_ptr->current_verb_index;
    if (ctrl_list_type == CTRL_LIST_VERB) {
        ctrl_list =
            uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].verb_ctrls;
    } else if (ctrl_list_type == CTRL_LIST_DEVICE) {
        ctrl_list =
            uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].device_ctrls;
    } else if (ctrl_list_type == CTRL_LIST_MODIFIER) {
        ctrl_list =
            uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].mod_ctrls;
    } else {
        ctrl_list = NULL;
    }
    if((verb_index < 0) ||
      (!strncmp(uc_mgr->card_ctxt_ptr->current_verb, SND_UCM_END_OF_LIST, 3)) ||
      (ctrl_list == NULL) || (ctrl_list[index].case_name == NULL)) {
        ALOGE("Invalid current verb value: %s - %d",
                uc_mgr->card_ctxt_ptr->current_verb, verb_index);
        return -EINVAL;
    }
    while(strncmp(ctrl_list[index].case_name, use_case, (strlen(use_case)+1))) {
        if (!strncmp(ctrl_list[index].case_name, SND_UCM_END_OF_LIST,
            strlen(SND_UCM_END_OF_LIST))) {
            ret = -EINVAL;
            break;
        }
        index++;
        if (ctrl_list[index].case_name == NULL) {
            ALOGE("Invalid case_name at index %d", index);
            ret = -EINVAL;
            break;
        }
    }
    if (ret < 0) {
        return ret;
    } else {
        return index;
    }
}

/* Apply the required mixer controls for specific use case
 * uc_mgr - UCM structure pointer
 * use_case - use case name
 * return 0 on sucess, otherwise a negative error code
 */
int snd_use_case_apply_mixer_controls(snd_use_case_mgr_t *uc_mgr,
const char *use_case, int enable, int ctrl_list_type, int uc_index)
{
    card_mctrl_t *ctrl_list;
    mixer_control_t *mixer_list;
    struct mixer_ctl *ctl;
    int i, ret = 0, index = 0, verb_index, mixer_count;

    verb_index = uc_mgr->card_ctxt_ptr->current_verb_index;
    if (ctrl_list_type == CTRL_LIST_VERB) {
        ctrl_list =
            uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].verb_ctrls;
    } else if (ctrl_list_type == CTRL_LIST_DEVICE) {
        ctrl_list =
            uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].device_ctrls;
    } else if (ctrl_list_type == CTRL_LIST_MODIFIER) {
        ctrl_list =
            uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].mod_ctrls;
    } else {
        ctrl_list = NULL;
    }
    if((verb_index < 0) ||
      (!strncmp(uc_mgr->card_ctxt_ptr->current_verb, SND_UCM_END_OF_LIST, 3)) ||
      (ctrl_list == NULL)) {
        ALOGE("Invalid current verb value: %s - %d",
            uc_mgr->card_ctxt_ptr->current_verb, verb_index);
        return -EINVAL;
    }
    if (uc_index < 0) {
        ALOGE("No valid use case found with the use case: %s", use_case);
        ret = -ENODEV;
    } else {
        if (!uc_mgr->card_ctxt_ptr->mixer_handle) {
            ALOGE("Control device not initialized");
            ret = -ENODEV;
        } else {
            if (enable &&
                (check_devices_for_voice_call(uc_mgr, use_case) != NULL))
                return ret;
            ALOGD("Set mixer controls for %s enable %d", use_case, enable);
            if (ctrl_list[uc_index].acdb_id && ctrl_list[uc_index].capability) {
                if (enable) {
                    if (snd_use_case_apply_voice_acdb(uc_mgr, uc_index)) {
                        ALOGV("acdb_id %d cap %d enable %d",
                            ctrl_list[uc_index].acdb_id,
                            ctrl_list[uc_index].capability, enable);
                        if (uc_mgr->acdb_handle) {
                            acdb_send_audio_cal = dlsym(uc_mgr->acdb_handle,"acdb_loader_send_audio_cal");
                            if (acdb_send_audio_cal == NULL) {
                                ALOGE("ucm:dlsym:Error:%s Loading acdb_loader_send_audio_cal", dlerror());
                            } else {
                                acdb_send_audio_cal(ctrl_list[uc_index].acdb_id,
                                                     ctrl_list[uc_index].capability);
                            }
                        }
                    }
                }
            }
            if (enable) {
                mixer_list = ctrl_list[uc_index].ena_mixer_list;
                mixer_count = ctrl_list[uc_index].ena_mixer_count;
            } else {
                mixer_list = ctrl_list[uc_index].dis_mixer_list;
                mixer_count = ctrl_list[uc_index].dis_mixer_count;
            }
            for(index = 0; index < mixer_count; index++) {
                if (mixer_list == NULL) {
                    ALOGE("No valid controls exist for this case: %s", use_case);
                    break;
                }
                ctl = mixer_get_control(uc_mgr->card_ctxt_ptr->mixer_handle,
                          mixer_list[index].control_name, 0);
                if (ctl) {
                    if (mixer_list[index].type == TYPE_INT) {
                        ALOGV("Setting mixer control: %s, value: %d",
                             mixer_list[index].control_name,
                             mixer_list[index].value);
                        ret = mixer_ctl_set(ctl, mixer_list[index].value);
                    } else if (mixer_list[index].type == TYPE_MULTI_VAL) {
                        ALOGD("Setting multi value: %s",
                            mixer_list[index].control_name);
                        ret = mixer_ctl_set_value(ctl, mixer_list[index].value,
                                mixer_list[index].mulval);
                        if (ret < 0)
                            ALOGE("Failed to set multi value control %s\n",
                                mixer_list[index].control_name);
                    } else {
                        ALOGV("Setting mixer control: %s, value: %s",
                            mixer_list[index].control_name,
                            mixer_list[index].string);
                        ret = mixer_ctl_select(ctl, mixer_list[index].string);
                    }
                    if ((ret != 0) && enable) {
                       /* Disable all the mixer controls which are
                        * already enabled before failure */
                       mixer_list = ctrl_list[uc_index].dis_mixer_list;
                       mixer_count = ctrl_list[uc_index].dis_mixer_count;
                       for(i = 0; i < mixer_count; i++) {
                           ctl = mixer_get_control(
                                     uc_mgr->card_ctxt_ptr->mixer_handle,
                                     mixer_list[i].control_name, 0);
                           if (ctl) {
                               if (mixer_list[i].type == TYPE_INT) {
                                   ret = mixer_ctl_set(ctl,
                                             mixer_list[i].value);
                               } else {
                                   ret = mixer_ctl_select(ctl,
                                             mixer_list[i].string);
                               }
                           }
                       }
                       ALOGE("Failed to enable the mixer controls for %s",
                            use_case);
                       break;
                    }
                }
            }
        }
    }
    return ret;
}

int getUseCaseType(const char *useCase)
{
    ALOGV("getUseCaseType: use case is %s\n", useCase);
    if (!strncmp(useCase, SND_USE_CASE_VERB_HIFI,
           MAX_LEN(useCase,SND_USE_CASE_VERB_HIFI)) ||
        !strncmp(useCase, SND_USE_CASE_VERB_HIFI_LOWLATENCY_MUSIC,
           MAX_LEN(useCase,SND_USE_CASE_VERB_HIFI_LOWLATENCY_MUSIC)) ||
        !strncmp(useCase, SND_USE_CASE_VERB_HIFI_LOW_POWER,
           MAX_LEN(useCase,SND_USE_CASE_VERB_HIFI_LOW_POWER)) ||
        !strncmp(useCase, SND_USE_CASE_VERB_HIFI_TUNNEL,
            MAX_LEN(useCase,SND_USE_CASE_VERB_HIFI_TUNNEL)) ||
        !strncmp(useCase, SND_USE_CASE_VERB_HIFI2,
            MAX_LEN(useCase,SND_USE_CASE_VERB_HIFI2)) ||
        !strncmp(useCase, SND_USE_CASE_VERB_DIGITAL_RADIO,
            MAX_LEN(useCase,SND_USE_CASE_VERB_DIGITAL_RADIO)) ||
        !strncmp(useCase, SND_USE_CASE_MOD_PLAY_MUSIC,
            MAX_LEN(useCase,SND_USE_CASE_MOD_PLAY_MUSIC)) ||
        !strncmp(useCase, SND_USE_CASE_MOD_PLAY_LOWLATENCY_MUSIC,
            MAX_LEN(useCase,SND_USE_CASE_MOD_PLAY_LOWLATENCY_MUSIC)) ||
        !strncmp(useCase, SND_USE_CASE_MOD_PLAY_MUSIC2,
            MAX_LEN(useCase,SND_USE_CASE_MOD_PLAY_MUSIC2)) ||
        !strncmp(useCase, SND_USE_CASE_MOD_PLAY_LPA,
            MAX_LEN(useCase,SND_USE_CASE_MOD_PLAY_LPA)) ||
        !strncmp(useCase, SND_USE_CASE_MOD_PLAY_TUNNEL,
            MAX_LEN(useCase,SND_USE_CASE_MOD_PLAY_TUNNEL)) ||
        !strncmp(useCase, SND_USE_CASE_MOD_PLAY_FM,
            MAX_LEN(useCase,SND_USE_CASE_MOD_PLAY_FM))) {
        return CAP_RX;
    } else if (!strncmp(useCase, SND_USE_CASE_VERB_HIFI_REC,
            MAX_LEN(useCase,SND_USE_CASE_VERB_HIFI_REC)) ||
        !strncmp(useCase, SND_USE_CASE_VERB_FM_REC,
            MAX_LEN(useCase,SND_USE_CASE_VERB_FM_REC)) ||
        !strncmp(useCase, SND_USE_CASE_VERB_FM_A2DP_REC,
            MAX_LEN(useCase,SND_USE_CASE_VERB_FM_A2DP_REC)) ||
        !strncmp(useCase, SND_USE_CASE_MOD_CAPTURE_MUSIC,
            MAX_LEN(useCase,SND_USE_CASE_MOD_CAPTURE_MUSIC)) ||
        !strncmp(useCase, SND_USE_CASE_VERB_HIFI_LOWLATENCY_REC,
            MAX_LEN(useCase,SND_USE_CASE_VERB_HIFI_LOWLATENCY_REC)) ||
        !strncmp(useCase, SND_USE_CASE_MOD_CAPTURE_LOWLATENCY_MUSIC,
            MAX_LEN(useCase,SND_USE_CASE_MOD_CAPTURE_LOWLATENCY_MUSIC)) ||
        !strncmp(useCase, SND_USE_CASE_MOD_CAPTURE_FM,
            MAX_LEN(useCase,SND_USE_CASE_MOD_CAPTURE_FM)) ||
        !strncmp(useCase, SND_USE_CASE_MOD_CAPTURE_A2DP_FM,
            MAX_LEN(useCase,SND_USE_CASE_MOD_CAPTURE_A2DP_FM))) {
        return CAP_TX;
    } else if (!strncmp(useCase, SND_USE_CASE_VERB_VOICECALL,
            MAX_LEN(useCase,SND_USE_CASE_VERB_VOICECALL)) ||
        !strncmp(useCase, SND_USE_CASE_VERB_IP_VOICECALL,
            MAX_LEN(useCase,SND_USE_CASE_VERB_IP_VOICECALL)) ||
        !strncmp(useCase, SND_USE_CASE_VERB_DL_REC,
            MAX_LEN(useCase,SND_USE_CASE_VERB_DL_REC)) ||
        !strncmp(useCase, SND_USE_CASE_VERB_UL_DL_REC,
            MAX_LEN(useCase,SND_USE_CASE_VERB_UL_DL_REC)) ||
        !strncmp(useCase, SND_USE_CASE_VERB_INCALL_REC,
            MAX_LEN(useCase,SND_USE_CASE_VERB_INCALL_REC)) ||
        !strncmp(useCase, SND_USE_CASE_MOD_PLAY_VOICE,
            MAX_LEN(useCase,SND_USE_CASE_MOD_PLAY_VOICE)) ||
        !strncmp(useCase, SND_USE_CASE_MOD_PLAY_VOIP,
            MAX_LEN(useCase,SND_USE_CASE_MOD_PLAY_VOIP)) ||
        !strncmp(useCase, SND_USE_CASE_MOD_CAPTURE_VOICE_DL,
            MAX_LEN(useCase,SND_USE_CASE_MOD_CAPTURE_VOICE_DL)) ||
        !strncmp(useCase, SND_USE_CASE_MOD_CAPTURE_VOICE_UL_DL,
            MAX_LEN(useCase,SND_USE_CASE_MOD_CAPTURE_VOICE_UL_DL)) ||
        !strncmp(useCase, SND_USE_CASE_VERB_VOLTE,
            MAX_LEN(useCase,SND_USE_CASE_VERB_VOLTE)) ||
        !strncmp(useCase, SND_USE_CASE_MOD_PLAY_VOLTE,
            MAX_LEN(useCase, SND_USE_CASE_MOD_PLAY_VOLTE))) {
        return CAP_VOICE;
    } else {
        ALOGE("unknown use case %s, returning voice capablity", useCase);
        return CAP_VOICE;
    }
}

/* Set/Reset mixer controls of specific use case for all current devices
 * uc_mgr - UCM structure pointer
 * ident  - use case name (verb or modifier)
 * enable - 1 for enable and 0 for disable
 * return 0 on sucess, otherwise a negative error code
 */
static int set_controls_of_usecase_for_all_devices(snd_use_case_mgr_t *uc_mgr,
const char *ident, int enable, int ctrl_list_type)
{
    card_mctrl_t *dev_list, *uc_list;
    char *current_device, use_case[MAX_UC_LEN];
    int list_size, index, uc_index, ret = 0, intdev_flag = 0;
    int verb_index, capability = 0, ident_cap = 0, dev_cap =0;

    ALOGV("set_use_case_ident_for_all_devices(): %s", ident);
    if ((verb_index = uc_mgr->card_ctxt_ptr->current_verb_index) < 0)
        verb_index = 0;
    dev_list =
        uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].device_ctrls;
    if (ctrl_list_type == CTRL_LIST_VERB) {
        uc_list =
            uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].verb_ctrls;
    } else if (ctrl_list_type == CTRL_LIST_MODIFIER) {
        uc_list =
            uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].mod_ctrls;
    } else {
        uc_list = NULL;
    }
    ident_cap = getUseCaseType(ident);
    list_size = snd_ucm_get_size_of_list(uc_mgr->card_ctxt_ptr->dev_list_head);
    for (index = 0; index < list_size; index++) {
        current_device =
        snd_ucm_get_value_at_index(uc_mgr->card_ctxt_ptr->dev_list_head, index);
        if (current_device != NULL) {
            uc_index = get_use_case_index(uc_mgr, current_device,
                       CTRL_LIST_DEVICE);
            dev_cap = dev_list[uc_index].capability;
            if (!capability) {
                capability = dev_list[uc_index].capability;
            } else if (capability != dev_list[uc_index].capability) {
                capability = CAP_VOICE;
            }
            if (ident_cap == CAP_VOICE  || ident_cap == dev_cap) {
                if (enable) {
                    if (!snd_ucm_get_status_at_index(
                        uc_mgr->card_ctxt_ptr->dev_list_head, current_device)) {
                        if (uc_index >= 0) {
                            ALOGV("Applying mixer controls for device: %s",
                                  current_device);
                            ret = snd_use_case_apply_mixer_controls(uc_mgr,
                                  current_device, enable, CTRL_LIST_DEVICE, uc_index);
                            if (!ret)
                                snd_ucm_set_status_at_index(
                                  uc_mgr->card_ctxt_ptr->dev_list_head,
                                  current_device, enable, dev_cap);
                        }
                     } else if (ident_cap == CAP_VOICE) {
                        snd_use_case_apply_voice_acdb(uc_mgr, uc_index);
                     }
                 }
                 strlcpy(use_case, ident, sizeof(use_case));
                 strlcat(use_case, current_device, sizeof(use_case));
                 ALOGV("Applying mixer controls for use case: %s", use_case);
                 if ((uc_index =
                      get_use_case_index(uc_mgr, use_case, ctrl_list_type)) < 0) {
                      ALOGV("No valid use case found: %s", use_case);
                      intdev_flag++;
                 } else {
                      if (capability == CAP_VOICE || ident_cap == CAP_VOICE ||
                          capability == ident_cap) {
                          ret = snd_use_case_apply_mixer_controls(uc_mgr, use_case,
                                enable, ctrl_list_type, uc_index);
                      }
                 }
                 use_case[0] = 0;
                 free(current_device);
             }
        }
    }
    if (intdev_flag) {
        if ((uc_index = get_use_case_index(uc_mgr, ident, ctrl_list_type)) < 0) {
            ALOGE("use case %s not valid without device combination", ident);
        } else {
            if (capability == CAP_VOICE || capability == ident_cap ||
                ident_cap == CAP_VOICE) {
                snd_use_case_apply_mixer_controls(uc_mgr, ident, enable,
                ctrl_list_type, uc_index);
            }
        }
    }
    return ret;
}

/* Set/Reset mixer controls of specific use case for a specific device
 * uc_mgr - UCM structure pointer
 * ident  - use case name (verb or modifier)
 * device - device for which use case needs to be set/reset
 * enable - 1 for enable and 0 for disable
 * return 0 on sucess, otherwise a negative error code
 */
static int set_controls_of_usecase_for_device(snd_use_case_mgr_t *uc_mgr,
const char *ident, const char *device, int enable, int ctrl_list_type)
{
    card_mctrl_t *dev_list;
    char use_case[MAX_UC_LEN];
    int list_size, index, dev_index, uc_index, ret = 0;
    int verb_index, capability = 0;

    ALOGV("set_use_case_ident_for_device(): use case %s device %s", ident,
        device);
    if ((verb_index = uc_mgr->card_ctxt_ptr->current_verb_index) < 0)
        verb_index = 0;
    dev_list =
        uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].device_ctrls;
    if (device != NULL) {
        if (enable) {
            dev_index = get_use_case_index(uc_mgr, device, CTRL_LIST_DEVICE);
            capability = dev_list[dev_index].capability;
            if (!snd_ucm_get_status_at_index(
                uc_mgr->card_ctxt_ptr->dev_list_head, device)) {
                ret = snd_use_case_apply_mixer_controls(uc_mgr, device,
                         enable, CTRL_LIST_DEVICE, dev_index);
                if (!ret)
                    snd_ucm_set_status_at_index(
                    uc_mgr->card_ctxt_ptr->dev_list_head, device, enable,
                    capability);
            }
        }
        strlcpy(use_case, ident, sizeof(use_case));
        strlcat(use_case, device, sizeof(use_case));
    ALOGV("Applying mixer controls for use case: %s", use_case);
        if ((uc_index = get_use_case_index(uc_mgr, use_case, ctrl_list_type)) < 0) {
            ALOGV("No valid use case found: %s", use_case );
            uc_index = get_use_case_index(uc_mgr, ident, ctrl_list_type);
            if (snd_use_case_apply_mixer_controls(uc_mgr, ident, enable,
                ctrl_list_type, uc_index) < 0) {
                 ALOGV("use case %s not valid without device combination also",
                     ident);
            }
        } else {
            ret = snd_use_case_apply_mixer_controls(uc_mgr, use_case, enable,
                      ctrl_list_type, uc_index);
        }
    } else {
        uc_index = get_use_case_index(uc_mgr, ident, ctrl_list_type);
        if (snd_use_case_apply_mixer_controls(uc_mgr, ident, enable,
            ctrl_list_type, uc_index) < 0) {
             ALOGV("use case %s not valid without device combination also",
                 ident);
        }
    }
    return ret;
}

/* Set/Reset mixer controls of specific device for all use cases
 * uc_mgr - UCM structure pointer
 * device - device name
 * enable - 1 for enable and 0 for disable
 * return 0 on sucess, otherwise a negative error code
 */
static int set_controls_of_device_for_all_usecases(snd_use_case_mgr_t *uc_mgr,
const char *device, int enable)
{
    card_mctrl_t *dev_list, *uc_list;
    char *ident_value, use_case[MAX_UC_LEN];
    int verb_index, uc_index, dev_index, capability = 0;
    int list_size, index = 0, ret = -ENODEV, flag = 0, intdev_flag = 0;

    ALOGV("set_controls_of_device_for_all_usecases: %s", device);
    if ((verb_index = uc_mgr->card_ctxt_ptr->current_verb_index) < 0)
        verb_index = 0;
    dev_list =
         uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].device_ctrls;
    dev_index = get_use_case_index(uc_mgr, device, CTRL_LIST_DEVICE);
    if (dev_index >= 0)
        capability = dev_list[dev_index].capability;
    if (strncmp(uc_mgr->card_ctxt_ptr->current_verb, SND_USE_CASE_VERB_INACTIVE,
        strlen(SND_USE_CASE_VERB_INACTIVE))) {
        uc_list =
            uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].verb_ctrls;
        if (capability == CAP_VOICE ||
            capability == getUseCaseType(uc_mgr->card_ctxt_ptr->current_verb) ||
            getUseCaseType(uc_mgr->card_ctxt_ptr->current_verb) == CAP_VOICE) {
            strlcpy(use_case, uc_mgr->card_ctxt_ptr->current_verb,
                sizeof(use_case));
            strlcat(use_case, device, sizeof(use_case));
            if ((uc_index =
                get_use_case_index(uc_mgr, use_case, CTRL_LIST_VERB)) < 0) {
                ALOGV("No valid use case found: %s", use_case);
                intdev_flag = 1;
            } else {
                if (enable) {
                    if (!snd_ucm_get_status_at_index(
                        uc_mgr->card_ctxt_ptr->dev_list_head, device)) {
                        ret = snd_use_case_apply_mixer_controls(uc_mgr, device,
                                  enable, CTRL_LIST_DEVICE, dev_index);
                        if (!ret)
                            snd_ucm_set_status_at_index(
                            uc_mgr->card_ctxt_ptr->dev_list_head, device,
                            enable, capability);
                            flag = 1;
                    }
                }
                ALOGV("set %d for use case value: %s", enable, use_case);
                ret = snd_use_case_apply_mixer_controls(uc_mgr, use_case,
                          enable, CTRL_LIST_VERB, uc_index);
                if (ret != 0)
                     ALOGE("No valid controls exists for usecase %s and device \
                          %s, enable: %d", use_case, device, enable);
            }
        }
        if (intdev_flag) {
            if (enable && !flag) {
                if (!snd_ucm_get_status_at_index(
                    uc_mgr->card_ctxt_ptr->dev_list_head, device)) {
                    ret = snd_use_case_apply_mixer_controls(uc_mgr,
                              device, enable, CTRL_LIST_DEVICE, dev_index);
                    if (!ret)
                        snd_ucm_set_status_at_index(
                        uc_mgr->card_ctxt_ptr->dev_list_head, device, enable,
                        capability);
                    flag = 1;
                }
            }
            use_case[0] = 0;
            strlcpy(use_case, uc_mgr->card_ctxt_ptr->current_verb,
                sizeof(use_case));
            uc_index = get_use_case_index(uc_mgr, use_case, CTRL_LIST_VERB);
            if (capability == CAP_VOICE ||
                capability ==
                getUseCaseType(uc_mgr->card_ctxt_ptr->current_verb) ||
                getUseCaseType(uc_mgr->card_ctxt_ptr->current_verb) ==
                CAP_VOICE) {
                ALOGV("set %d for use case value: %s", enable, use_case);
                ret = snd_use_case_apply_mixer_controls(uc_mgr, use_case,
                          enable, CTRL_LIST_VERB, uc_index);
                if (ret != 0)
                      ALOGE("No valid controls exists for usecase %s and \
                           device %s, enable: %d", use_case, device, enable);
            }
            intdev_flag = 0;
        }
        use_case[0] = 0;
    }
    snd_ucm_print_list(uc_mgr->card_ctxt_ptr->mod_list_head);
    uc_list =
        uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].mod_ctrls;
    list_size = snd_ucm_get_size_of_list(uc_mgr->card_ctxt_ptr->mod_list_head);
    for (index = 0; index < list_size; index++) {
        if ((ident_value =
            snd_ucm_get_value_at_index(uc_mgr->card_ctxt_ptr->mod_list_head,
            index))) {
            if (capability == CAP_VOICE ||
                getUseCaseType(ident_value) == CAP_VOICE ||
                capability == getUseCaseType(ident_value)) {
                strlcpy(use_case, ident_value, sizeof(use_case));
                strlcat(use_case, device, sizeof(use_case));
                if ((uc_index = get_use_case_index(uc_mgr, use_case,
                    CTRL_LIST_MODIFIER)) < 0) {
                    ALOGV("No valid use case found: %s", use_case);
                    intdev_flag = 1;
                } else {
                    if (enable && !flag) {
                        if (!snd_ucm_get_status_at_index(
                            uc_mgr->card_ctxt_ptr->dev_list_head, device)) {
                            ret = snd_use_case_apply_mixer_controls(uc_mgr,
                                      device, enable, CTRL_LIST_DEVICE,
                                      dev_index);
                            if (!ret)
                                snd_ucm_set_status_at_index(
                                    uc_mgr->card_ctxt_ptr->dev_list_head,
                                    device, enable, capability);
                            flag = 1;
                        }
                    }
                    ALOGV("set %d for use case value: %s", enable, use_case);
                    ret = snd_use_case_apply_mixer_controls(uc_mgr,
                          use_case, enable, CTRL_LIST_MODIFIER, uc_index);
                    if (ret != 0)
                        ALOGE("No valid controls exists for usecase %s and \
                            device %s, enable: %d", use_case, device, enable);
                }
            }
            if (intdev_flag) {
                if (enable && !flag) {
                    if (!snd_ucm_get_status_at_index(
                         uc_mgr->card_ctxt_ptr->dev_list_head, device)) {
                        ret = snd_use_case_apply_mixer_controls(uc_mgr,
                                  device, enable, CTRL_LIST_DEVICE, dev_index);
                        if (!ret)
                            snd_ucm_set_status_at_index(
                            uc_mgr->card_ctxt_ptr->dev_list_head, device,
                            enable, capability);
                        flag = 1;
                    }
                }
                use_case[0] = 0;
                strlcpy(use_case, ident_value, sizeof(use_case));
                uc_index =
                    get_use_case_index(uc_mgr, ident_value, CTRL_LIST_MODIFIER);
                if (capability == CAP_VOICE ||
                    capability == getUseCaseType(ident_value) ||
                    getUseCaseType(ident_value) == CAP_VOICE) {
                    ALOGV("set %d for use case value: %s", enable, use_case);
                    ret = snd_use_case_apply_mixer_controls(uc_mgr, use_case,
                          enable, CTRL_LIST_MODIFIER, uc_index);
                    if (ret != 0)
                         ALOGE("No valid controls exists for usecase %s and \
                              device %s, enable: %d", use_case, device, enable);
                }
                intdev_flag = 0;
            }
            use_case[0] = 0;
            free(ident_value);
        }
    }
    if (!enable) {
        ret = snd_use_case_apply_mixer_controls(uc_mgr, device, enable,
                  CTRL_LIST_DEVICE, dev_index);
        if (!ret)
            snd_ucm_set_status_at_index(uc_mgr->card_ctxt_ptr->dev_list_head,
                device, enable, capability);
    }
    return ret;
}

/* Returns usecase type i.e. either verb or modifier
 * uc_mgr - UCM structure pointer
 * usecase - usecase name either verb or modifier
 * return CTRL_LIST_VERB or CTRL_LIST_MODIFIER for verb/modifier respectively
 */
static int get_usecase_type(snd_use_case_mgr_t *uc_mgr, const char *usecase)
{
    int ret = -EINVAL, index = 0;

    while (strncmp(uc_mgr->card_ctxt_ptr->verb_list[index],
        SND_UCM_END_OF_LIST, strlen(SND_UCM_END_OF_LIST))) {
        if (!strncmp(uc_mgr->card_ctxt_ptr->verb_list[index], usecase,
            (strlen(usecase)+1))) {
            ret = 0;
            break;
        }
        index++;
    }
    if (ret == 0)
        return CTRL_LIST_VERB;
    else
        return CTRL_LIST_MODIFIER;
}

/* Set/Reset mixer controls of specific device and specific use cases
 * uc_mgr - UCM structure pointer
 * device - device name
 * usecase - use case for which device needs to be enabled
 * enable - 1 for enable and 0 for disable
 * return 0 on sucess, otherwise a negative error code
 */
static int set_controls_of_device_for_usecase(snd_use_case_mgr_t *uc_mgr,
    const char *device, const char *usecase, int enable)
{
    card_mctrl_t *dev_list;
    char use_case[MAX_UC_LEN];
    int ret = -ENODEV, uc_index, dev_index;
    int verb_index, capability = 0;

    ALOGV("set_device_for_ident(): %s %s", device, usecase);
    if ((verb_index = uc_mgr->card_ctxt_ptr->current_verb_index) < 0)
        verb_index = 0;
    dev_list =
         uc_mgr->card_ctxt_ptr->use_case_verb_list[verb_index].device_ctrls;
    dev_index = get_use_case_index(uc_mgr, device, CTRL_LIST_DEVICE);
    capability = dev_list[dev_index].capability;
    if (usecase != NULL) {
        strlcpy(use_case, usecase, sizeof(use_case));
        strlcat(use_case, device, sizeof(use_case));
        if ((uc_index = get_use_case_index(uc_mgr, use_case,
            get_usecase_type(uc_mgr, usecase))) < 0) {
            ALOGV("No valid use case found: %s", use_case);
        } else {
            if (enable) {
                if (!snd_ucm_get_status_at_index(
                    uc_mgr->card_ctxt_ptr->dev_list_head, device)) {
                    ret = snd_use_case_apply_mixer_controls(uc_mgr, device,
                          enable, CTRL_LIST_DEVICE, dev_index);
                    if (!ret)
                        snd_ucm_set_status_at_index
                        (uc_mgr->card_ctxt_ptr->dev_list_head, device, enable,
                        capability);
                }
            }
            ALOGV("set %d for use case value: %s", enable, use_case);
            ret = snd_use_case_apply_mixer_controls(uc_mgr, use_case, enable,
                      get_usecase_type(uc_mgr, usecase), uc_index);
            if (ret != 0)
                ALOGE("No valid controls exists for usecase %s and device %s, \
                     enable: %d", use_case, device, enable);
        }
        use_case[0] = 0;
    } else {
        if (enable) {
            if (!snd_ucm_get_status_at_index(
                 uc_mgr->card_ctxt_ptr->dev_list_head, device)) {
                ret = snd_use_case_apply_mixer_controls(uc_mgr, device, enable,
                          CTRL_LIST_DEVICE, dev_index);
                if (!ret)
                    snd_ucm_set_status_at_index(
                        uc_mgr->card_ctxt_ptr->dev_list_head, device, enable,
                        capability);
            }
        }
    }
    if (!enable) {
        ret = snd_use_case_apply_mixer_controls(uc_mgr, device, enable,
                  CTRL_LIST_DEVICE, dev_index);
        if (!ret)
            snd_ucm_set_status_at_index(uc_mgr->card_ctxt_ptr->dev_list_head,
                device, enable, capability);
    }
    return ret;
}

/**
 * Set new value for an identifier
 * uc_mgr - UCM structure
 * identifier - _verb, _enadev, _disdev, _enamod, _dismod
 *        _swdev, _swmod
 * value - Value to be set
 * returns 0 on success, otherwise a negative error code
 */
int snd_use_case_set(snd_use_case_mgr_t *uc_mgr,
                     const char *identifier,
                     const char *value)
{
    use_case_verb_t *verb_list;
    char ident[MAX_STR_LEN], *ident1, *ident2, *temp_ptr;
    int verb_index, list_size, index = 0, ret = -EINVAL;

    pthread_mutex_lock(&uc_mgr->card_ctxt_ptr->card_lock);
    if ((uc_mgr->snd_card_index >= (int)MAX_NUM_CARDS) || (value == NULL) ||
        (uc_mgr->snd_card_index < 0) || (uc_mgr->card_ctxt_ptr == NULL) ||
        (identifier == NULL)) {
        ALOGE("snd_use_case_set(): failed, invalid arguments");
        pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
        return -EINVAL;
    }

    ALOGD("snd_use_case_set(): uc_mgr %p identifier %s value %s", uc_mgr,
         identifier, value);
    strlcpy(ident, identifier, sizeof(ident));
    if(!(ident1 = strtok_r(ident, "/", &temp_ptr))) {
        ALOGV("No multiple identifiers found in identifier value");
        ident[0] = 0;
    } else {
        if (!strncmp(ident1, "_swdev", 6)) {
            if(!(ident2 = strtok_r(NULL, "/", &temp_ptr))) {
                ALOGD("Invalid disable device value: %s, but enabling new \
                     device", ident2);
            } else {
                ret = snd_ucm_del_ident_from_list(
                          &uc_mgr->card_ctxt_ptr->dev_list_head, ident2);
                if (ret < 0) {
                    ALOGV("Ignore device %s disable, device not part of \
                         enabled list", ident2);
                } else {
                    ALOGV("swdev: device value to be disabled: %s", ident2);
                    /* Disable mixer controls for
                     * corresponding use cases and device */
                    ret = set_controls_of_device_for_all_usecases(uc_mgr,
                              ident2, 0);
                    if (ret < 0) {
                        ALOGV("Device %s not disabled, no valid use case \
                              found: %d", ident2, errno);
                    }
                }
            }
            pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
            ret = snd_use_case_set(uc_mgr, "_enadev", value);
            if (ret < 0) {
                ALOGV("Device %s not enabled, no valid use case found: %d",
                    value, errno);
            }
            return ret;
        } else if (!strncmp(ident1, "_swmod", 6)) {
            pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
            if(!(ident2 = strtok_r(NULL, "/", &temp_ptr))) {
                ALOGD("Invalid modifier value: %s, but enabling new modifier",
                    ident2);
            } else {
                ret = snd_use_case_set(uc_mgr, "_dismod", ident2);
                if (ret < 0) {
                    ALOGV("Modifier %s not disabled, no valid use case \
                         found: %d", ident2, errno);
                }
            }
            ret = snd_use_case_set(uc_mgr, "_enamod", value);
            if (ret < 0) {
                ALOGV("Modifier %s not enabled, no valid use case found: %d",
                    value, errno);
            }
            return ret;
        } else {
            ALOGV("No switch device/modifier option found: %s", ident1);
        }
        ident[0] = 0;
    }

    if (!strncmp(identifier, "_verb", 5)) {
        /* Check if value is valid verb */
        while (strncmp(uc_mgr->card_ctxt_ptr->verb_list[index],
               SND_UCM_END_OF_LIST, strlen(SND_UCM_END_OF_LIST))) {
            if (!strncmp(uc_mgr->card_ctxt_ptr->verb_list[index], value,
                (strlen(value)+1))) {
                ret = 0;
                break;
            }
            index++;
        }
        if ((ret < 0) && (strncmp(value, SND_USE_CASE_VERB_INACTIVE,
            strlen(SND_USE_CASE_VERB_INACTIVE)))) {
            ALOGE("Invalid verb identifier value");
        } else {
            ALOGV("Index:%d Verb:%s", index,
                uc_mgr->card_ctxt_ptr->verb_list[index]);
            /* Disable the mixer controls for current use case
             * for all the enabled devices */
            if (strncmp(uc_mgr->card_ctxt_ptr->current_verb,
                SND_USE_CASE_VERB_INACTIVE,
                strlen(SND_USE_CASE_VERB_INACTIVE))) {
                ret = set_controls_of_usecase_for_all_devices(uc_mgr,
                      uc_mgr->card_ctxt_ptr->current_verb, 0, CTRL_LIST_VERB);
                if (ret != 0)
                    ALOGE("Failed to disable controls for use case: %s",
                        uc_mgr->card_ctxt_ptr->current_verb);
            }
            strlcpy(uc_mgr->card_ctxt_ptr->current_verb, value, MAX_STR_LEN);
            /* Enable the mixer controls for the new use case
             * for all the enabled devices */
            if (strncmp(uc_mgr->card_ctxt_ptr->current_verb,
               SND_USE_CASE_VERB_INACTIVE,
               strlen(SND_USE_CASE_VERB_INACTIVE))) {
               uc_mgr->card_ctxt_ptr->current_verb_index = index;
               ret = set_controls_of_usecase_for_all_devices(uc_mgr,
                     uc_mgr->card_ctxt_ptr->current_verb, 1, CTRL_LIST_VERB);
            }
        }
    } else if (!strncmp(identifier, "_enadev", 7)) {
        index = 0; ret = 0;
        list_size =
            snd_ucm_get_size_of_list(uc_mgr->card_ctxt_ptr->dev_list_head);
        for (index = 0; index < list_size; index++) {
            if ((ident1 =
                snd_ucm_get_value_at_index(uc_mgr->card_ctxt_ptr->dev_list_head,
                index))) {
                if (!strncmp(ident1, value, (strlen(value)+1))) {
                    ALOGV("Ignore enable as %s device is already part of \
                         enabled list", value);
                    free(ident1);
                    break;
                }
                free(ident1);
            }
        }
        if (index == list_size) {
            ALOGV("enadev: device value to be enabled: %s", value);
            snd_ucm_add_ident_to_list(&uc_mgr->card_ctxt_ptr->dev_list_head,
                value);
        }
        snd_ucm_print_list(uc_mgr->card_ctxt_ptr->dev_list_head);
        /* Apply Mixer controls of all verb and modifiers for this device*/
        ret = set_controls_of_device_for_all_usecases(uc_mgr, value, 1);
    } else if (!strncmp(identifier, "_disdev", 7)) {
        ret = snd_ucm_get_status_at_index(uc_mgr->card_ctxt_ptr->dev_list_head,
                  value);
        if (ret < 0) {
            ALOGD("disdev: device %s not enabled, no need to disable", value);
        } else if (ret == 0) {
            ALOGV("disdev: device %s not active, remove from the list", value);
            ret =
            snd_ucm_del_ident_from_list(&uc_mgr->card_ctxt_ptr->dev_list_head,
            value);
            if (ret < 0) {
                ALOGE("Invalid device: Device not part of enabled device list");
            }
        } else {
            ret =
            snd_ucm_del_ident_from_list(&uc_mgr->card_ctxt_ptr->dev_list_head,
            value);
            if (ret < 0) {
                ALOGE("Invalid device: Device not part of enabled device list");
            } else {
                ALOGV("disdev: device value to be disabled: %s", value);
                index = get_use_case_index(uc_mgr, value, CTRL_LIST_DEVICE);
                /* Apply Mixer controls for corresponding device and modifier */
                ret = snd_use_case_apply_mixer_controls(uc_mgr, value, 0,
                          CTRL_LIST_DEVICE, index);
            }
        }
    } else if (!strncmp(identifier, "_enamod", 7)) {
        index = 0; ret = 0;
        verb_index = uc_mgr->card_ctxt_ptr->current_verb_index;
        if (verb_index < 0) {
            ALOGE("Invalid verb identifier value");
        } else {
            ALOGV("Index:%d Verb:%s", verb_index,
                 uc_mgr->card_ctxt_ptr->verb_list[verb_index]);
            verb_list = uc_mgr->card_ctxt_ptr->use_case_verb_list;
            while(strncmp(verb_list[verb_index].modifier_list[index], value,
                  (strlen(value)+1))) {
                if (!strncmp(verb_list[verb_index].modifier_list[index],
                    SND_UCM_END_OF_LIST, strlen(SND_UCM_END_OF_LIST))){
                    ret = -EINVAL;
                    break;
                }
                index++;
            }
            if (ret < 0) {
                ALOGE("Invalid modifier identifier value");
            } else {
                snd_ucm_add_ident_to_list(&uc_mgr->card_ctxt_ptr->mod_list_head,
                    value);
                /* Enable the mixer controls for the new use case
                 * for all the enabled devices */
                ret = set_controls_of_usecase_for_all_devices(uc_mgr, value, 1,
                          CTRL_LIST_MODIFIER);
            }
        }
    } else if (!strncmp(identifier, "_dismod", 7)) {
        ret = snd_ucm_del_ident_from_list(&uc_mgr->card_ctxt_ptr->mod_list_head,
                  value);
        if (ret < 0) {
            ALOGE("Modifier not enabled currently, invalid modifier");
        } else {
            ALOGV("dismod: modifier value to be disabled: %s", value);
            /* Enable the mixer controls for the new use case
             * for all the enabled devices */
            ret = set_controls_of_usecase_for_all_devices(uc_mgr, value, 0,
                      CTRL_LIST_MODIFIER);
        }
    } else {
        ALOGE("Unknown identifier value: %s", identifier);
    }
    pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
    return ret;
}

/**
 * Set new value for an identifier based on use case
 * uc_mgr - UCM structure
 * identifier - _verb, _enadev, _disdev, _enamod, _dismod
 *        _swdev, _swmod
 * value - Value to be set
 * usecase - usecase/device for which this command needs to be executed
 * returns 0 on success, otherwise a negative error code
 */
int snd_use_case_set_case(snd_use_case_mgr_t *uc_mgr,
                     const char *identifier,
                     const char *value, const char *usecase)
{
    use_case_verb_t *verb_list;
    char ident[MAX_STR_LEN], *ident1, *ident2, *temp_ptr;
    int verb_index, list_size, index = 0, ret = -EINVAL;

    pthread_mutex_lock(&uc_mgr->card_ctxt_ptr->card_lock);
    if ((uc_mgr->snd_card_index >= (int)MAX_NUM_CARDS) || (value == NULL) ||
        (uc_mgr->snd_card_index < 0) || (uc_mgr->card_ctxt_ptr == NULL) ||
        (identifier == NULL)) {
        ALOGE("snd_use_case_set_case(): failed, invalid arguments");
        pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
        return -EINVAL;
    }

    ALOGD("snd_use_case_set_case(): uc_mgr %p identifier %s value %s",
        uc_mgr, identifier, value);
    strlcpy(ident, identifier, sizeof(ident));
    if(!(ident1 = strtok_r(ident, "/", &temp_ptr))) {
        ALOGV("No multiple identifiers found in identifier value");
        ident[0] = 0;
    } else {
        if (!strncmp(ident1, "_swdev", 6)) {
            if(!(ident2 = strtok_r(NULL, "/", &temp_ptr))) {
                ALOGD("Invalid disable device value: %s, but enabling new \
                     device", ident2);
            } else {
                ret = snd_ucm_del_ident_from_list(
                          &uc_mgr->card_ctxt_ptr->dev_list_head, ident2);
                if (ret < 0) {
                    ALOGV("Ignore device %s disable, device not part of \
                         enabled list", ident2);
                } else {
                    ALOGV("swdev: device value to be disabled: %s", ident2);
                    /* Disable mixer controls for
                     * corresponding use cases and device */
                    ret = set_controls_of_device_for_usecase(uc_mgr, ident2,
                              usecase, 0);
                    if (ret < 0) {
                        ALOGV("Device %s not disabled, no valid use case \
                             found: %d", ident2, errno);
                    }
                }
            }
            pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
            ret = snd_use_case_set_case(uc_mgr, "_enadev", value, usecase);
            if (ret < 0) {
                ALOGV("Device %s not enabled, no valid use case found: %d",
                    value, errno);
            }
            return ret;
        } else if (!strncmp(ident1, "_swmod", 6)) {
            pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
            if(!(ident2 = strtok_r(NULL, "/", &temp_ptr))) {
                ALOGD("Invalid modifier value: %s, but enabling new modifier",
                    ident2);
            } else {
                ret = snd_use_case_set_case(uc_mgr, "_dismod", ident2, usecase);
                if (ret < 0) {
                    ALOGV("Modifier %s not disabled, no valid use case \
                         found: %d", ident2, errno);
                }
            }
            ret = snd_use_case_set_case(uc_mgr, "_enamod", value, usecase);
            if (ret < 0) {
                ALOGV("Modifier %s not enabled, no valid use case found: %d",
                    value, errno);
            }
            return ret;
        } else {
            ALOGV("No switch device/modifier option found: %s", ident1);
        }
        ident[0] = 0;
    }

    if (!strncmp(identifier, "_verb", 5)) {
        /* Check if value is valid verb */
        while (strncmp(uc_mgr->card_ctxt_ptr->verb_list[index],
               SND_UCM_END_OF_LIST, MAX_STR_LEN)) {
            if (!strncmp(uc_mgr->card_ctxt_ptr->verb_list[index],
                value, MAX_STR_LEN)) {
                ret = 0;
                break;
            }
            index++;
        }
        if ((ret < 0) && (strncmp(value, SND_USE_CASE_VERB_INACTIVE,
            MAX_STR_LEN))) {
            ALOGE("Invalid verb identifier value");
        } else {
            ALOGV("Index:%d Verb:%s", index,
                 uc_mgr->card_ctxt_ptr->verb_list[index]);
            /* Disable the mixer controls for current use case
             * for specified device */
            if (strncmp(uc_mgr->card_ctxt_ptr->current_verb,
                SND_USE_CASE_VERB_INACTIVE, MAX_STR_LEN)) {
                ret = set_controls_of_usecase_for_device(uc_mgr,
                          uc_mgr->card_ctxt_ptr->current_verb, usecase,
                          0, CTRL_LIST_VERB);
                if (ret != 0)
                    ALOGE("Failed to disable controls for use case: %s",
                        uc_mgr->card_ctxt_ptr->current_verb);
            }
            strlcpy(uc_mgr->card_ctxt_ptr->current_verb, value, MAX_STR_LEN);
            /* Enable the mixer controls for the new use case
             * for specified device */
            if (strncmp(uc_mgr->card_ctxt_ptr->current_verb,
                SND_USE_CASE_VERB_INACTIVE, MAX_STR_LEN)) {
               uc_mgr->card_ctxt_ptr->current_verb_index = index;
               index = 0;
               list_size =
               snd_ucm_get_size_of_list(uc_mgr->card_ctxt_ptr->dev_list_head);
               for (index = 0; index < list_size; index++) {
                   if ((ident1 = snd_ucm_get_value_at_index(
                       uc_mgr->card_ctxt_ptr->dev_list_head, index))) {
                       if (!strncmp(ident1, usecase, MAX_STR_LEN)) {
                           ALOGV("Device already part of enabled list: %s",
                               usecase);
                           free(ident1);
                           break;
                       }
                       free(ident1);
                   }
               }
               if (index == list_size) {
                   ALOGV("enadev: device value to be enabled: %s", usecase);
                   snd_ucm_add_ident_to_list(&uc_mgr->card_ctxt_ptr->dev_list_head,
                        usecase);
               }
               ret = set_controls_of_usecase_for_device(uc_mgr,
                         uc_mgr->card_ctxt_ptr->current_verb, usecase,
                         1, CTRL_LIST_VERB);
            }
        }
    } else if (!strncmp(identifier, "_enadev", 7)) {
        index = 0; ret = 0;
        list_size =
            snd_ucm_get_size_of_list(uc_mgr->card_ctxt_ptr->dev_list_head);
        for (index = 0; index < list_size; index++) {
            if ((ident1 =
                snd_ucm_get_value_at_index(uc_mgr->card_ctxt_ptr->dev_list_head,
                index))) {
                if (!strncmp(ident1, value, MAX_STR_LEN)) {
                    ALOGV("Device already part of enabled list: %s", value);
                    free(ident1);
                    break;
                }
                free(ident1);
            }
        }
        if (index == list_size) {
            ALOGV("enadev: device value to be enabled: %s", value);
            snd_ucm_add_ident_to_list(&uc_mgr->card_ctxt_ptr->dev_list_head,
                value);
        }
        snd_ucm_print_list(uc_mgr->card_ctxt_ptr->dev_list_head);
        /* Apply Mixer controls of usecase for this device*/
        ret = set_controls_of_device_for_usecase(uc_mgr, value, usecase, 1);
    } else if (!strncmp(identifier, "_disdev", 7)) {
        ret = snd_ucm_get_status_at_index(uc_mgr->card_ctxt_ptr->dev_list_head,
                  value);
        if (ret < 0) {
            ALOGD("disdev: device %s not enabled, no need to disable", value);
        } else if (ret == 0) {
            ALOGV("disdev: device %s not active, remove from the list", value);
            ret =
            snd_ucm_del_ident_from_list(&uc_mgr->card_ctxt_ptr->dev_list_head,
            value);
            if (ret < 0) {
                ALOGE("Invalid device: Device not part of enabled device list");
            }
        } else {
            ret =
            snd_ucm_del_ident_from_list(&uc_mgr->card_ctxt_ptr->dev_list_head,
            value);
            if (ret < 0) {
                ALOGE("Invalid device: Device not part of enabled device list");
            } else {
                ALOGV("disdev: device value to be disabled: %s", value);
                /* Apply Mixer controls of usecase for this device*/
                ret = set_controls_of_device_for_usecase(uc_mgr, value,
                          usecase, 0);
            }
        }
    } else if (!strncmp(identifier, "_enamod", 7)) {
        if (!strncmp(uc_mgr->card_ctxt_ptr->current_verb,
            SND_USE_CASE_VERB_INACTIVE, MAX_STR_LEN)) {
            ALOGE("Invalid use case verb value");
            ret = -EINVAL;
        } else {
            ret = 0;
            while(strncmp(uc_mgr->card_ctxt_ptr->verb_list[index],
                  uc_mgr->card_ctxt_ptr->current_verb, MAX_STR_LEN)) {
                if (!strncmp(uc_mgr->card_ctxt_ptr->verb_list[index],
                    SND_UCM_END_OF_LIST, MAX_STR_LEN)){
                    ret = -EINVAL;
                    break;
                }
                index++;
            }
        }
        if (ret < 0) {
            ALOGE("Invalid verb identifier value");
        } else {
            verb_index = index; index = 0; ret = 0;
            verb_list = uc_mgr->card_ctxt_ptr->use_case_verb_list;
            ALOGV("Index:%d Verb:%s", verb_index,
                 uc_mgr->card_ctxt_ptr->verb_list[verb_index]);
            while(strncmp(verb_list[verb_index].modifier_list[index],
                value, MAX_STR_LEN)) {
                if (!strncmp(verb_list[verb_index].modifier_list[index],
                    SND_UCM_END_OF_LIST, MAX_STR_LEN)){
                    ret = -EINVAL;
                    break;
                }
                index++;
            }
            if (ret < 0) {
                ALOGE("Invalid modifier identifier value");
            } else {
                index = 0;
                list_size =
                snd_ucm_get_size_of_list(uc_mgr->card_ctxt_ptr->dev_list_head);
                for (index = 0; index < list_size; index++) {
                    if ((ident1 = snd_ucm_get_value_at_index(
                        uc_mgr->card_ctxt_ptr->dev_list_head, index))) {
                        if (!strncmp(ident1, usecase, MAX_STR_LEN)) {
                            ALOGV("Device already part of enabled list: %s",
                                usecase);
                            free(ident1);
                            break;
                        }
                        free(ident1);
                    }
                }
                if (index == list_size) {
                    ALOGV("enadev: device value to be enabled: %s", usecase);
                    snd_ucm_add_ident_to_list(&uc_mgr->card_ctxt_ptr->dev_list_head,
                         usecase);
                }
                snd_ucm_add_ident_to_list(&uc_mgr->card_ctxt_ptr->mod_list_head,
                    value);
                /* Enable the mixer controls for the new use case
                 * for all the enabled devices */
                ret = set_controls_of_usecase_for_device(uc_mgr, value,
                      usecase, 1, CTRL_LIST_MODIFIER);
            }
        }
    } else if (!strncmp(identifier, "_dismod", 7)) {
        ret = snd_ucm_del_ident_from_list(&uc_mgr->card_ctxt_ptr->mod_list_head,
              value);
        if (ret < 0) {
            ALOGE("Modifier not enabled currently, invalid modifier");
        } else {
            ALOGV("dismod: modifier value to be disabled: %s", value);
            /* Enable the mixer controls for the new use case
             * for all the enabled devices */
            ret = set_controls_of_usecase_for_device(uc_mgr, value, usecase,
                      0, CTRL_LIST_MODIFIER);
        }
    } else {
        ALOGE("Unknown identifier value: %s", identifier);
    }
    pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
    return ret;
}

/**
 * Open and initialise use case core for sound card
 * uc_mgr - Returned use case manager pointer
 * card_name - Sound card name.
 * returns 0 on success, otherwise a negative error code
 */
int snd_use_case_mgr_open(snd_use_case_mgr_t **uc_mgr, const char *card_name)
{
    snd_use_case_mgr_t *uc_mgr_ptr = NULL;
    int index, ret = -EINVAL;
    char tmp[2];

    ALOGV("snd_use_case_open(): card_name %s", card_name);

    if (card_name == NULL) {
        ALOGE("snd_use_case_mgr_open: failed, invalid arguments");
        return ret;
    }

    for (index = 0; index < (int)MAX_NUM_CARDS; index++) {
        if(!strncmp(card_name, card_mapping_list[index].card_name,
           (strlen(card_mapping_list[index].card_name)+1))) {
            ret = 0;
            break;
        }
    }

    if (ret < 0) {
        ALOGE("Card %s not found", card_name);
    } else {
        uc_mgr_ptr = (snd_use_case_mgr_t *)calloc(1,
                         sizeof(snd_use_case_mgr_t));
        if (uc_mgr_ptr == NULL) {
            ALOGE("Failed to allocate memory for instance");
            return -ENOMEM;
        }
        uc_mgr_ptr->snd_card_index = index;
        uc_mgr_ptr->card_ctxt_ptr = (card_ctxt_t *)calloc(1,
                                        sizeof(card_ctxt_t));
        if (uc_mgr_ptr->card_ctxt_ptr == NULL) {
            ALOGE("Failed to allocate memory for card context");
            free(uc_mgr_ptr);
            uc_mgr_ptr = NULL;
            return -ENOMEM;
        }
        uc_mgr_ptr->card_ctxt_ptr->card_number =
            card_mapping_list[index].card_number;
        uc_mgr_ptr->card_ctxt_ptr->card_name =
            (char *)malloc((strlen(card_name)+1)*sizeof(char));
        if (uc_mgr_ptr->card_ctxt_ptr->card_name == NULL) {
            ALOGE("Failed to allocate memory for card name");
            free(uc_mgr_ptr->card_ctxt_ptr);
            free(uc_mgr_ptr);
            uc_mgr_ptr = NULL;
            return -ENOMEM;
        }
        strlcpy(uc_mgr_ptr->card_ctxt_ptr->card_name, card_name,
            ((strlen(card_name)+1)*sizeof(char)));
        uc_mgr_ptr->card_ctxt_ptr->control_device =
            (char *)malloc((strlen("/dev/snd/controlC")+2)*sizeof(char));
        if (uc_mgr_ptr->card_ctxt_ptr->control_device == NULL) {
            ALOGE("Failed to allocate memory for control device string");
            free(uc_mgr_ptr->card_ctxt_ptr->card_name);
            free(uc_mgr_ptr->card_ctxt_ptr);
            free(uc_mgr_ptr);
            uc_mgr_ptr = NULL;
            return -ENOMEM;
        }
        strlcpy(uc_mgr_ptr->card_ctxt_ptr->control_device,
            "/dev/snd/controlC", 18);
        snprintf(tmp, sizeof(tmp), "%d",
            uc_mgr_ptr->card_ctxt_ptr->card_number);
        strlcat(uc_mgr_ptr->card_ctxt_ptr->control_device, tmp,
            (strlen("/dev/snd/controlC")+2)*sizeof(char));
        uc_mgr_ptr->device_list_count = 0;
        uc_mgr_ptr->modifier_list_count = 0;
        uc_mgr_ptr->current_device_list = NULL;
        uc_mgr_ptr->current_modifier_list = NULL;
        uc_mgr_ptr->current_tx_device = -1;
        uc_mgr_ptr->current_rx_device = -1;
        pthread_mutexattr_init(&uc_mgr_ptr->card_ctxt_ptr->card_lock_attr);
        pthread_mutex_init(&uc_mgr_ptr->card_ctxt_ptr->card_lock,
            &uc_mgr_ptr->card_ctxt_ptr->card_lock_attr);
        strlcpy(uc_mgr_ptr->card_ctxt_ptr->current_verb,
                SND_USE_CASE_VERB_INACTIVE, MAX_STR_LEN);
        /* Reset all mixer controls if any applied
         * previously for the same card */
    snd_use_case_mgr_reset(uc_mgr_ptr);
        uc_mgr_ptr->card_ctxt_ptr->current_verb_index = -1;
        /* Parse config files and update mixer controls */
        ret = snd_ucm_parse(&uc_mgr_ptr);
        if(ret < 0) {
            ALOGE("Failed to parse config files: %d", ret);
            snd_ucm_free_mixer_list(&uc_mgr_ptr);
        }
        ALOGV("Open mixer device: %s",
            uc_mgr_ptr->card_ctxt_ptr->control_device);
        uc_mgr_ptr->card_ctxt_ptr->mixer_handle =
            mixer_open(uc_mgr_ptr->card_ctxt_ptr->control_device);
        ALOGV("Mixer handle %p", uc_mgr_ptr->card_ctxt_ptr->mixer_handle);
        *uc_mgr = uc_mgr_ptr;
    }
    ALOGV("snd_use_case_open(): returning instance %p", uc_mgr_ptr);
    return ret;
}


/**
 * \brief Reload and re-parse use case configuration files for sound card.
 * \param uc_mgr Use case manager
 * \return zero if success, otherwise a negative error code
 */
int snd_use_case_mgr_reload(snd_use_case_mgr_t *uc_mgr) {
    ALOGE("Reload is not implemented for now as there is no use case currently");
    return 0;
}

/**
 * \brief Close use case manager
 * \param uc_mgr Use case manager
 * \return zero if success, otherwise a negative error code
 */
int snd_use_case_mgr_close(snd_use_case_mgr_t *uc_mgr)
{
    int ret = 0;

    if ((uc_mgr->snd_card_index >= (int)MAX_NUM_CARDS) ||
        (uc_mgr->snd_card_index < 0) || (uc_mgr->card_ctxt_ptr == NULL)) {
        ALOGE("snd_use_case_mgr_close(): failed, invalid arguments");
        return -EINVAL;
    }

    ALOGV("snd_use_case_close(): instance %p", uc_mgr);
    ret = snd_use_case_mgr_reset(uc_mgr);
    if (ret < 0)
        ALOGE("Failed to reset ucm session");
    snd_ucm_free_mixer_list(&uc_mgr);
    pthread_mutexattr_destroy(&uc_mgr->card_ctxt_ptr->card_lock_attr);
    pthread_mutex_destroy(&uc_mgr->card_ctxt_ptr->card_lock);
    if (uc_mgr->card_ctxt_ptr->mixer_handle) {
        mixer_close(uc_mgr->card_ctxt_ptr->mixer_handle);
        uc_mgr->card_ctxt_ptr->mixer_handle = NULL;
    }
    uc_mgr->snd_card_index = -1;
    uc_mgr->current_tx_device = -1;
    uc_mgr->current_rx_device = -1;
    free(uc_mgr->card_ctxt_ptr->control_device);
    free(uc_mgr->card_ctxt_ptr->card_name);
    free(uc_mgr->card_ctxt_ptr);
    uc_mgr->card_ctxt_ptr = NULL;
    free(uc_mgr);
    uc_mgr = NULL;
    ALOGV("snd_use_case_mgr_close(): card instace closed successfully");
    return ret;
}

/**
 * \brief Reset use case manager verb, device, modifier to deafult settings.
 * \param uc_mgr Use case manager
 * \return zero if success, otherwise a negative error code
 */
int snd_use_case_mgr_reset(snd_use_case_mgr_t *uc_mgr)
{
    char *ident_value;
    int index, list_size, ret = 0;

    ALOGV("snd_use_case_reset(): instance %p", uc_mgr);
    pthread_mutex_lock(&uc_mgr->card_ctxt_ptr->card_lock);
    if ((uc_mgr->snd_card_index >= (int)MAX_NUM_CARDS) ||
        (uc_mgr->snd_card_index < 0) || (uc_mgr->card_ctxt_ptr == NULL)) {
        ALOGE("snd_use_case_mgr_reset(): failed, invalid arguments");
        pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
        return -EINVAL;
    }

    /* Disable mixer controls of all the enabled modifiers */
    list_size = snd_ucm_get_size_of_list(uc_mgr->card_ctxt_ptr->mod_list_head);
    for (index = (list_size-1); index >= 0; index--) {
        if ((ident_value =
            snd_ucm_get_value_at_index(uc_mgr->card_ctxt_ptr->mod_list_head,
                index))) {
            snd_ucm_del_ident_from_list(&uc_mgr->card_ctxt_ptr->mod_list_head,
                ident_value);
            ret = set_controls_of_usecase_for_all_devices(uc_mgr,
                      ident_value, 0, CTRL_LIST_MODIFIER);
        if (ret != 0)
                ALOGE("Failed to disable mixer controls for %s", ident_value);
            free(ident_value);
        }
    }
    /* Clear the enabled modifiers list */
    if (uc_mgr->modifier_list_count) {
        for (index = 0; index < uc_mgr->modifier_list_count; index++) {
            free(uc_mgr->current_modifier_list[index]);
            uc_mgr->current_modifier_list[index] = NULL;
        }
        free(uc_mgr->current_modifier_list);
        uc_mgr->current_modifier_list = NULL;
        uc_mgr->modifier_list_count = 0;
    }
    /* Disable mixer controls of current use case verb */
    if(strncmp(uc_mgr->card_ctxt_ptr->current_verb, SND_USE_CASE_VERB_INACTIVE,
       strlen(SND_USE_CASE_VERB_INACTIVE))) {
        ret = set_controls_of_usecase_for_all_devices(uc_mgr,
                  uc_mgr->card_ctxt_ptr->current_verb, 0, CTRL_LIST_VERB);
        if (ret != 0)
            ALOGE("Failed to disable mixer controls for %s",
                uc_mgr->card_ctxt_ptr->current_verb);
        strlcpy(uc_mgr->card_ctxt_ptr->current_verb, SND_USE_CASE_VERB_INACTIVE,
            MAX_STR_LEN);
    }
    /* Disable mixer controls of all the enabled devices */
    list_size = snd_ucm_get_size_of_list(uc_mgr->card_ctxt_ptr->dev_list_head);
    for (index = (list_size-1); index >= 0; index--) {
        if ((ident_value =
            snd_ucm_get_value_at_index(uc_mgr->card_ctxt_ptr->dev_list_head,
                index))) {
            snd_ucm_del_ident_from_list(&uc_mgr->card_ctxt_ptr->dev_list_head,
                ident_value);
            ret = set_controls_of_device_for_all_usecases(uc_mgr,
                      ident_value, 0);
        if (ret != 0)
                ALOGE("Failed to disable or no mixer controls set for %s",
                    ident_value);
        free(ident_value);
        }
    }
    /* Clear the enabled devices list */
    if (uc_mgr->device_list_count) {
        for (index = 0; index < uc_mgr->device_list_count; index++) {
            free(uc_mgr->current_device_list[index]);
            uc_mgr->current_device_list[index] = NULL;
        }
        free(uc_mgr->current_device_list);
        uc_mgr->current_device_list = NULL;
        uc_mgr->device_list_count = 0;
    }
    uc_mgr->current_tx_device = -1;
    uc_mgr->current_rx_device = -1;
    pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
    return ret;
}

/* 2nd stage parsing done in seperate thread */
void *second_stage_parsing_thread(void *uc_mgr_ptr)
{
    use_case_verb_t *verb_list;
    char path[200];
    struct stat st;
    int fd, index = 0, ret = 0, rc = 0;
    char *read_buf = NULL, *next_str = NULL, *current_str = NULL, *buf = NULL;
    char *p = NULL, *verb_name = NULL, *file_name = NULL, *temp_ptr = NULL;
    snd_use_case_mgr_t **uc_mgr = (snd_use_case_mgr_t **)&uc_mgr_ptr;

    strlcpy(path, CONFIG_DIR, (strlen(CONFIG_DIR)+1));
    strlcat(path, (*uc_mgr)->card_ctxt_ptr->card_name, sizeof(path));
    ALOGV("master config file path:%s", path);
    fd = open(path, O_RDONLY);
    if (fd < 0) {
        ALOGE("failed to open config file %s error %d\n", path, errno);
        return NULL;
    }
    if (fstat(fd, &st) < 0) {
        ALOGE("failed to stat %s error %d\n", path, errno);
        close(fd);
        return NULL;
    }
    read_buf = (char *) mmap(0, st.st_size, PROT_READ | PROT_WRITE,
               MAP_PRIVATE, fd, 0);
    if (read_buf == MAP_FAILED) {
        ALOGE("failed to mmap file error %d\n", errno);
        close(fd);
        return NULL;
    }
    current_str = read_buf;
    verb_name = NULL;
    while (*current_str != (char)EOF)  {
        next_str = strchr(current_str, '\n');
        if (!next_str)
            break;
        *next_str++ = '\0';
        if (verb_name == NULL) {
            buf = strstr(current_str, "SectionUseCase");
            if (buf == NULL) {
                if((current_str = next_str) == NULL)
                    break;
                else
                    continue;
            }
            /* Ignore parsing first use case (HiFi) as it is already parsed
             * in 1st stage of parsing */
            if (index == 0) {
                index++;
                if((current_str = next_str) == NULL)
                    break;
                else
                    continue;
            }
            p = strtok_r(buf, ".", &temp_ptr);
            while (p != NULL) {
                p = strtok_r(NULL, "\"", &temp_ptr);
                if (p == NULL)
                    break;
                verb_name = (char *)malloc((strlen(p)+1)*sizeof(char));
                if(verb_name == NULL) {
                    ret = -ENOMEM;
                    break;
                }
                strlcpy(verb_name, p, (strlen(p)+1)*sizeof(char));
                break;
            }
        } else {
            buf = strstr(current_str, "File");
            if (buf == NULL) {
                if((current_str = next_str) == NULL)
                    break;
                else
                    continue;
            }
            p = strtok_r(buf, "\"", &temp_ptr);
            while (p != NULL) {
                p = strtok_r(NULL, "\"", &temp_ptr);
                if (p == NULL)
                    break;
                file_name = (char *)malloc((strlen(p)+1)*sizeof(char));
                if(file_name == NULL) {
                    ret = -ENOMEM;
                    break;
                }
                strlcpy(file_name, p, (strlen(p)+1)*sizeof(char));
                break;
            }
            verb_list = (*uc_mgr)->card_ctxt_ptr->use_case_verb_list;
            if (file_name != NULL) {
                ret = snd_ucm_parse_verb(uc_mgr, file_name, index);
                verb_list[index].use_case_name =
                    (char *)malloc((strlen(verb_name)+1)*sizeof(char));
                strlcpy(verb_list[index].use_case_name, verb_name,
                    ((strlen(verb_name)+1)*sizeof(char)));
                /* Verb list might have been appended with END OF LIST in
                 * 1st stage parsing. Delete this entry so that new verbs
                 * are appended from here and END OF LIST will be added
                 * again at the end of 2nd stage parsing
                 */
                if((*uc_mgr)->card_ctxt_ptr->verb_list[index]) {
                    free((*uc_mgr)->card_ctxt_ptr->verb_list[index]);
                    (*uc_mgr)->card_ctxt_ptr->verb_list[index] = NULL;
                }
                (*uc_mgr)->card_ctxt_ptr->verb_list[index] =
                    (char *)malloc((strlen(verb_name)+1)*sizeof(char));
                strlcpy((*uc_mgr)->card_ctxt_ptr->verb_list[index], verb_name,
                    ((strlen(verb_name)+1)*sizeof(char)));
                free(verb_name);
                verb_name = NULL;
                free(file_name);
                file_name = NULL;
            }
            index++;
            (*uc_mgr)->card_ctxt_ptr->verb_list[index] =
                (char *)malloc((strlen(SND_UCM_END_OF_LIST)+1)*sizeof(char));
            strlcpy((*uc_mgr)->card_ctxt_ptr->verb_list[index],
                 SND_UCM_END_OF_LIST,
                 ((strlen(SND_UCM_END_OF_LIST)+1)*sizeof(char)));
        }
        if((current_str = next_str) == NULL)
            break;
    }
    if (verb_name != NULL) {
        free(verb_name);
        verb_name = NULL;
    }
    if (file_name != NULL) {
        free(file_name);
        file_name = NULL;
    }
    munmap(read_buf, st.st_size);
    close(fd);
#if PARSE_DEBUG
        /* Prints use cases and mixer controls parsed from config files */
        snd_ucm_print((*uc_mgr));
#endif
    if(ret < 0)
        ALOGE("Failed to parse config files: %d", ret);
    ALOGE("Exiting parsing thread uc_mgr %p\n", uc_mgr);
    return NULL;
}

/* Function can be used by UCM clients to wait until parsing completes
 * uc_mgr - use case manager structure
 * Returns 0 on success, error number otherwise
*/
int snd_use_case_mgr_wait_for_parsing(snd_use_case_mgr_t *uc_mgr)
{
    int ret;

    ret = pthread_join(uc_mgr->thr, NULL);
    return ret;
}

/* Parse config files and update mixer controls for the use cases
 * 1st stage parsing done to parse HiFi config file
 * uc_mgr - use case manager structure
 * Returns 0 on sucess, negative error code otherwise
 */
static int snd_ucm_parse(snd_use_case_mgr_t **uc_mgr)
{
    use_case_verb_t *verb_list;
    struct stat st;
    int fd, verb_count, index = 0, ret = 0, rc;
    char *read_buf, *next_str, *current_str, *buf, *p, *verb_name;
    char *file_name = NULL, *temp_ptr;
    char path[200];

    strlcpy(path, CONFIG_DIR, (strlen(CONFIG_DIR)+1));
    strlcat(path, (*uc_mgr)->card_ctxt_ptr->card_name, sizeof(path));
    ALOGV("master config file path:%s", path);
    fd = open(path, O_RDONLY);
    if (fd < 0) {
        ALOGE("failed to open config file %s error %d\n", path, errno);
        return -EINVAL;
    }
    if (fstat(fd, &st) < 0) {
        ALOGE("failed to stat %s error %d\n", path, errno);
        close(fd);
        return -EINVAL;
    }
    read_buf = (char *) mmap(0, st.st_size, PROT_READ | PROT_WRITE,
               MAP_PRIVATE, fd, 0);
    if (read_buf == MAP_FAILED) {
        ALOGE("failed to mmap file error %d\n", errno);
        close(fd);
        return -EINVAL;
    }
    current_str = read_buf;
    verb_count = get_verb_count(current_str);
    (*uc_mgr)->card_ctxt_ptr->use_case_verb_list =
        (use_case_verb_t *)malloc((verb_count+1)*(sizeof(use_case_verb_t)));
    if ((*uc_mgr)->card_ctxt_ptr->use_case_verb_list == NULL) {
        ALOGE("failed to allocate memory for use case verb list\n");
        munmap(read_buf, st.st_size);
        close(fd);
        return -ENOMEM;
    }
    if (((*uc_mgr)->card_ctxt_ptr->verb_list =
        (char **)malloc((verb_count+2)*(sizeof(char *)))) == NULL) {
        ALOGE("failed to allocate memory for verb list\n");
        munmap(read_buf, st.st_size);
        close(fd);
        return -ENOMEM;
    }
    verb_name = NULL;
    if ((ret = is_single_config_format(current_str))) {
        ALOGD("Single config file format detected\n");
        ret = parse_single_config_format(uc_mgr, current_str, verb_count);
        munmap(read_buf, st.st_size);
        close(fd);
        return ret;
    }
    while (*current_str != (char)EOF)  {
        next_str = strchr(current_str, '\n');
        if (!next_str)
            break;
        *next_str++ = '\0';
        if (verb_name == NULL) {
            buf = strstr(current_str, "SectionUseCase");
            if (buf == NULL) {
                if((current_str = next_str) == NULL)
                    break;
                else
                    continue;
            }
            verb_list = (*uc_mgr)->card_ctxt_ptr->use_case_verb_list;
            p = strtok_r(buf, ".", &temp_ptr);
            while (p != NULL) {
                p = strtok_r(NULL, "\"", &temp_ptr);
                if (p == NULL)
                    break;
                verb_name = (char *)malloc((strlen(p)+1)*sizeof(char));
                if(verb_name == NULL) {
                    ret = -ENOMEM;
                    break;
                }
                strlcpy(verb_name, p, (strlen(p)+1)*sizeof(char));
                if ((verb_list[index].use_case_name =
                    (char *)malloc((strlen(verb_name)+1)*sizeof(char)))) {
                    strlcpy(verb_list[index].use_case_name,
                        verb_name, ((strlen(verb_name)+1)*sizeof(char)));
                } else {
                    ret = -ENOMEM;
                    break;
                }
                if (((*uc_mgr)->card_ctxt_ptr->verb_list[index] =
                    (char *)malloc((strlen(verb_name)+1)*sizeof(char)))) {
                    strlcpy((*uc_mgr)->card_ctxt_ptr->verb_list[index],
                        verb_name, ((strlen(verb_name)+1)*sizeof(char)));
                } else {
                    ret = -ENOMEM;
                    break;
                }
                break;
            }
        } else {
            buf = strstr(current_str, "File");
            if (buf == NULL) {
                if((current_str = next_str) == NULL)
                    break;
                else
                    continue;
            }
            p = strtok_r(buf, "\"", &temp_ptr);
            while (p != NULL) {
                p = strtok_r(NULL, "\"", &temp_ptr);
                if (p == NULL)
                    break;
                file_name = (char *)malloc((strlen(p)+1)*sizeof(char));
                if(file_name == NULL) {
                    ret = -ENOMEM;
                    break;
                }
                strlcpy(file_name, p, (strlen(p)+1)*sizeof(char));
                break;
            }
            if (file_name != NULL) {
                ret = snd_ucm_parse_verb(uc_mgr, file_name, index);
                if (ret < 0)
                    ALOGE("Failed to parse config file %s\n", file_name);
                free(verb_name);
                verb_name = NULL;
                free(file_name);
                file_name = NULL;
            }
            index++;
            /* Break here so that only one first use case config file (HiFi)
             * from master config file is parsed initially and all other
             * config files are parsed in seperate thread created below so
             * that audio HAL can initialize faster during boot-up
             */
            break;
        }
        if((current_str = next_str) == NULL)
            break;
    }
    munmap(read_buf, st.st_size);
    close(fd);
    if (((*uc_mgr)->card_ctxt_ptr->verb_list[index] =
        (char *)malloc((strlen(SND_UCM_END_OF_LIST)+1)*sizeof(char)))) {
        strlcpy((*uc_mgr)->card_ctxt_ptr->verb_list[index], SND_UCM_END_OF_LIST,
                ((strlen(SND_UCM_END_OF_LIST)+1)*sizeof(char)));
    } else {
        ALOGE("Failed to allocate memory\n");
        ret = -ENOMEM;
    }
    if (!ret) {
        ALOGD("Creating Parsing thread uc_mgr %p\n", uc_mgr);
        rc = pthread_create(&(*uc_mgr)->thr, 0, second_stage_parsing_thread,
                 (void*)(*uc_mgr));
        if(rc < 0) {
            ALOGE("Failed to create parsing thread rc %d errno %d\n", rc, errno);
        } else {
            ALOGV("Prasing thread created successfully\n");
        }
    }
    if (verb_name)
        free(verb_name);
    if (file_name)
        free(file_name);
    return ret;
}

/* Parse a single config file format
 * uc_mgr - use case manager structure
 * buf - config file buffer to be parsed
 * Returns 0 on sucess, negative error code otherwise
 */
static int parse_single_config_format(snd_use_case_mgr_t **uc_mgr,
char *current_str, int num_verbs)
{
    struct stat st;
    card_mctrl_t *list;
    use_case_verb_t *verb_list;
    int verb_count = 0, device_count = 0, mod_count = 0, index = -1, ret = 0;
    char *next_str, *buf, *p, *verb_ptr, *temp_ptr;

    verb_list = (*uc_mgr)->card_ctxt_ptr->use_case_verb_list;
    while (*current_str != (char)EOF)  {
        next_str = strchr(current_str, '\n');
        if (!next_str)
            break;
        *next_str++ = '\0';
        if ((buf = strcasestr(current_str, "SectionUseCase")) != NULL) {
            if (index != -1) {
                list = (verb_list[index].verb_ctrls +
                            verb_list[index].verb_count);
                list->case_name = (char *)
                    malloc((strlen(SND_UCM_END_OF_LIST)+1)*sizeof(char));
                if(list->case_name == NULL) {
                    free(verb_list[index].verb_ctrls);
                    return -ENOMEM;
                }
                strlcpy(list->case_name, SND_UCM_END_OF_LIST,
                   (strlen(SND_UCM_END_OF_LIST)+1)*sizeof(char));
                list->ena_mixer_list = NULL;
                list->dis_mixer_list = NULL;
                list->ena_mixer_count = 0;
                list->dis_mixer_count = 0;
                list->playback_dev_name = NULL;
                list->capture_dev_name = NULL;
                list->acdb_id = 0;
                list->capability = 0;
            }
            index++;
            p = strtok_r(buf, ".", &temp_ptr);
            while (p != NULL) {
                p = strtok_r(NULL, "\"", &temp_ptr);
                if (p == NULL)
                    break;
                if ((verb_list[index].use_case_name =
                    (char *)malloc((strlen(p)+1)*sizeof(char)))) {
                    strlcpy(verb_list[index].use_case_name,
                        p, ((strlen(p)+1)*sizeof(char)));
                } else {
                    ret = -ENOMEM;
                    break;
                }
                if (((*uc_mgr)->card_ctxt_ptr->verb_list[index] =
                    (char *)malloc((strlen(p)+1)*sizeof(char)))) {
                    strlcpy((*uc_mgr)->card_ctxt_ptr->verb_list[index],
                       p, ((strlen(p)+1)*sizeof(char)));
                } else {
                    ret = -ENOMEM;
                    break;
                }
                break;
            }
            verb_list[index].verb_count = 0;
            verb_list[index].device_count = 0;
            verb_list[index].mod_count = 0;
            verb_list[index].device_list = NULL;
            verb_list[index].modifier_list = NULL;
            verb_list[index].verb_ctrls = NULL;
            verb_list[index].device_ctrls = NULL;
            verb_list[index].mod_ctrls = NULL;
            verb_count = get_num_verbs_config_format(next_str);
            verb_list[index].verb_ctrls = (card_mctrl_t *)
                malloc((verb_count+1)*sizeof(card_mctrl_t));
            if (verb_list[index].verb_ctrls == NULL) {
               ret = -ENOMEM;
               break;
            }
            verb_list[index].verb_count = 0;
        } else if (!strncasecmp(current_str, "SectionVerb", 11)) {
            ret = snd_ucm_parse_section(uc_mgr, &current_str,
                    &next_str, index, CTRL_LIST_VERB);
            if (ret < 0)
                break;
        } else if (!strncasecmp(current_str, "SectionDevice", 13)) {
            if (device_count == 0) {
                device_count = get_num_device_config_format(next_str);
                verb_list[0].device_ctrls = (card_mctrl_t *)
                    malloc((device_count+1)*sizeof(card_mctrl_t));
                if (verb_list[0].device_ctrls == NULL) {
                    ret = -ENOMEM;
                    break;
                }
                verb_list[0].device_list =
                    (char **)malloc((device_count+1)*sizeof(char *));
                if (verb_list[0].device_list == NULL)
                    return -ENOMEM;
                verb_list[0].device_count = 0;
            }
            ret = snd_ucm_parse_section(uc_mgr, &current_str,
                      &next_str, 0, CTRL_LIST_DEVICE);
            if (ret < 0) {
                break;
            } else {
                list = (verb_list[0].device_ctrls +
                           (verb_list[0].device_count - 1));
                verb_ptr = (char *)
                    malloc((strlen(list->case_name)+1)*sizeof(char));
                    if (verb_ptr == NULL) {
                        ret = -ENOMEM;
                        break;
                    }
                    strlcpy(verb_ptr, list->case_name,
                        ((strlen(list->case_name)+1)*sizeof(char)));
                    verb_list[0].device_list[(verb_list[0].device_count-1)]
                        = verb_ptr;
            }
        } else if (!strncasecmp(current_str, "SectionModifier", 15)) {
            if (mod_count == 0) {
                mod_count = get_num_mod_config_format(next_str);
                verb_list[0].mod_ctrls = (card_mctrl_t *)
                    malloc((mod_count+1)*sizeof(card_mctrl_t));
                if (verb_list[0].mod_ctrls == NULL) {
                    ret = -ENOMEM;
                    break;
                }
                verb_list[0].modifier_list =
                    (char **)malloc((mod_count+1)*sizeof(char *));
                if (verb_list[0].modifier_list == NULL)
                    return -ENOMEM;
                verb_list[0].mod_count = 0;
            }
            ret = snd_ucm_parse_section(uc_mgr, &current_str,
                     &next_str, 0, CTRL_LIST_MODIFIER);
            if (ret < 0) {
                break;
            } else {
                list = (verb_list[0].mod_ctrls +
                        (verb_list[0].mod_count - 1));
                verb_ptr = (char *)
                    malloc((strlen(list->case_name)+1)*sizeof(char));
                if (verb_ptr == NULL) {
                    ret = -ENOMEM;
                    break;
                }
                strlcpy(verb_ptr, list->case_name,
                   ((strlen(list->case_name)+1)*sizeof(char)));
                verb_list[0].modifier_list[(verb_list[0].mod_count - 1)]
                    = verb_ptr;
            }
        }
        if((current_str = next_str) == NULL)
            break;
    }
    list = (verb_list[index].verb_ctrls +
            verb_list[index].verb_count);
    list->case_name =
        (char *)malloc((strlen(SND_UCM_END_OF_LIST)+1)*sizeof(char));
    if(list->case_name == NULL) {
        free(verb_list[index].verb_ctrls);
        return -ENOMEM;
    }
    strlcpy(list->case_name, SND_UCM_END_OF_LIST,
        (strlen(SND_UCM_END_OF_LIST)+1)*sizeof(char));
    list->ena_mixer_list = NULL;
    list->dis_mixer_list = NULL;
    list->ena_mixer_count = 0;
    list->dis_mixer_count = 0;
    list->playback_dev_name = NULL;
    list->capture_dev_name = NULL;
    list->acdb_id = 0;
    list->capability = 0;
    index++;
    if (index != -1) {
        if (((*uc_mgr)->card_ctxt_ptr->verb_list[index] =
            (char *)malloc((strlen(SND_UCM_END_OF_LIST)+1)*sizeof(char)))) {
            strlcpy((*uc_mgr)->card_ctxt_ptr->verb_list[index],
                SND_UCM_END_OF_LIST,
                ((strlen(SND_UCM_END_OF_LIST)+1)*sizeof(char)));
        } else {
            ALOGE("Failed to allocate memory\n");
            ret = -ENOMEM;
        }
    }
    /* Add end of list to device list */
    verb_ptr =
       (char *)malloc((strlen(SND_UCM_END_OF_LIST)+1)*sizeof(char));
    if (verb_ptr == NULL)
        return -ENOMEM;
    strlcpy(verb_ptr, SND_UCM_END_OF_LIST,
        ((strlen(SND_UCM_END_OF_LIST)+1)*sizeof(char)));
    verb_list[0].device_list[verb_list[0].device_count] = verb_ptr;
    /* Add end of list to modifier list */
    verb_ptr =
    (char *)malloc((strlen(SND_UCM_END_OF_LIST)+1)*sizeof(char));
        if (verb_ptr == NULL)
            return -ENOMEM;
    strlcpy(verb_ptr, SND_UCM_END_OF_LIST,
        ((strlen(SND_UCM_END_OF_LIST)+1)*sizeof(char)));
    verb_list[0].modifier_list[verb_list[0].mod_count] = verb_ptr;
    /* Add end of list to device controls list */
    list = (verb_list[0].device_ctrls +
               verb_list[0].device_count);
    list->case_name =
        (char *)malloc((strlen(SND_UCM_END_OF_LIST)+1)*sizeof(char));
    if(list->case_name == NULL) {
        free(verb_list[0].device_ctrls);
        return -ENOMEM;
    }
    strlcpy(list->case_name, SND_UCM_END_OF_LIST,
        (strlen(SND_UCM_END_OF_LIST)+1)*sizeof(char));
    list->ena_mixer_list = NULL;
    list->dis_mixer_list = NULL;
    list->ena_mixer_count = 0;
    list->dis_mixer_count = 0;
    list->playback_dev_name = NULL;
    list->capture_dev_name = NULL;
    list->acdb_id = 0;
    list->capability = 0;
    /* Add end of list to modifier controls list */
    list = (verb_list[0].mod_ctrls +
        verb_list[0].mod_count);
    list->case_name =
        (char *)malloc((strlen(SND_UCM_END_OF_LIST)+1)*sizeof(char));
    if(list->case_name == NULL) {
        free(verb_list[0].mod_ctrls);
        return -ENOMEM;
    }
    strlcpy(list->case_name, SND_UCM_END_OF_LIST,
        (strlen(SND_UCM_END_OF_LIST)+1)*sizeof(char));
    list->ena_mixer_list = NULL;
    list->dis_mixer_list = NULL;
    list->ena_mixer_count = 0;
    list->dis_mixer_count = 0;
    list->playback_dev_name = NULL;
    list->capture_dev_name = NULL;
    list->acdb_id = 0;
    list->capability = 0;
    for (index = 1; index < num_verbs; index++) {
        verb_list[index].device_ctrls = verb_list[0].device_ctrls;
        verb_list[index].device_list = verb_list[0].device_list;
        verb_list[index].device_count = verb_list[0].device_count;
        verb_list[index].mod_ctrls = verb_list[0].mod_ctrls;
        verb_list[index].modifier_list = verb_list[0].modifier_list;
        verb_list[index].mod_count = verb_list[0].mod_count;
    }
    if (ret < 0) {
        ALOGE("Failed to parse config file ret %d errno %d\n", ret, errno);
    } else {
        ALOGV("Prasing done successfully\n");
#if PARSE_DEBUG
        /* Prints use cases and mixer controls parsed from config files */
        snd_ucm_print((*uc_mgr));
#endif
    }
    return ret;
}

/* Returns number of verb sections for specific use case verb*/
static int get_num_verbs_config_format(const char *nxt_str)
{
    char *current_str, *next_str, *str_addr, *buf;
    int count = 0;

    next_str = (char *)malloc((strlen(nxt_str)+1)*sizeof(char));
    if (next_str == NULL) {
        ALOGE("Failed to allocate memory");
        return -ENOMEM;
    }
    strlcpy(next_str, nxt_str, ((strlen(nxt_str)+1)*sizeof(char)));
    str_addr = next_str;
    current_str = next_str;
    while(1) {
        next_str = strchr(current_str, '\n');
        if (!next_str)
            break;
        *next_str++ = '\0';
        buf = strcasestr(current_str, "SectionUseCase");
        if (buf != NULL)
            break;
        buf = strcasestr(current_str, "SectionVerb");
        if (buf != NULL)
            count++;
        if (*next_str == (char)EOF)
            break;
        if((current_str = next_str) == NULL)
            break;
    }
    free(str_addr);
    return count;
}

/* Returns number of common device sections for all use case verbs*/
static int get_num_device_config_format(const char *nxt_str)
{
    char *current_str, *next_str, *str_addr, *buf;
    int count = 1;

    next_str = (char *)malloc((strlen(nxt_str)+1)*sizeof(char));
    if (next_str == NULL) {
        ALOGE("Failed to allocate memory");
        return -ENOMEM;
    }
    strlcpy(next_str, nxt_str, ((strlen(nxt_str)+1)*sizeof(char)));
    str_addr = next_str;
    current_str = next_str;
    while(1) {
        next_str = strchr(current_str, '\n');
        if (!next_str)
            break;
        *next_str++ = '\0';
        buf = strcasestr(current_str, "SectionDevice");
        if (buf != NULL)
            count++;
        if (*next_str == (char)EOF)
            break;
        if((current_str = next_str) == NULL)
            break;
    }
    free(str_addr);
    return count;
}

/* Returns number of common modifier sections for all use case verbs*/
static int get_num_mod_config_format(const char *nxt_str)
{
    char *current_str, *next_str, *str_addr, *buf;
    int count = 1;

    next_str = (char *)malloc((strlen(nxt_str)+1)*sizeof(char));
    if (next_str == NULL) {
        ALOGE("Failed to allocate memory");
        return -ENOMEM;
    }
    strlcpy(next_str, nxt_str, ((strlen(nxt_str)+1)*sizeof(char)));
    str_addr = next_str;
    current_str = next_str;
    while(1) {
        next_str = strchr(current_str, '\n');
        if (!next_str)
            break;
        *next_str++ = '\0';
        buf = strcasestr(current_str, "SectionModifier");
        if (buf != NULL)
            count++;
        if (*next_str == (char)EOF)
            break;
        if((current_str = next_str) == NULL)
            break;
    }
    free(str_addr);
    return count;
}

/* Gets the number of use case verbs defined by master config file */
static int get_verb_count(const char *nxt_str)
{
    char *current_str, *next_str, *str_addr, *buf, *p;
    int count = 0;

    next_str = (char *)malloc((strlen(nxt_str)+1)*sizeof(char));
    if (next_str == NULL) {
        ALOGE("Failed to allocate memory");
        return -ENOMEM;
    }
    strlcpy(next_str, nxt_str, ((strlen(nxt_str)+1)*sizeof(char)));
    str_addr = next_str;
    current_str = next_str;
    while(1) {
        next_str = strchr(current_str, '\n');
        if (!next_str)
            break;
        *next_str++ = '\0';
        buf = strstr(current_str, "SectionUseCase");
        if (buf != NULL)
            count++;
        if (*next_str == (char)EOF)
            break;
        if((current_str = next_str) == NULL)
            break;
    }
    free(str_addr);
    return count;
}

/* Returns one if single config file per sound card format is being used */
static int is_single_config_format(const char *nxt_str)
{
    char *current_str, *next_str, *str_addr, *buf;
    int ret = 1, count = 0;

    next_str = (char *)malloc((strlen(nxt_str)+1)*sizeof(char));
    if (next_str == NULL) {
        ALOGE("Failed to allocate memory");
        return -ENOMEM;
    }
    strlcpy(next_str, nxt_str, ((strlen(nxt_str)+1)*sizeof(char)));
    str_addr = next_str;
    current_str = next_str;
    while(1) {
        next_str = strchr(current_str, '\n');
        if (!next_str)
            break;
        *next_str++ = '\0';
        buf = strstr(current_str, "SectionUseCase");
        if (buf != NULL)
            count++;
        buf = strstr(current_str, "File");
        if (buf != NULL)
            ret = 0;
        if ((*next_str == (char)EOF) || (count == 2))
            break;
        if((current_str = next_str) == NULL)
            break;
    }
    free(str_addr);
    return ret;
}

/* Parse a use case verb config files and update mixer controls for the verb
 * uc_mgr - use case manager structure
 * file_name - use case verb config file name
 * index - index of the verb in the list
 * Returns 0 on sucess, negative error code otherwise
 */
static int snd_ucm_parse_verb(snd_use_case_mgr_t **uc_mgr,
const char *file_name, int index)
{
    struct stat st;
    card_mctrl_t *list;
    int device_count, modifier_count;
    int fd, ret = 0, parse_count = 0;
    char *read_buf, *next_str, *current_str, *verb_ptr;
    char path[200];
    use_case_verb_t *verb_list;

    strlcpy(path, CONFIG_DIR, (strlen(CONFIG_DIR)+1));
    strlcat(path, file_name, sizeof(path));
    ALOGV("path:%s", path);
    verb_list = (*uc_mgr)->card_ctxt_ptr->use_case_verb_list;
    while(1) {
        device_count = 0; modifier_count = 0;
        if (parse_count == 0) {
            verb_list[index].verb_count = 0;
            verb_list[index].device_count = 0;
            verb_list[index].mod_count = 0;
            verb_list[index].device_list = NULL;
            verb_list[index].modifier_list = NULL;
            verb_list[index].verb_ctrls = NULL;
            verb_list[index].device_ctrls = NULL;
            verb_list[index].mod_ctrls = NULL;
        }
        fd = open(path, O_RDONLY);
        if (fd < 0) {
             ALOGE("failed to open config file %s error %d\n", path, errno);
             return -EINVAL;
        }
        if (fstat(fd, &st) < 0) {
            ALOGE("failed to stat %s error %d\n", path, errno);
            close(fd);
            return -EINVAL;
        }
        read_buf = (char *) mmap(0, st.st_size, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE, fd, 0);
        if (read_buf == MAP_FAILED) {
            ALOGE("failed to mmap file error %d\n", errno);
            close(fd);
            return -EINVAL;
        }
        current_str = read_buf;
        while (*current_str != (char)EOF)  {
            next_str = strchr(current_str, '\n');
            if (!next_str)
                break;
            *next_str++ = '\0';
            if (!strncasecmp(current_str, "SectionVerb", 11)) {
                if (parse_count == 0) {
                    verb_list[index].verb_count++;
                } else {
                    ret = snd_ucm_parse_section(uc_mgr, &current_str,
                              &next_str, index, CTRL_LIST_VERB);
                    if (ret < 0)
                        break;
                }
            } else if (!strncasecmp(current_str, "SectionDevice", 13)) {
                if (parse_count == 0) {
                    verb_list[index].device_count++;
                    device_count++;
                } else {
                    ret = snd_ucm_parse_section(uc_mgr, &current_str,
                              &next_str, index, CTRL_LIST_DEVICE);
                    if (ret < 0) {
                        break;
                    } else {
                        list = (verb_list[index].device_ctrls +
                                   (verb_list[index].device_count - 1));
                        verb_ptr = (char *)
                            malloc((strlen(list->case_name)+1)*sizeof(char));
                        if (verb_ptr == NULL) {
                            ret = -ENOMEM;
                            break;
                        }
                        strlcpy(verb_ptr, list->case_name,
                            ((strlen(list->case_name)+1)*sizeof(char)));
                        verb_list[index].device_list[device_count] = verb_ptr;
                        device_count++;
                    }
                }
            } else if (!strncasecmp(current_str, "SectionModifier", 15)) {
                if (parse_count == 0) {
                    verb_list[index].mod_count++;
                    modifier_count++;
                } else {
                    ret = snd_ucm_parse_section(uc_mgr, &current_str,
                              &next_str, index, CTRL_LIST_MODIFIER);
                    if (ret < 0) {
                        break;
                    } else {
                        list = (verb_list[index].mod_ctrls +
                               (verb_list[index].mod_count - 1));
                        verb_ptr = (char *)
                            malloc((strlen(list->case_name)+1)*sizeof(char));
                        if (verb_ptr == NULL) {
                            ret = -ENOMEM;
                            break;
                        }
                        strlcpy(verb_ptr, list->case_name,
                            ((strlen(list->case_name)+1)*sizeof(char)));
                        verb_list[index].modifier_list[modifier_count]
                            = verb_ptr;
                        modifier_count++;
                    }
                }
            }
            if((current_str = next_str) == NULL)
                break;
        }
        munmap(read_buf, st.st_size);
        close(fd);
        if(ret < 0)
            return ret;
        if (parse_count == 0) {
            verb_list[index].device_list =
                (char **)malloc((device_count+1)*sizeof(char *));
            if (verb_list[index].device_list == NULL)
                return -ENOMEM;
            verb_list[index].modifier_list =
                (char **)malloc((modifier_count+1)*sizeof(char *));
            if (verb_list[index].modifier_list == NULL)
                return -ENOMEM;
            parse_count += verb_list[index].verb_count;
            verb_list[index].verb_ctrls = (card_mctrl_t *)
                malloc((verb_list[index].verb_count+1)*sizeof(card_mctrl_t));
            if (verb_list[index].verb_ctrls == NULL) {
               ret = -ENOMEM;
               break;
            }
            verb_list[index].verb_count = 0;
            parse_count += verb_list[index].device_count;
            verb_list[index].device_ctrls = (card_mctrl_t *)
                malloc((verb_list[index].device_count+1)*sizeof(card_mctrl_t));
            if (verb_list[index].device_ctrls == NULL) {
               ret = -ENOMEM;
               break;
            }
            verb_list[index].device_count = 0;
            parse_count += verb_list[index].mod_count;
            verb_list[index].mod_ctrls = (card_mctrl_t *)
                malloc((verb_list[index].mod_count+1)*sizeof(card_mctrl_t));
            if (verb_list[index].mod_ctrls == NULL) {
               ret = -ENOMEM;
               break;
            }
            verb_list[index].mod_count = 0;
            continue;
        } else {
            verb_ptr =
            (char *)malloc((strlen(SND_UCM_END_OF_LIST)+1)*sizeof(char));
            if (verb_ptr == NULL)
                return -ENOMEM;
            strlcpy(verb_ptr, SND_UCM_END_OF_LIST,
                ((strlen(SND_UCM_END_OF_LIST)+1)*sizeof(char)));
            verb_list[index].device_list[device_count] = verb_ptr;
            verb_ptr =
            (char *)malloc((strlen(SND_UCM_END_OF_LIST)+1)*sizeof(char));
            if (verb_ptr == NULL)
                return -ENOMEM;
            strlcpy(verb_ptr, SND_UCM_END_OF_LIST,
                ((strlen(SND_UCM_END_OF_LIST)+1)*sizeof(char)));
            verb_list[index].modifier_list[modifier_count] = verb_ptr;
            list = (verb_list[index].verb_ctrls +
                       verb_list[index].verb_count);
            list->case_name =
                 (char *)malloc((strlen(SND_UCM_END_OF_LIST)+1)*sizeof(char));
            if(list->case_name == NULL) {
                free(verb_list[index].verb_ctrls);
                return -ENOMEM;
            }
            strlcpy(list->case_name, SND_UCM_END_OF_LIST,
               (strlen(SND_UCM_END_OF_LIST)+1)*sizeof(char));
            list->ena_mixer_list = NULL;
            list->dis_mixer_list = NULL;
            list->ena_mixer_count = 0;
            list->dis_mixer_count = 0;
            list->playback_dev_name = NULL;
            list->capture_dev_name = NULL;
            list->acdb_id = 0;
            list->capability = 0;
            list = (verb_list[index].device_ctrls +
                       verb_list[index].device_count);
            list->case_name =
                 (char *)malloc((strlen(SND_UCM_END_OF_LIST)+1)*sizeof(char));
            if(list->case_name == NULL) {
                free(verb_list[index].device_ctrls);
                return -ENOMEM;
            }
            strlcpy(list->case_name, SND_UCM_END_OF_LIST,
               (strlen(SND_UCM_END_OF_LIST)+1)*sizeof(char));
            list->ena_mixer_list = NULL;
            list->dis_mixer_list = NULL;
            list->ena_mixer_count = 0;
            list->dis_mixer_count = 0;
            list->playback_dev_name = NULL;
            list->capture_dev_name = NULL;
            list->acdb_id = 0;
            list->capability = 0;
            list = (verb_list[index].mod_ctrls +
                       verb_list[index].mod_count);
            list->case_name =
                 (char *)malloc((strlen(SND_UCM_END_OF_LIST)+1)*sizeof(char));
            if(list->case_name == NULL) {
                free(verb_list[index].mod_ctrls);
                return -ENOMEM;
            }
            strlcpy(list->case_name, SND_UCM_END_OF_LIST,
               (strlen(SND_UCM_END_OF_LIST)+1)*sizeof(char));
            list->ena_mixer_list = NULL;
            list->dis_mixer_list = NULL;
            list->ena_mixer_count = 0;
            list->dis_mixer_count = 0;
            list->playback_dev_name = NULL;
            list->capture_dev_name = NULL;
            list->acdb_id = 0;
            list->capability = 0;
            parse_count = 0;
            break;
        }
    }
    return ret;
}

/* Print mixer controls in a specific list
 * list - list to be printed
 * verb_index - verb index
 * count - number of elements in the list
 * Returns 0 on sucess, negative error code otherwise
 */
static int print_list(card_mctrl_t *list, int verb_index, int count)
{
    int i, j;

    for(i=0; i < count; i++) {
        ALOGD("\tcase name: %s\n", list[i].case_name);
        ALOGD("\tEnable sequence: %d\n", list[i].ena_mixer_count);
        for(j=0; j<list[i].ena_mixer_count; j++) {
            ALOGD("\t\t%s : %d : %d: %s\n",
                list[i].ena_mixer_list[j].control_name,
                list[i].ena_mixer_list[j].type,
                list[i].ena_mixer_list[j].value,
                list[i].ena_mixer_list[j].string);
        }
        ALOGD("\tDisable sequence: %d\n", list[i].dis_mixer_count);
        for(j=0; j<list[i].dis_mixer_count; j++) {
            ALOGD("\t\t%s : %d : %d : %s\n",
                list[i].dis_mixer_list[j].control_name,
                list[i].dis_mixer_list[j].type,
                list[i].dis_mixer_list[j].value,
                list[i].dis_mixer_list[j].string);
        }
    }
    return 0;
}

/* Print mixer controls extracted from config files
 * uc_mgr - use case manager structure
 * Returns 0 on sucess, negative error code otherwise
 */
static int snd_ucm_print(snd_use_case_mgr_t *uc_mgr)
{
    card_mctrl_t *list;
    int i, j, verb_index = 0;
    use_case_verb_t *verb_list;

    pthread_mutex_lock(&uc_mgr->card_ctxt_ptr->card_lock);
    verb_list = uc_mgr->card_ctxt_ptr->use_case_verb_list;
    while(strncmp(uc_mgr->card_ctxt_ptr->verb_list[verb_index],
         SND_UCM_END_OF_LIST, 3)) {
        ALOGD("\nuse case verb: %s\n",
            uc_mgr->card_ctxt_ptr->verb_list[verb_index]);
        if(verb_list[verb_index].device_list) {
            ALOGD("\tValid device list:");
            i = 0;
            while(strncmp(verb_list[verb_index].device_list[i],
                  SND_UCM_END_OF_LIST, 3)) {
                ALOGD("\t\t%s", verb_list[verb_index].device_list[i]);
                i++;
            }
        }
        if(verb_list[verb_index].modifier_list) {
            ALOGD("\tValid modifier list:");
            i = 0;
            while(strncmp(verb_list[verb_index].modifier_list[i],
                  SND_UCM_END_OF_LIST, 3)) {
                ALOGD("\t\t%s", verb_list[verb_index].modifier_list[i]);
                i++;
            }
        }
        ALOGD("Verbs:\n");
        list = verb_list[verb_index].verb_ctrls;
        print_list(list, verb_index, verb_list[verb_index].verb_count);
        ALOGD("Devices:\n");
        list = verb_list[verb_index].device_ctrls;
        print_list(list, verb_index, verb_list[verb_index].device_count);
        ALOGD("Modifier:\n");
        list = verb_list[verb_index].mod_ctrls;
        print_list(list, verb_index, verb_list[verb_index].mod_count);
        verb_index++;
    }
    pthread_mutex_unlock(&uc_mgr->card_ctxt_ptr->card_lock);
    return 0;
}

/* Gets the number of controls for specific sequence of a use cae */
static int get_controls_count(const char *nxt_str)
{
    char *current_str, *next_str, *str_addr;
    int count = 0;

    next_str = (char *)malloc((strlen(nxt_str)+1)*sizeof(char));
    if (next_str == NULL) {
        ALOGE("Failed to allocate memory");
        return -ENOMEM;
    }
    strlcpy(next_str, nxt_str, ((strlen(nxt_str)+1)*sizeof(char)));
    str_addr = next_str;
    while(1) {
        current_str = next_str;
        next_str = strchr(current_str, '\n');
        if ((!next_str) || (!strncasecmp(current_str, "EndSection", 10)))
            break;
        *next_str++ = '\0';
        if (strcasestr(current_str, "EndSequence") != NULL) {
            break;
        } else {
            count++;
        }
        if (*next_str == (char)EOF)
            break;
        if(!strncasecmp(current_str, "EndSection", 10))
            break;
    }
    free(str_addr);
    return count;
}

/* Parse a section of config files
 * uc_mgr - use case manager structure
 * Returns 0 on sucess, negative error code otherwise
 */
static int snd_ucm_parse_section(snd_use_case_mgr_t **uc_mgr, char **cur_str,
char **nxt_str, int verb_index, int ctrl_list_type)
{
    use_case_verb_t *verb_list;
    card_mctrl_t *list;
    int enable_seq = 0, disable_seq = 0, controls_count = 0, ret = 0;
    char *p, *current_str, *next_str, *name;

    verb_list = (*uc_mgr)->card_ctxt_ptr->use_case_verb_list;
    if (ctrl_list_type == CTRL_LIST_VERB) {
        list = (verb_list[verb_index].verb_ctrls +
            verb_list[verb_index].verb_count);
    } else if (ctrl_list_type == CTRL_LIST_DEVICE) {
        list = (verb_list[verb_index].device_ctrls +
            verb_list[verb_index].device_count);
    } else if (ctrl_list_type == CTRL_LIST_MODIFIER) {
        list = (verb_list[verb_index].mod_ctrls +
            verb_list[verb_index].mod_count);
    } else {
        ALOGE("Invalid list type: %d\n", ctrl_list_type);
        return -EINVAL;
    }
    list->case_name = NULL;
    list->ena_mixer_list = NULL;
    list->dis_mixer_list = NULL;
    list->ena_mixer_count = 0;
    list->dis_mixer_count = 0;
    list->playback_dev_name = NULL;
    list->capture_dev_name = NULL;
    list->acdb_id = 0;
    list->capability = 0;
    list->effects_mixer_ctl = NULL;
    current_str = *cur_str; next_str = *nxt_str;
    while(strncasecmp(current_str, "EndSection", 10)) {
        current_str = next_str;
        next_str = strchr(current_str, '\n');
        if ((!next_str) || (!strncasecmp(current_str, "EndSection", 10)))
            break;
        *next_str++ = '\0';
        if (strcasestr(current_str, "EndSequence") != NULL) {
            if (enable_seq == 1)
                enable_seq = 0;
            else if (disable_seq == 1)
                disable_seq = 0;
            else
                ALOGE("Error: improper config file\n");
        }
        if (enable_seq == 1) {
            ret = snd_ucm_extract_controls(current_str, &list->ena_mixer_list,
                  list->ena_mixer_count);
            if (ret < 0)
                break;
            list->ena_mixer_count++;
        } else if (disable_seq == 1) {
            ret = snd_ucm_extract_controls(current_str, &list->dis_mixer_list,
                  list->dis_mixer_count);
            if (ret < 0)
                break;
            list->dis_mixer_count++;
        } else if (strcasestr(current_str, "Name") != NULL) {
            ret = snd_ucm_extract_name(current_str, &list->case_name);
            if (ret < 0)
                break;
            ALOGV("Name of section is %s\n", list->case_name);
        } else if (strcasestr(current_str, "PlaybackPCM") != NULL) {
            ret = snd_ucm_extract_dev_name(current_str,
                      &list->playback_dev_name);
            if (ret < 0)
                break;
            ALOGV("Device name of playback is %s\n",
                list->playback_dev_name);
        } else if (strcasestr(current_str, "CapturePCM") != NULL) {
            ret = snd_ucm_extract_dev_name(current_str,
                      &list->capture_dev_name);
            if (ret < 0)
                break;
            ALOGV("Device name of capture is %s\n", list->capture_dev_name);
        } else if (strcasestr(current_str, "ACDBID") != NULL) {
            ret = snd_ucm_extract_acdb(current_str, &list->acdb_id,
                      &list->capability);
            if (ret < 0)
                break;
            ALOGV("ACDB ID: %d CAPABILITY: %d\n", list->acdb_id,
                list->capability);
        } else if (strcasestr(current_str, "EffectsMixerCTL") != NULL) {
            ret = snd_ucm_extract_effects_mixer_ctl(current_str,
                      &list->effects_mixer_ctl);
            if (ret < 0)
                break;
            ALOGV("Effects mixer ctl: %s: %d\n", list->effects_mixer_ctl);
        }
        if (strcasestr(current_str, "EnableSequence") != NULL) {
            controls_count = get_controls_count(next_str);
            if (controls_count < 0) {
                ret = -ENOMEM;
                break;
            }
            list->ena_mixer_list =
            (mixer_control_t *)malloc((controls_count*sizeof(mixer_control_t)));
            if (list->ena_mixer_list == NULL) {
                ret = -ENOMEM;
                break;
            }
            enable_seq = 1;
        } else if (strcasestr(current_str, "DisableSequence") != NULL) {
            controls_count = get_controls_count(next_str);
            if (controls_count < 0) {
                ret = -ENOMEM;
                break;
            }
            list->dis_mixer_list =
            (mixer_control_t *)malloc((controls_count*sizeof(mixer_control_t)));
            if (list->dis_mixer_list == NULL) {
                ret = -ENOMEM;
                break;
            }
            disable_seq = 1;
        }
        if (*next_str == (char)EOF)
             break;
    }
    if(ret == 0) {
        *cur_str = current_str; *nxt_str = next_str;
        if (ctrl_list_type == CTRL_LIST_VERB) {
            verb_list[verb_index].verb_count++;
        } else if (ctrl_list_type == CTRL_LIST_DEVICE) {
            verb_list[verb_index].device_count++;
        } else if (ctrl_list_type == CTRL_LIST_MODIFIER) {
            verb_list[verb_index].mod_count++;
        }
    }
    return ret;
}

/* Extract a mixer control name from config file
 * Returns 0 on sucess, negative error code otherwise
 */
static int snd_ucm_extract_name(char *buf, char **case_name)
{
    int ret = 0;
    char *p, *name = *case_name, *temp_ptr;

    p = strtok_r(buf, "\"", &temp_ptr);
    while (p != NULL) {
        p = strtok_r(NULL, "\"", &temp_ptr);
        if (p == NULL)
            break;
        name = (char *)malloc((strlen(p)+1)*sizeof(char));
        if(name == NULL) {
            ret = -ENOMEM;
            break;
        }
        strlcpy(name, p, (strlen(p)+1)*sizeof(char));
        *case_name = name;
        break;
    }
    return ret;
}

/* Extract a ACDB ID and capability of use case from config file
 * Returns 0 on sucess, negative error code otherwise
 */
static int snd_ucm_extract_acdb(char *buf, int *id, int *cap)
{
    char *p, key[] = "0123456789", *temp_ptr;

    p = strpbrk(buf, key);
    if (p == NULL) {
        *id = 0;
        *cap = 0;
    } else {
        p = strtok_r(p, ":", &temp_ptr);
        while (p != NULL) {
            *id = atoi(p);
            p = strtok_r(NULL, "\0", &temp_ptr);
            if (p == NULL)
                break;
            *cap = atoi(p);
            break;
        }
    }
    return 0;
}

/* Extract Effects Mixer ID of device from config file
 * Returns 0 on sucess, negative error code otherwise
 */
static int snd_ucm_extract_effects_mixer_ctl(char *buf, char **mixer_name)
{
    int ret = 0;
    char *p, *name = *mixer_name, *temp_ptr;

    p = strtok_r(buf, "\"", &temp_ptr);
    while (p != NULL) {
        p = strtok_r(NULL, "\"", &temp_ptr);
        if (p == NULL)
            break;
        name = (char *)malloc((strlen(p)+1)*sizeof(char));
        if(name == NULL) {
            ret = -ENOMEM;
            break;
        }
        strlcpy(name, p, (strlen(p)+1)*sizeof(char));
        *mixer_name = name;
        break;
    }
    return ret;
}

/* Extract a playback and capture device name of use case from config file
 * Returns 0 on sucess, negative error code otherwise
 */
static int snd_ucm_extract_dev_name(char *buf, char **dev_name)
{
    char key[] = "0123456789";
    char *p, *name = *dev_name;
    char dev_pre[] = "hw:0,";
    char *temp_ptr;

    p = strpbrk(buf, key);
    if (p == NULL) {
        *dev_name = NULL;
    } else {
        p = strtok_r(p, "\r\n", &temp_ptr);
        if (p == NULL) {
            *dev_name = NULL;
        } else {
            name = (char *)malloc((strlen(p)+strlen(dev_pre)+1)*sizeof(char));
            if(name == NULL)
                 return -ENOMEM;
            strlcpy(name, dev_pre, (strlen(p)+strlen(dev_pre)+1)*sizeof(char));
            strlcat(name, p, (strlen(p)+strlen(dev_pre)+1)*sizeof(char));
            *dev_name = name;
        }
    }
    return 0;
}

static int get_num_values(const char *buf)
{
    char *buf_addr, *p;
    int count = 0;
    char *temp_ptr;

    buf_addr = (char *)malloc((strlen(buf)+1)*sizeof(char));
    if (buf_addr == NULL) {
        ALOGE("Failed to allocate memory");
        return -ENOMEM;
    }
    strlcpy(buf_addr, buf, ((strlen(buf)+1)*sizeof(char)));
    p = strtok_r(buf_addr, " ", &temp_ptr);
    while (p != NULL) {
        count++;
        p = strtok_r(NULL, " ", &temp_ptr);
        if (p == NULL)
            break;
    }
    free(buf_addr);
    return count;
}

/* Extract a mixer control from config file
 * Returns 0 on sucess, negative error code otherwise
 */
static int snd_ucm_extract_controls(char *buf, mixer_control_t **mixer_list,
int size)
{
    unsigned long temp;
    int ret = -EINVAL, i, index = 0, count = 0;
    char *p, *ps, *pmv, temp_coeff[20];
    mixer_control_t *list;
    static const char *const seps = "\r\n";
    char *temp_ptr, *temp_vol_ptr;

    p = strtok_r(buf, "'", &temp_ptr);
    while (p != NULL) {
        p = strtok_r(NULL, "'", &temp_ptr);
        if (p == NULL)
            break;
        list = ((*mixer_list)+size);
        list->control_name = (char *)malloc((strlen(p)+1)*sizeof(char));
        if(list->control_name == NULL) {
            ret = -ENOMEM;
            free((*mixer_list));
            break;
        }
        strlcpy(list->control_name, p, (strlen(p)+1)*sizeof(char));
        p = strtok_r(NULL, ":", &temp_ptr);
        if (p == NULL)
            break;
        if(!strncmp(p, "0", 1)) {
            list->type = TYPE_STR;
        } else if(!strncmp(p, "1", 1)) {
            list->type = TYPE_INT;
        } else if(!strncmp(p, "2", 1)) {
            list->type = TYPE_MULTI_VAL;
        } else {
            ALOGE("Unknown type: p %s\n", p);
        }
        p = strtok_r(NULL, seps, &temp_ptr);
        if (p == NULL)
            break;
        if(list->type == TYPE_INT) {
            list->value = atoi(p);
            list->string = NULL;
            list->mulval = NULL;
        } else if(list->type == TYPE_STR) {
            list->value = -1;
            list->string = (char *)malloc((strlen(p)+1)*sizeof(char));
            list->mulval = NULL;
            if(list->string == NULL) {
                ret = -ENOMEM;
                free((*mixer_list));
                free(list->control_name);
                break;
            }
            strlcpy(list->string, p, (strlen(p)+1)*sizeof(char));
        } else if(list->type == TYPE_MULTI_VAL) {
            if (p != NULL) {
                count = get_num_values(p);
                list->mulval = (char **)malloc(count*sizeof(char *));
                if (list->mulval == NULL) {
                    ret = -ENOMEM;
                    free((*mixer_list));
                    free(list->control_name);
                    break;
                }
                index = 0;
                /* To support volume values in percentage */
                if ((count == 1) && (strstr(p, "%") != NULL)) {
                    pmv = strtok_r(p, " ", &temp_vol_ptr);
                    while (pmv != NULL) {
                        list->mulval[index] =
                            (char *)malloc((strlen(pmv)+1)*sizeof(char));
                        strlcpy(list->mulval[index], pmv, (strlen(pmv)+1));
                        index++;
                        pmv = strtok_r(NULL, " ", &temp_vol_ptr);
                        if (pmv == NULL)
                            break;
                    }
                } else {
                    pmv = strtok_r(p, " ", &temp_vol_ptr);
                    while (pmv != NULL) {
                        temp = strtoul(pmv, &ps, 16);
                        snprintf(temp_coeff, sizeof(temp_coeff),"%lu", temp);
                        list->mulval[index] =
                            (char *)malloc((strlen(temp_coeff)+1)*sizeof(char));
                        strlcpy(list->mulval[index], temp_coeff,
                            (strlen(temp_coeff)+1));
                        index++;
                        pmv = strtok_r(NULL, " ", &temp_vol_ptr);
                        if (pmv == NULL)
                            break;
                    }
                }
                list->value = count;
                list->string = NULL;
            }
        } else {
            ALOGE("Unknown type: p %s\n", p);
            list->value = -1;
            list->string = NULL;
        }
        ret = 0;
        break;
    }
    return ret;
}

void free_list(card_mctrl_t *list, int verb_index, int count)
{
    int case_index = 0, index = 0, mindex = 0;

    for(case_index = 0; case_index < count; case_index++) {
        for(index = 0; index < list[case_index].ena_mixer_count; index++) {
            if(list[case_index].ena_mixer_list[index].control_name) {
                free(list[case_index].ena_mixer_list[index].control_name);
            }
            if(list[case_index].ena_mixer_list[index].string) {
                free(list[case_index].ena_mixer_list[index].string);
            }
            if(list[case_index].ena_mixer_list[index].mulval) {
                for(mindex = 0;
                    mindex < list[case_index].ena_mixer_list[index].value;
                    mindex++) {
                    free(list[case_index].ena_mixer_list[index].mulval[mindex]);
                }
                if(list[case_index].ena_mixer_list[index].mulval) {
                    free(list[case_index].ena_mixer_list[index].mulval);
                }
            }
        }
        for(index = 0; index < list[case_index].dis_mixer_count; index++) {
            if(list[case_index].dis_mixer_list[index].control_name) {
                free(list[case_index].dis_mixer_list[index].control_name);
            }
            if(list[case_index].dis_mixer_list[index].string) {
                free(list[case_index].dis_mixer_list[index].string);
            }
            if(list[case_index].dis_mixer_list[index].mulval) {
                for(mindex = 0;
                    mindex < list[case_index].dis_mixer_list[index].value;
                    mindex++) {
                    free(list[case_index].dis_mixer_list[index].mulval[mindex]);
                }
                if(list[case_index].dis_mixer_list[index].mulval) {
                    free(list[case_index].dis_mixer_list[index].mulval);
                }
            }
        }
        if(list[case_index].case_name) {
            free(list[case_index].case_name);
        }
        if(list[case_index].ena_mixer_list) {
            free(list[case_index].ena_mixer_list);
        }
        if(list[case_index].dis_mixer_list) {
            free(list[case_index].dis_mixer_list);
        }
        if(list[case_index].playback_dev_name) {
            free(list[case_index].playback_dev_name);
        }
        if(list[case_index].capture_dev_name) {
            free(list[case_index].capture_dev_name);
        }
        if(list[case_index].effects_mixer_ctl) {
            list[case_index].effects_mixer_ctl = NULL;
        }
    }
}

void snd_ucm_free_mixer_list(snd_use_case_mgr_t **uc_mgr)
{
    card_mctrl_t *ctrl_list;
    use_case_verb_t *verb_list;
    int index = 0, verb_index = 0;

    pthread_mutex_lock(&(*uc_mgr)->card_ctxt_ptr->card_lock);
    verb_list = (*uc_mgr)->card_ctxt_ptr->use_case_verb_list;
    while(strncmp((*uc_mgr)->card_ctxt_ptr->verb_list[verb_index],
          SND_UCM_END_OF_LIST, 3)) {
        ctrl_list = verb_list[verb_index].verb_ctrls;
        free_list(ctrl_list, verb_index, verb_list[verb_index].verb_count);
        if(verb_list[verb_index].use_case_name)
            free(verb_list[verb_index].use_case_name);
        if((*uc_mgr)->card_ctxt_ptr->verb_list[verb_index]) {
            free((*uc_mgr)->card_ctxt_ptr->verb_list[verb_index]);
        }
        verb_index++;
    }
    verb_index -= 1;
    ctrl_list = verb_list[verb_index].device_ctrls;
    free_list(ctrl_list, verb_index, verb_list[verb_index].device_count);
    ctrl_list = verb_list[verb_index].mod_ctrls;
    free_list(ctrl_list, verb_index, verb_list[verb_index].mod_count);
    index = 0;
    while(1) {
        if (verb_list[verb_index].device_list[index]) {
            if (!strncmp(verb_list[verb_index].device_list[index],
                SND_UCM_END_OF_LIST, 3)) {
                free(verb_list[verb_index].device_list[index]);
                break;
            } else {
                free(verb_list[verb_index].device_list[index]);
                index++;
            }
        }
    }
    if (verb_list[verb_index].device_list)
        free(verb_list[verb_index].device_list);
    index = 0;
    while(1) {
        if (verb_list[verb_index].modifier_list[index]) {
            if (!strncmp(verb_list[verb_index].modifier_list[index],
                SND_UCM_END_OF_LIST, 3)) {
                free(verb_list[verb_index].modifier_list[index]);
                break;
            } else {
                free(verb_list[verb_index].modifier_list[index]);
                index++;
            }
        }
    }
    if (verb_list[verb_index].modifier_list)
        free(verb_list[verb_index].modifier_list);
    if((*uc_mgr)->card_ctxt_ptr->use_case_verb_list)
        free((*uc_mgr)->card_ctxt_ptr->use_case_verb_list);
    if((*uc_mgr)->card_ctxt_ptr->verb_list)
        free((*uc_mgr)->card_ctxt_ptr->verb_list);
    pthread_mutex_unlock(&(*uc_mgr)->card_ctxt_ptr->card_lock);
}

/* Add an identifier to the respective list
 * head - list head
 * value - node value that needs to be added
 * Returns 0 on sucess, negative error code otherwise
 */
static int snd_ucm_add_ident_to_list(struct snd_ucm_ident_node **head,
const char *value)
{
    struct snd_ucm_ident_node *temp, *node;

    node =
        (struct snd_ucm_ident_node *)malloc(sizeof(struct snd_ucm_ident_node));
    if (node == NULL) {
        ALOGE("Failed to allocate memory for new node");
        return -ENOMEM;
    } else {
        node->next = NULL;
        strlcpy(node->ident, value, MAX_STR_LEN);
        node->active = 0;
        node->capability = 0;
    }
    if (*head == NULL) {
        *head = node;
    } else {
        temp = *head;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = node;
    }
    ALOGV("add_to_list: head %p, value %s", *head, node->ident);
    return 0;
}

/* Get the status of identifier at particulare index of the list
 * head - list head
 * ident - identifier value for which status needs to be get
 * status - status to be set (1 - active, 0 - inactive)
 */
static int snd_ucm_get_status_at_index(struct snd_ucm_ident_node *head,
const char *ident)
{
    while (head != NULL) {
        if(!strncmp(ident, head->ident, (strlen(head->ident)+1))) {
            break;
        }
        head = head->next;
    }
    if (head == NULL) {
        ALOGV("Element not found in the list");
    } else {
        return(head->active);
    }
    return -EINVAL;
}

/* Get the node at particular index
 * head - list head
 * index - index value
 */
struct snd_ucm_ident_node *snd_ucm_get_device_node(struct snd_ucm_ident_node *head,
int index)
{
    if (head == NULL) {
        ALOGV("Empty list");
        return NULL;
    }

    if ((index < 0) || (index >= (snd_ucm_get_size_of_list(head)))) {
        ALOGE("Element with given index %d doesn't exist in the list", index);
        return NULL;
    }

    while (index) {
        head = head->next;
        index--;
    }

    return head;
}

/* Set the status of identifier at particulare index of the list
 * head - list head
 * ident - identifier value for which status needs to be set
 * status - status to be set (1 - active, 0 - inactive)
 */
static void snd_ucm_set_status_at_index(struct snd_ucm_ident_node *head,
const char *ident, int status, int capability)
{
    while (head != NULL) {
        if(!strncmp(ident, head->ident, (strlen(head->ident)+1))) {
            break;
        }
        head = head->next;
    }
    if (head == NULL) {
        ALOGE("Element not found to set the status");
    } else {
        head->active = status;
        head->capability = capability;
    }
}

/* Get the identifier value at particulare index of the list
 * head - list head
 * index - node index value
 * Returns node idetifier value at index on sucess, NULL otherwise
 */
static char *snd_ucm_get_value_at_index(struct snd_ucm_ident_node *head,
int index)
{
    if (head == NULL) {
        ALOGV("Empty list");
        return NULL;
    }

    if ((index < 0) || (index >= (snd_ucm_get_size_of_list(head)))) {
        ALOGE("Element with given index %d doesn't exist in the list", index);
        return NULL;
    }

    while (index) {
        head = head->next;
        index--;
    }

    return (strdup(head->ident));
}

/* Get the size of the list
 * head - list head
 * Returns size of list on sucess, negative error code otherwise
 */
static int snd_ucm_get_size_of_list(struct snd_ucm_ident_node *head)
{
    int index = 0;

    if (head == NULL) {
        ALOGV("Empty list");
        return 0;
    }

    while (head->next != NULL) {
        index++;
        head = head->next;
    }

    return (index+1);
}

static void snd_ucm_print_list(struct snd_ucm_ident_node *head)
{
    int index = 0;

    ALOGV("print_list: head %p", head);
    if (head == NULL) {
        ALOGV("Empty list");
        return;
    }

    while (head->next != NULL) {
        ALOGV("index: %d, value: %s", index, head->ident);
        index++;
        head = head->next;
    }
    ALOGV("index: %d, value: %s", index, head->ident);
}

/* Delete an identifier from respective list
 * head - list head
 * value - node value that needs to be deleted
 * Returns 0 on sucess, negative error code otherwise
 *
 */
static int snd_ucm_del_ident_from_list(struct snd_ucm_ident_node **head,
const char *value)
{
    struct snd_ucm_ident_node *temp1, *temp2;
    int ret = -EINVAL;

    if (*head == NULL) {
        ALOGE("del_from_list: Empty list");
        return -EINVAL;
    } else if (!strncmp((*head)->ident, value, (strlen(value)+1))) {
            temp2 = *head;
            *head = temp2->next;
            ret = 0;
    } else {
        temp1 = *head;
        temp2 = temp1->next;
        while (temp2 != NULL) {
            if (!strncmp(temp2->ident, value, (strlen(value)+1))) {
                temp1->next = temp2->next;
                ret = 0;
                break;
            }
            temp1 = temp1->next;
            temp2 = temp1->next;
        }
    }
    if (ret < 0) {
        ALOGE("Element not found in enabled list");
    } else {
        temp2->next = NULL;
        temp2->ident[0] = 0;
        temp2->active = 0;
        temp2->capability = 0;
        free(temp2);
        temp2 = NULL;
    }
    return ret;
}
