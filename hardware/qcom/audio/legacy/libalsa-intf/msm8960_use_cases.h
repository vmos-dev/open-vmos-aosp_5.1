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
#ifndef _MSM8960_USE_CASES_H_
#define _MSM8960_USE_CASES_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "alsa_ucm.h"
#include "alsa_audio.h"
#include <stdbool.h>
#include <pthread.h>
#define SND_UCM_END_OF_LIST "end"

/* ACDB Device ID macros */
#define CAP_RX 0x1
#define CAP_TX 0x2
#define CAP_VOICE 0x4
#define DEVICE_HANDSET_RX_ACDB_ID                       7 // HANDSET_SPKR
#define DEVICE_HANDSET_RX_TMUS_ACDB_ID                  81// HANDSET_SPKR
#define DEVICE_HANDSET_TX_ACDB_ID                       4 // HANDSET_MIC
#define DEVICE_SPEAKER_MONO_RX_ACDB_ID                  14// SPKR_PHONE_SPKR_MONO
#define DEVICE_SPEAKER_STEREO_RX_ACDB_ID                15// SPKR_PHONE_SPKR_STEREO
#define DEVICE_SPEAKER_TX_ACDB_ID                       11// SPKR_PHONE_MIC
#define DEVICE_HEADSET_RX_ACDB_ID                       10// HEADSET_SPKR_STEREO
#define DEVICE_HEADSET_TX_ACDB_ID                       8 // HEADSET_MIC
#define DEVICE_DUALMIC_HANDSET_TX_BROADSIDE_ACDB_ID     5 // HANDSET_MIC_BROADSIDE
#define DEVICE_DUALMIC_HANDSET_TX_ENDFIRE_ACDB_ID       6 // HANDSET_MIC_ENDFIRE
#define DEVICE_DUALMIC_HANDSET_TX_ENDFIRE_TMUS_ACDB_ID  91// HANDSET_MIC_ENDFIRE
#define DEVICE_DUALMIC_SPEAKER_TX_BROADSIDE_ACDB_ID     12// SPKR_PHONE_MIC_BROADSIDE
#define DEVICE_DUALMIC_SPEAKER_TX_ENDFIRE_ACDB_ID       13// SPKR_PHONE_MIC_ENDFIRE
#define DEVICE_TTY_HEADSET_MONO_RX_ACDB_ID              17// TTY_HEADSET_SPKR
#define DEVICE_TTY_HEADSET_MONO_TX_ACDB_ID              16// TTY_HEADSET_MIC
#define DEVICE_BT_SCO_RX_ACDB_ID                        22// BT_SCO_SPKR
#define DEVICE_BT_SCO_TX_ACDB_ID                        21// BT_SCO_SPKR
#define DEVICE_BT_SCO_RX_WB_ACDB_ID                     39// BT_SCO_WB_SPKR
#define DEVICE_BT_SCO_TX_WB_ACDB_ID                     38// BT_SCO_WB_MIC
#define DEVICE_SPEAKER_HEADSET_RX_ACDB_ID               DEVICE_HEADSET_RX_ACDB_ID // Use headset calibration
#define DEVICE_HDMI_STEREO_RX_ACDB_ID                   18// HDMI_SPKR
#define DEVICE_ANC_HEADSET_STEREO_RX_ACDB_ID            26// ANC RX, same as regular headset
#define DEVICE_QUADMIC_ACDB_ID                          19// QUADMIC_SKPR
#define DEVICE_PROXY_RX_ACDB_ID                         DEVICE_HDMI_STEREO_RX_ACDB_ID
#define DEVICE_TTY_VCO_HANDSET_TX_ACDB_ID               36// TTY_VCO_HANDSET_MIC
#define DEVICE_TTY_HCO_HANDSET_RX_ACDB_ID               37// TTY_HCO_HANDSET_SPRK
#define DEVICE_HANDSET_TX_FV5_ACDB_ID                   50
#define DEVICE_DUALMIC_HANDSET_TX_ENDFIRE_FV5_ACDB_ID   51
#define DEVICE_SPEAKER_TX_FV5_ACDB_ID                   52
#define DEVICE_DUALMIC_SPEAKER_TX_ENDFIRE_FV5_ACDB_ID   53
#define DEVICE_INCALL_VOICE_RECORD_STEREO_ACDB_ID       45
#define DEVICE_INCALL_MUSIC_DELIVERY_MONO_ACDB_ID       46
#define DEVICE_INCALL_VOICE_RECORD_MONO_ACDB_ID         47
#define DEVICE_CAMCORDER_TX_ACDB_ID                     61// CAMCORDER_TX
#define DEVICE_VOICE_RECOGNITION_ACDB_ID                62// VOICE_RECOGNITION

