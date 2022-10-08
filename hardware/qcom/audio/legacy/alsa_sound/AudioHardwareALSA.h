/* AudioHardwareALSA.h
 **
 ** Copyright 2008-2010, Wind River Systems
 ** Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **     http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */

#ifndef ANDROID_AUDIO_HARDWARE_ALSA_H
#define ANDROID_AUDIO_HARDWARE_ALSA_H

#define QCOM_CSDCLIENT_ENABLED 1

#include <utils/List.h>
#include <hardware_legacy/AudioHardwareBase.h>

#include <hardware_legacy/AudioHardwareInterface.h>
#include <hardware_legacy/AudioSystemLegacy.h>
#include <system/audio.h>
#include <hardware/audio.h>
#include <utils/threads.h>
#include <dlfcn.h>

#ifdef QCOM_USBAUDIO_ENABLED
#include <AudioUsbALSA.h>
#endif

extern "C" {
   #include <sound/asound.h>
   #include "alsa_audio.h"
   #include "msm8960_use_cases.h"
}

#include <hardware/hardware.h>

namespace android_audio_legacy
{
using android::List;
using android::Mutex;
class AudioHardwareALSA;

/**
 * The id of ALSA module
 */
#define ALSA_HARDWARE_MODULE_ID "alsa"
#define ALSA_HARDWARE_NAME      "alsa"

#define DEFAULT_SAMPLING_RATE 48000
#define DEFAULT_CHANNEL_MODE  2
#define VOICE_SAMPLING_RATE   8000
#define VOICE_CHANNEL_MODE    1
#define PLAYBACK_LATENCY      170000
#define RECORD_LATENCY        96000
#define VOICE_LATENCY         85333
#define DEFAULT_BUFFER_SIZE   4096
//4032 = 336(kernel buffer size) * 2(bytes pcm_16) * 6(number of channels)
#define DEFAULT_MULTI_CHANNEL_BUF_SIZE    4032
#define DEFAULT_VOICE_BUFFER_SIZE   2048
#define PLAYBACK_LOW_LATENCY_BUFFER_SIZE   1024
#define PLAYBACK_LOW_LATENCY  22000
#define PLAYBACK_LOW_LATENCY_MEASURED  42000
#define DEFAULT_IN_BUFFER_SIZE 320
#define MIN_CAPTURE_BUFFER_SIZE_PER_CH   320
#define MAX_CAPTURE_BUFFER_SIZE_PER_CH   2048
#define FM_BUFFER_SIZE        1024

#define VOIP_SAMPLING_RATE_8K 8000
#define VOIP_SAMPLING_RATE_16K 16000
#define VOIP_DEFAULT_CHANNEL_MODE  1
#define VOIP_BUFFER_SIZE_8K    320
#define VOIP_BUFFER_SIZE_16K   640
#define VOIP_BUFFER_MAX_SIZE   VOIP_BUFFER_SIZE_16K
#define VOIP_PLAYBACK_LATENCY      6400
#define VOIP_RECORD_LATENCY        6400

#define MODE_IS127              0x2
#define MODE_4GV_NB             0x3
#define MODE_4GV_WB             0x4
#define MODE_AMR                0x5
#define MODE_AMR_WB             0xD
#define MODE_PCM                0xC

#define DUALMIC_KEY         "dualmic_enabled"
#define FLUENCE_KEY         "fluence"
#define ANC_KEY             "anc_enabled"
#define TTY_MODE_KEY        "tty_mode"
#define BT_SAMPLERATE_KEY   "bt_samplerate"
#define BTHEADSET_VGS       "bt_headset_vgs"
#define WIDEVOICE_KEY       "wide_voice_enable"
#define VOIPRATE_KEY        "voip_rate"
#define FENS_KEY            "fens_enable"
#define ST_KEY              "st_enable"
#define INCALLMUSIC_KEY     "incall_music_enabled"

#define ANC_FLAG        0x00000001
#define DMIC_FLAG       0x00000002
#define QMIC_FLAG       0x00000004
#ifdef QCOM_SSR_ENABLED
#define SSRQMIC_FLAG    0x00000008
#endif

#define TTY_OFF         0x00000010
#define TTY_FULL        0x00000020
#define TTY_VCO         0x00000040
#define TTY_HCO         0x00000080
#define TTY_CLEAR       0xFFFFFF0F

#define LPA_SESSION_ID 1
#define TUNNEL_SESSION_ID 2
#ifdef QCOM_USBAUDIO_ENABLED
static int USBPLAYBACKBIT_MUSIC = (1 << 0);
static int USBPLAYBACKBIT_VOICECALL = (1 << 1);
static int USBPLAYBACKBIT_VOIPCALL = (1 << 2);
static int USBPLAYBACKBIT_FM = (1 << 3);
static int USBPLAYBACKBIT_LPA = (1 << 4);

static int USBRECBIT_REC = (1 << 0);
static int USBRECBIT_VOICECALL = (1 << 1);
static int USBRECBIT_VOIPCALL = (1 << 2);
static int USBRECBIT_FM = (1 << 3);
#endif

#define DEVICE_SPEAKER_HEADSET "Speaker Headset"
#define DEVICE_HEADSET "Headset"
#define DEVICE_HEADPHONES "Headphones"

#ifdef QCOM_SSR_ENABLED
#define COEFF_ARRAY_SIZE          4
#define FILT_SIZE                 ((512+1)* 6)    /* # ((FFT bins)/2+1)*numOutputs */
#define SSR_FRAME_SIZE            512
#define SSR_INPUT_FRAME_SIZE      (SSR_FRAME_SIZE * 4)
#define SSR_OUTPUT_FRAME_SIZE     (SSR_FRAME_SIZE * 6)
#endif

#define MODE_CALL_KEY  "CALL_KEY"

struct alsa_device_t;
static uint32_t FLUENCE_MODE_ENDFIRE   = 0;
static uint32_t FLUENCE_MODE_BROADSIDE = 1;

enum {
    INCALL_REC_MONO,
    INCALL_REC_STEREO,
};

enum audio_call_mode {
    CS_INACTIVE   = 0x0,
    CS_ACTIVE     = 0x1,
    CS_HOLD       = 0x2,
    IMS_INACTIVE  = 0x0,
    IMS_ACTIVE    = 0x10,
    IMS_HOLD      = 0x20
};


struct alsa_handle_t {
    alsa_device_t *     module;
    uint32_t            devices;
    char                useCase[MAX_STR_LEN];
    struct pcm *        handle;
    snd_pcm_format_t    format;
    uint32_t            channels;
    audio_channel_mask_t channelMask;
    uint32_t            sampleRate;
    unsigned int        latency;         // Delay in usec
    unsigned int        bufferSize;      // Size of sample buffer
    unsigned int        periodSize;
    bool                isDeepbufferOutput;
    struct pcm *        rxHandle;
    snd_use_case_mgr_t  *ucMgr;
};

typedef List < alsa_handle_t > ALSAHandleList;

struct use_case_t {
    char                useCase[MAX_STR_LEN];
};

typedef List < use_case_t > ALSAUseCaseList;

struct alsa_device_t {
    hw_device_t common;

