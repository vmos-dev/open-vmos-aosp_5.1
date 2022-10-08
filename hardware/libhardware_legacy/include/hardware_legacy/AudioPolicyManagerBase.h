/*
 * Copyright (C) 2009 The Android Open Source Project
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


#include <stdint.h>
#include <sys/types.h>
#include <cutils/config_utils.h>
#include <cutils/misc.h>
#include <utils/Timers.h>
#include <utils/Errors.h>
#include <utils/KeyedVector.h>
#include <utils/SortedVector.h>
#include <hardware_legacy/AudioPolicyInterface.h>


namespace android_audio_legacy {
    using android::KeyedVector;
    using android::DefaultKeyedVector;
    using android::SortedVector;

// ----------------------------------------------------------------------------

#define MAX_DEVICE_ADDRESS_LEN 20
// Attenuation applied to STRATEGY_SONIFICATION streams when a headset is connected: 6dB
#define SONIFICATION_HEADSET_VOLUME_FACTOR 0.5
// Min volume for STRATEGY_SONIFICATION streams when limited by music volume: -36dB
#define SONIFICATION_HEADSET_VOLUME_MIN  0.016
// Time in milliseconds during which we consider that music is still active after a music
// track was stopped - see computeVolume()
#define SONIFICATION_HEADSET_MUSIC_DELAY  5000
// Time in milliseconds after media stopped playing during which we consider that the
// sonification should be as unobtrusive as during the time media was playing.
#define SONIFICATION_RESPECTFUL_AFTER_MUSIC_DELAY 5000
// Time in milliseconds during witch some streams are muted while the audio path
// is switched
#define MUTE_TIME_MS 2000

#define NUM_TEST_OUTPUTS 5

#define NUM_VOL_CURVE_KNEES 2

// Default minimum length allowed for offloading a compressed track
// Can be overridden by the audio.offload.min.duration.secs property
#define OFFLOAD_DEFAULT_MIN_DURATION_SECS 60

// ----------------------------------------------------------------------------
// AudioPolicyManagerBase implements audio policy manager behavior common to all platforms.
// Each platform must implement an AudioPolicyManager class derived from AudioPolicyManagerBase
// and override methods for which the platform specific behavior differs from the implementation
// in AudioPolicyManagerBase. Even if no specific behavior is required, the AudioPolicyManager
// class must be implemented as well as the class factory function createAudioPolicyManager()
// and provided in a shared library libaudiopolicy.so.
// ----------------------------------------------------------------------------

class AudioPolicyManagerBase: public AudioPolicyInterface
#ifdef AUDIO_POLICY_TEST
    , public Thread
#endif //AUDIO_POLICY_TEST
{

public:
                AudioPolicyManagerBase(AudioPolicyClientInterface *clientInterface);
        virtual ~AudioPolicyManagerBase();

        // AudioPolicyInterface
        virtual status_t setDeviceConnectionState(audio_devices_t device,
                                                          AudioSystem::device_connection_state state,
                                                          const char *device_address);
        virtual AudioSystem::device_connection_state getDeviceConnectionState(audio_devices_t device,
                                                                              const char *device_address);
        virtual void setPhoneState(int state);
        virtual void setForceUse(AudioSystem::force_use usage, AudioSystem::forced_config config);
        virtual AudioSystem::forced_config getForceUse(AudioSystem::force_use usage);
        virtual void setSystemProperty(const char* property, const char* value);
        virtual status_t initCheck();
        virtual audio_io_handle_t getOutput(AudioSystem::stream_type stream,
                                            uint32_t samplingRate,
                                            audio_format_t format,
                                            audio_channel_mask_t channelMask,
                                            AudioSystem::output_flags flags,
                                            const audio_offload_info_t *offloadInfo);
        virtual status_t startOutput(audio_io_handle_t output,
                                     AudioSystem::stream_type stream,
                                     int session = 0);
        virtual status_t stopOutput(audio_io_handle_t output,
                                    AudioSystem::stream_type stream,
                                    int session = 0);
        virtual void releaseOutput(audio_io_handle_t output);
        virtual audio_io_handle_t getInput(int inputSource,
                                            uint32_t samplingRate,
                                            audio_format_t format,
                                            audio_channel_mask_t channelMask,
                                            AudioSystem::audio_in_acoustics acoustics);

        // indicates to the audio policy manager that the input starts being used.
        virtual status_t startInput(audio_io_handle_t input);

        // indicates to the audio policy manager that the input stops being used.
        virtual status_t stopInput(audio_io_handle_t input);
        virtual void releaseInput(audio_io_handle_t input);
        virtual void closeAllInputs();
        virtual void initStreamVolume(AudioSystem::stream_type stream,
                                                    int indexMin,
                                                    int indexMax);
        virtual status_t setStreamVolumeIndex(AudioSystem::stream_type stream,
                                              int index,
                                              audio_devices_t device);
        virtual status_t getStreamVolumeIndex(AudioSystem::stream_type stream,
                                              int *index,
                                              audio_devices_t device);

        // return the strategy corresponding to a given stream type
        virtual uint32_t getStrategyForStream(AudioSystem::stream_type stream);

        // return the enabled output devices for the given stream type
        virtual audio_devices_t getDevicesForStream(AudioSystem::stream_type stream);

        virtual audio_io_handle_t getOutputForEffect(const effect_descriptor_t *desc = NULL);
        virtual status_t registerEffect(const effect_descriptor_t *desc,
                                        audio_io_handle_t io,
                                        uint32_t strategy,
                                        int session,
                                        int id);
        virtual status_t unregisterEffect(int id);
        virtual status_t setEffectEnabled(int id, bool enabled);

        virtual bool isStreamActive(int stream, uint32_t inPastMs = 0) const;
        // return whether a stream is playing remotely, override to change the definition of
        //   local/remote playback, used for instance by notification manager to not make
        //   media players lose audio focus when not playing locally
        virtual bool isStreamActiveRemotely(int stream, uint32_t inPastMs = 0) const;
        virtual bool isSourceActive(audio_source_t source) const;

        virtual status_t dump(int fd);

        virtual bool isOffloadSupported(const audio_offload_info_t& offloadInfo);

protected:

        enum routing_strategy {
            STRATEGY_MEDIA,
            STRATEGY_PHONE,
            STRATEGY_SONIFICATION,
            STRATEGY_SONIFICATION_RESPECTFUL,
            STRATEGY_DTMF,
            STRATEGY_ENFORCED_AUDIBLE,
            NUM_STRATEGIES
        };

        // 4 points to define the volume attenuation curve, each characterized by the volume
        // index (from 0 to 100) at which they apply, and the attenuation in dB at that index.
        // we use 100 steps to avoid rounding errors when computing the volume in volIndexToAmpl()

        enum { VOLMIN = 0, VOLKNEE1 = 1, VOLKNEE2 = 2, VOLMAX = 3, VOLCNT = 4};

        class VolumeCurvePoint
        {
        public:
            int mIndex;
            float mDBAttenuation;
        };

        // device categories used for volume curve management.
        enum device_category {
            DEVICE_CATEGORY_HEADSET,
            DEVICE_CATEGORY_SPEAKER,
            DEVICE_CATEGORY_EARPIECE,
            DEVICE_CATEGORY_CNT
        };

        class IOProfile;

        class HwModule {
        public:
                    HwModule(const char *name);
                    ~HwModule();

            void dump(int fd);

            const char *const mName; // base name of the audio HW module (primary, a2dp ...)
            audio_module_handle_t mHandle;
            Vector <IOProfile *> mOutputProfiles; // output profiles exposed by this module
            Vector <IOProfile *> mInputProfiles;  // input profiles exposed by this module
        };

        // the IOProfile class describes the capabilities of an output or input stream.
        // It is currently assumed that all combination of listed parameters are supported.
        // It is used by the policy manager to determine if an output or input is suitable for
        // a given use case,  open/close it accordingly and connect/disconnect audio tracks
        // to/from it.
        class IOProfile
        {
        public:
            IOProfile(HwModule *module);
            ~IOProfile();

            bool isCompatibleProfile(audio_devices_t device,
                                     uint32_t samplingRate,
                                     audio_format_t format,
                                     audio_channel_mask_t channelMask,
                                     audio_output_flags_t flags) const;

            void dump(int fd);
            void log();

            // by convention, "0' in the first entry in mSamplingRates, mChannelMasks or mFormats
            // indicates the supported parameters should be read from the output stream
            // after it is opened for the first time
            Vector <uint32_t> mSamplingRates; // supported sampling rates
            Vector <audio_channel_mask_t> mChannelMasks; // supported channel masks
            Vector <audio_format_t> mFormats; // supported audio formats
            audio_devices_t mSupportedDevices; // supported devices (devices this output can be
                                               // routed to)
            audio_output_flags_t mFlags; // attribute flags (e.g primary output,
                                                // direct output...). For outputs only.
            HwModule *mModule;                     // audio HW module exposing this I/O stream
        };

        // default volume curve
        static const VolumeCurvePoint sDefaultVolumeCurve[AudioPolicyManagerBase::VOLCNT];
        // default volume curve for media strategy
        static const VolumeCurvePoint sDefaultMediaVolumeCurve[AudioPolicyManagerBase::VOLCNT];
        // volume curve for media strategy on speakers
        static const VolumeCurvePoint sSpeakerMediaVolumeCurve[AudioPolicyManagerBase::VOLCNT];
        // volume curve for sonification strategy on speakers
        static const VolumeCurvePoint sSpeakerSonificationVolumeCurve[AudioPolicyManagerBase::VOLCNT];
        static const VolumeCurvePoint sSpeakerSonificationVolumeCurveDrc[AudioPolicyManagerBase::VOLCNT];
        static const VolumeCurvePoint sDefaultSystemVolumeCurve[AudioPolicyManagerBase::VOLCNT];
        static const VolumeCurvePoint sDefaultSystemVolumeCurveDrc[AudioPolicyManagerBase::VOLCNT];
        static const VolumeCurvePoint sHeadsetSystemVolumeCurve[AudioPolicyManagerBase::VOLCNT];
        static const VolumeCurvePoint sDefaultVoiceVolumeCurve[AudioPolicyManagerBase::VOLCNT];
        static const VolumeCurvePoint sSpeakerVoiceVolumeCurve[AudioPolicyManagerBase::VOLCNT];
        // default volume curves per stream and device category. See initializeVolumeCurves()
        static const VolumeCurvePoint *sVolumeProfiles[AudioSystem::NUM_STREAM_TYPES][DEVICE_CATEGORY_CNT];

        // descriptor for audio outputs. Used to maintain current configuration of each opened audio output
        // and keep track of the usage of this output by each audio stream type.
        class AudioOutputDescriptor
        {
        public:
            AudioOutputDescriptor(const IOProfile *profile);

            status_t    dump(int fd);

            audio_devices_t device() const;
            void changeRefCount(AudioSystem::stream_type stream, int delta);

            bool isDuplicated() const { return (mOutput1 != NULL && mOutput2 != NULL); }
            audio_devices_t supportedDevices();
            uint32_t latency();
            bool sharesHwModuleWith(const AudioOutputDescriptor *outputDesc);
            bool isActive(uint32_t inPastMs = 0) const;
            bool isStreamActive(AudioSystem::stream_type stream,
                                uint32_t inPastMs = 0,
                                nsecs_t sysTime = 0) const;
            bool isStrategyActive(routing_strategy strategy,
                             uint32_t inPastMs = 0,
                             nsecs_t sysTime = 0) const;

            audio_io_handle_t mId;              // output handle
            uint32_t mSamplingRate;             //
            audio_format_t mFormat;             //
            audio_channel_mask_t mChannelMask;     // output configuration
            uint32_t mLatency;                  //
            audio_output_flags_t mFlags;   //
            audio_devices_t mDevice;                   // current device this output is routed to
            uint32_t mRefCount[AudioSystem::NUM_STREAM_TYPES]; // number of streams of each type using this output
            nsecs_t mStopTime[AudioSystem::NUM_STREAM_TYPES];
            AudioOutputDescriptor *mOutput1;    // used by duplicated outputs: first output
            AudioOutputDescriptor *mOutput2;    // used by duplicated outputs: second output
            float mCurVolume[AudioSystem::NUM_STREAM_TYPES];   // current stream volume
            int mMuteCount[AudioSystem::NUM_STREAM_TYPES];     // mute request counter
            const IOProfile *mProfile;          // I/O profile this output derives from
            bool mStrategyMutedByDevice[NUM_STRATEGIES]; // strategies muted because of incompatible
                                                // device selection. See checkDeviceMuteStrategies()
            uint32_t mDirectOpenCount; // number of clients using this output (direct outputs only)
        };

        // descriptor for audio inputs. Used to maintain current configuration of each opened audio input
        // and keep track of the usage of this input.
        class AudioInputDescriptor
        {
        public:
            AudioInputDescriptor(const IOProfile *profile);

            status_t    dump(int fd);

            audio_io_handle_t mId;                      // input handle
            uint32_t mSamplingRate;                     //
            audio_format_t mFormat;                     // input configuration
            audio_channel_mask_t mChannelMask;             //
            audio_devices_t mDevice;                    // current device this input is routed to
            uint32_t mRefCount;                         // number of AudioRecord clients using this output
            int      mInputSource;                      // input source selected by application (mediarecorder.h)
            const IOProfile *mProfile;                  // I/O profile this output derives from
        };

        // stream descriptor used for volume control
        class StreamDescriptor
        {
        public:
            StreamDescriptor();

            int getVolumeIndex(audio_devices_t device);
            void dump(int fd);

            int mIndexMin;      // min volume index
            int mIndexMax;      // max volume index
            KeyedVector<audio_devices_t, int> mIndexCur;   // current volume index per device
            bool mCanBeMuted;   // true is the stream can be muted

            const VolumeCurvePoint *mVolumeCurve[DEVICE_CATEGORY_CNT];
        };

        // stream descriptor used for volume control
        class EffectDescriptor
        {
        public:

            status_t dump(int fd);

            int mIo;                // io the effect is attached to
            routing_strategy mStrategy; // routing strategy the effect is associated to
            int mSession;               // audio session the effect is on
            effect_descriptor_t mDesc;  // effect descriptor
            bool mEnabled;              // enabled state: CPU load being used or not
        };

        void addOutput(audio_io_handle_t id, AudioOutputDescriptor *outputDesc);
        void addInput(audio_io_handle_t id, AudioInputDescriptor *inputDesc);

        // return the strategy corresponding to a given stream type
        static routing_strategy getStrategy(AudioSystem::stream_type stream);

        // return appropriate device for streams handled by the specified strategy according to current
        // phone state, connected devices...
        // if fromCache is true, the device is returned from mDeviceForStrategy[],
        // otherwise it is determine by current state
        // (device connected,phone state, force use, a2dp output...)
        // This allows to:
        //  1 speed up process when the state is stable (when starting or stopping an output)
        //  2 access to either current device selection (fromCache == true) or
        // "future" device selection (fromCache == false) when called from a context
        //  where conditions are changing (setDeviceConnectionState(), setPhoneState()...) AND
        //  before updateDevicesAndOutputs() is called.
        virtual audio_devices_t getDeviceForStrategy(routing_strategy strategy,
                                                     bool fromCache);

        // change the route of the specified output. Returns the number of ms we have slept to
        // allow new routing to take effect in certain cases.
        uint32_t setOutputDevice(audio_io_handle_t output,
                             audio_devices_t device,
                             bool force = false,
                             int delayMs = 0);

        // select input device corresponding to requested audio source
        virtual audio_devices_t getDeviceForInputSource(int inputSource);

        // return io handle of active input or 0 if no input is active
        //    Only considers inputs from physical devices (e.g. main mic, headset mic) when
        //    ignoreVirtualInputs is true.
        audio_io_handle_t getActiveInput(bool ignoreVirtualInputs = true);

        // initialize volume curves for each strategy and device category
        void initializeVolumeCurves();

        // compute the actual volume for a given stream according to the requested index and a particular
        // device
        virtual float computeVolume(int stream, int index, audio_io_handle_t output, audio_devices_t device);

        // check that volume change is permitted, compute and send new volume to audio hardware
        status_t checkAndSetVolume(int stream, int index, audio_io_handle_t output, audio_devices_t device, int delayMs = 0, bool force = false);

        // apply all stream volumes to the specified output and device
        void applyStreamVolumes(audio_io_handle_t output, audio_devices_t device, int delayMs = 0, bool force = false);

        // Mute or unmute all streams handled by the specified strategy on the specified output
        void setStrategyMute(routing_strategy strategy,
                             bool on,
                             audio_io_handle_t output,
                             int delayMs = 0,
                             audio_devices_t device = (audio_devices_t)0);

        // Mute or unmute the stream on the specified output
        void setStreamMute(int stream,
                           bool on,
                           audio_io_handle_t output,
                           int delayMs = 0,
                           audio_devices_t device = (audio_devices_t)0);

        // handle special cases for sonification strategy while in call: mute streams or replace by
        // a special tone in the device used for communication
        void handleIncallSonification(int stream, bool starting, bool stateChange);

        // true if device is in a telephony or VoIP call
        virtual bool isInCall();

        // true if given state represents a device in a telephony or VoIP call
        virtual bool isStateInCall(int state);

        // when a device is connected, checks if an open output can be routed
        // to this device. If none is open, tries to open one of the available outputs.
        // Returns an output suitable to this device or 0.
        // when a device is disconnected, checks if an output is not used any more and
        // returns its handle if any.
        // transfers the audio tracks and effects from one output thread to another accordingly.
        status_t checkOutputsForDevice(audio_devices_t device,
                                       AudioSystem::device_connection_state state,
                                       SortedVector<audio_io_handle_t>& outputs,
                                       const String8 paramStr);

        status_t checkInputsForDevice(audio_devices_t device,
                                      AudioSystem::device_connection_state state,
                                      SortedVector<audio_io_handle_t>& inputs,
                                      const String8 paramStr);

        // close an output and its companion duplicating output.
        void closeOutput(audio_io_handle_t output);

        // checks and if necessary changes outputs used for all strategies.
        // must be called every time a condition that affects the output choice for a given strategy
        // changes: connected device, phone state, force use...
        // Must be called before updateDevicesAndOutputs()
        void checkOutputForStrategy(routing_strategy strategy);

        // Same as checkOutputForStrategy() but for a all strategies in order of priority
        void checkOutputForAllStrategies();

        // manages A2DP output suspend/restore according to phone state and BT SCO usage
        void checkA2dpSuspend();

        // returns the A2DP output handle if it is open or 0 otherwise
        audio_io_handle_t getA2dpOutput();

        // selects the most appropriate device on output for current state
        // must be called every time a condition that affects the device choice for a given output is
        // changed: connected device, phone state, force use, output start, output stop..
        // see getDeviceForStrategy() for the use of fromCache parameter

        audio_devices_t getNewDevice(audio_io_handle_t output, bool fromCache);
        // updates cache of device used by all strategies (mDeviceForStrategy[])
        // must be called every time a condition that affects the device choice for a given strategy is
        // changed: connected device, phone state, force use...
        // cached values are used by getDeviceForStrategy() if parameter fromCache is true.
         // Must be called after checkOutputForAllStrategies()

        void updateDevicesAndOutputs();

        virtual uint32_t getMaxEffectsCpuLoad();
        virtual uint32_t getMaxEffectsMemory();
#ifdef AUDIO_POLICY_TEST
        virtual     bool        threadLoop();
                    void        exit();
        int testOutputIndex(audio_io_handle_t output);
#endif //AUDIO_POLICY_TEST

        status_t setEffectEnabled(EffectDescriptor *pDesc, bool enabled);

        // returns the category the device belongs to with regard to volume curve management
        static device_category getDeviceCategory(audio_devices_t device);

        // extract one device relevant for volume control from multiple device selection
        static audio_devices_t getDeviceForVolume(audio_devices_t device);

        SortedVector<audio_io_handle_t> getOutputsForDevice(audio_devices_t device,
                        DefaultKeyedVector<audio_io_handle_t, AudioOutputDescriptor *> openOutputs);
        bool vectorsEqual(SortedVector<audio_io_handle_t>& outputs1,
                                           SortedVector<audio_io_handle_t>& outputs2);

        // mute/unmute strategies using an incompatible device combination
        // if muting, wait for the audio in pcm buffer to be drained before proceeding
        // if unmuting, unmute only after the specified delay
        // Returns the number of ms waited
        uint32_t  checkDeviceMuteStrategies(AudioOutputDescriptor *outputDesc,
                                            audio_devices_t prevDevice,
                                            uint32_t delayMs);

        audio_io_handle_t selectOutput(const SortedVector<audio_io_handle_t>& outputs,
                                       AudioSystem::output_flags flags);
        IOProfile *getInputProfile(audio_devices_t device,
                                   uint32_t samplingRate,
                                   audio_format_t format,
                                   audio_channel_mask_t channelMask);
        IOProfile *getProfileForDirectOutput(audio_devices_t device,
                                                       uint32_t samplingRate,
                                                       audio_format_t format,
                                                       audio_channel_mask_t channelMask,
                                                       audio_output_flags_t flags);

        audio_io_handle_t selectOutputForEffects(const SortedVector<audio_io_handle_t>& outputs);

        bool isNonOffloadableEffectEnabled();

        //
        // Audio policy configuration file parsing (audio_policy.conf)
        //
        static uint32_t stringToEnum(const struct StringToEnum *table,
                                     size_t size,
                                     const char *name);
        static bool stringToBool(const char *value);
        static audio_output_flags_t parseFlagNames(char *name);
        static audio_devices_t parseDeviceNames(char *name);
        void loadSamplingRates(char *name, IOProfile *profile);
        void loadFormats(char *name, IOProfile *profile);
        void loadOutChannels(char *name, IOProfile *profile);
        void loadInChannels(char *name, IOProfile *profile);
        status_t loadOutput(cnode *root,  HwModule *module);
        status_t loadInput(cnode *root,  HwModule *module);
        void loadHwModule(cnode *root);
        void loadHwModules(cnode *root);
        void loadGlobalConfig(cnode *root);
        status_t loadAudioPolicyConfig(const char *path);
        void defaultAudioPolicyConfig(void);


        AudioPolicyClientInterface *mpClientInterface;  // audio policy client interface
        audio_io_handle_t mPrimaryOutput;              // primary output handle
        // list of descriptors for outputs currently opened
        DefaultKeyedVector<audio_io_handle_t, AudioOutputDescriptor *> mOutputs;
        // copy of mOutputs before setDeviceConnectionState() opens new outputs
        // reset to mOutputs when updateDevicesAndOutputs() is called.
        DefaultKeyedVector<audio_io_handle_t, AudioOutputDescriptor *> mPreviousOutputs;

        // list of input descriptors currently opened
        DefaultKeyedVector<audio_io_handle_t, AudioInputDescriptor *> mInputs;

        audio_devices_t mAvailableOutputDevices; // bit field of all available output devices
        audio_devices_t mAvailableInputDevices; // bit field of all available input devices
                                                // without AUDIO_DEVICE_BIT_IN to allow direct bit
                                                // field comparisons
        int mPhoneState;                                                    // current phone state
        AudioSystem::forced_config mForceUse[AudioSystem::NUM_FORCE_USE];   // current forced use configuration

        StreamDescriptor mStreams[AudioSystem::NUM_STREAM_TYPES];           // stream descriptors for volume control
        String8 mA2dpDeviceAddress;                                         // A2DP device MAC address
        String8 mScoDeviceAddress;                                          // SCO device MAC address
        String8 mUsbOutCardAndDevice;                                       // USB audio ALSA card and device numbers:
                                                                            // card=<card_number>;device=<><device_number>
        bool    mLimitRingtoneVolume;                                       // limit ringtone volume to music volume if headset connected
        audio_devices_t mDeviceForStrategy[NUM_STRATEGIES];
        float   mLastVoiceVolume;                                           // last voice volume value sent to audio HAL

        // Maximum CPU load allocated to audio effects in 0.1 MIPS (ARMv5TE, 0 WS memory) units
        static const uint32_t MAX_EFFECTS_CPU_LOAD = 1000;
        // Maximum memory allocated to audio effects in KB
        static const uint32_t MAX_EFFECTS_MEMORY = 512;
        uint32_t mTotalEffectsCpuLoad; // current CPU load used by effects
        uint32_t mTotalEffectsMemory;  // current memory used by effects
        KeyedVector<int, EffectDescriptor *> mEffects;  // list of registered audio effects
        bool    mA2dpSuspended;  // true if A2DP output is suspended
        bool mHasA2dp; // true on platforms with support for bluetooth A2DP
        bool mHasUsb; // true on platforms with support for USB audio
        bool mHasRemoteSubmix; // true on platforms with support for remote presentation of a submix
        audio_devices_t mAttachedOutputDevices; // output devices always available on the platform
        audio_devices_t mDefaultOutputDevice; // output device selected by default at boot time
                                              // (must be in mAttachedOutputDevices)
        bool mSpeakerDrcEnabled;// true on devices that use DRC on the DEVICE_CATEGORY_SPEAKER path
                                // to boost soft sounds, used to adjust volume curves accordingly

        Vector <HwModule *> mHwModules;

#ifdef AUDIO_POLICY_TEST
        Mutex   mLock;
        Condition mWaitWorkCV;

        int             mCurOutput;
        bool            mDirectOutput;
        audio_io_handle_t mTestOutputs[NUM_TEST_OUTPUTS];
        int             mTestInput;
        uint32_t        mTestDevice;
        uint32_t        mTestSamplingRate;
        uint32_t        mTestFormat;
        uint32_t        mTestChannels;
        uint32_t        mTestLatencyMs;
#endif //AUDIO_POLICY_TEST

private:
        static float volIndexToAmpl(audio_devices_t device, const StreamDescriptor& streamDesc,
                int indexInUi);
        // updates device caching and output for streams that can influence the
        //    routing of notifications
        void handleNotificationRoutingForStream(AudioSystem::stream_type stream);
        static bool isVirtualInputDevice(audio_devices_t device);
};

};
