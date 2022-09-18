//
// Created by dominick on 3/6/22.
//

#include "EngineChannel.h"

ambientify::EngineChannel::~EngineChannel() {
    if (_fchannel) {
        _fchannel->setCallback(nullptr);
        _fchannel->stop();
        _sound->release();
        _sound = nullptr;
    }
}

void ambientify::EngineChannel::unload() {
    _noUnload = false;
    _randomizationEnabled = false;
    _crossfadeEnabled = false;
    stop();
    if (_secondaryChannel && _secondaryChannel->_isLoaded) {
        _secondaryChannel->_noUnload = false;
        _secondaryChannel->unload();
        _secondaryChannel = nullptr;
    }
}

void ambientify::EngineChannel::load(const std::string *soundFileName) {
    LOG_DEBUG("Loading channel %d", _id);
    _noUnload = true;
    FMOD_MODE loopMode = (_crossfadeEnabled || _randomizationEnabled) ? FMOD_LOOP_OFF
                                                                      : FMOD_LOOP_NORMAL;
    // if it's a secondary channel disable the loop regardless
    if (_isSecondary) loopMode = FMOD_LOOP_OFF;
    _isLoading = true;
    _result = system->createStream(soundFileName->c_str(), FMOD_DEFAULT | loopMode, nullptr,
                                   &_sound);
    // remove if FMOD_NON_BLOCKING
    _isLoaded = _result == FMOD_OK;
    _isLoading = false;
    ERRCHECK(_result);
    _currentFilePath = *soundFileName;
    prepare();
}

void ambientify::EngineChannel::load(const std::string *soundFileName, bool shouldPlay) {
    _shouldPlay = shouldPlay;
    load(soundFileName);
}

void ambientify::EngineChannel::loadSecondary() {
    if (_crossfadeEnabled && !_isSecondary) {
        if (!_secondaryChannel) {
            auto newId = constants::MAX_CHANNELS + _id + 1;
            _secondaryChannel = std::make_shared<EngineChannel>(system, _channelsCallback, newId,
                                                                &_currentFilePath, false);
            _secondaryChannel->_noUnload = true;
        } else {
            if (_secondaryChannel->isLoaded) _secondaryChannel->unload();
            _secondaryChannel->load(&_currentFilePath);
        }
        _secondaryChannel->_fchannel->setVolume(0);
        _secondaryChannel->_cfDurationMs = _cfDurationMs;
        _secondaryChannel->_cfPercentageStart = _cfPercentageStart;
        _secondaryChannel->_updateSyncPoints();
    }
}

// load() and prepare() are currently separated to make it easier to transition to eventually transition to NON_BLOCKING
// although it will probably stay this way since i haven't noticed any issues with the blocking version
void ambientify::EngineChannel::prepare() {
    LOG_DEBUG("Preparing channel %d", _id);
    // only parent channels should have a child. If id > MAX_CHANNELS then it's a child
    if (!_isSecondary && crossfadeEnabled &&
        (!_secondaryChannel || !_secondaryChannel->isPlaying)) {
        loadSecondary();
    }
    _result = system->playSound(_sound, nullptr, true, &_fchannel);
    ERRCHECK(_result);
    _fchannel->setUserData(&_id);
    ERRCHECK(_result);
    _result = _sound->getLength(&_durationMs, FMOD_TIMEUNIT_MS);
    ERRCHECK(_result);
    // @todo: implement function that generates the minimum amount of minutes and times
    if (!randomizationEnabled) {
        _rSettings.minutes = ceil(_durationMs / 60000);
        _rSettings.minutes = _rSettings.minutes > 0 ? _rSettings.minutes : 1;
        _rSettings.times = (_rSettings.minutes * 60000) / _durationMs;
    }
    _result = _fchannel->setVolume(_isCrossfading ? 0 : _volume);
    ERRCHECK(_result);
    _updateCfDuration();
    _updateSyncPoints();
    if (!_isSecondary) _setLooping(!(crossfadeEnabled || randomizationEnabled));
    _result = _fchannel->setPan(_pan);
    ERRCHECK(_result);
    _result = _fchannel->setCallback(_channelsCallback);
    ERRCHECK(_result);
    _didJustFinish = false;
    _didJustPause = false;
    _shouldPlay = false;
    _isLoaded = true;
    _isLoading = false;
    setMuted(_isMuted);
}

