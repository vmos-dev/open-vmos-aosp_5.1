/*
 * Copyright (C) 2012 The Android Open Source Project
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

#include <errno.h>
#include <fcntl.h>

#define LOG_TAG "eS305VoiceProcessing"
//#define LOG_NDEBUG 0
#include <cutils/log.h>

#include "eS305VoiceProcessing.h"
#include <audio_effects/effect_aec.h>
#include <audio_effects/effect_ns.h>
#include <audio_effects/effect_agc.h>

extern "C" {

//------------------------------------------------------------------------------
// local definitions
//------------------------------------------------------------------------------

// number of sessions this effect bundle can be used for
#define ADNC_PFX_NUM_SESSION 8

// types of pre processing modules
enum adnc_pfx_id
{
    PFX_ID_AEC = 0,  // Acoustic Echo Cancellation
    PFX_ID_NS,       // Noise Suppression
    PFX_ID_AGC,      // Automatic Gain Control
    PFX_ID_CNT
};

// Session state
enum adnc_pfx_session_state {
    PFX_SESSION_STATE_INIT,        // initialized
    PFX_SESSION_STATE_CONFIG       // configuration received
};

// Effect/Preprocessor state
enum adnc_pfx_effect_state {
    PFX_EFFECT_STATE_INIT,         // initialized
    PFX_EFFECT_STATE_CREATED,      // effect created
    PFX_EFFECT_STATE_CONFIG,       // configuration received/disabled
    PFX_EFFECT_STATE_ACTIVE        // active/enabled
};

typedef struct adnc_pfx_session_s adnc_pfx_session_t;
typedef struct adnc_pfx_effect_s  adnc_pfx_effect_t;
typedef struct adnc_pfx_ops_s     adnc_pfx_ops_t;

// Effect operation table. Functions for all pre processors are declared in sPreProcOps[] table.
// Function pointer can be null if no action required.
struct adnc_pfx_ops_s {
    int (* create)(adnc_pfx_effect_t *fx);
    int (* init)(adnc_pfx_effect_t *fx);
    int (* reset)(adnc_pfx_effect_t *fx);
    void (* enable)(adnc_pfx_effect_t *fx);
    void (* disable)(adnc_pfx_effect_t *fx);
    int (* set_parameter)(adnc_pfx_effect_t *fx, void *param, void *value);
    int (* get_parameter)(adnc_pfx_effect_t *fx, void *param, size_t *size, void *value);
    int (* set_device)(adnc_pfx_effect_t *fx, uint32_t device);
};

// Effect context
struct adnc_pfx_effect_s {
    const struct effect_interface_s *itfe;
    uint32_t procId;                // type of pre processor (enum adnc_pfx_id)
    uint32_t state;                 // current state (enum adnc_pfx_effect_state)
    adnc_pfx_session_t *session;     // session the effect is on
    const adnc_pfx_ops_t *ops;       // effect ops table
};

// Session context
struct adnc_pfx_session_s {
    uint32_t state;                     // current state (enum adnc_pfx_session_state)
    audio_source_t audioSource;
    // FIXME not used, delete?
    //int audioSessionId;             // audio session ID
    int ioHandle;                     // handle of input stream this session is on
    uint32_t createdMsk;              // bit field containing IDs of created pre processors
    uint32_t activeMsk;               // bit field containing IDs of pre processors currently active
    struct adnc_pfx_effect_s effects[PFX_ID_CNT]; // effects in this session

    // effect settings
    //   none controllable from public API here
};

//-----------------------------------------
// forward declarations
//-----------------------------------------
int Adnc_SetNoiseSuppressionInt_l(bool);
int Adnc_SetAutomaticGainControlInt_l(bool);
int Adnc_SetEchoCancellationInt_l(bool);
int Adnc_ReevaluateUsageInt_l(audio_io_handle_t);
int Adnc_SleepInt_l();

//------------------------------------------------------------------------------
// eS305 control
//------------------------------------------------------------------------------
#define ES305_SYSFS_PATH "/sys/class/i2c-dev/i2c-4/device/4-003e/"
#define ES305_VOICE_PROCESSING_PATH ES305_SYSFS_PATH "voice_processing"
#define ES305_PRESET_PATH           ES305_SYSFS_PATH "preset"
#define ES305_TX_NS_LEVEL_PATH      ES305_SYSFS_PATH "tx_ns_level"
#define ES305_TX_AGC_ENABLE_PATH    ES305_SYSFS_PATH "tx_agc_enable"
#define ES305_AEC_ENABLE_PATH       ES305_SYSFS_PATH "aec_enable"
#define ES305_SLEEP_PATH            ES305_SYSFS_PATH "sleep"

enum eS305_controls {
    ES305_CTRL_VOICE_PROCESSING = 0,
    ES305_CTRL_PRESET,
    ES305_CTRL_TX_NS_LEVEL,
    ES305_CTRL_TX_AGC_ENABLE,
    ES305_CTRL_AEC_ENABLE,
    ES305_CTRL_SLEEP,
    ES305_NUM_CTRL
};

struct eS305_ctrl_s {
    int fd[ES305_NUM_CTRL];
    int current_preset;
    int requested_preset;
    int ioHandle;
};
typedef struct eS305_ctrl_s eS305_ctrl_t;

static eS305_ctrl_t eS305_ctrl = {
        { -1/*vp*/, -1/*preset*/, -1/*ns*/, -1/*agc*/, -1/*aec*/, -1/*sleep*/},
        ES305_PRESET_OFF  /*current_preset*/,
        ES305_PRESET_INIT /*requested_preset, an invalid preset, different from current_preset*/,
        ES305_IO_HANDLE_NONE
};

//------------------------------------------------------------------------------
// Effect descriptors
//------------------------------------------------------------------------------

// UUIDs for effect types have been generated from http://www.itu.int/ITU-T/asn1/uuid.html
// as the pre processing effects are not defined by OpenSL ES

// Acoustic Echo Cancellation
static const effect_descriptor_t aec_descriptor = {
        FX_IID_AEC_, // type
        { 0xfd90ff00, 0x0b55, 0x11e2, 0x892e, { 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66 } }, // uuid
        EFFECT_CONTROL_API_VERSION,
        (EFFECT_FLAG_TYPE_PRE_PROC|EFFECT_FLAG_DEVICE_IND),
        0, //FIXME indicate CPU load
        0, //FIXME indicate memory usage
        "Acoustic Echo Canceler",
        "Audience"
};

// Noise suppression
static const effect_descriptor_t ns_descriptor = {
        FX_IID_NS_, // type
        { 0x08fa98b0, 0x0b56, 0x11e2, 0x892e, { 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66 } }, // uuid
        EFFECT_CONTROL_API_VERSION,
        (EFFECT_FLAG_TYPE_PRE_PROC|EFFECT_FLAG_DEVICE_IND),
        0, //FIXME indicate CPU load
        0, //FIXME indicate memory usage
        "Noise Suppression",
        "Audience"
};