/* mixer control type */
#define TYPE_INT            0
#define TYPE_STR            1
#define TYPE_MULTI_VAL      2

/* Maximum string length of use case and device combination */
#define MAX_UC_LEN 100
/* Maximum string length of use case or device */
#define MAX_STR_LEN 50

/* Returns maximum length of strings x and y */
#define MAX_LEN(x,y) ((strlen(x)>strlen(y))?strlen(x):strlen(y))

/* Mixer control list type enum*/
enum {
    CTRL_LIST_VERB,
    CTRL_LIST_DEVICE,
    CTRL_LIST_MODIFIER,
};

/* mixer control structure */
typedef struct mixer_control {
    char *control_name;
    unsigned type;
    unsigned value;
    char *string;
    char **mulval;
}mixer_control_t;

/* Use case mixer controls structure */
typedef struct card_mctrl {
    char *case_name;
    int ena_mixer_count;
    mixer_control_t *ena_mixer_list;
    int dis_mixer_count;
    mixer_control_t *dis_mixer_list;
    char *playback_dev_name;
    char *capture_dev_name;
    int acdb_id;
    int capability;
    char *effects_mixer_ctl;
}card_mctrl_t;

/* identifier node structure for identifier list*/
struct snd_ucm_ident_node {
    int active;
    int capability;
    char ident[MAX_STR_LEN];
    struct snd_ucm_ident_node *next;
};

/* Structure to maintain the valid devices and
 * modifiers list per each use case */
typedef struct use_case_verb {
    char *use_case_name;
    char **device_list;
    char **modifier_list;
    int verb_count;
    int device_count;
    int mod_count;
    card_mctrl_t *verb_ctrls;
    card_mctrl_t *device_ctrls;
    card_mctrl_t *mod_ctrls;
}use_case_verb_t;

/* SND card context structure */
typedef struct card_ctxt {
    char *card_name;
    int card_number;
    char *control_device;
    struct mixer *mixer_handle;
    char current_verb[MAX_STR_LEN];
    struct snd_ucm_ident_node *dev_list_head;
    struct snd_ucm_ident_node *mod_list_head;
    pthread_mutex_t card_lock;
    pthread_mutexattr_t card_lock_attr;
    int current_verb_index;
    use_case_verb_t *use_case_verb_list;
    char **verb_list;
}card_ctxt_t;

/** use case manager structure */
struct snd_use_case_mgr {
    int snd_card_index;
    int device_list_count;
    int modifier_list_count;
    char **current_device_list;
    char **current_modifier_list;
    int current_tx_device;
    int current_rx_device;
    card_ctxt_t *card_ctxt_ptr;
    pthread_t thr;
    void *acdb_handle;
    bool isFusion3Platform;
};

#define MAX_NUM_CARDS (sizeof(card_list)/sizeof(char *))

/* Valid sound cards list */
static const char *card_list[] = {
    "snd_soc_msm",
    "snd_soc_msm_2x",
    "snd_soc_msm_2x_Fusion3",
    "snd_soc_msm_Sitar",
};

typedef struct card_mapping {
    char card_name[50];
    int card_number;
}card_mapping_t;

/* sound card name and number mapping */
static card_mapping_t card_mapping_list[] = {
    {"snd_soc_msm", 0},
    {"snd_soc_msm_2x", 0},
    {"snd_soc_msm_2x_Fusion3", 0},
    {"snd_soc_msm_Sitar", 0},
};

/* New use cases, devices and modifiers added
 * which are not part of existing macros
 */
#define SND_USE_CASE_VERB_FM_REC         "FM REC"
#define SND_USE_CASE_VERB_FM_A2DP_REC   "FM A2DP REC"
#define SND_USE_CASE_VERB_HIFI_REC       "HiFi Rec"
#define SND_USE_CASE_VERB_HIFI_LOWLATENCY_REC       "HiFi Lowlatency Rec"
#define SND_USE_CASE_VERB_DL_REC	 "DL REC"
#define SND_USE_CASE_VERB_UL_DL_REC      "UL DL REC"
#define SND_USE_CASE_VERB_HIFI_TUNNEL    "HiFi Tunnel"
#define SND_USE_CASE_VERB_HIFI_LOWLATENCY_MUSIC    "HiFi Lowlatency"
#define SND_USE_CASE_VERB_HIFI2       "HiFi2"
#define SND_USE_CASE_VERB_INCALL_REC   "Incall REC"
#define SND_USE_CASE_VERB_MI2S        "MI2S"
#define SND_USE_CASE_VERB_VOLTE    "VoLTE"
#define SND_USE_CASE_VERB_ADSP_TESTFWK "ADSP testfwk"