void ambientify::EngineChannel::play() {
    if (_isPlaying && !randomizationEnabled) return;
    if (randomizationEnabled && _didJustPause) {
        _didJustPause = false;
    }
    if (!_isLoaded || _isLoading) {
        throw ambientify::commons::ASoundEngineException("Sound is not yet loaded");
    }
    _result = _fchannel->setPaused(false);
    ERRCHECK(_result);
    if (!isCrossfading && !_randomizationEnabled) setVolume(_volume, _pan);
    _isPlaying = true;
    _didJustPause = false;
    neverStarted = false;
    if (_crossfadeEnabled && (!_secondaryChannel || !_secondaryChannel->_isLoaded)) {
        loadSecondary();
    }
    _updateSerializedStatus();
}

void ambientify::EngineChannel::stop() {
    if (!_isLoaded || _isLoading)
        throw ambientify::commons::ASoundEngineException("Sound is not yet loaded");

    if (!_isSecondary && _secondaryChannel && _secondaryChannel->isPlaying) {
        std::scoped_lock lock(_cfMutex);
        _secondaryChannel->stop();
    }
    if (_fchannel != nullptr) {
        if (_noUnload) {
            _result = _fchannel->setPaused(true);
            ERRCHECK(_result);
            _result = _fchannel->setPosition(0, FMOD_TIMEUNIT_MS);
            ERRCHECK(_result);
        } else {
            _result = _fchannel->setCallback(nullptr);
            ERRCHECK(_result);
            _result = _fchannel->stop();
            ERRCHECK(_result);
            _isLoaded = false;
            _isLoading = false;
        }
    }
    _didJustPause = true;
    _isPlaying = false;
    _isCrossfading = false;
    _updateSerializedStatus();
}

void ambientify::EngineChannel::onChannelEndCallback() {
    _isPlaying = false;
    _didJustFinish = true;
    _fchannel = nullptr;
    _sound = nullptr;
    if (_noUnload && !neverStarted) load(&_currentFilePath, false);
}

void ambientify::EngineChannel::crossfade(bool inverted) {
    std::scoped_lock lock(_cfMutex);
    _cfIsSecondaryFadingOut = inverted;
    _cfT_A = std::chrono::system_clock::now();
    _cfT_B = std::chrono::system_clock::now();
    _cfStartMs = std::chrono::system_clock::now();
    if (!_secondaryChannel->_isLoaded || !_isLoaded || !_crossfadeEnabled) return;
    _isCrossfading = true;
    if (!_cfIsSecondaryFadingOut) {
        _secondaryChannel->setVolume(0, _pan);
        if (isMuted) _secondaryChannel->setMuted(true);
        _secondaryChannel->play();
        LOG_DEBUG("Starting secondary with volume %f", _secondaryChannel->_volume);
    } else {
        _fchannel->setVolume(0);
        play();
        LOG_DEBUG("Starting primary with volume %d", 0);
    }
}

void ambientify::EngineChannel::_updateCrossfade() {
    using namespace std::chrono;
    if (_cfT_B - _cfT_A > milliseconds(_cfDurationMs / _cfSteps)) {
        std::scoped_lock lock(_cfMutex);
        if (!_isLoaded || _isLoading || !_secondaryChannel->_isLoaded ||
            _secondaryChannel->_isLoading)
            return;
        auto now = time_point_cast<milliseconds>(system_clock::now()).time_since_epoch().count();
        auto cfStartMs = time_point_cast<milliseconds>(_cfStartMs).time_since_epoch().count();
        auto t_ms = now - cfStartMs;

        auto toFadeOut = _cfIsSecondaryFadingOut ? _secondaryChannel->_fchannel : _fchannel;
        auto toFadeIn = _cfIsSecondaryFadingOut ? _fchannel : _secondaryChannel->_fchannel;

        // get percentage of time elapsed since crossfade started
        float progress = (float) t_ms / (float) _cfDurationMs;

        try {
            if (progress < 1) {
                float newFadeOutVol = _volume * (float) cos(progress * 0.5 * M_PI);
                float newFadeInVol = _volume * (float) sin(progress * 0.5 * M_PI);
                LOG_DEBUG("Crossfading, new volumes: %f , %f", newFadeOutVol, newFadeInVol);
                _result = toFadeOut->setVolume(newFadeOutVol);
                ERRCHECK(_result);
                _result = toFadeIn->setPan(_pan); // update pan in case changed
                ERRCHECK(_result);
                _result = toFadeIn->setVolume(newFadeInVol);
                ERRCHECK(_result);
                _result = toFadeOut->setPan(_pan); // update pan in case changed
                ERRCHECK(_result);
            } else {
                _isCrossfading = false;
                _cfIsSecondaryFadingOut = !_cfIsSecondaryFadingOut;
                _result = toFadeIn->setVolume(_volume);
                ERRCHECK(_result);
                LOG_DEBUG("Setting fadeIn to original volume %f", _volume);
                // we need to update pan in case changed (secondary gets updated because EngineChannel::setVolume is used instead
                _result = toFadeIn->setPan(_pan);
                ERRCHECK(_result);
            }

        } catch (const commons::ASoundEngineException &e) {
            LOG_DEBUG("%s", e.what());
        }
        _cfT_A = system_clock::now();
    }
    _cfT_B = system_clock::now();
}