    status_t (*init)(alsa_device_t *, ALSAHandleList &);
    status_t (*open)(alsa_handle_t *);
    status_t (*close)(alsa_handle_t *);
    status_t (*standby)(alsa_handle_t *);
    status_t (*route)(alsa_handle_t *, uint32_t, int);
    status_t (*startVoiceCall)(alsa_handle_t *);
    status_t (*startVoipCall)(alsa_handle_t *);
    status_t (*startFm)(alsa_handle_t *);
    void     (*setVoiceVolume)(int);
    void     (*setVoipVolume)(int);
    void     (*setMicMute)(int);
    void     (*setVoipMicMute)(int);
    void     (*setVoipConfig)(int, int);
    status_t (*setFmVolume)(int);
    void     (*setBtscoRate)(int);
    status_t (*setLpaVolume)(int);
    void     (*enableWideVoice)(bool);
    void     (*enableFENS)(bool);
    void     (*setFlags)(uint32_t);
    status_t (*setCompressedVolume)(int);
    void     (*enableSlowTalk)(bool);
    void     (*setVocRecMode)(uint8_t);
    void     (*setVoLTEMicMute)(int);
    void     (*setVoLTEVolume)(int);
#ifdef SEPERATED_AUDIO_INPUT
    void     (*setInput)(int);
#endif
#ifdef QCOM_CSDCLIENT_ENABLED
    void     (*setCsdHandle)(void*);
#endif
};

// ----------------------------------------------------------------------------

class ALSAMixer
{
public:
    ALSAMixer();
    virtual                ~ALSAMixer();

