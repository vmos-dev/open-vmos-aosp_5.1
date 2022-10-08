/*
 * Copyright (C) 2008 The Android Open Source Project
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

#ifndef ANDROID_AUDIOSYSTEM_LEGACY_H_
#define ANDROID_AUDIOSYSTEM_LEGACY_H_

#include <utils/Errors.h>
#include <media/AudioParameter.h>

#include <system/audio.h>
#include <system/audio_policy.h>

namespace android_audio_legacy {

using android::status_t;
using android::AudioParameter;

enum {
    OK                  = android::OK,
    NO_ERROR            = android::NO_ERROR,

    UNKNOWN_ERROR       = android::UNKNOWN_ERROR,

    NO_MEMORY           = android::NO_MEMORY,
    INVALID_OPERATION   = android::INVALID_OPERATION,
    BAD_VALUE           = android::BAD_VALUE,
    BAD_TYPE            = android::BAD_TYPE,
    NAME_NOT_FOUND      = android::NAME_NOT_FOUND,
    PERMISSION_DENIED   = android::PERMISSION_DENIED,
    NO_INIT             = android::NO_INIT,
    ALREADY_EXISTS      = android::ALREADY_EXISTS,
    DEAD_OBJECT         = android::DEAD_OBJECT,
    FAILED_TRANSACTION  = android::FAILED_TRANSACTION,
    JPARKS_BROKE_IT     = android::JPARKS_BROKE_IT,
    BAD_INDEX           = android::BAD_INDEX,
    NOT_ENOUGH_DATA     = android::NOT_ENOUGH_DATA,
    WOULD_BLOCK         = android::WOULD_BLOCK,
    TIMED_OUT           = android::TIMED_OUT,
    UNKNOWN_TRANSACTION = android::UNKNOWN_TRANSACTION,
};

enum audio_source {
    AUDIO_SOURCE_DEFAULT = 0,
    AUDIO_SOURCE_MIC = 1,
    AUDIO_SOURCE_VOICE_UPLINK = 2,
    AUDIO_SOURCE_VOICE_DOWNLINK = 3,
    AUDIO_SOURCE_VOICE_CALL = 4,
    AUDIO_SOURCE_CAMCORDER = 5,
    AUDIO_SOURCE_VOICE_RECOGNITION = 6,
    AUDIO_SOURCE_VOICE_COMMUNICATION = 7,
    AUDIO_SOURCE_MAX = AUDIO_SOURCE_VOICE_COMMUNICATION,

    AUDIO_SOURCE_LIST_END  // must be last - used to validate audio source type
};

class AudioSystem {
public:
#if 1
    enum stream_type {
        DEFAULT          =-1,
        VOICE_CALL       = 0,
        SYSTEM           = 1,
        RING             = 2,
        MUSIC            = 3,
        ALARM            = 4,
        NOTIFICATION     = 5,
        BLUETOOTH_SCO    = 6,
        ENFORCED_AUDIBLE = 7, // Sounds that cannot be muted by user and must be routed to speaker
        DTMF             = 8,
        TTS              = 9,
        NUM_STREAM_TYPES
    };

    // Audio sub formats (see AudioSystem::audio_format).
    enum pcm_sub_format {
        PCM_SUB_16_BIT          = 0x1, // must be 1 for backward compatibility
        PCM_SUB_8_BIT           = 0x2, // must be 2 for backward compatibility
    };

    enum audio_sessions {
        SESSION_OUTPUT_STAGE = AUDIO_SESSION_OUTPUT_STAGE,
        SESSION_OUTPUT_MIX = AUDIO_SESSION_OUTPUT_MIX,
    };

    // MP3 sub format field definition : can use 11 LSBs in the same way as MP3 frame header to specify
    // bit rate, stereo mode, version...
    enum mp3_sub_format {
        //TODO
    };

    // AMR NB/WB sub format field definition: specify frame block interleaving, bandwidth efficient or octet aligned,
    // encoding mode for recording...
    enum amr_sub_format {
        //TODO
    };

    // AAC sub format field definition: specify profile or bitrate for recording...
    enum aac_sub_format {
        //TODO
    };

    // VORBIS sub format field definition: specify quality for recording...
    enum vorbis_sub_format {
        //TODO
    };

    // Audio format consists in a main format field (upper 8 bits) and a sub format field (lower 24 bits).
    // The main format indicates the main codec type. The sub format field indicates options and parameters
    // for each format. The sub format is mainly used for record to indicate for instance the requested bitrate
    // or profile. It can also be used for certain formats to give informations not present in the encoded
    // audio stream (e.g. octet alignement for AMR).
    enum audio_format {
        INVALID_FORMAT      = -1,
        FORMAT_DEFAULT      = 0,
        PCM                 = 0x00000000, // must be 0 for backward compatibility
        MP3                 = 0x01000000,
        AMR_NB              = 0x02000000,
        AMR_WB              = 0x03000000,
        AAC                 = 0x04000000,
        HE_AAC_V1           = 0x05000000,
        HE_AAC_V2           = 0x06000000,
        VORBIS              = 0x07000000,
        MAIN_FORMAT_MASK    = 0xFF000000,
        SUB_FORMAT_MASK     = 0x00FFFFFF,
        // Aliases
        PCM_16_BIT          = (PCM|PCM_SUB_16_BIT),
        PCM_8_BIT          = (PCM|PCM_SUB_8_BIT)
    };

    enum audio_channels {
        // output channels
        CHANNEL_OUT_FRONT_LEFT            = 0x1,
        CHANNEL_OUT_FRONT_RIGHT           = 0x2,
        CHANNEL_OUT_FRONT_CENTER          = 0x4,
        CHANNEL_OUT_LOW_FREQUENCY         = 0x8,
        CHANNEL_OUT_BACK_LEFT             = 0x10,
        CHANNEL_OUT_BACK_RIGHT            = 0x20,
        CHANNEL_OUT_FRONT_LEFT_OF_CENTER  = 0x40,
        CHANNEL_OUT_FRONT_RIGHT_OF_CENTER = 0x80,
        CHANNEL_OUT_BACK_CENTER           = 0x100,
        CHANNEL_OUT_SIDE_LEFT             = 0x200,
        CHANNEL_OUT_SIDE_RIGHT            = 0x400,
        CHANNEL_OUT_TOP_CENTER            = 0x800,
        CHANNEL_OUT_TOP_FRONT_LEFT        = 0x1000,
        CHANNEL_OUT_TOP_FRONT_CENTER      = 0x2000,
        CHANNEL_OUT_TOP_FRONT_RIGHT       = 0x4000,
        CHANNEL_OUT_TOP_BACK_LEFT         = 0x8000,
        CHANNEL_OUT_TOP_BACK_CENTER       = 0x10000,
        CHANNEL_OUT_TOP_BACK_RIGHT        = 0x20000,

        CHANNEL_OUT_MONO = CHANNEL_OUT_FRONT_LEFT,
        CHANNEL_OUT_STEREO = (CHANNEL_OUT_FRONT_LEFT | CHANNEL_OUT_FRONT_RIGHT),
        CHANNEL_OUT_QUAD = (CHANNEL_OUT_FRONT_LEFT | CHANNEL_OUT_FRONT_RIGHT |
                CHANNEL_OUT_BACK_LEFT | CHANNEL_OUT_BACK_RIGHT),
        CHANNEL_OUT_SURROUND = (CHANNEL_OUT_FRONT_LEFT | CHANNEL_OUT_FRONT_RIGHT |
                CHANNEL_OUT_FRONT_CENTER | CHANNEL_OUT_BACK_CENTER),
        CHANNEL_OUT_5POINT1 = (CHANNEL_OUT_FRONT_LEFT | CHANNEL_OUT_FRONT_RIGHT |
                CHANNEL_OUT_FRONT_CENTER | CHANNEL_OUT_LOW_FREQUENCY |
                CHANNEL_OUT_BACK_LEFT | CHANNEL_OUT_BACK_RIGHT),
        // matches the correct AudioFormat.CHANNEL_OUT_7POINT1_SURROUND definition for 7.1
        CHANNEL_OUT_7POINT1 = (CHANNEL_OUT_FRONT_LEFT | CHANNEL_OUT_FRONT_RIGHT |
                CHANNEL_OUT_FRONT_CENTER | CHANNEL_OUT_LOW_FREQUENCY |
                CHANNEL_OUT_BACK_LEFT | CHANNEL_OUT_BACK_RIGHT |
                CHANNEL_OUT_SIDE_LEFT | CHANNEL_OUT_SIDE_RIGHT),
        CHANNEL_OUT_ALL = (CHANNEL_OUT_FRONT_LEFT | CHANNEL_OUT_FRONT_RIGHT |
                CHANNEL_OUT_FRONT_CENTER | CHANNEL_OUT_LOW_FREQUENCY | CHANNEL_OUT_BACK_LEFT |
                CHANNEL_OUT_BACK_RIGHT | CHANNEL_OUT_FRONT_LEFT_OF_CENTER |
                CHANNEL_OUT_FRONT_RIGHT_OF_CENTER | CHANNEL_OUT_BACK_CENTER |
                CHANNEL_OUT_SIDE_LEFT | CHANNEL_OUT_SIDE_RIGHT | CHANNEL_OUT_TOP_CENTER |
                CHANNEL_OUT_TOP_FRONT_LEFT | CHANNEL_OUT_TOP_FRONT_CENTER |
                CHANNEL_OUT_TOP_FRONT_RIGHT | CHANNEL_OUT_TOP_BACK_LEFT |
                CHANNEL_OUT_TOP_BACK_CENTER | CHANNEL_OUT_TOP_BACK_RIGHT),

        // input channels
        CHANNEL_IN_LEFT = 0x4,
        CHANNEL_IN_RIGHT = 0x8,
        CHANNEL_IN_FRONT = 0x10,
        CHANNEL_IN_BACK = 0x20,
        CHANNEL_IN_LEFT_PROCESSED = 0x40,
        CHANNEL_IN_RIGHT_PROCESSED = 0x80,
        CHANNEL_IN_FRONT_PROCESSED = 0x100,
        CHANNEL_IN_BACK_PROCESSED = 0x200,
        CHANNEL_IN_PRESSURE = 0x400,
        CHANNEL_IN_X_AXIS = 0x800,
        CHANNEL_IN_Y_AXIS = 0x1000,
        CHANNEL_IN_Z_AXIS = 0x2000,
        CHANNEL_IN_VOICE_UPLINK = 0x4000,
        CHANNEL_IN_VOICE_DNLINK = 0x8000,
        CHANNEL_IN_MONO = CHANNEL_IN_FRONT,
        CHANNEL_IN_STEREO = (CHANNEL_IN_LEFT | CHANNEL_IN_RIGHT),
        CHANNEL_IN_ALL = (CHANNEL_IN_LEFT | CHANNEL_IN_RIGHT | CHANNEL_IN_FRONT | CHANNEL_IN_BACK|
                CHANNEL_IN_LEFT_PROCESSED | CHANNEL_IN_RIGHT_PROCESSED | CHANNEL_IN_FRONT_PROCESSED | CHANNEL_IN_BACK_PROCESSED|
                CHANNEL_IN_PRESSURE | CHANNEL_IN_X_AXIS | CHANNEL_IN_Y_AXIS | CHANNEL_IN_Z_AXIS |
                CHANNEL_IN_VOICE_UPLINK | CHANNEL_IN_VOICE_DNLINK)
    };

    enum audio_mode {
        MODE_INVALID = -2,
        MODE_CURRENT = -1,
        MODE_NORMAL = 0,
        MODE_RINGTONE,
        MODE_IN_CALL,
        MODE_IN_COMMUNICATION,
        NUM_MODES  // not a valid entry, denotes end-of-list
    };

    enum audio_in_acoustics {
        AGC_ENABLE    = 0x0001,
        AGC_DISABLE   = 0,
        NS_ENABLE     = 0x0002,
        NS_DISABLE    = 0,
        TX_IIR_ENABLE = 0x0004,
        TX_DISABLE    = 0
    };

    // DO NOT USE: the "audio_devices" enumeration below is obsolete, use type "audio_devices_t" and
    //   audio device enumeration from system/audio.h instead.
    enum audio_devices {
        // output devices
        DEVICE_OUT_EARPIECE = 0x1,
        DEVICE_OUT_SPEAKER = 0x2,
        DEVICE_OUT_WIRED_HEADSET = 0x4,
        DEVICE_OUT_WIRED_HEADPHONE = 0x8,
        DEVICE_OUT_BLUETOOTH_SCO = 0x10,
        DEVICE_OUT_BLUETOOTH_SCO_HEADSET = 0x20,
        DEVICE_OUT_BLUETOOTH_SCO_CARKIT = 0x40,
        DEVICE_OUT_BLUETOOTH_A2DP = 0x80,
        DEVICE_OUT_BLUETOOTH_A2DP_HEADPHONES = 0x100,
        DEVICE_OUT_BLUETOOTH_A2DP_SPEAKER = 0x200,
        DEVICE_OUT_AUX_DIGITAL = 0x400,
        DEVICE_OUT_ANLG_DOCK_HEADSET = 0x800,
        DEVICE_OUT_DGTL_DOCK_HEADSET = 0x1000,
        DEVICE_OUT_DEFAULT = 0x8000,
        DEVICE_OUT_ALL = (DEVICE_OUT_EARPIECE | DEVICE_OUT_SPEAKER | DEVICE_OUT_WIRED_HEADSET |
                DEVICE_OUT_WIRED_HEADPHONE | DEVICE_OUT_BLUETOOTH_SCO | DEVICE_OUT_BLUETOOTH_SCO_HEADSET |
                DEVICE_OUT_BLUETOOTH_SCO_CARKIT | DEVICE_OUT_BLUETOOTH_A2DP | DEVICE_OUT_BLUETOOTH_A2DP_HEADPHONES |
                DEVICE_OUT_BLUETOOTH_A2DP_SPEAKER | DEVICE_OUT_AUX_DIGITAL |
                DEVICE_OUT_ANLG_DOCK_HEADSET | DEVICE_OUT_DGTL_DOCK_HEADSET |
                DEVICE_OUT_DEFAULT),
        DEVICE_OUT_ALL_A2DP = (DEVICE_OUT_BLUETOOTH_A2DP | DEVICE_OUT_BLUETOOTH_A2DP_HEADPHONES |
                DEVICE_OUT_BLUETOOTH_A2DP_SPEAKER),

        // input devices
        DEVICE_IN_COMMUNICATION = 0x10000,
        DEVICE_IN_AMBIENT = 0x20000,
        DEVICE_IN_BUILTIN_MIC = 0x40000,
        DEVICE_IN_BLUETOOTH_SCO_HEADSET = 0x80000,
        DEVICE_IN_WIRED_HEADSET = 0x100000,
        DEVICE_IN_AUX_DIGITAL = 0x200000,
        DEVICE_IN_VOICE_CALL = 0x400000,
        DEVICE_IN_BACK_MIC = 0x800000,
        DEVICE_IN_DEFAULT = 0x80000000,

        DEVICE_IN_ALL = (DEVICE_IN_COMMUNICATION | DEVICE_IN_AMBIENT | DEVICE_IN_BUILTIN_MIC |
                DEVICE_IN_BLUETOOTH_SCO_HEADSET | DEVICE_IN_WIRED_HEADSET | DEVICE_IN_AUX_DIGITAL |
                DEVICE_IN_VOICE_CALL | DEVICE_IN_BACK_MIC | DEVICE_IN_DEFAULT)
    };

    // request to open a direct output with getOutput() (by opposition to sharing an output with other AudioTracks)
    enum output_flags {
        OUTPUT_FLAG_INDIRECT = 0x0,
        OUTPUT_FLAG_DIRECT = 0x1
    };

    // device categories used for setForceUse()
    enum forced_config {
        FORCE_NONE,
        FORCE_SPEAKER,
        FORCE_HEADPHONES,
        FORCE_BT_SCO,
        FORCE_BT_A2DP,
        FORCE_WIRED_ACCESSORY,
        FORCE_BT_CAR_DOCK,
        FORCE_BT_DESK_DOCK,
        FORCE_ANALOG_DOCK,
        FORCE_DIGITAL_DOCK,
        FORCE_NO_BT_A2DP,
        FORCE_SYSTEM_ENFORCED,
        NUM_FORCE_CONFIG,
        FORCE_DEFAULT = FORCE_NONE
    };

    // usages used for setForceUse()
    enum force_use {
        FOR_COMMUNICATION,
        FOR_MEDIA,
        FOR_RECORD,
        FOR_DOCK,
        FOR_SYSTEM,
        NUM_FORCE_USE
    };

    //
    // AudioPolicyService interface
    //

    // device connection states used for setDeviceConnectionState()
    enum device_connection_state {
        DEVICE_STATE_UNAVAILABLE,
        DEVICE_STATE_AVAILABLE,
        NUM_DEVICE_STATES
    };

#endif

    static uint32_t popCount(uint32_t u) {
        return popcount(u);
    }

#if 1
    static bool isOutputDevice(audio_devices device) {
        if ((popcount(device) == 1) && ((device & ~DEVICE_OUT_ALL) == 0))
             return true;
         else
             return false;
    }
    static bool isInputDevice(audio_devices device) {
        if ((popcount(device) == 1) && ((device & ~DEVICE_IN_ALL) == 0))
             return true;
         else
             return false;
    }
    static bool isA2dpDevice(audio_devices device) {
        return audio_is_a2dp_device((audio_devices_t)device);
    }
    static bool isBluetoothScoDevice(audio_devices device) {
        return audio_is_bluetooth_sco_device((audio_devices_t)device);
    }
    static bool isLowVisibility(stream_type stream) {
        return audio_is_low_visibility((audio_stream_type_t)stream);
    }
    static bool isValidFormat(uint32_t format) {
        return audio_is_valid_format((audio_format_t) format);
    }
    static bool isLinearPCM(uint32_t format) {
        return audio_is_linear_pcm((audio_format_t) format);
    }
    static bool isOutputChannel(uint32_t channel) {
        return audio_is_output_channel(channel);
    }
    static bool isInputChannel(uint32_t channel) {
        return audio_is_input_channel(channel);
    }

#endif
};

};  // namespace android

#endif // ANDROID_AUDIOSYSTEM_LEGACY_H_
