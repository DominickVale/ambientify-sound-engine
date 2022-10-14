//
// Created by dominick on 3/4/22.
//
#pragma once

#include <jni.h>
#include "common.h"
#include "EngineChannel.h"

namespace ambientify {

    class SoundEngine {
    public:
        SoundEngine(SoundEngine const &) = delete;

        ~SoundEngine();

        JNIEnv *env;
        JavaVM *jvm;
        static std::shared_ptr<SoundEngine> GetInstance(JNIEnv *jniEnv, JavaVM *javaVm) {
            if (_instance == nullptr) {
                _instance = std::shared_ptr<SoundEngine>(new SoundEngine(jniEnv, javaVm));
            }
            return _instance;
        }

        bool isEngineReady = false;

        bool isEngineReadyFn() {
            return isEngineReady;
        }

        void update();

        int createChannel();

        int createChannel(const std::string *path);

        void loadChannel(int channelId, const std::string *path);

        void unloadChannel(int channelId);

        bool toggleChannelPlayback(int channelId);

        bool playChannel(int channelId);

        bool stopChannel(int channelId);

        void playAll();

        void stopAll();

        void loadChannelStatus(int channelId, const std::shared_ptr<std::map<std::string, std::any>> &newStatus);

        void setChannelVolume(int channelId, float volume, float pan);

        void setMasterVolume(float volume);

        double getMasterVolume();

        bool setChannelCfEnabled(int channelId, bool enabled);

        bool setChannelRandomizationEnabled(int channelId, bool enabled);

        bool toggleChannelMuted(int channelId);

        void setChannelMuted(int channelId, bool muted);

        void setChannelRandomizationSettings(int channelId, const std::shared_ptr<ChannelRandomizationDataSettings> &);

        void resetChannel(int channelId);

        static std::vector<std::shared_ptr<ambientify::EngineChannel>> channels;

    private:
        SoundEngine(JNIEnv *jniEnv, JavaVM *javaVm);

        static std::shared_ptr<SoundEngine> _instance;
        static constexpr auto TAG = "ASoundEngine";

        bool _stopped = false;

        FMOD::System *system{};
        FMOD_RESULT result;

        std::shared_ptr<std::function<void(std::shared_ptr<std::vector<SerializedChannelStatus *>>)>> _onStatusUpdate;

        static FMOD_RESULT F_CALLBACK channelCallback(FMOD_CHANNELCONTROL *, FMOD_CHANNELCONTROL_TYPE,
                                                      FMOD_CHANNELCONTROL_CALLBACK_TYPE, void *, void *);
    };
}