// Automatic Gain Control
static const effect_descriptor_t agc_descriptor = {
        FX_IID_AGC_, // type
        { 0xe9e87eb0, 0x0b55, 0x11e2, 0x892e, { 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66 } }, // uuid
        EFFECT_CONTROL_API_VERSION,
        (EFFECT_FLAG_TYPE_PRE_PROC|EFFECT_FLAG_DEVICE_IND),
        0, //FIXME indicate CPU load
        0, //FIXME indicate memory usage
        "Automatic Gain Control",
        "Audience"
};

static const effect_descriptor_t *adnc_pfx_descriptors[PFX_ID_CNT] = {
        &aec_descriptor,
        &ns_descriptor,
        &agc_descriptor
};


//------------------------------------------------------------------------------
// Helper functions
//------------------------------------------------------------------------------
static const effect_uuid_t * const sAdncUuidTable[PFX_ID_CNT] = {
        FX_IID_AEC,
        FX_IID_NS,
        FX_IID_AGC
};

const effect_uuid_t * Adnc_ProcIdToUuid(int procId)
{
    if (procId >= PFX_ID_CNT) {
        return EFFECT_UUID_NULL;
    }
    return sAdncUuidTable[procId];
}

uint32_t Adnc_UuidToProcId(const effect_uuid_t * uuid)
{
    size_t i;
    for (i = 0; i < PFX_ID_CNT; i++) {
        if (memcmp(uuid, sAdncUuidTable[i], sizeof(*uuid)) == 0) {
            break;
        }
    }
    return i;
}


//------------------------------------------------------------------------------
// Acoustic Echo Canceler (AEC)
//------------------------------------------------------------------------------
int aec_init (adnc_pfx_effect_t *effect)
{
    ALOGV("aec_init [noop]");
    return 0;
}

int aec_create(adnc_pfx_effect_t *effect)
{
    ALOGV("aec_create [noop]");
    return aec_init (effect);
}

int aec_reset(adnc_pfx_effect_t *effect)
{
    ALOGV("aec_reset [noop]");
    return 0;
}

int aec_get_parameter(adnc_pfx_effect_t     *effect,
                    void              *pParam,
                    size_t            *pValueSize,
                    void              *pValue)
{
    int status = 0;
    uint32_t param = *(uint32_t *)pParam;

    if (*pValueSize < sizeof(uint32_t)) {
        return -EINVAL;
    }
    /* NOT SUPPORTED
    switch (param) {
    case AEC_PARAM_ECHO_DELAY:
    case AEC_PARAM_PROPERTIES:
        break;
    default:
        ALOGW("aec_get_parameter() unknown param %08x value %08x", param, *(uint32_t *)pValue);
        status = -EINVAL;
        break;
    }
    return status;
    */
    return -EINVAL;
}

int aec_set_parameter (adnc_pfx_effect_t *effect, void *pParam, void *pValue)
{
    int status = 0;
    uint32_t param = *(uint32_t *)pParam;
    uint32_t value = *(uint32_t *)pValue;

    /* NOT SUPPORTED
    switch (param) {
    case AEC_PARAM_ECHO_DELAY:
    case AEC_PARAM_PROPERTIES:
        ALOGV("aec_setParameter() echo delay %d us, status %d", value, status);
        break;
    default:
        ALOGW("aec_setParameter() unknown param %08x value %08x", param, *(uint32_t *)pValue);
        status = -EINVAL;
        break;
    }
    */
    return status;
}

void aec_enable(adnc_pfx_effect_t *effect)
{
    ALOGV("aec_enable [noop]");
}

void aec_disable(adnc_pfx_effect_t *effect)
{
    ALOGV("aec_disable [noop]");
}

int aec_set_device(adnc_pfx_effect_t *effect, uint32_t device)
{
    ALOGV("aec_set_device(device=%08x) [noop]", device);

    /*
    switch(device) {
    case AUDIO_DEVICE_OUT_EARPIECE:
        break;
    case AUDIO_DEVICE_OUT_SPEAKER:
        break;
    case AUDIO_DEVICE_OUT_WIRED_HEADSET:
    case AUDIO_DEVICE_OUT_WIRED_HEADPHONE:
    default:
        break;
    }
    */

    return 0;
}

static const adnc_pfx_ops_t aec_ops = {
        aec_create,
        aec_init,
        aec_reset,
        aec_enable,
        aec_disable,
        aec_set_parameter,
        aec_get_parameter,
        aec_set_device,
};


//------------------------------------------------------------------------------
// Noise Suppression (NS)
//------------------------------------------------------------------------------
int ns_init (adnc_pfx_effect_t *effect)
{
    ALOGV("ns_init [noop]");

    return 0;
}

int ns_create(adnc_pfx_effect_t *effect)
{
    ALOGV("ns_create %p", effect);

    return ns_init (effect);
}

int ns_get_parameter(adnc_pfx_effect_t     *effect,
                   void              *pParam,
                   size_t            *pValueSize,
                   void              *pValue)
{
    int status = 0;
    return status;
}

int ns_set_parameter(adnc_pfx_effect_t *effect, void *pParam, void *pValue)
{
    int status = 0;
    return status;
}

void ns_enable(adnc_pfx_effect_t *effect)
{
    ALOGV("ns_enable [noop]");
}

void ns_disable(adnc_pfx_effect_t *effect)
{
    ALOGV("ns_disable [noop]");
}

static const adnc_pfx_ops_t ns_ops = {
        ns_create,
        ns_init,
        NULL,
        ns_enable,
        ns_disable,
        ns_set_parameter,
        ns_get_parameter,
        NULL,
};


//------------------------------------------------------------------------------
// Automatic Gain Control (AGC)
//------------------------------------------------------------------------------
int agc_init (adnc_pfx_effect_t *effect)
{
    ALOGV("agc_init  [noop]");

    return 0;
}

int agc_create(adnc_pfx_effect_t *effect)
{
    ALOGV("agc_create %p", effect);

    return agc_init (effect);
}

int agc_get_parameter(adnc_pfx_effect_t     *effect,
                   void              *pParam,
                   size_t            *pValueSize,
                   void              *pValue)
{
    int status = 0;
    return status;
}

int agc_set_parameter(adnc_pfx_effect_t *effect, void *pParam, void *pValue)
{
    int status = 0;
    return status;
}

void agc_enable(adnc_pfx_effect_t *effect)
{
    ALOGV("agc_enable [noop]");
}

void agc_disable(adnc_pfx_effect_t *effect)
{
    ALOGV("agc_disable [noop]");
}

