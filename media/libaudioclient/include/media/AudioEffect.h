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

#ifndef ANDROID_AUDIOEFFECT_H
#define ANDROID_AUDIOEFFECT_H

#include <stdint.h>
#include <sys/types.h>

#include <media/IAudioFlinger.h>
#include <media/AudioSystem.h>
#include <system/audio_effect.h>
#include <android/content/AttributionSourceState.h>

#include <utils/RefBase.h>
#include <utils/Errors.h>
#include <binder/IInterface.h>

#include "android/media/IEffect.h"
#include "android/media/BnEffectClient.h"

namespace android {

// ----------------------------------------------------------------------------

struct effect_param_cblk_t;

// ----------------------------------------------------------------------------

class AudioEffect : public virtual RefBase
{
public:

    /*
     *  Static methods for effects enumeration.
     */

    /*
     * Returns the number of effects available. This method together
     * with queryEffect() is used to enumerate all effects:
     * The enumeration sequence is:
     *      queryNumberEffects(&num_effects);
     *      for (i = 0; i < num_effects; i++)
     *          queryEffect(i,...);
     *
     * Parameters:
     *      numEffects:    address where the number of effects should be returned.
     *
     * Returned status (from utils/Errors.h) can be:
     *      NO_ERROR   successful operation.
     *      PERMISSION_DENIED could not get AudioFlinger interface
     *      NO_INIT    effect library failed to initialize
     *      BAD_VALUE  invalid numEffects pointer
     *
     * Returned value
     *   *numEffects:     updated with number of effects available
     */
    static status_t queryNumberEffects(uint32_t *numEffects);

    /*
     * Returns an effect descriptor during effect
     * enumeration.
     *
     * Parameters:
     *      index:      index of the queried effect.
     *      descriptor: address where the effect descriptor should be returned.
     *
     * Returned status (from utils/Errors.h) can be:
     *      NO_ERROR        successful operation.
     *      PERMISSION_DENIED could not get AudioFlinger interface
     *      NO_INIT         effect library failed to initialize
     *      BAD_VALUE       invalid descriptor pointer or index
     *      INVALID_OPERATION  effect list has changed since last execution of queryNumberEffects()
     *
     * Returned value
     *   *descriptor:     updated with effect descriptor
     */
    static status_t queryEffect(uint32_t index, effect_descriptor_t *descriptor);

    /*
     * Returns a descriptor for the specified effect uuid or type.
     *
     * Lookup an effect by uuid, or if that's unspecified (EFFECT_UUID_NULL),
     * do so by type and preferred flags instead.
     *
     * Parameters:
     *      uuid:       pointer to effect uuid.
     *      type:       pointer to effect type uuid.
     *      preferredTypeFlags: if multiple effects of the given type exist,
     *                  one with a matching type flag will be chosen over one without.
     *                  Use EFFECT_FLAG_TYPE_MASK to indicate no preference.
     *      descriptor: address where the effect descriptor should be returned.
     *
     * Returned status (from utils/Errors.h) can be:
     *      NO_ERROR        successful operation.
     *      PERMISSION_DENIED could not get AudioFlinger interface
     *      NO_INIT         effect library failed to initialize
     *      BAD_VALUE       invalid type or descriptor pointers
     *      NAME_NOT_FOUND  no effect with this uuid found
     *
     * Returned value
     *   *descriptor updated with effect descriptor
     */
    static status_t getEffectDescriptor(const effect_uuid_t *uuid,
                                        const effect_uuid_t *type,
                                        uint32_t preferredTypeFlag,
                                        effect_descriptor_t *descriptor);

    /*
     * Returns a list of descriptors corresponding to the pre processings enabled by default
     * on an AudioRecord with the supplied audio session ID.
     *
     * Parameters:
     *      audioSession:  audio session ID.
     *      descriptors: address where the effect descriptors should be returned.
     *      count: as input, the maximum number of descriptor than should be returned
     *             as output, the number of descriptor returned if status is NO_ERROR or the actual
     *             number of enabled pre processings if status is NO_MEMORY
     *
     * Returned status (from utils/Errors.h) can be:
     *      NO_ERROR        successful operation.
     *      NO_MEMORY       the number of descriptor to return is more than the maximum number
     *                      indicated by count.
     *      PERMISSION_DENIED could not get AudioFlinger interface
     *      NO_INIT         effect library failed to initialize
     *      BAD_VALUE       invalid audio session, or invalid descriptor or count pointers
     *
     * Returned value
     *   *descriptor updated with descriptors of pre processings enabled by default
     *   *count      number of descriptors returned if returned status is NO_ERROR.
     *               total number of pre processing enabled by default if returned status is
     *               NO_MEMORY. This happens if the count passed as input is less than the number
     *               of descriptors to return.
     *               *count is limited to kMaxPreProcessing on return.
     */
    static status_t queryDefaultPreProcessing(audio_session_t audioSession,
                                              effect_descriptor_t *descriptors,
                                              uint32_t *count);