    bool                    isValid() { return 1;}
    status_t                setMasterVolume(float volume);
    status_t                setMasterGain(float gain);

    status_t                setVolume(uint32_t device, float left, float right);
    status_t                setGain(uint32_t device, float gain);

    status_t                setCaptureMuteState(uint32_t device, bool state);
    status_t                getCaptureMuteState(uint32_t device, bool *state);
    status_t                setPlaybackMuteState(uint32_t device, bool state);
    status_t                getPlaybackMuteState(uint32_t device, bool *state);

};

class ALSAControl
{
public:
    ALSAControl(const char *device = "/dev/snd/controlC0");
    virtual                ~ALSAControl();

    status_t                get(const char *name, unsigned int &value, int index = 0);
    status_t                set(const char *name, unsigned int value, int index = -1);
    status_t                set(const char *name, const char *);
    status_t                setext(const char *name, int count, char **setValues);

private:
    struct mixer*             mHandle;
};

class ALSAStreamOps
{
public:
    ALSAStreamOps(AudioHardwareALSA *parent, alsa_handle_t *handle);
    virtual            ~ALSAStreamOps();

    status_t            set(int *format, uint32_t *channels, uint32_t *rate, uint32_t device);

    status_t            setParameters(const String8& keyValuePairs);
    String8             getParameters(const String8& keys);

    uint32_t            sampleRate() const;
    size_t              bufferSize() const;
    int                 format() const;
    uint32_t            channels() const;

    status_t            open(int mode);
    void                close();

protected:
    friend class AudioHardwareALSA;

    AudioHardwareALSA *     mParent;
    alsa_handle_t *         mHandle;
    uint32_t                mDevices;
};

// ----------------------------------------------------------------------------

class AudioStreamOutALSA : public AudioStreamOut, public ALSAStreamOps
{
public:
    AudioStreamOutALSA(AudioHardwareALSA *parent, alsa_handle_t *handle);
    virtual            ~AudioStreamOutALSA();

    virtual uint32_t    sampleRate() const
    {
        return ALSAStreamOps::sampleRate();
    }

    virtual size_t      bufferSize() const
    {
        return ALSAStreamOps::bufferSize();
    }

    virtual uint32_t    channels() const;

    virtual int         format() const
    {
        return ALSAStreamOps::format();
    }

    virtual uint32_t    latency() const;

    virtual ssize_t     write(const void *buffer, size_t bytes);
    virtual status_t    dump(int fd, const Vector<String16>& args);

    status_t            setVolume(float left, float right);

    virtual status_t    standby();

    virtual status_t    setParameters(const String8& keyValuePairs) {
        return ALSAStreamOps::setParameters(keyValuePairs);
    }

    virtual String8     getParameters(const String8& keys) {
        return ALSAStreamOps::getParameters(keys);
    }

    // return the number of audio frames written by the audio dsp to DAC since
    // the output has exited standby
    virtual status_t    getRenderPosition(uint32_t *dspFrames);