FMOD_RESULT ambientify::EngineChannel::update() {
    if (!isLoaded) {
        _updateSerializedStatus();
        return FMOD_OK;
    }
    if (!_sound && !_isSecondary) {
        _updateSerializedStatus();
        return FMOD_ERR_INVALID_HANDLE;
    }
    _sound->getOpenState(&_openState, nullptr, nullptr, nullptr);
    if (_secondaryChannel) _secondaryChannel->update();
    switch (_openState) {
        case FMOD_OPENSTATE_READY:
            _isLoading = false;
            _isLoaded = true;
            break;
        case FMOD_OPENSTATE_BUFFERING:
        case FMOD_OPENSTATE_LOADING:
            _isLoading = true;
            _isLoaded = false;
            break;
        case FMOD_OPENSTATE_ERROR:
            _isLoading = false;
            _isLoaded = false;
            break;
        default:
            break;
    }
    if (_shouldPlay && !_isPlaying) play();
    if (!_isSecondary) _updateSerializedStatus();
    if (_isCrossfading) _updateCrossfade();
    if (_randomizationEnabled) runNextRandomFrame();
    return _result;
}

void ambientify::EngineChannel::setVolume(float volume, float pan) {
    if (!_isLoaded || _isLoading) {
        throw ambientify::commons::ASoundEngineException("Sound is not yet loaded");
    }
    _volume = volume;
    _result = _fchannel->setVolume(_volume);
    ERRCHECK(_result);
    if (pan >= -1 && pan <= 1) {
        _pan = pan;
        _result = _fchannel->setPan(_pan);
        ERRCHECK(_result);
    }
}

void ambientify::EngineChannel::_updateSerializedStatus() {
    _serializedStatus.currentFilePath = _currentFilePath;
    _serializedStatus.isLoaded = _isLoaded;
    _serializedStatus.isLoading = _isLoading;
    // isPlaying should be true if either parent of secondary channels are playing
    _serializedStatus.isPlaying = _crossfadeEnabled ? _isPlaying || (_secondaryChannel &&
                                                                     _secondaryChannel->_isPlaying)
                                                    :
                                  (_randomizationEnabled && !_didJustPause) || _isPlaying;
    _serializedStatus.didJustFinish = _didJustFinish;
    _serializedStatus.pan = _pan;
    _serializedStatus.pitch = _pitch;
    _serializedStatus.volume = _volume;
    _serializedStatus.isMuted = _isMuted;
    _serializedStatus.durationMs = _durationMs ? _durationMs : 0;
    _serializedStatus.cfPercentageStart = _cfPercentageStart;
    _serializedStatus.crossfadeEnabled = _crossfadeEnabled;
    _serializedStatus.noUnload = _noUnload;
    _serializedStatus.randomizationEnabled = _randomizationEnabled;
    _serializedStatus.rSettings = _rSettings;
}

void ambientify::EngineChannel::loadStatus(
        const std::shared_ptr<std::map<std::string, std::any>> &newStatus) {
    if (newStatus->contains("volume"))
        setVolume(std::any_cast<float>(newStatus->at("volume")), _pan);
    if (newStatus->contains("pan")) setVolume(_volume, std::any_cast<float>(newStatus->at("pan")));
    if (newStatus->contains("cfPercentageStart")) {
        const auto newPercentageStart = std::any_cast<float>(newStatus->at("cfPercentageStart"));
        if (newPercentageStart <= 0.5)
            throw ambientify::commons::ASoundEngineException(
                    "Crossfade percentage start must be greater than 0.5");
        _cfPercentageStart = newPercentageStart;
        const auto newCfDuration = _durationMs - (_durationMs * _cfPercentageStart);
        _cfDurationMs = newCfDuration;
        if (_secondaryChannel) {
            _secondaryChannel->_cfDurationMs = newCfDuration;
            _secondaryChannel->_cfPercentageStart = _cfPercentageStart;
            _updateSyncPoints();
            if (_secondaryChannel->isLoaded) {
                _secondaryChannel->_updateSyncPoints();
            }
        }
    }
    if (newStatus->contains("noUnload")) _noUnload = std::any_cast<bool>(newStatus->at("noUnload"));
}