    /*
     * Gets a new system-wide unique effect id.
     *
     * Parameters:
     *      id: The address to return the generated id.
     *
     * Returned status (from utils/Errors.h) can be:
     *      NO_ERROR        successful operation.
     *      PERMISSION_DENIED could not get AudioFlinger interface
     *                        or caller lacks required permissions.
     *      BAD_VALUE       invalid pointer to id
     * Returned value
     *   *id:  The new unique system-wide effect id.
     */
    static status_t newEffectUniqueId(audio_unique_id_t* id);

    /*
     * Static methods for adding/removing system-wide effects.
     */

    /*
     * Adds an effect to the list of default output effects for a given source type.
     *
     * If the effect is no longer available when a source of the given type
     * is created, the system will continue without adding it.
     *
     * Parameters:
     *   typeStr:  Type uuid of effect to be a default: can be null if uuidStr is specified.
     *             This may correspond to the OpenSL ES interface implemented by this effect,
     *             or could be some vendor-defined type.
     *   opPackageName: The package name used for app op checks.
     *   uuidStr:  Uuid of effect to be a default: can be null if type is specified.
     *             This uuid corresponds to a particular implementation of an effect type.
     *             Note if both uuidStr and typeStr are specified, typeStr is ignored.
     *   priority: Requested priority for effect control: the priority level corresponds to the
     *             value of priority parameter: negative values indicate lower priorities, positive
     *             values higher priorities, 0 being the normal priority.
     *   source:   The source this effect should be a default for.
     *   id:       Address where the system-wide unique id of the default effect should be returned.
     *
     * Returned status (from utils/Errors.h) can be:
     *      NO_ERROR        successful operation.
     *      PERMISSION_DENIED could not get AudioFlinger interface
     *                        or caller lacks required permissions.
     *      NO_INIT         effect library failed to initialize.
     *      BAD_VALUE       invalid source, type uuid or implementation uuid, or id pointer
     *      NAME_NOT_FOUND  no effect with this uuid or type found.
     *
     * Returned value
     *   *id:  The system-wide unique id of the added default effect.
     */
    static status_t addSourceDefaultEffect(const char* typeStr,
                                           const String16& opPackageName,
                                           const char* uuidStr,
                                           int32_t priority,
                                           audio_source_t source,
                                           audio_unique_id_t* id);

    /*
     * Adds an effect to the list of default output effects for a given stream type.
     *
     * If the effect is no longer available when a stream of the given type
     * is created, the system will continue without adding it.
     *
     * Parameters:
     *   typeStr:  Type uuid of effect to be a default: can be null if uuidStr is specified.
     *             This may correspond to the OpenSL ES interface implemented by this effect,
     *             or could be some vendor-defined type.
     *   opPackageName: The package name used for app op checks.
     *   uuidStr:  Uuid of effect to be a default: can be null if type is specified.
     *             This uuid corresponds to a particular implementation of an effect type.
     *             Note if both uuidStr and typeStr are specified, typeStr is ignored.
     *   priority: Requested priority for effect control: the priority level corresponds to the
     *             value of priority parameter: negative values indicate lower priorities, positive
     *             values higher priorities, 0 being the normal priority.
     *   usage:    The usage this effect should be a default for. Unrecognized values will be
     *             treated as AUDIO_USAGE_UNKNOWN.
     *   id:       Address where the system-wide unique id of the default effect should be returned.
     *
     * Returned status (from utils/Errors.h) can be:
     *      NO_ERROR        successful operation.
     *      PERMISSION_DENIED could not get AudioFlinger interface
     *                        or caller lacks required permissions.
     *      NO_INIT         effect library failed to initialize.
     *      BAD_VALUE       invalid type uuid or implementation uuid, or id pointer
     *      NAME_NOT_FOUND  no effect with this uuid or type found.
     *
     * Returned value
     *   *id:  The system-wide unique id of the added default effect.
     */
    static status_t addStreamDefaultEffect(const char* typeStr,
                                           const String16& opPackageName,
                                           const char* uuidStr,
                                           int32_t priority,
                                           audio_usage_t usage,
                                           audio_unique_id_t* id);