    status_t            open(int mode);
    status_t            close();

private:
    uint32_t            mFrameCount;

protected:
    AudioHardwareALSA *     mParent;
};

class AudioStreamInALSA : public AudioStreamIn, public ALSAStreamOps
{
public:
    AudioStreamInALSA(AudioHardwareALSA *parent,
            alsa_handle_t *handle,
            AudioSystem::audio_in_acoustics audio_acoustics);
    virtual            ~AudioStreamInALSA();

    virtual uint32_t    sampleRate() const
    {
        return ALSAStreamOps::sampleRate();
    }

    virtual size_t      bufferSize() const
    {
        return ALSAStreamOps::bufferSize();
    }

    virtual uint32_t    channels() const
    {
        return ALSAStreamOps::channels();
    }

    virtual int         format() const
    {
        return ALSAStreamOps::format();
    }

    virtual ssize_t     read(void* buffer, ssize_t bytes);
    virtual status_t    dump(int fd, const Vector<String16>& args);

    virtual status_t    setGain(float gain);

    virtual status_t    standby();

    virtual status_t    setParameters(const String8& keyValuePairs)
    {
        return ALSAStreamOps::setParameters(keyValuePairs);
    }

    virtual String8     getParameters(const String8& keys)
    {
        return ALSAStreamOps::getParameters(keys);
    }

    // Return the amount of input frames lost in the audio driver since the last call of this function.
    // Audio driver is expected to reset the value to 0 and restart counting upon returning the current value by this function call.
    // Such loss typically occurs when the user space process is blocked longer than the capacity of audio driver buffers.
    // Unit: the number of input audio frames
    virtual unsigned int  getInputFramesLost() const;

    virtual status_t addAudioEffect(effect_handle_t effect)
    {
        return BAD_VALUE;
    }
   
    virtual status_t removeAudioEffect(effect_handle_t effect)
    {
        return BAD_VALUE;
    }
    status_t            setAcousticParams(void* params);

    status_t            open(int mode);
    status_t            close();
#ifdef QCOM_SSR_ENABLED
    // Helper function to initialize the Surround Sound library.
    status_t initSurroundSoundLibrary(unsigned long buffersize);
#endif

private:
    void                resetFramesLost();

#ifdef QCOM_CSDCLIENT_ENABLED
    int                 start_csd_record(int);
    int                 stop_csd_record(void);
#endif

    unsigned int        mFramesLost;
    AudioSystem::audio_in_acoustics mAcoustics;

#ifdef QCOM_SSR_ENABLED
    // Function to read coefficients from files.
    status_t            readCoeffsFromFile();

    FILE                *mFp_4ch;
    FILE                *mFp_6ch;
    int16_t             **mRealCoeffs;
    int16_t             **mImagCoeffs;
    void                *mSurroundObj;

    int16_t             *mSurroundInputBuffer;
    int16_t             *mSurroundOutputBuffer;
    int                 mSurroundInputBufferIdx;
    int                 mSurroundOutputBufferIdx;
#endif

protected:
    AudioHardwareALSA *     mParent;
};

class AudioHardwareALSA : public AudioHardwareBase
{
public:
    AudioHardwareALSA();
    virtual            ~AudioHardwareALSA();

    /**
     * check to see if the audio hardware interface has been initialized.
     * return status based on values defined in include/utils/Errors.h
     */
    virtual status_t    initCheck();

    /** set the audio volume of a voice call. Range is between 0.0 and 1.0 */
    virtual status_t    setVoiceVolume(float volume);

    /**
     * set the audio volume for all audio activities other than voice call.
     * Range between 0.0 and 1.0. If any value other than NO_ERROR is returned,
     * the software mixer will emulate this capability.
     */
    virtual status_t    setMasterVolume(float volume);
#ifdef QCOM_FM_ENABLED
    virtual status_t    setFmVolume(float volume);
#endif
    /**
     * setMode is called when the audio mode changes. NORMAL mode is for
     * standard audio playback, RINGTONE when a ringtone is playing, and IN_CALL
     * when a call is in progress.
     */
    virtual status_t    setMode(int mode);

