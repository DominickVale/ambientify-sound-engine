//
// Created by dominick on 3/4/22.
//
#pragma once

#include "common.h"
#include "EngineChannel.h"

namespace ambientify {

    class SoundEngine {
    public:
        SoundEngine(SoundEngine const &) = delete;

        ~SoundEngine();

        static std::vector<std::shared_ptr<ambientify::EngineChannel>> channels;

        static std::shared_ptr<SoundEngine> GetInstance() {
            if (_instance == nullptr) {
                _instance = std::shared_ptr<SoundEngine>(new SoundEngine());
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

        static void loadChannel(int channelId, const std::string *path);

        static void unloadChannel(int channelId);

        static bool toggleChannelPlayback(int channelId);

        static void playChannel(int channelId);
        static void stopChannel(int channelId);

        static void loadChannelStatus(int channelId, const std::shared_ptr<std::map<std::string, std::any>>& newStatus);

        static void setChannelVolume(int channelId, float volume, float pan);

        static void setChannelCfEnabled(int channelId, bool enabled);

        static void setChannelRandomizationEnabled(int channelId, bool enabled);

        static bool toggleChannelMuted(int channelId);

        static void setChannelMuted(int channelId, bool muted);

        static void setChannelRandomizationSettings(int channelId, const std::shared_ptr<ChannelRandomizationDataSettings>&);

        static void resetChannel(int channelId);

    private:
        SoundEngine();

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