    /*
     * Removes an effect from the list of default output effects for a given source type.
     *
     * Parameters:
     *      id: The system-wide unique id of the effect that should no longer be a default.
     *
     * Returned status (from utils/Errors.h) can be:
     *      NO_ERROR        successful operation.
     *      PERMISSION_DENIED could not get AudioFlinger interface
     *                        or caller lacks required permissions.
     *      NO_INIT         effect library failed to initialize.
     *      BAD_VALUE       invalid id.
     */
    static status_t removeSourceDefaultEffect(audio_unique_id_t id);

    /*
     * Removes an effect from the list of default output effects for a given stream type.
     *
     * Parameters:
     *      id: The system-wide unique id of the effect that should no longer be a default.
     *
     * Returned status (from utils/Errors.h) can be:
     *      NO_ERROR        successful operation.
     *      PERMISSION_DENIED could not get AudioFlinger interface
     *                        or caller lacks required permissions.
     *      NO_INIT         effect library failed to initialize.
     *      BAD_VALUE       invalid id.
     */
    static status_t removeStreamDefaultEffect(audio_unique_id_t id);

    /*
     * Events used by callback function (legacy_callback_t).
     */
    enum event_type {
        EVENT_CONTROL_STATUS_CHANGED = 0,
        EVENT_ENABLE_STATUS_CHANGED = 1,
        EVENT_PARAMETER_CHANGED = 2,
        EVENT_ERROR = 3,
        EVENT_FRAMES_PROCESSED = 4,
    };

    class IAudioEffectCallback : public virtual RefBase {
        friend AudioEffect;
     protected:
        /* The event is received when an application loses or
         * gains the control of the effect engine. Loss of control happens
         * if another application requests the use of the engine by creating an AudioEffect for
         * the same effect type but with a higher priority. Control is returned when the
         * application having the control deletes its AudioEffect object.
         * @param isGranted: True if control has been granted. False if stolen.
         */
        virtual void onControlStatusChanged([[maybe_unused]] bool isGranted) {}

        /* The event is received by all applications not having the
         * control of the effect engine when the effect is enabled or disabled.
         * @param isEnabled: True if enabled. False if disabled.
         */
        virtual void onEnableStatusChanged([[maybe_unused]] bool isEnabled) {}

        /* The event is received by all applications not having the
         * control of the effect engine when an effect parameter is changed.
         * @param param: A vector containing the raw bytes of a effect_param_type containing
         *   raw data representing a param type, value pair.
         */
        // TODO pass an AIDL parcel instead of effect_param_type
        virtual void onParameterChanged([[maybe_unused]] std::vector<uint8_t> param) {}

        /* The event is received when the binder connection to the mediaserver
         * is no longer valid. Typically the server has been killed.
         * @param errorCode: A code representing the type of error.
         */
        virtual void onError([[maybe_unused]] status_t errorCode) {}


        /* The event is received when the audio server has processed a block of
         * data.
         * @param framesProcessed: The number of frames the audio server has
         *   processed.
         */
        virtual void onFramesProcessed([[maybe_unused]] int32_t framesProcessed) {}
    };

    /* Callback function notifying client application of a change in effect engine state or
     * configuration.
     * An effect engine can be shared by several applications but only one has the control
     * of the engine activity and configuration at a time.
     * The EVENT_CONTROL_STATUS_CHANGED event is received when an application loses or
     * retrieves the control of the effect engine. Loss of control happens
     * if another application requests the use of the engine by creating an AudioEffect for
     * the same effect type but with a higher priority. Control is returned when the
     * application having the control deletes its AudioEffect object.
     * The EVENT_ENABLE_STATUS_CHANGED event is received by all applications not having the
     * control of the effect engine when the effect is enabled or disabled.
     * The EVENT_PARAMETER_CHANGED event is received by all applications not having the
     * control of the effect engine when an effect parameter is changed.
     * The EVENT_ERROR event is received when the media server process dies.
     *
     * Parameters:
     *
     * event:   type of event notified (see enum AudioEffect::event_type).
     * user:    Pointer to context for use by the callback receiver.
     * info:    Pointer to optional parameter according to event type:
     *  - EVENT_CONTROL_STATUS_CHANGED:  boolean indicating if control is granted (true)
     *  or stolen (false).
     *  - EVENT_ENABLE_STATUS_CHANGED: boolean indicating if effect is now enabled (true)
     *  or disabled (false).
     *  - EVENT_PARAMETER_CHANGED: pointer to a effect_param_t structure.
     *  - EVENT_ERROR:  status_t indicating the error (DEAD_OBJECT when media server dies).
     */