    // mic mute
    virtual status_t    setMicMute(bool state);
    virtual status_t    getMicMute(bool* state);

    // set/get global audio parameters
    virtual status_t    setParameters(const String8& keyValuePairs);
    virtual String8     getParameters(const String8& keys);

    // Returns audio input buffer size according to parameters passed or 0 if one of the
    // parameters is not supported
    virtual size_t    getInputBufferSize(uint32_t sampleRate, int format, int channels);

#ifdef QCOM_TUNNEL_LPA_ENABLED
    /** This method creates and opens the audio hardware output
      *  session for LPA */
    virtual AudioStreamOut* openOutputSession(
            uint32_t devices,
            int *format,
            status_t *status,
            int sessionId,
            uint32_t samplingRate=0,
            uint32_t channels=0);
    virtual void closeOutputSession(AudioStreamOut* out);
#endif

    /** This method creates and opens the audio hardware output stream */
    virtual AudioStreamOut* openOutputStream(
            uint32_t devices,
            int *format=0,
            uint32_t *channels=0,
            uint32_t *sampleRate=0,
            status_t *status=0);
    virtual    void        closeOutputStream(AudioStreamOut* out);

    /** This method creates and opens the audio hardware input stream */
    virtual AudioStreamIn* openInputStream(
            uint32_t devices,
            int *format,
            uint32_t *channels,
            uint32_t *sampleRate,
            status_t *status,
            AudioSystem::audio_in_acoustics acoustics);
    virtual    void        closeInputStream(AudioStreamIn* in);

    /**This method dumps the state of the audio hardware */
    //virtual status_t dumpState(int fd, const Vector<String16>& args);

    static AudioHardwareInterface* create();

    int                 mode()
    {
        return mMode;
    }

protected:
    virtual status_t    dump(int fd, const Vector<String16>& args);
    virtual uint32_t    getVoipMode(int format);
    void                doRouting(int device);
#ifdef QCOM_FM_ENABLED
    void                handleFm(int device);
#endif
#ifdef QCOM_USBAUDIO_ENABLED
    void                closeUSBPlayback();
    void                closeUSBRecording();
    void                closeUsbRecordingIfNothingActive();
    void                closeUsbPlaybackIfNothingActive();
    void                startUsbPlaybackIfNotStarted();
    void                startUsbRecordingIfNotStarted();
#endif

    void                disableVoiceCall(char* verb, char* modifier, int mode, int device);
    void                enableVoiceCall(char* verb, char* modifier, int mode, int device);
    bool                routeVoiceCall(int device, int	newMode);
    bool                routeVoLTECall(int device, int newMode);
    friend class AudioStreamOutALSA;
    friend class AudioStreamInALSA;
    friend class ALSAStreamOps;

    alsa_device_t *     mALSADevice;

    ALSAHandleList      mDeviceList;

#ifdef QCOM_USBAUDIO_ENABLED
    AudioUsbALSA        *mAudioUsbALSA;
#endif

    Mutex                   mLock;

    snd_use_case_mgr_t *mUcMgr;

    uint32_t            mCurDevice;
    /* The flag holds all the audio related device settings from
     * Settings and Qualcomm Settings applications */
    uint32_t            mDevSettingsFlag;
    uint32_t            mVoipStreamCount;
    uint32_t            mVoipBitRate;
    uint32_t            mIncallMode;

    bool                mMicMute;
    int mCSCallActive;
    int mVolteCallActive;
    int mCallState;
    int mIsFmActive;
    bool mBluetoothVGS;
    bool mFusion3Platform;
#ifdef QCOM_USBAUDIO_ENABLED
    int musbPlaybackState;
    int musbRecordingState;
#endif
    void *mAcdbHandle;
    void *mCsdHandle;
};

// ----------------------------------------------------------------------------

};        // namespace android_audio_legacy
#endif    // ANDROID_AUDIO_HARDWARE_ALSA_H