void ambientify::EngineChannel::setCrossfadeEnabled(bool shouldCrossfade) {
    std::scoped_lock lock(_cfMutex);
    _isCrossfading = false;
    _randomizationEnabled = false;
    bool isSecondaryPlaying = (_secondaryChannel && _secondaryChannel->_isPlaying);
    bool wasPlaying = _isPlaying || isSecondaryPlaying;
    if (crossfadeEnabled && isSecondaryPlaying) {
        _secondaryChannel->unload();
        _secondaryChannel = nullptr;
    }
    _crossfadeEnabled = shouldCrossfade;
    _updateCfDuration();
    _updateSyncPoints();
    _setLooping(!shouldCrossfade);
    // reset volume in case was crossfading
    setVolume(_volume, _pan);
    if (shouldCrossfade) {
        loadSecondary();
    }
    if (wasPlaying) play();
    _updateSerializedStatus();
}

void ambientify::EngineChannel::_setLooping(bool shouldLoop) {
    FMOD_MODE loopMode = shouldLoop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;
    FMOD_MODE oldMode;
    _result = _fchannel->getMode(&oldMode);
    ERRCHECK(_result);
    _result = _fchannel->setMode(loopMode);
    ERRCHECK(_result);
    _result = _fchannel->setLoopCount(shouldLoop ? -1 : 0);
    ERRCHECK(_result);
    _result = _fchannel->setLoopPoints(0, FMOD_TIMEUNIT_MS, _durationMs - 20, FMOD_TIMEUNIT_MS);
    ERRCHECK(_result);
    // mitigate possible https://www.fmod.com/resources/documentation-api?version=2.02&page=glossary.html#streaming-issues
    _result = _fchannel->setPosition(0, FMOD_TIMEUNIT_MS);
    ERRCHECK(_result);
}

void ambientify::EngineChannel::_updateCfDuration() {
    const auto newCfDuration = _durationMs - (_durationMs * _cfPercentageStart);
    _cfDurationMs = newCfDuration;
}

void ambientify::EngineChannel::_updateSyncPoints() {
    int nSyncPoints;
    _sound->getNumSyncPoints(&nSyncPoints);
    if (nSyncPoints > 0) {
        _result = _sound->deleteSyncPoint(_cfSyncPoint);
        ERRCHECK(_result);
    }
    _result = _sound->addSyncPoint(_durationMs * _cfPercentageStart, FMOD_TIMEUNIT_MS, "cf_start",
                                   &_cfSyncPoint);
    ERRCHECK(_result);
}

void ambientify::EngineChannel::setRandomizationSettings(
        const std::shared_ptr<ChannelRandomizationDataSettings> &newSettings) {
    // copy the new values
    _rSettings = *newSettings;
    _generateRandomTimeframes();
    _updateSerializedStatus();
}

void ambientify::EngineChannel::runNextRandomFrame() {
    using namespace std::chrono;
    if (_rTimeframes.empty() || _didJustPause) return;
    bool _fcplaying;
    bool _fcpaused;
    _result = _fchannel->isPlaying(&_fcplaying);
    ERRCHECK(_result);
    _result = _fchannel->getPaused(&_fcpaused);
    ERRCHECK(_result);
    const bool isFchannelPlaying = !_fcpaused && _fcplaying;
    if (_rT_B - _rT_A > milliseconds(_rTimeframes[_currRandomTimeframe].delayMs)) {
        if (_currRandomTimeframe < _rTimeframes.size() - 1) {
            // Skip the frame if channel is still playing. This happens when the previous
            // sound was played with a lower pitch and the duration of the sound increased
            if (!isFchannelPlaying) {
                const auto[delay, volume, pan, pitch] = _rTimeframes[_currRandomTimeframe];
                _result = _fchannel->setVolume(volume);
                ERRCHECK(_result);
                _result = _fchannel->setPan(pan);
                ERRCHECK(_result);
                _result = _fchannel->setPitch(pitch);
                ERRCHECK(_result);
                play();
            }
            _currRandomTimeframe += 1;
        } else if (_randomizationEnabled) {
            _generateRandomTimeframes();
            _currRandomTimeframe = 0;
        }
        _rT_A = system_clock::now();
    }
    _rT_B = system_clock::now();
}