    typedef void (*legacy_callback_t)(int32_t event, void* user, void *info);


    /* Constructor.
     * AudioEffect is the base class for creating and controlling an effect engine from
     * the application process. Creating an AudioEffect object will create the effect engine
     * in the AudioFlinger if no engine of the specified type exists. If one exists, this engine
     * will be used. The application creating the AudioEffect object (or a derived class like
     * Reverb for instance) will either receive control of the effect engine or not, depending
     * on the priority parameter. If priority is higher than the priority used by the current
     * effect engine owner, the control will be transfered to the new application. Otherwise
     * control will remain to the previous application. In this case, the new application will be
     * notified of changes in effect engine state or control ownership by the effect callback.
     * After creating the AudioEffect, the application must call the initCheck() method and
     * check the creation status before trying to control the effect engine (see initCheck()).
     * If the effect is to be applied to an AudioTrack or MediaPlayer only the application
     * must specify the audio session ID corresponding to this player.
     */

    /* Simple Constructor.
     *
     * Parameters:
     *
     * client:      Attribution source for app-op checks
     */
    explicit AudioEffect(const android::content::AttributionSourceState& client);

    /* Terminates the AudioEffect and unregisters it from AudioFlinger.
     * The effect engine is also destroyed if this AudioEffect was the last controlling
     * the engine.
     */
                        ~AudioEffect();

    /**
     * Initialize an uninitialized AudioEffect.
     *
     * Parameters:
     *
     * type:  type of effect created: can be null if uuid is specified. This corresponds to
     *        the OpenSL ES interface implemented by this effect.
     * uuid:  Uuid of effect created: can be null if type is specified. This uuid corresponds to
     *        a particular implementation of an effect type.
     * priority:    requested priority for effect control: the priority level corresponds to the
     *      value of priority parameter: negative values indicate lower priorities, positive values
     *      higher priorities, 0 being the normal priority.
     * cbf:         optional callback function (see legacy_callback_t)
     * user:        pointer to context for use by the callback receiver.
     * sessionId:   audio session this effect is associated to.
     *      If equal to AUDIO_SESSION_OUTPUT_MIX, the effect will be global to
     *      the output mix.  Otherwise, the effect will be applied to all players
     *      (AudioTrack or MediaPLayer) within the same audio session.
     * io:  HAL audio output or input stream to which this effect must be attached. Leave at 0 for
     *      automatic output selection by AudioFlinger.
     * device: An audio device descriptor. Only used when "sessionID" is AUDIO_SESSION_DEVICE.
     *         Specifies the audio device type and address the effect must be attached to.
     *         If "sessionID" is AUDIO_SESSION_DEVICE then "io" must be AUDIO_IO_HANDLE_NONE.
     * probe: true if created in a degraded mode to only verify if effect creation is possible.
     *        In this mode, no IEffect interface to AudioFlinger is created and all actions
     *        besides getters implemented in client AudioEffect object are no ops
     *        after effect creation.
     *
     * Returned status (from utils/Errors.h) can be:
     *  - NO_ERROR or ALREADY_EXISTS: successful initialization
     *  - INVALID_OPERATION: AudioEffect is already initialized
     *  - BAD_VALUE: invalid parameter
     *  - NO_INIT: audio flinger or audio hardware not initialized
     */
            status_t    set(const effect_uuid_t *type,
                            const effect_uuid_t *uuid = nullptr,
                            int32_t priority = 0,
                            const wp<IAudioEffectCallback>& callback = nullptr,
                            audio_session_t sessionId = AUDIO_SESSION_OUTPUT_MIX,
                            audio_io_handle_t io = AUDIO_IO_HANDLE_NONE,
                            const AudioDeviceTypeAddr& device = {},
                            bool probe = false,
                            bool notifyFramesProcessed = false);