#define SND_USE_CASE_DEV_FM_TX           "FM Tx"
#define SND_USE_CASE_DEV_ANC_HEADSET     "ANC Headset"
#define SND_USE_CASE_DEV_BTSCO_NB_RX        "BT SCO Rx"
#define SND_USE_CASE_DEV_BTSCO_NB_TX        "BT SCO Tx"
#define SND_USE_CASE_DEV_BTSCO_WB_RX        "BT SCO WB Rx"
#define SND_USE_CASE_DEV_BTSCO_WB_TX        "BT SCO WB Tx"
#define SND_USE_CASE_DEV_SPEAKER_HEADSET "Speaker Headset"
#define SND_USE_CASE_DEV_SPEAKER_ANC_HEADSET "Speaker ANC Headset"
#define SND_USE_CASE_DEV_SPEAKER_FM_TX   "Speaker FM Tx"
#define SND_USE_CASE_DEV_TTY_HEADSET_RX  "TTY Headset Rx"
#define SND_USE_CASE_DEV_TTY_HEADSET_TX  "TTY Headset Tx"
#define SND_USE_CASE_DEV_TTY_FULL_RX  "TTY Full Rx"
#define SND_USE_CASE_DEV_TTY_FULL_TX  "TTY Full Tx"
#define SND_USE_CASE_DEV_TTY_HANDSET_RX  "TTY Handset Rx"
#define SND_USE_CASE_DEV_TTY_HANDSET_TX  "TTY Handset Tx"
#define SND_USE_CASE_DEV_TTY_HANDSET_ANALOG_TX  "TTY Handset Analog Tx"
#define SND_USE_CASE_DEV_DUAL_MIC_BROADSIDE "DMIC Broadside"
#define SND_USE_CASE_DEV_DUAL_MIC_BROADSIDE_VREC "DMIC Broadside Voice Rec"
#define SND_USE_CASE_DEV_DUAL_MIC_ENDFIRE "DMIC Endfire"
#define SND_USE_CASE_DEV_DUAL_MIC_ENDFIRE_TMUS "DMIC Endfire TMUS"
#define SND_USE_CASE_DEV_DUAL_MIC_ENDFIRE_VREC "DMIC Endfire Voice Rec"
#define SND_USE_CASE_DEV_SPEAKER_DUAL_MIC_BROADSIDE "Speaker DMIC Broadside"
#define SND_USE_CASE_DEV_SPEAKER_DUAL_MIC_ENDFIRE "Speaker DMIC Endfire"
#define SND_USE_CASE_DEV_HDMI_TX             "HDMI Tx"
#define SND_USE_CASE_DEV_HDMI_SPDIF          "HDMI SPDIF"
#define SND_USE_CASE_DEV_QUAD_MIC "QMIC"
#define SND_USE_CASE_DEV_SSR_QUAD_MIC "SSR QMIC"
#define SND_USE_CASE_DEV_PROXY_RX     "PROXY Rx"
#define SND_USE_CASE_DEV_PROXY_TX     "PROXY Tx"
#define SND_USE_CASE_DEV_HDMI_SPEAKER     "HDMI Speaker"
#define SND_USE_CASE_DEV_SPDIF_SPEAKER     "SPDIF Speaker"
#define SND_USE_CASE_DEV_SPDIF_HANDSET     "SPDIF Earpiece"
#define SND_USE_CASE_DEV_SPDIF_HEADSET     "SPDIF Headphones"
#define SND_USE_CASE_DEV_SPDIF_ANC_HEADSET     "SPDIF ANC Headset"
#define SND_USE_CASE_DEV_SPDIF_SPEAKER_HEADSET "SPDIF Speaker Headset"
#define SND_USE_CASE_DEV_SPDIF_SPEAKER_ANC_HEADSET "SPDIF Speaker ANC Headset"
#define SND_USE_CASE_DEV_DUMMY_TX "Dummy Tx"
#define SND_USE_CASE_DEV_PROXY_RX_SPEAKER     "PROXY Rx Speaker"
#define SND_USE_CASE_DEV_PROXY_RX_HANDSET     "PROXY Rx Earpiece"
#define SND_USE_CASE_DEV_PROXY_RX_HEADSET     "PROXY Rx Headphones"
#define SND_USE_CASE_DEV_PROXY_RX_ANC_HEADSET     "PROXY Rx ANC Headset"
#define SND_USE_CASE_DEV_PROXY_RX_SPEAKER_HEADSET "PROXY Rx Speaker Headset"
#define SND_USE_CASE_DEV_PROXY_RX_SPEAKER_ANC_HEADSET "PROXY Rx Speaker ANC Headset"
#define SND_USE_CASE_DEV_CAMCORDER_TX       "Camcorder Tx"
#define SND_USE_CASE_DEV_VOICE_RECOGNITION  "Voice Recognition"
#define SND_USE_CASE_DEV_VOC_EARPIECE       "Voice Earpiece"
#define SND_USE_CASE_DEV_VOC_EARPIECE_TMUS  "Voice Earpiece TMUS"
#define SND_USE_CASE_DEV_VOC_HEADPHONE      "Voice Headphones"
#define SND_USE_CASE_DEV_VOC_HEADSET        "Voice Headset"
#define SND_USE_CASE_DEV_VOC_ANC_HEADSET    "Voice ANC Headset"
#define SND_USE_CASE_DEV_VOC_SPEAKER        "Voice Speaker"
#define SND_USE_CASE_DEV_VOC_LINE           "Voice Line"