static const adnc_pfx_ops_t agc_ops = {
        agc_create,
        agc_init,
        NULL,
        agc_enable,
        agc_disable,
        agc_set_parameter,
        agc_get_parameter,
        NULL,
};

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
static const adnc_pfx_ops_t *sPreProcOps[PFX_ID_CNT] = {
        &aec_ops,
        &ns_ops,
        &agc_ops
};

//------------------------------------------------------------------------------
// Pre-processing effect functions
//------------------------------------------------------------------------------
extern const struct effect_interface_s sEffectInterface;

#define BAD_STATE_ABORT(from, to) \
        LOG_ALWAYS_FATAL("Bad state transition from %d to %d", from, to);

void AdncSession_SetProcEnabled(adnc_pfx_session_t *session, uint32_t procId, bool enabled);

int AdncPreProFx_SetState(adnc_pfx_effect_t *effect, uint32_t state)
{
    int status = 0;
    ALOGV("AdncPreProFx_SetState procId %d, new %d old %d", effect->procId, state, effect->state);
    switch(state) {
    case PFX_EFFECT_STATE_INIT:
        switch(effect->state) {
        case PFX_EFFECT_STATE_ACTIVE:
            effect->ops->disable(effect);
            AdncSession_SetProcEnabled(effect->session, effect->procId, false);
        case PFX_EFFECT_STATE_CONFIG:
        case PFX_EFFECT_STATE_CREATED:
        case PFX_EFFECT_STATE_INIT:
            break;
        default:
            BAD_STATE_ABORT(effect->state, state);
        }
        break;
    case PFX_EFFECT_STATE_CREATED:
        switch(effect->state) {
        case PFX_EFFECT_STATE_INIT:
            status = effect->ops->create(effect);
            break;
        case PFX_EFFECT_STATE_CREATED:
        case PFX_EFFECT_STATE_ACTIVE:
        case PFX_EFFECT_STATE_CONFIG:
            ALOGE("Effect_SetState invalid transition");
            status = -ENOSYS;
            break;
        default:
            BAD_STATE_ABORT(effect->state, state);
        }
        break;
    case PFX_EFFECT_STATE_CONFIG:
        switch(effect->state) {
        case PFX_EFFECT_STATE_INIT:
            ALOGE("Effect_SetState invalid transition");
            status = -ENOSYS;
            break;
        case PFX_EFFECT_STATE_ACTIVE:
            effect->ops->disable(effect);
            AdncSession_SetProcEnabled(effect->session, effect->procId, false);
            break;
        case PFX_EFFECT_STATE_CREATED:
        case PFX_EFFECT_STATE_CONFIG:
            break;
        default:
            BAD_STATE_ABORT(effect->state, state);
        }
        break;
    case PFX_EFFECT_STATE_ACTIVE:
        switch(effect->state) {
        case PFX_EFFECT_STATE_INIT:
        case PFX_EFFECT_STATE_CREATED:
            ALOGE("Effect_SetState invalid transition");
            status = -ENOSYS;
            break;
        case PFX_EFFECT_STATE_ACTIVE:
            // enabling an already enabled effect is just ignored
            break;
        case PFX_EFFECT_STATE_CONFIG:
            effect->ops->enable(effect);
            AdncSession_SetProcEnabled(effect->session, effect->procId, true);
            break;
        default:
            BAD_STATE_ABORT(effect->state, state);
        }
        break;
    default:
        BAD_STATE_ABORT(effect->state, state);
    }
    if (status == 0) {
        effect->state = state;
    }
    return status;
}

int AdncPreProFx_Init(adnc_pfx_effect_t *effect, uint32_t procId)
{
    ALOGV(" AdncPreProFx_Init(procId=%d)", procId);
    effect->itfe = &sEffectInterface;
    effect->ops = sPreProcOps[procId];
    effect->procId = procId;
    effect->state = PFX_EFFECT_STATE_INIT;
    return 0;
}

int AdncPreProFx_Create(adnc_pfx_effect_t *effect,
               adnc_pfx_session_t *session,
               effect_handle_t  *interface)
{
    ALOGV(" AdncPreProFx_Create(effect=%p)", effect);
    effect->session = session;
    *interface = (effect_handle_t)&effect->itfe;
    return AdncPreProFx_SetState(effect, PFX_EFFECT_STATE_CREATED);
}

int AdncPreProFx_Release(adnc_pfx_effect_t *effect)
{
    return AdncPreProFx_SetState(effect, PFX_EFFECT_STATE_INIT);
}


//------------------------------------------------------------------------------
// Session functions
//------------------------------------------------------------------------------
/*
 *  Initialize a session context.
 *  Must be called with a lock on sAdncBundleLock.
 */
int AdncSession_Init_l(adnc_pfx_session_t *session)
{
    ALOGV("AdncSession_Init()");
    size_t i;
    int status = 0;

    session->state = PFX_SESSION_STATE_INIT;
    session->audioSource = AUDIO_SOURCE_DEFAULT;
    //session->audioSessionId = ES305_SESSION_ID_NONE;  // FIXME not used delete?
    session->ioHandle = ES305_IO_HANDLE_NONE;
    session->createdMsk = 0;
    session->activeMsk  = 0;
    // initialize each effect for this session context
    for (i = 0; i < PFX_ID_CNT && status == 0; i++) {
        status = AdncPreProFx_Init(&session->effects[i], i);
    }
    return status;
}

/*
 * Must be called with a lock on sAdncBundleLock.
 */
int AdncSession_CreateEffect_l(adnc_pfx_session_t *session,
                             int32_t procId,
                             effect_handle_t  *interface)
{
    int status = -ENOMEM;
    ALOGV("AdncSession_CreateEffect handle=%d procId %d, old createdMsk %08x",
            session->ioHandle, procId, session->createdMsk);

    status = AdncPreProFx_Create(&session->effects[procId], session, interface);
    if (status >= 0) {
        ALOGV("  AdncSession_CreateEffect OK");
        session->createdMsk |= (1 << procId);
    }
    return status;
}

int AdncSession_SetConfig(adnc_pfx_session_t *session, effect_config_t *config)
{
    ALOGV("AdncSession_SetConfig [noop]");
    return 0;
}

void AdncSession_GetConfig(adnc_pfx_session_t *session, effect_config_t *config)
{
    ALOGV("AdncSession_GetConfig [noop]");
}

int AdncSession_SetReverseConfig(adnc_pfx_session_t *session, effect_config_t *config)
{
    ALOGV("AdncSession_SetReverseConfig [noop]");
    return 0;
}

void AdncSession_GetReverseConfig(adnc_pfx_session_t *session, effect_config_t *config)
{
    ALOGV("AdncSession_GetReverseConfig [noop]");
}

