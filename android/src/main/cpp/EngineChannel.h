//
// Created by dominick on 3/6/22.
//

#pragma once

#include <fmod.hpp>
#include "common.h"

namespace ambientify {

    struct ChannelRandomizationDataSettings {
        int times;
        int minutes;
        std::optional<std::pair<float, float>> volumeRange;
        std::optional<std::pair<float, float>> panRange;
        std::optional<std::pair<float, float>> pitchRange;
    };

    struct SerializedChannelStatus {
        std::string currentFilePath;
        int id;
        unsigned int durationMs;
        bool isLoaded;
        bool isLoading;
        bool isPlaying;
        bool didJustFinish;
        bool isMuted;
        float volume;
        float pan;
        float pitch;
        bool crossfadeEnabled;
        float cfPercentageStart;
        bool noUnload;
        bool randomizationEnabled;
        ChannelRandomizationDataSettings rSettings;
    };

    struct GeneratedRandomizationData {
        long int delayMs;
        float volume;
        float pan;
        float pitch;
    };

    class EngineChannel {
    public:
        EngineChannel(
                FMOD::System *system,
                FMOD_CHANNELCONTROL_CALLBACK channelsCallback,
                int id
        ) : system(system),
            _channelsCallback(channelsCallback),
            _id(id) {};

        EngineChannel(
                FMOD::System *system,
                FMOD_CHANNELCONTROL_CALLBACK channelsCallback,
                int id,
                const std::string *path,
                bool shouldPlay
        ) : system(system),
            _channelsCallback(channelsCallback),
            _id(id),
            _shouldPlay(shouldPlay) {
            load(path);
        }

        ~EngineChannel();

        void load(const std::string *soundFileName, bool shouldPlay);

        void load(const std::string *soundFileName);

        void loadSecondary();

        void unload();

        void prepare();

        void play();

        void stop();

        void setCrossfadeEnabled(bool);

        bool setMuted(bool);

        void setRandomizationEnabled(bool);

        void setRandomizationSettings(const std::shared_ptr<ChannelRandomizationDataSettings>&);

        void runNextRandomFrame();

        SerializedChannelStatus *getSerializedStatus() { return &_serializedStatus; }

        FMOD_RESULT update();

        void loadStatus(const std::shared_ptr<std::map<std::string, std::any>> &newStatus);

        void setVolume(float volume, float pan);

        std::shared_ptr<EngineChannel> getSecondaryInstance() { return _secondaryChannel; }

        void crossfade(bool inverted);

        void reset();

        unsigned int getDurationMs() { return _durationMs; };

        auto getPositionMs() {
            unsigned int positionMs;
            _fchannel->getPosition(&positionMs, FMOD_TIMEUNIT_MS);
            return positionMs;
        };

        void onChannelEndCallback();

        bool neverStarted = true;
        const float &cfPercentageStart = _cfPercentageStart;
        const std::string &currFilePath = _currentFilePath;
        const int &id = _id;
        const bool &isCrossfading = _isCrossfading;
        const bool &crossfadeEnabled = _crossfadeEnabled;
        const bool &isLoading = _isLoading;
        const bool &isLoaded = _isLoaded;
        const bool &isPlaying = _isPlaying;
        const bool &randomizationEnabled = _randomizationEnabled;
        const bool &isMuted = _isMuted;

    private:
        FMOD::System *system{};
        FMOD::Sound *_sound;
        FMOD::Channel *_fchannel;
        std::shared_ptr<EngineChannel> _secondaryChannel;
        FMOD_RESULT _result;
        FMOD_OPENSTATE _openState;
        FMOD_SYNCPOINT *_cfSyncPoint = nullptr;

        FMOD_CHANNELCONTROL_CALLBACK _channelsCallback;

        // Randomization
        bool _randomizationEnabled = false;
        ChannelRandomizationDataSettings _rSettings {1, 1};
        int _currRandomTimeframe = 0;
        std::vector<GeneratedRandomizationData> _rTimeframes;
        std::chrono::system_clock::time_point _rT_A;
        std::chrono::system_clock::time_point _rT_B;

        std::mutex _cfMutex;
        int _id;
        // is this a secondary channel (used for crossfading the primary/parent)
        bool _isSecondary = _id > constants::MAX_CHANNELS;
        bool _isLoaded = false;
        bool _isMuted = false;
        bool _isLoading = false;
        bool _isPlaying = false;
        bool _isCrossfading = false;
        bool _crossfadeEnabled = false;
        bool _noUnload = true;
        // is secondary turn to fade out in crosssfade?
        bool _cfIsSecondaryFadingOut = false;
        bool _didJustFinish = false;
        bool _shouldPlay = false;
        unsigned int _durationMs = 0;
        float _volume = 1;
        float _pan = 0;
        float _pitch = 1;
        float _cfPercentageStart = 0.75;
        std::string _currentFilePath = "";

        SerializedChannelStatus _serializedStatus{
                _currentFilePath = "",
                _id,
                _durationMs,
                _isLoaded,
                _isLoading,
                _isPlaying,
                _didJustFinish,
                _isMuted,
                _volume,
                _pan,
                _pitch,
                _crossfadeEnabled,
                _cfPercentageStart,
                _noUnload,
                _randomizationEnabled,
                _rSettings
        };

        std::chrono::system_clock::time_point _cfStartMs;
        int _cfDurationMs = 1000; // arbitrary; will be set based on sound duration
        static constexpr int _cfSteps = 10;
        std::chrono::system_clock::time_point _cfT_A;
        std::chrono::system_clock::time_point _cfT_B;

        void _updateCrossfade();

        void _updateSerializedStatus();

        void _updateSyncPoints();

        void _updateCfDuration();

        void _setLooping(bool);

        void _generateRandomTimeframes();
    };
}