            status_t    set(const effect_uuid_t *type,
                            const effect_uuid_t *uuid,
                            int32_t priority,
                            legacy_callback_t cbf,
                            void* user,
                            audio_session_t sessionId = AUDIO_SESSION_OUTPUT_MIX,
                            audio_io_handle_t io = AUDIO_IO_HANDLE_NONE,
                            const AudioDeviceTypeAddr& device = {},
                            bool probe = false,
                            bool notifyFramesProcessed = false);
    /*
     * Same as above but with type and uuid specified by character strings.
     */
            status_t    set(const char *typeStr,
                            const char *uuidStr = nullptr,
                            int32_t priority = 0,
                            const wp<IAudioEffectCallback>& callback = nullptr,
                            audio_session_t sessionId = AUDIO_SESSION_OUTPUT_MIX,
                            audio_io_handle_t io = AUDIO_IO_HANDLE_NONE,
                            const AudioDeviceTypeAddr& device = {},
                            bool probe = false,
                            bool notifyFramesProcessed = false);


            status_t    set(const char *typeStr,
                            const char *uuidStr,
                            int32_t priority,
                            legacy_callback_t cbf,
                            void* user,
                            audio_session_t sessionId = AUDIO_SESSION_OUTPUT_MIX,
                            audio_io_handle_t io = AUDIO_IO_HANDLE_NONE,
                            const AudioDeviceTypeAddr& device = {},
                            bool probe = false,
                            bool notifyFramesProcessed = false);

    /* Result of constructing the AudioEffect. This must be checked
     * before using any AudioEffect API.
     * initCheck() can return:
     *  - NO_ERROR:    the effect engine is successfully created and the application has control.
     *  - ALREADY_EXISTS: the effect engine is successfully created but the application does not
     *              have control.
     *  - NO_INIT:     the effect creation failed.
     *
     */
            status_t    initCheck() const;


    /* Returns the unique effect Id for the controlled effect engine. This ID is unique
     * system wide and is used for instance in the case of auxiliary effects to attach
     * the effect to an AudioTrack or MediaPlayer.
     *
     */
            int32_t     id() const { return mId; }

    /* Returns a descriptor for the effect (see effect_descriptor_t in audio_effect.h).
     */
            effect_descriptor_t descriptor() const;

    /* Returns effect control priority of this AudioEffect object.
     */
            int32_t     priority() const { return mPriority; }


    /* Enables or disables the effect engine.
     *
     * Parameters:
     *  enabled: requested enable state.
     *
     * Returned status (from utils/Errors.h) can be:
     *  - NO_ERROR: successful operation
     *  - INVALID_OPERATION: the application does not have control of the effect engine or the
     *  effect is already in the requested state.
     */
    virtual status_t    setEnabled(bool enabled);
            bool        getEnabled() const;

    /* Sets a parameter value.
     *
     * Parameters:
     *      param:  pointer to effect_param_t structure containing the parameter
     *          and its value (See audio_effect.h).
     * Returned status (from utils/Errors.h) can be:
     *  - NO_ERROR: successful operation.
     *  - INVALID_OPERATION: the application does not have control of the effect engine.
     *  - BAD_VALUE: invalid parameter structure pointer, or invalid identifier or value.
     *  - DEAD_OBJECT: the effect engine has been deleted.
     */
     virtual status_t   setParameter(effect_param_t *param);

    /* Prepare a new parameter value that will be set by next call to
     * setParameterCommit(). This method can be used to set multiple parameters
     * in a synchronous manner or to avoid multiple binder calls for each
     * parameter.
     *
     * Parameters:
     *      param:  pointer to effect_param_t structure containing the parameter
     *          and its value (See audio_effect.h).
     *
     * Returned status (from utils/Errors.h) can be:
     *  - NO_ERROR: successful operation.
     *  - INVALID_OPERATION: the application does not have control of the effect engine.
     *  - NO_MEMORY: no more space available in shared memory used for deferred parameter
     *  setting.
     */
     virtual status_t   setParameterDeferred(effect_param_t *param);

     /* Commit all parameter values previously prepared by setParameterDeferred().
      *
      * Parameters:
      *     none
      *
      * Returned status (from utils/Errors.h) can be:
      *  - NO_ERROR: successful operation.
      *  - INVALID_OPERATION: No new parameter values ready for commit.
      *  - BAD_VALUE: invalid parameter identifier or value: there is no indication
      *     as to which of the parameters caused this error.
      *  - DEAD_OBJECT: the effect engine has been deleted.
      */
     virtual status_t   setParameterCommit();

    /* Gets a parameter value.
     *
     * Parameters:
     *      param:  pointer to effect_param_t structure containing the parameter
     *          and the returned value (See audio_effect.h).
     *
     * Returned status (from utils/Errors.h) can be:
     *  - NO_ERROR: successful operation.
     *  - INVALID_OPERATION: the AudioEffect was not successfully initialized.
     *  - BAD_VALUE: invalid parameter structure pointer, or invalid parameter identifier.
     *  - DEAD_OBJECT: the effect engine has been deleted.
     */
     virtual status_t   getParameter(effect_param_t *param);