void ambientify::EngineChannel::_generateRandomTimeframes() {
    const auto[times, minutes, pitchRange, volumeRange, panRange] = _rSettings;
    const auto d = _durationMs / 1000; // duration in seconds
    const auto n = times; // times
    const auto t = minutes * 60; // max time in seconds
    const auto totalGapTime = t - d * n;
    if (totalGapTime <= 0) return;
    auto randomIntervals = std::vector<float>{};
    float randomSum = 0;
    auto normalizedIntervals = std::vector<int>{};

    for (int i = 0; i < n; i++) {
        const auto r = a_utils::generateRandomFloat(0, (float) totalGapTime);
        randomIntervals.push_back(r);
        randomSum += r;
    }

    for (const auto &rv: randomIntervals) {
        const auto s = ((rv / randomSum) * totalGapTime) * 1000;
        normalizedIntervals.push_back((int) s);
    }

    std::vector<GeneratedRandomizationData> result;
    std::transform(normalizedIntervals.begin(), normalizedIntervals.end(),
                   std::back_inserter(result),
                   [this](const auto &delayS) {
                       const auto[times, minutes, volumeRange, panRange, pitchRange] = _rSettings;
                       GeneratedRandomizationData result{};
                       result.delayMs = delayS;
                       if (volumeRange.has_value()) {
                           result.volume = a_utils::generateRandomFloat(volumeRange->first,
                                                                        volumeRange->second);
                       } else result.volume = _volume;
                       if (panRange.has_value()) {
                           result.pan = a_utils::generateRandomFloat(panRange->first,
                                                                     panRange->second);
                       } else result.pan = _pan;
                       if (pitchRange.has_value()) {
                           result.pitch = a_utils::generateRandomFloat(pitchRange->first,
                                                                       pitchRange->second);
                       } else result.pitch = _pitch;
                       return result;
                   });
    _rTimeframes = result;
}

void ambientify::EngineChannel::setRandomizationEnabled(bool shouldRandomize) {
    _rT_A = std::chrono::system_clock::now();
    _rT_B = std::chrono::system_clock::now();
    _randomizationEnabled = shouldRandomize;
    _currRandomTimeframe = 0;
    if (crossfadeEnabled) setCrossfadeEnabled(false);
    _setLooping(!shouldRandomize);
    if (!shouldRandomize) {
        _result = _fchannel->setVolume(_volume);
        ERRCHECK(_result);
        _result = _fchannel->setPan(_pan);
        ERRCHECK(_result);
        _result = _fchannel->setPitch(_pitch);
        ERRCHECK(_result);
        if (isPlaying) stop();
    }
    _updateSerializedStatus();
}

bool ambientify::EngineChannel::setMuted(bool muted) {
    if (!_isLoaded || _isLoading)
        throw ambientify::commons::ASoundEngineException("Sound is not yet loaded");
    _result = _fchannel->setMute(muted);
    ERRCHECK(_result);
    if (crossfadeEnabled && _secondaryChannel && _secondaryChannel->isLoaded) {
        _secondaryChannel->setMuted(muted);
    } else if (_secondaryChannel) {
        _secondaryChannel->_isMuted = muted;
    }
    _isMuted = muted;
    return muted;
}

void ambientify::EngineChannel::reset() {
    _noUnload = false;
    if (_isLoaded) unload();
    neverStarted = true;
    _isLoaded = false;
    _isLoading = false;
    _isPlaying = false;
    _crossfadeEnabled = false;
    _isMuted = false;
    _volume = 0.75;
    _pan = 0;
    _pitch = 1;
    _isCrossfading = false;
    _cfPercentageStart = 0.75;
    _didJustFinish = false;
    _didJustPause = false;
    _randomizationEnabled = false;
    _currRandomTimeframe = 0;
    _fchannel = nullptr;
    _sound = nullptr;
    _currentFilePath = "";
    _durationMs = 0;
    _rTimeframes = std::vector<GeneratedRandomizationData>{};
    _rSettings = {1, 1};
    _noUnload = true;
    _updateSerializedStatus();
}