void AdncSession_SetProcEnabled(adnc_pfx_session_t *session, uint32_t procId, bool enabled)
{
    ALOGV("AdncSession_SetProcEnabled [noop] proc %d, enabled %d", procId, enabled);
    //no need to reevaluate session settings, if recording is currently ongoing, we'll reevaluate
    //  through eS305_AddEffect()
}

int AdncSession_SetSource(adnc_pfx_session_t *session, audio_source_t source)
{
    session->audioSource = source;
    return 0;
}

//------------------------------------------------------------------------------
// Bundle functions
//------------------------------------------------------------------------------
#define ADNC_BUNDLE_NO_INIT 1
static int sAdncBundleInitStatus = ADNC_BUNDLE_NO_INIT;
static adnc_pfx_session_t sAdncSessions[ADNC_PFX_NUM_SESSION];
static pthread_mutex_t sAdncBundleLock;

/* Returns a session context for the given IO handle
 * Returns an existing session context if the IO handle matches, initializes a new one otherwise.
 * Returns NULL if no more session contexts are available
 * Must be called with a lock on sAdncBundleLock
 */
adnc_pfx_session_t *AdncBundle_GetSession_l(int32_t procId, int32_t sessionId, int32_t ioId)
{
    size_t i;
    for (i = 0; i < ADNC_PFX_NUM_SESSION; i++) {
        if (sAdncSessions[i].ioHandle == ioId) {
            return &sAdncSessions[i];
        }
    }
    for (i = 0; i < ADNC_PFX_NUM_SESSION; i++) {
        if (sAdncSessions[i].ioHandle == ES305_IO_HANDLE_NONE) {
            //sAdncSessions[i].audioSessionId = sessionId; // FIXME not used delete?
            sAdncSessions[i].ioHandle = ioId;
            return &sAdncSessions[i];
        }
    }
    ALOGV("AdncBundle_GetSession_l");
    return NULL;
}

/*
 * Must be called with a lock on sAdncBundleLock.
 */
int AdncBundle_Init_l() {
    size_t i;
    int status = 0;

    if (sAdncBundleInitStatus <= 0) {
        return sAdncBundleInitStatus;
    }
    // initialize all the session contexts that this bundle supports
    for (i = 0; i < ADNC_PFX_NUM_SESSION && status == 0; i++) {
        status = AdncSession_Init_l(&sAdncSessions[i]);
    }
    sAdncBundleInitStatus = status;
    return sAdncBundleInitStatus;
}

/*
 * Must be called with a lock on sAdncBundleLock.
 */
int AdncBundle_Release_l() {
    ALOGV("AdncBundle_Release_l()");

    Adnc_SleepInt_l();

    for (int i = 0 ; i < ES305_NUM_CTRL ; i++) {
        if (eS305_ctrl.fd[i] >= 0) {
            close(eS305_ctrl.fd[i]);
        }
        eS305_ctrl.fd[i] = -1;
    }
    return 0;
}

const effect_descriptor_t *AdncBundle_GetDescriptor(const effect_uuid_t *uuid)
{
    size_t i;
    for (i = 0; i < PFX_ID_CNT; i++) {
        if (memcmp(&adnc_pfx_descriptors[i]->uuid, uuid, sizeof(effect_uuid_t)) == 0) {
            return adnc_pfx_descriptors[i];
        }
    }
    return NULL;
}

/*
 * Debug only: display session contexts
 */
void AdncBundle_logv_dumpSessions() {
    ALOGV("Sessions:");
    for (int i=0 ; i<ADNC_PFX_NUM_SESSION ; i++) {
        ALOGV(" session %d handle=%d cre=%2x act=%2x",
           i, sAdncSessions[i].ioHandle, sAdncSessions[i].createdMsk, sAdncSessions[i].activeMsk);
    }
}

