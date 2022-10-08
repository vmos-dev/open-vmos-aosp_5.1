/****************************************************************************
 ****************************************************************************
 ***
 ***   This header was automatically generated from a Linux kernel header
 ***   of the same name, to make information necessary for userspace to
 ***   call into the kernel available to libc.  It contains only constants,
 ***   structures, and macros generated from the original header, and thus,
 ***   contains no copyrightable information.
 ***
 ***   To edit the content of this header, modify the corresponding
 ***   source file (e.g. under external/kernel-headers/original/) then
 ***   run bionic/libc/kernel/tools/update_all.py
 ***
 ***   Any manual change here will be lost the next time this script will
 ***   be run. You've been warned!
 ***
 ****************************************************************************
 ****************************************************************************/
#ifndef _UAPI_LSM_PARAMS_H__
#define _UAPI_LSM_PARAMS_H__
#include <linux/types.h>
#include <sound/asound.h>
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define SNDRV_LSM_VERSION SNDRV_PROTOCOL_VERSION(0, 1, 0)
enum lsm_app_id {
 LSM_VOICE_WAKEUP_APP_ID = 1,
 LSM_VOICE_WAKEUP_APP_ID_V2 = 2,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
enum lsm_detection_mode {
 LSM_MODE_KEYWORD_ONLY_DETECTION = 1,
 LSM_MODE_USER_KEYWORD_DETECTION
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct snd_lsm_sound_model {
 __u8 __user *data;
 __u32 data_size;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 enum lsm_detection_mode detection_mode;
 __u16 min_keyw_confidence;
 __u16 min_user_confidence;
 bool detect_failure;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct snd_lsm_sound_model_v2 {
 __u8 __user *data;
 __u8 *confidence_level;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u32 data_size;
 enum lsm_detection_mode detection_mode;
 __u8 num_confidence_levels;
 bool detect_failure;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct snd_lsm_session_data {
 enum lsm_app_id app_id;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct snd_lsm_event_status {
 __u16 status;
 __u16 payload_size;
 __u8 payload[0];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
#define SNDRV_LSM_REG_SND_MODEL _IOW('U', 0x00, struct snd_lsm_sound_model)
#define SNDRV_LSM_DEREG_SND_MODEL _IOW('U', 0x01, int)
#define SNDRV_LSM_EVENT_STATUS _IOW('U', 0x02, struct snd_lsm_event_status)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define SNDRV_LSM_ABORT_EVENT _IOW('U', 0x03, int)
#define SNDRV_LSM_START _IOW('U', 0x04, int)
#define SNDRV_LSM_STOP _IOW('U', 0x05, int)
#define SNDRV_LSM_SET_SESSION_DATA _IOW('U', 0x06, struct snd_lsm_session_data)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define SNDRV_LSM_REG_SND_MODEL_V2 _IOW('U', 0x07,  struct snd_lsm_sound_model_v2)
#endif