     /* Sends a command and receives a response to/from effect engine.
      *     See audio_effect.h for details on effect command() function, valid command codes
      *     and formats.
      */
     virtual status_t command(uint32_t cmdCode,
                              uint32_t cmdSize,
                              void *cmdData,
                              uint32_t *replySize,
                              void *replyData);


     /*
      * Utility functions.
      */

     /* Converts the string passed as first argument to the effect_uuid_t
      * pointed to by second argument
      */
     static status_t stringToGuid(const char *str, effect_uuid_t *guid);
     /* Converts the effect_uuid_t pointed to by first argument to the
      * string passed as second argument
      */
     static status_t guidToString(const effect_uuid_t *guid, char *str, size_t maxLen);

     // kMaxPreProcessing is a reasonable value for the maximum number of preprocessing effects
     // that can be applied simultaneously.
     static const uint32_t kMaxPreProcessing = 10;

protected:
     android::content::AttributionSourceState mClientAttributionSource; // source for app op checks.
     bool                     mEnabled = false;   // enable state
     audio_session_t          mSessionId = AUDIO_SESSION_OUTPUT_MIX; // audio session ID
     int32_t                  mPriority = 0;      // priority for effect control
     status_t                 mStatus = NO_INIT;  // effect status
     bool                     mProbe = false;     // effect created in probe mode: all commands
                                                 // are no ops because mIEffect is nullptr

     wp<IAudioEffectCallback> mCallback = nullptr; // callback interface for status, control and
                                                   // parameter changes notifications
     sp<IAudioEffectCallback> mLegacyWrapper = nullptr;
     effect_descriptor_t      mDescriptor = {};   // effect descriptor
     int32_t                  mId = -1;           // system wide unique effect engine instance ID
     Mutex                    mLock;              // Mutex for mEnabled access


     // IEffectClient
     virtual void controlStatusChanged(bool controlGranted);
     virtual void enableStatusChanged(bool enabled);
     virtual void commandExecuted(int32_t cmdCode,
                                  const std::vector<uint8_t>& cmdData,
                                  const std::vector<uint8_t>& replyData);
     virtual void framesProcessed(int32_t frames);

private:

     // Implements the IEffectClient interface
    class EffectClient :
        public media::BnEffectClient, public android::IBinder::DeathRecipient
    {
    public:

        explicit EffectClient(const sp<AudioEffect>& effect) : mEffect(effect){}

        // IEffectClient
        binder::Status controlStatusChanged(bool controlGranted) override {
            sp<AudioEffect> effect = mEffect.promote();
            if (effect != 0) {
                effect->controlStatusChanged(controlGranted);
            }
            return binder::Status::ok();
        }
        binder::Status enableStatusChanged(bool enabled) override {
            sp<AudioEffect> effect = mEffect.promote();
            if (effect != 0) {
                effect->enableStatusChanged(enabled);
            }
            return binder::Status::ok();
        }
        binder::Status commandExecuted(int32_t cmdCode,
                             const std::vector<uint8_t>& cmdData,
                             const std::vector<uint8_t>& replyData) override {
            sp<AudioEffect> effect = mEffect.promote();
            if (effect != 0) {
                effect->commandExecuted(cmdCode, cmdData, replyData);
            }
            return binder::Status::ok();
        }
        binder::Status framesProcessed(int32_t frames) override {
            sp<AudioEffect> effect = mEffect.promote();
            if (effect != 0) {
                effect->framesProcessed(frames);
            }
            return binder::Status::ok();
        }


        // IBinder::DeathRecipient
        virtual void binderDied(const wp<IBinder>& /*who*/) {
            sp<AudioEffect> effect = mEffect.promote();
            if (effect != 0) {
                effect->binderDied();
            }
        }

    private:
        wp<AudioEffect> mEffect;
    };

    void binderDied();

    sp<media::IEffect>      mIEffect;           // IEffect binder interface
    sp<EffectClient>        mIEffectClient;     // IEffectClient implementation
    sp<IMemory>             mCblkMemory;        // shared memory for deferred parameter setting
    effect_param_cblk_t*    mCblk = nullptr;    // control block for deferred parameter setting
};


}; // namespace android

#endif // ANDROID_AUDIOEFFECT_H