//------------------------------------------------------------------------------
// Effect Control Interface Implementation
//------------------------------------------------------------------------------
int AdncVoiceProcessingFx_Command(effect_handle_t  self,
                            uint32_t            cmdCode,
                            uint32_t            cmdSize,
                            void                *pCmdData,
                            uint32_t            *replySize,
                            void                *pReplyData)
{
    adnc_pfx_effect_t * effect = (adnc_pfx_effect_t *) self;
    int retsize;
    int status;

    if (effect == NULL){
        return -EINVAL;
    }

    ALOGV("AdncVoiceProcessingFx_Command: command %d cmdSize %d",cmdCode, cmdSize);

    switch (cmdCode){
        case EFFECT_CMD_INIT:
            if (pReplyData == NULL || *replySize != sizeof(int)){
                return -EINVAL;
            }
            if (effect->ops->init) {
                effect->ops->init(effect);
            }
            *(int *)pReplyData = 0;
            break;

        case EFFECT_CMD_SET_CONFIG: {
            if (pCmdData    == NULL||
                cmdSize     != sizeof(effect_config_t)||
                pReplyData  == NULL||
                *replySize  != sizeof(int)){
                ALOGV("AdncVoiceProcessingFx_Command cmdCode Case: "
                        "EFFECT_CMD_SET_CONFIG: ERROR");
                return -EINVAL;
            }

            *(int *)pReplyData = AdncSession_SetConfig(effect->session, (effect_config_t *)pCmdData);

            if (*(int *)pReplyData != 0) {
                break;
            }
            if (effect->state != PFX_EFFECT_STATE_ACTIVE) {
                *(int *)pReplyData = AdncPreProFx_SetState(effect, PFX_EFFECT_STATE_CONFIG);
            }
            } break;

        case EFFECT_CMD_GET_CONFIG:
            if (pReplyData == NULL ||
                *replySize != sizeof(effect_config_t)) {
                ALOGV("\tLVM_ERROR : AdncVoiceProcessingFx_Command cmdCode Case: "
                        "EFFECT_CMD_GET_CONFIG: ERROR");
                return -EINVAL;
            }

            AdncSession_GetConfig(effect->session, (effect_config_t *)pReplyData);
            break;

        case EFFECT_CMD_SET_CONFIG_REVERSE:
            if (pCmdData == NULL ||
                cmdSize != sizeof(effect_config_t) ||
                pReplyData == NULL ||
                *replySize != sizeof(int)) {
                ALOGV("AdncVoiceProcessingFx_Command cmdCode Case: "
                        "EFFECT_CMD_SET_CONFIG_REVERSE: ERROR");
                return -EINVAL;
            }
            *(int *)pReplyData = AdncSession_SetReverseConfig(effect->session,
                                                          (effect_config_t *)pCmdData);
            if (*(int *)pReplyData != 0) {
                break;
            }
            break;

        case EFFECT_CMD_GET_CONFIG_REVERSE:
            if (pReplyData == NULL ||
                *replySize != sizeof(effect_config_t)){
                ALOGV("AdncVoiceProcessingFx_Command cmdCode Case: "
                        "EFFECT_CMD_GET_CONFIG_REVERSE: ERROR");
                return -EINVAL;
            }
            AdncSession_GetReverseConfig(effect->session, (effect_config_t *)pCmdData);
            break;

        case EFFECT_CMD_RESET:
            if (effect->ops->reset) {
                effect->ops->reset(effect);
            }
            break;

        case EFFECT_CMD_GET_PARAM:{
            if (pCmdData == NULL ||
                    cmdSize < (int)sizeof(effect_param_t) ||
                    pReplyData == NULL ||
                    *replySize < (int)sizeof(effect_param_t)){
                ALOGV("AdncVoiceProcessingFx_Command cmdCode Case: "
                        "EFFECT_CMD_GET_PARAM: ERROR");
                return -EINVAL;
            }
            effect_param_t *p = (effect_param_t *)pCmdData;

            memcpy(pReplyData, pCmdData, sizeof(effect_param_t) + p->psize);

            p = (effect_param_t *)pReplyData;

            int voffset = ((p->psize - 1) / sizeof(int32_t) + 1) * sizeof(int32_t);

            if (effect->ops->get_parameter) {
                p->status = effect->ops->get_parameter(effect, p->data,
                                                       (size_t  *)&p->vsize,
                                                       p->data + voffset);
                *replySize = sizeof(effect_param_t) + voffset + p->vsize;
            }
        } break;

        case EFFECT_CMD_SET_PARAM:{
            if (pCmdData == NULL||
                    cmdSize < (int)sizeof(effect_param_t) ||
                    pReplyData == NULL ||
                    *replySize != sizeof(int32_t)){
                ALOGV("AdncVoiceProcessingFx_Command cmdCode Case: "
                        "EFFECT_CMD_SET_PARAM: ERROR");
                return -EINVAL;
            }
            effect_param_t *p = (effect_param_t *) pCmdData;

            if (p->psize != sizeof(int32_t)){
                ALOGV("AdncVoiceProcessingFx_Command cmdCode Case: "
                        "EFFECT_CMD_SET_PARAM: ERROR, psize is not sizeof(int32_t)");
                return -EINVAL;
            }
            if (effect->ops->set_parameter) {
                *(int *)pReplyData = effect->ops->set_parameter(effect,
                                                                (void *)p->data,
                                                                p->data + p->psize);
            }
        } break;

        case EFFECT_CMD_ENABLE:
            if (pReplyData == NULL || *replySize != sizeof(int)){
                ALOGV("AdncVoiceProcessingFx_Command cmdCode Case: EFFECT_CMD_ENABLE: ERROR");
                return -EINVAL;
            }
            *(int *)pReplyData = AdncPreProFx_SetState(effect, PFX_EFFECT_STATE_ACTIVE);
            break;

        case EFFECT_CMD_DISABLE:
            if (pReplyData == NULL || *replySize != sizeof(int)){
                ALOGV("AdncVoiceProcessingFx_Command cmdCode Case: EFFECT_CMD_DISABLE: ERROR");
                return -EINVAL;
            }
            *(int *)pReplyData  = AdncPreProFx_SetState(effect, PFX_EFFECT_STATE_CONFIG);
            break;

        case EFFECT_CMD_SET_DEVICE:
        case EFFECT_CMD_SET_INPUT_DEVICE:
            if (pCmdData == NULL ||
                cmdSize != sizeof(uint32_t)) {
                ALOGV("AdncVoiceProcessingFx_Command cmdCode Case: EFFECT_CMD_SET_DEVICE: ERROR");
                return -EINVAL;
            }

            if (effect->ops->set_device) {
                effect->ops->set_device(effect, *(uint32_t *)pCmdData);
            }
            break;

        case EFFECT_CMD_SET_VOLUME:
        case EFFECT_CMD_SET_AUDIO_MODE:
        case EFFECT_CMD_SET_FEATURE_CONFIG:
            break;

        case EFFECT_CMD_SET_AUDIO_SOURCE:
            if (pCmdData == NULL ||
                    cmdSize != sizeof(uint32_t)) {
                ALOGV("AdncVoiceProcessingFx_Command cmdCode Case: EFFECT_CMD_SET_AUDIO_SOURCE: ERROR");
                return -EINVAL;
            }
            return AdncSession_SetSource(effect->session, (audio_source_t) *(uint32_t *)pCmdData);
            break;

        default:
            return -EINVAL;
    }
    return 0;
}


int AdncVoiceProcessingFx_GetDescriptor(effect_handle_t   self,
                                  effect_descriptor_t *pDescriptor)
{
    adnc_pfx_effect_t * effect = (adnc_pfx_effect_t *) self;

    if (effect == NULL || pDescriptor == NULL) {
        return -EINVAL;
    }

    memcpy(pDescriptor, adnc_pfx_descriptors[effect->procId], sizeof(effect_descriptor_t));

    return 0;
}


// effect_handle_t interface implementation for effect
const struct effect_interface_s sEffectInterface = {
        NULL, /* Process */
        AdncVoiceProcessingFx_Command,
        AdncVoiceProcessingFx_GetDescriptor,
        NULL
};
//------------------------------------------------------------------------------
// Effect Library Interface Implementation
//------------------------------------------------------------------------------

int adnc_create(const effect_uuid_t *uuid,
            int32_t         sessionId,
            int32_t         ioId,
            effect_handle_t *pInterface)
{
    ALOGV("adnc_create: uuid: %08x session %d IO: %d", uuid->timeLow, sessionId, ioId);

    int status = 0;
    const effect_descriptor_t *desc;
    adnc_pfx_session_t *session;
    uint32_t procId;

    pthread_mutex_lock(&sAdncBundleLock);

    if (AdncBundle_Init_l() != 0) {
        status = sAdncBundleInitStatus;
        goto exit;
    }

    desc =  AdncBundle_GetDescriptor(uuid);
    if (desc == NULL) {
        ALOGW("  adnc_create: fx not found uuid: %08x", uuid->timeLow);
        status = -EINVAL;
        goto exit;
    }
    procId = Adnc_UuidToProcId(&desc->type);

    session = AdncBundle_GetSession_l(procId, sessionId, ioId);
    if (session == NULL) {
        ALOGW("  adnc_create: no more session available");
        status = -EINVAL;
        goto exit;
    }

    status = AdncSession_CreateEffect_l(session, procId, pInterface);

    if (status < 0 && session->createdMsk == 0) {
        session->ioHandle = ES305_IO_HANDLE_NONE;
    }

exit:
    pthread_mutex_unlock(&sAdncBundleLock);
    return status;
}