#define SND_USE_CASE_MOD_PLAY_FM         "Play FM"
#define SND_USE_CASE_MOD_CAPTURE_FM      "Capture FM"
#define SND_USE_CASE_MOD_CAPTURE_LOWLATENCY_MUSIC     "Capture Lowlatency Music"
#define SND_USE_CASE_MOD_CAPTURE_A2DP_FM "Capture A2DP FM"
#define SND_USE_CASE_MOD_PLAY_LPA        "Play LPA"
#define SND_USE_CASE_MOD_PLAY_VOIP       "Play VOIP"
#define SND_USE_CASE_MOD_CAPTURE_VOIP    "Capture VOIP"
#define SND_USE_CASE_MOD_CAPTURE_VOICE_DL       "Capture Voice Downlink"
#define SND_USE_CASE_MOD_CAPTURE_VOICE_UL_DL    "Capture Voice Uplink Downlink"
#define SND_USE_CASE_MOD_PLAY_TUNNEL     "Play Tunnel"
#define SND_USE_CASE_MOD_PLAY_LOWLATENCY_MUSIC     "Play Lowlatency Music"
#define SND_USE_CASE_MOD_PLAY_MUSIC2       "Play Music2"
#define SND_USE_CASE_MOD_PLAY_MI2S       "Play MI2S"
#define SND_USE_CASE_MOD_PLAY_VOLTE   "Play VoLTE"

/* List utility functions for maintaining enabled devices and modifiers */
static int snd_ucm_add_ident_to_list(struct snd_ucm_ident_node **head, const char *value);
static char *snd_ucm_get_value_at_index(struct snd_ucm_ident_node *head, int index);
static int snd_ucm_get_size_of_list(struct snd_ucm_ident_node *head);
static int snd_ucm_del_ident_from_list(struct snd_ucm_ident_node **head, const char *value);
static int snd_ucm_free_list(struct snd_ucm_ident_node **head);
static void snd_ucm_print_list(struct snd_ucm_ident_node *head);
static void snd_ucm_set_status_at_index(struct snd_ucm_ident_node *head, const char *ident, int status, int capability);
static int snd_ucm_get_status_at_index(struct snd_ucm_ident_node *head, const char *ident);
struct snd_ucm_ident_node *snd_ucm_get_device_node(struct snd_ucm_ident_node *head, int index);
static int snd_ucm_parse_verb(snd_use_case_mgr_t **uc_mgr, const char *file_name, int index);
static int get_verb_count(const char *nxt_str);
int snd_use_case_mgr_wait_for_parsing(snd_use_case_mgr_t *uc_mgr);
int snd_use_case_set_case(snd_use_case_mgr_t *uc_mgr, const char *identifier,
                          const char *value, const char *usecase);
static int get_usecase_type(snd_use_case_mgr_t *uc_mgr, const char *usecase);
static int parse_single_config_format(snd_use_case_mgr_t **uc_mgr, char *current_str, int num_verbs);
static int get_num_verbs_config_format(const char *nxt_str);
static int get_num_device_config_format(const char *nxt_str);
static int get_num_mod_config_format(const char *nxt_str);
static int is_single_config_format(const char *nxt_str);
/* Parse functions */
static int snd_ucm_parse(snd_use_case_mgr_t **uc_mgr);
static int snd_ucm_parse_section(snd_use_case_mgr_t **uc_mgr, char **cur_str, char **nxt_str, int verb_index, int ctrl_list_type);
static int snd_ucm_extract_name(char *buf, char **case_name);
static int snd_ucm_extract_acdb(char *buf, int *id, int *cap);
static int snd_ucm_extract_effects_mixer_ctl(char *buf, char **mixer_name);
static int snd_ucm_extract_dev_name(char *buf, char **dev_name);
static int snd_ucm_extract_controls(char *buf, mixer_control_t **mixer_list, int count);
static int snd_ucm_print(snd_use_case_mgr_t *uc_mgr);
static void snd_ucm_free_mixer_list(snd_use_case_mgr_t **uc_mgr);
#ifdef __cplusplus
}
#endif

#endif