int adnc_release(effect_handle_t interface)
{
    int i, status = 0;
    ALOGV("adnc_release %p", interface);

    // the effect handle comes from the effect framework, ok to cast
    const adnc_pfx_effect_t * fx = (adnc_pfx_effect_t *) interface;

    const uint32_t removalMsk = ~(1 << fx->procId);

    pthread_mutex_lock(&sAdncBundleLock);

    if (AdncBundle_Init_l() != 0) {
        status = sAdncBundleInitStatus;
        goto exit;
    }

    if (fx->session->ioHandle == 0) {
        status = -EINVAL;
        goto exit;
    }

    // effect is released, flag it as inactive and not created
    fx->session->createdMsk &= removalMsk;
    fx->session->activeMsk  &= removalMsk;

    // configuration has changed, reevaluate
    status = Adnc_ReevaluateUsageInt_l(fx->session->ioHandle);
    // not checking the return status here: if there was an error,
    //    we still need to free the session and wouldn't exit here

    // free session if it has no more effects
    if (fx->session->createdMsk == 0) {
        ALOGV(" resetting session on handle %d after effect release", fx->session->ioHandle);
        const int statusInit = AdncSession_Init_l(fx->session);
        if (status == 0) {
            status = statusInit;
        }
    }

exit:
    pthread_mutex_unlock(&sAdncBundleLock);
    return status;
}

int adnc_get_descriptor(const effect_uuid_t *uuid, effect_descriptor_t *pDescriptor) {
    if (pDescriptor == NULL || uuid == NULL){
        ALOGV("adnc_get_descriptor() invalid params");
        return -EINVAL;
    }

    const effect_descriptor_t *desc = AdncBundle_GetDescriptor(uuid);
    if (desc == NULL) {
        ALOGV("adnc_get_descriptor() not found");
        return -EINVAL;
    }

    ALOGV("adnc_get_descriptor() got fx %s", desc->name);

    memcpy(pDescriptor, desc, sizeof(effect_descriptor_t));
    return 0;
}

audio_effect_library_t AUDIO_EFFECT_LIBRARY_INFO_SYM = {
    .tag = AUDIO_EFFECT_LIBRARY_TAG,
    .version = EFFECT_LIBRARY_API_VERSION,
    .name = "Audience Voice Preprocessing Library",
    .implementor = "The Android Open Source Project",
    .create_effect = adnc_create,
    .release_effect = adnc_release,
    .get_descriptor = adnc_get_descriptor
};

//-------------------------------------------------------
// eS305 control interface
//-------------------------------------------------------
int Adnc_SetAutomaticGainControlInt_l(bool agc_on)
{
    ALOGV("Adnc_SetAutomaticGainControlInt_l(%d)", agc_on);

    if (eS305_ctrl.fd[ES305_CTRL_TX_AGC_ENABLE] < 0) {
        ALOGV("  opening eS305 path for agc");
        eS305_ctrl.fd[ES305_CTRL_TX_AGC_ENABLE] = open(ES305_TX_AGC_ENABLE_PATH, O_RDWR);
        if (eS305_ctrl.fd[ES305_CTRL_TX_AGC_ENABLE] < 0) {
            ALOGE("  Cannot open eS305 path for agc: %s", strerror(errno));
            return -ENODEV;
        }
    }

    if (agc_on) {
        write(eS305_ctrl.fd[ES305_CTRL_TX_AGC_ENABLE], ES305_AGC_ON, strlen(ES305_AGC_ON));
    } else {
        write(eS305_ctrl.fd[ES305_CTRL_TX_AGC_ENABLE], ES305_AEC_OFF, strlen(ES305_AGC_OFF));
    }
    return 0;
}

int Adnc_SetEchoCancellationInt_l(bool aec_on)
{
    ALOGV("Adnc_SetEchoCancellationInt_l(%d)", aec_on);

    if (eS305_ctrl.fd[ES305_CTRL_AEC_ENABLE] < 0) {
        ALOGV("  opening eS305 path for aec");
        eS305_ctrl.fd[ES305_CTRL_AEC_ENABLE] = open(ES305_AEC_ENABLE_PATH, O_RDWR);
        if (eS305_ctrl.fd[ES305_CTRL_AEC_ENABLE] < 0) {
            ALOGE("  Cannot open eS305 path for aec: %s", strerror(errno));
            return -ENODEV;
        }
    }

    if (aec_on) {
        write(eS305_ctrl.fd[ES305_CTRL_AEC_ENABLE], ES305_AEC_ON, strlen(ES305_AEC_ON));
    } else {
        write(eS305_ctrl.fd[ES305_CTRL_AEC_ENABLE], ES305_AEC_OFF, strlen(ES305_AEC_OFF));
    }
    return 0;
}

int Adnc_SetNoiseSuppressionInt_l(bool ns_on)
{
    ALOGV("Adnc_SetNoiseSuppressionInt(%d)", ns_on);

    if (eS305_ctrl.fd[ES305_CTRL_TX_NS_LEVEL] < 0) {
        ALOGV("  opening eS305 path for ns");
        eS305_ctrl.fd[ES305_CTRL_TX_NS_LEVEL] = open(ES305_TX_NS_LEVEL_PATH, O_RDWR);
        if (eS305_ctrl.fd[ES305_CTRL_TX_NS_LEVEL] < 0) {
            ALOGE("  Cannot open eS305 path for ns: %s", strerror(errno));
            return -ENODEV;
        }
    }

    if (ns_on) {
        if (eS305_ctrl.requested_preset == ES305_PRESET_ASRA_HANDHELD) {
            ALOGV("  setting ns to %s", ES305_NS_VOICE_REC_HANDHELD_ON);
            write(eS305_ctrl.fd[ES305_CTRL_TX_NS_LEVEL],
                    ES305_NS_VOICE_REC_HANDHELD_ON, strlen(ES305_NS_VOICE_REC_HANDHELD_ON));
        } else if ((eS305_ctrl.requested_preset == ES305_PRESET_ASRA_DESKTOP)
                || (eS305_ctrl.requested_preset == ES305_PRESET_ASRA_HEADSET)) {
            ALOGV("  setting ns to %s", ES305_NS_VOICE_REC_SINGLE_MIC_ON);
            write(eS305_ctrl.fd[ES305_CTRL_TX_NS_LEVEL],
                    ES305_NS_VOICE_REC_SINGLE_MIC_ON, strlen(ES305_NS_VOICE_REC_SINGLE_MIC_ON));
        } else {
            ALOGV("  setting ns to %s", ES305_NS_DEFAULT_ON);
            write(eS305_ctrl.fd[ES305_CTRL_TX_NS_LEVEL],
                    ES305_NS_DEFAULT_ON, strlen(ES305_NS_DEFAULT_ON));
        }
    } else {
        ALOGV("  setting ns to %s", ES305_NS_OFF);
        write(eS305_ctrl.fd[ES305_CTRL_TX_NS_LEVEL], ES305_NS_OFF, strlen(ES305_NS_OFF));
    }
    return 0;
}

int Adnc_SetVoiceProcessingInt_l(bool vp_on)
{
    if (eS305_ctrl.fd[ES305_CTRL_VOICE_PROCESSING] < 0) {
        ALOGV("  opening eS305 path for VP");
        eS305_ctrl.fd[ES305_CTRL_VOICE_PROCESSING] = open(ES305_VOICE_PROCESSING_PATH, O_RDWR);
        if (eS305_ctrl.fd[ES305_CTRL_VOICE_PROCESSING] < 0) {
            ALOGE("    cannot open eS305 path for VP: %s", strerror(errno));
            return -ENODEV;
        }
    }
    if (vp_on) {
        write(eS305_ctrl.fd[ES305_CTRL_VOICE_PROCESSING], ES305_ON, strlen(ES305_ON));
    } else {
        write(eS305_ctrl.fd[ES305_CTRL_VOICE_PROCESSING], ES305_OFF, strlen(ES305_OFF));
    }
    return 0;
}

/*
 * Put the eS305 to sleep
 * Post condition when no error: eS305_ctrl.current_preset == ES305_PRESET_OFF
 */
int Adnc_SleepInt_l()
{
    if (eS305_ctrl.current_preset == ES305_PRESET_OFF) {
        return 0;
    }

    ALOGV(" Adnc_SleepInt()_l setting VP off + sleep 1");

    Adnc_SetVoiceProcessingInt_l(false /*vp_on*/);

    ALOGV("  Adnc_SetSleepInt_l");
    if (eS305_ctrl.fd[ES305_CTRL_SLEEP] < 0) {
        ALOGV("  opening eS305 path for sleep");
        eS305_ctrl.fd[ES305_CTRL_SLEEP] = open(ES305_SLEEP_PATH, O_RDWR);
        if (eS305_ctrl.fd[ES305_CTRL_SLEEP] < 0) {
            ALOGE("    cannot open eS305 path for sleep: %s", strerror(errno));
            return -ENODEV;
        }
    }

    write(eS305_ctrl.fd[ES305_CTRL_SLEEP], ES305_ON, strlen(ES305_ON));

    eS305_ctrl.current_preset = ES305_PRESET_OFF;

    return 0;
}

/*
 * Apply the eS305_ctrl.requested_preset preset after turning VP on
 * Post condition when no error: eS305_ctrl.current_preset == eS305_ctrl.requested_preset
 */
int Adnc_ApplyPresetInt_l()
{
    ALOGV("Adnc_ApplyPresetInt() current_preset=%d, requested_preset=%d",
            eS305_ctrl.current_preset, eS305_ctrl.requested_preset);

    if (eS305_ctrl.requested_preset == eS305_ctrl.current_preset) {
        ALOGV("  nothing to do, preset %d is current", eS305_ctrl.requested_preset);
        return 0;
    }

    // preset off implies going to sleep
    if (eS305_ctrl.requested_preset == ES305_PRESET_OFF) {
        return Adnc_SleepInt_l();
    }

    // voice processing must be on before setting the preset
    if ((eS305_ctrl.current_preset == ES305_PRESET_OFF)
            || (eS305_ctrl.current_preset == ES305_PRESET_INIT)) {
        const int status = Adnc_SetVoiceProcessingInt_l(true /*vp_on*/);
        if (status != 0) {
            return status;
        }
    }

    if (eS305_ctrl.fd[ES305_CTRL_PRESET] < 0) {
        ALOGV("  opening eS305 path for PRESET");
        eS305_ctrl.fd[ES305_CTRL_PRESET] = open(ES305_PRESET_PATH, O_RDWR);
    }
    if (eS305_ctrl.fd[ES305_CTRL_PRESET] < 0) {
        ALOGE("  Cannot open eS305 path for PRESET: %s", strerror(errno));
        return -ENODEV;
    }

    char str[8];
    sprintf(str, "%d", eS305_ctrl.requested_preset);
    write(eS305_ctrl.fd[ES305_CTRL_PRESET], str, strlen(str));

    eS305_ctrl.current_preset = eS305_ctrl.requested_preset;

    return 0;
}


/*
 * Apply the settings of given the session context
 */
int Adnc_ApplySettingsFromSessionContextInt_l(adnc_pfx_session_t * session)
{
    ALOGV("Adnc_ApplySettingsFromSessionContextInt_l cre=%2x ac=%2x handle=%d",
                  session->createdMsk, session->activeMsk, session->ioHandle);
    int status = 0;

    if (session->ioHandle != eS305_ctrl.ioHandle) {
        return status;
    }

    // NS: special case of noise suppression, always reset according to effect state
    //     as default desirable value might differ from the preset
    const bool ns_on = ((session->activeMsk & (1 << PFX_ID_NS)) != 0);
    status = Adnc_SetNoiseSuppressionInt_l(ns_on /*ns_on*/);

    // AEC
    if (session->createdMsk & (1 << PFX_ID_AEC)) {        /* the effect has been created */
        const bool aec_on = ((session->activeMsk & (1 << PFX_ID_AEC)) != 0);
        int aec_status = Adnc_SetEchoCancellationInt_l(aec_on /*aec_on*/);
        if (status == 0) {
            status = aec_status;
        }
    }

    // AGC
    if (session->createdMsk & (1 << PFX_ID_AGC)) {        /* the effect has been created */
        const bool agc_on = ((session->activeMsk & (1 << PFX_ID_AGC)) != 0);
        int agc_status = Adnc_SetAutomaticGainControlInt_l(agc_on /*agc_on*/);
        if (status == 0) {
            status = agc_status;
        }
    }

    return status;
}

/*
 * Return a value between 0 and ADNC_PFX_NUM_SESSION-1 if a session context has the given handle,
 *        -1 if the handle isn't in handled by one of the sessions.
 * Must be called with a lock on sAdncBundleLock
 */
int Adnc_SessionNumberForHandle_l(audio_io_handle_t handle)
{
    for (int i = 0 ; i < ADNC_PFX_NUM_SESSION ; i++) {
        if (sAdncSessions[i].ioHandle == handle) {
            return i;
        }
    }
    return -1;
}


/*
 * Apply the settings of the session matching the given IO handle.
 * Must be called with a lock on sAdncBundleLock
 */
int Adnc_ApplySettingsForHandleInt_l(audio_io_handle_t handle)
{
    ALOGV(" Adnc_ApplySettingsForHandleInt_l(handle=%d)", handle);
    // indicates whether this effect bundle currently has a session context for this IO handle
    bool hasSession = false;
    int status = 0;
    int i;

    if (sAdncBundleInitStatus != 0) {
        // This assumes that the default config of the eS305 after setting a preset
        //    is the correct configuration.
        ALOGV(" no effect settings to apply for IO handle %d, no effect bundle", handle);
        return status;
    }

    const int sessionId = Adnc_SessionNumberForHandle_l(handle);
    if (sessionId >= 0) {
        ALOGV("  applying settings from session num %d", sessionId);
        status = Adnc_ApplySettingsFromSessionContextInt_l( &sAdncSessions[sessionId] );
    }
    else {
        ALOGV("  no session found for handle %d", handle);
    }

    return status;
}

/*
 * Reevaluate the usage of the eS305 based on the given IO handle.
 * Must be called with a lock on sAdncBundleLock
 */
int Adnc_ReevaluateUsageInt_l(audio_io_handle_t handle)
{
    ALOGV(" Adnc_ReevaluateUsageInt_l(handle=%d) current_preset=%d requested_preset=%d",
            handle, eS305_ctrl.current_preset, eS305_ctrl.requested_preset);
    int status = 0;
    if ((eS305_ctrl.requested_preset == ES305_PRESET_OFF) || (handle == ES305_IO_HANDLE_NONE)) {
        status = Adnc_SleepInt_l();
    } else {
        const int sessionId = Adnc_SessionNumberForHandle_l(handle);
        if (sessionId >= 0) {
            // recording active, use the preset only if there is an effect,
            //                   reset preset to off otherwise
            if (sAdncSessions[sessionId].activeMsk != 0) {
                status = Adnc_ApplyPresetInt_l();
                if (status == 0) {
                    //apply the settings of the session associated with the handle (if any)
                    status = Adnc_ApplySettingsForHandleInt_l(handle);
                }
            } else {
                status = Adnc_SleepInt_l();
            }
        }
    }
    return status;
}


//-------------------------------------------------------
// eS305 public control interface from HAL
//-------------------------------------------------------
int eS305_UsePreset(int preset)
{
    ALOGV("eS305_UsePreset(%d) current=%d handle=%d",
            preset, eS305_ctrl.current_preset, eS305_ctrl.ioHandle);

    int status = 0;

    pthread_mutex_lock(&sAdncBundleLock);

    //if (preset != -1) { AdncBundle_logv_dumpSessions(); }

    // allow preset transition from any preset to any other during recording,
    //    except from one ASRA preset to another
    if (eS305_ctrl.ioHandle != ES305_IO_HANDLE_NONE) {
        switch(eS305_ctrl.current_preset) {
        case ES305_PRESET_ASRA_HANDHELD:
        case ES305_PRESET_ASRA_DESKTOP:
        case ES305_PRESET_ASRA_HEADSET:
            switch(preset) {
            case ES305_PRESET_ASRA_HANDHELD:
            case ES305_PRESET_ASRA_DESKTOP:
            case ES305_PRESET_ASRA_HEADSET:
                ALOGV("  not switching from ASRA preset %d to %d during voice recognition",
                        eS305_ctrl.current_preset, preset);
                status = -EINVAL;
                goto exit;
            default:
                // transitioning from ASRA to non-ASRA: valid
                break;
            }
            break;
        default:
            // transitioning from non-ASRA: valid
            break;
        }
    }

    eS305_ctrl.requested_preset = preset;

    status = AdncBundle_Init_l();
    if (status != 0) {
        ALOGE(" error applying preset, bundle failed to initialize");
        goto exit;
    }

    status = Adnc_ReevaluateUsageInt_l(eS305_ctrl.ioHandle);

exit:
    pthread_mutex_unlock(&sAdncBundleLock);
    return status;
}


int eS305_SetActiveIoHandle(audio_io_handle_t handle)
{
    ALOGV("eS305_SetActiveIoHandle(%d)", handle);

    pthread_mutex_lock(&sAdncBundleLock);

    int status = AdncBundle_Init_l();
    if (status != 0) {
        ALOGE(" error setting active handle, bundle failed to initialize");
        pthread_mutex_unlock(&sAdncBundleLock);
        return status;
    }

    status = Adnc_ReevaluateUsageInt_l(handle);

    if (status == 0) {
        eS305_ctrl.ioHandle = handle;
    } else {
        ALOGE("  failed to update for new handle %d (current preset = %d)",
                handle, eS305_ctrl.current_preset);
    }

    pthread_mutex_unlock(&sAdncBundleLock);

    return status;
}


int eS305_AddEffect(effect_descriptor_t * descr, audio_io_handle_t handle)
{
    ALOGV("eS305_AddEffect(handle=%d)", handle);

    pthread_mutex_lock(&sAdncBundleLock);

    int status = AdncBundle_Init_l();
    if (status != 0) {
        ALOGE(" error setting adding effect, bundle failed to initialize");
        pthread_mutex_unlock(&sAdncBundleLock);
        return status;
    }

    if (descr == NULL){
        ALOGV(" eS305_AddEffect() ERROR effect descriptor is NULL");
        pthread_mutex_unlock(&sAdncBundleLock);
        return -EINVAL;
    }

    uint32_t procId = Adnc_UuidToProcId(&descr->type);

    adnc_pfx_session_t * session = AdncBundle_GetSession_l(
            procId, ES305_SESSION_ID_NONE, handle/*ioId*/);

    if (session != NULL) {
        // mark the effect as active
        session->activeMsk  |= (1 << procId);

        // update settings if necessary
        Adnc_ReevaluateUsageInt_l(session->ioHandle);
    }

    pthread_mutex_unlock(&sAdncBundleLock);

    return status;
}


int eS305_RemoveEffect(effect_descriptor_t * descr, audio_io_handle_t handle)
{
    ALOGV("eS305_RemoveEffect()");

    pthread_mutex_lock(&sAdncBundleLock);

    int status = AdncBundle_Init_l();
    if (status != 0) {
        ALOGE(" error setting removing effect, bundle failed to initialize");
        pthread_mutex_unlock(&sAdncBundleLock);
        return status;
    }

    if (descr == NULL){
        ALOGV(" eS305_AddEffect() ERROR effect descriptor is NULL");
        pthread_mutex_unlock(&sAdncBundleLock);
        return -EINVAL;
    }

    uint32_t procId = Adnc_UuidToProcId(&descr->type);

    adnc_pfx_session_t * session = AdncBundle_GetSession_l(
            procId, ES305_SESSION_ID_NONE, handle/*ioId*/);

    if (session != NULL) {
        // mark the effect as inactive
        session->activeMsk  &= ~(1 << procId);

        // update settings if necessary
        Adnc_ReevaluateUsageInt_l(session->ioHandle);
    }

    pthread_mutex_unlock(&sAdncBundleLock);

    return status;
}


int eS305_Release() {
    ALOGV("eS305_Release()");

    pthread_mutex_lock(&sAdncBundleLock);

    AdncBundle_Release_l();

    pthread_mutex_unlock(&sAdncBundleLock);

    return 0;
}

} // extern "C"
