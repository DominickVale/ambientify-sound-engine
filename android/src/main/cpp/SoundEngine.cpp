// Created by dominick on 3/4/22.
//

#include "SoundEngine.h"

FMOD_RESULT F_CALLBACK ambientify::SoundEngine::channelCallback(FMOD_CHANNELCONTROL *channelControl, FMOD_CHANNELCONTROL_TYPE controlType,
                                                                FMOD_CHANNELCONTROL_CALLBACK_TYPE callbackType, void *commanData1, void *commanData2) {
    std::shared_ptr<ambientify::SoundEngine> soundEngine = ambientify::SoundEngine::GetInstance( nullptr, nullptr);

    auto _ch = reinterpret_cast<FMOD::Channel *>(channelControl);
    void *userData;
    _ch->getUserData(&userData);

    auto channelId = *static_cast<int *>(userData);
    // if channelId is > MAX_CHANNELS then it's a secondary channel of that channel
    // e.g if channelId = 16 then the parent ch is 0
    bool isSecondaryChannel = channelId > constants::MAX_CHANNELS;
    auto ch = channels[isSecondaryChannel ? channelId - constants::MAX_CHANNELS - 1 : channelId];

    if (controlType == FMOD_CHANNELCONTROL_CHANNEL && callbackType == FMOD_CHANNELCONTROL_CALLBACK_END) {
        LOG_DEBUG("_ch ended with id: %d", (int) channelId);
        if (isSecondaryChannel) {
            ch->getSecondaryInstance()->onChannelEndCallback();
        } else {
            ch->onChannelEndCallback();
        }
    } else if (callbackType == FMOD_CHANNELCONTROL_CALLBACK_SYNCPOINT && ch->crossfadeEnabled) {
        if (isSecondaryChannel) {
            auto durationMs = ch->getDurationMs();
            auto positionMs = ch->getSecondaryInstance()->getPositionMs();
            auto percent = (float) positionMs / (float) durationMs;
            if (percent >= ch->cfPercentageStart && !ch->isCrossfading) {
                ch->crossfade(true);
            }
        } else {
            auto durationMs = ch->getDurationMs();
            auto positionMs = ch->getPositionMs();
            auto percent = (float) positionMs / (float) durationMs;
            if (percent >= ch->cfPercentageStart && !ch->isCrossfading) {
                ch->crossfade(false);
            }
        }
    }
    return FMOD_OK;
}

std::shared_ptr<ambientify::SoundEngine> ambientify::SoundEngine::_instance;
std::vector<std::shared_ptr<ambientify::EngineChannel>> ambientify::SoundEngine::channels;

std::shared_ptr<ambientify::EngineChannel> getChannelById(int channelId) {
    using namespace ambientify;
    if (channelId < 0 || channelId >= SoundEngine::channels.size()) {
        throw commons::ASoundEngineException(
                fmt::format("Channel with id {} does not exist.", channelId));
    } else {
        return SoundEngine::channels[channelId];
    }
}

void ambientify::SoundEngine::update() {
    while (!_stopped) {
        if (env) {
            if (!isEngineRunning) {
                const auto error = jvm->AttachCurrentThread( &env, nullptr);
                if (error) {
                    LOG_ERR("Failed to attach thread to JVM. Error: %d", error);
                    throw commons::ASoundEngineException(fmt::format("Failed to attach thread to JVM. Error: {}", error));
                    return;
                }
                isEngineRunning = true;
            }
            result = system->update();
            ERRCHECK(result);
            for (auto &channel: channels) {
                if (!channel) break;
                result = channel->update();
                if (result == FMOD_ERR_INVALID_HANDLE) { break; }
                else
                    ERRCHECK(result);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

// @todo: refactor, dry up
int ambientify::SoundEngine::createChannel() {
    const auto latestIdx = channels.size();
    if (latestIdx >= constants::MAX_CHANNELS) {
        throw commons::ASoundEngineException("Too many channels");
    }

    auto channel = std::make_shared<EngineChannel>(system, &channelCallback, latestIdx);
    channels.push_back(channel);
    // There is no way there could be more than int channels... Right?
    return (int) channels.size() - 1;
}

int ambientify::SoundEngine::createChannel(const std::string *path) {
    const auto latestIdx = channels.size();
    if (latestIdx >= constants::MAX_CHANNELS) {
        throw commons::ASoundEngineException("Too many channels");
    }

    auto channel = std::make_shared<EngineChannel>(system, &channelCallback, latestIdx, path, false);
    channels.push_back(channel);
    // There is no way there could be more than int channels... Right?
    return (int) channels.size() - 1;
}

bool ambientify::SoundEngine::toggleChannelPlayback(int channelId) {
    const auto ch = getChannelById(channelId);
    if (ch->isPlaying || (ch->crossfadeEnabled && ch->getSecondaryInstance()->isPlaying) || (ch->randomizationEnabled && !ch->didJustPause)) {
        ch->stop();
    } else {
        playChannel(channelId);
    }
    return ch->isPlaying;
}

void ambientify::SoundEngine::setChannelVolume(int channelId, float volume, float pan) {
    const auto ch = getChannelById(channelId);
    ch->setVolume(volume, pan);
}

void ambientify::SoundEngine::setMasterVolume(float volume) {
    FMOD::ChannelGroup *masterGroup;
    result = system->getMasterChannelGroup(&masterGroup);
    ERRCHECK(result);
    result = masterGroup->setVolume(volume);
    ERRCHECK(result);
}

void ambientify::SoundEngine::loadChannel(int channelId, const std::string *path) {
    const auto ch = getChannelById(channelId);
    if (ch->isLoaded) {
        ch->unload();
        ch->load(path);
    } else if (ch->isLoading) {
        throw commons::ASoundEngineException("Channel is already loading");
    } else {
        ch->load(path);
    }
}

bool ambientify::SoundEngine::playChannel(int channelId) {
    const auto ch = getChannelById(channelId);
    if (ch->isPlaying) return true;
    if (ch->isLoaded) {
        ch->play();
    } else if (!ch->currFilePath.empty()) {
        ch->load(&ch->currFilePath);
        ch->play();
    } else {
        throw commons::ASoundEngineException("Channel is not loaded");
    }
    return ch->isPlaying;
}

bool ambientify::SoundEngine::stopChannel(int channelId) {
    const auto ch = getChannelById(channelId);
    if (!ch->isPlaying) return false;
    if (ch->isLoaded) {
        ch->stop();
    } else {
        throw commons::ASoundEngineException("Channel is not loaded");
    }
    return ch->isPlaying;
}

void ambientify::SoundEngine::unloadChannel(int channelId) {
    const auto ch = getChannelById(channelId);
    ch->unload();
}

void ambientify::SoundEngine::loadChannelStatus(int channelId, const std::shared_ptr<std::map<std::string, std::any>> &newStatus) {
    const auto ch = getChannelById(channelId);
    ch->loadStatus(newStatus);
}

bool ambientify::SoundEngine::setChannelCfEnabled(int channelId, bool enabled) {
    const auto ch = getChannelById(channelId);
    ch->setCrossfadeEnabled(enabled);
    return ch->crossfadeEnabled;
}

bool ambientify::SoundEngine::setChannelRandomizationEnabled(int channelId, bool enabled) {
    const auto ch = getChannelById(channelId);
    ch->setRandomizationEnabled(enabled);
    return ch->randomizationEnabled;
}

void ambientify::SoundEngine::setChannelRandomizationSettings(int channelId, const std::shared_ptr<ChannelRandomizationDataSettings> &newSettings) {
    const auto ch = getChannelById(channelId);
    ch->setRandomizationSettings(newSettings);
}

void ambientify::SoundEngine::setChannelMuted(int channelId, bool muted) {
    const auto ch = getChannelById(channelId);
    ch->setMuted(muted);
}

bool ambientify::SoundEngine::toggleChannelMuted(int channelId) {
    const auto ch = getChannelById(channelId);
    return ch->setMuted(!ch->isMuted);
}

void ambientify::SoundEngine::resetChannel(int channelId) {
    const auto ch = getChannelById(channelId);
    ch->reset();
}

void ambientify::SoundEngine::playAll() {
    for (const auto &ch : channels) {
        if (ch->isLoaded) {
            ch->play();
        }
    }
}

void ambientify::SoundEngine::stopAll() {
    for (const auto &ch : channels) {
        if (ch->isLoaded) {
            ch->stop();
        }
    }
}

double ambientify::SoundEngine::getMasterVolume() {
    float vol;
    FMOD::ChannelGroup *masterGroup;
    result = system->getMasterChannelGroup(&masterGroup);
    ERRCHECK(result);
    result = masterGroup->getVolume(&vol);
    ERRCHECK(result);
    return (double) vol;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_ambientifysoundengine_EngineService_toggleMaster(JNIEnv *env, jobject thiz) {
    if (ambientify::SoundEngine::isEngineRunning) {
        //jvm param not needed
        std::shared_ptr<ambientify::SoundEngine> engine = ambientify::SoundEngine::GetInstance(env, nullptr);
        bool noneLoaded = true;
        engine->jvm->AttachCurrentThread(&env, nullptr);
        const auto isPlaying = std::any_of(
                engine->channels.begin(),
                engine->channels.end(),
                [&noneLoaded](const std::shared_ptr<ambientify::EngineChannel> &ch) {
                    if (ch->isLoaded) noneLoaded = false;
                    return ch->isPlaying;
                });
        if (noneLoaded) return false;
        if (isPlaying) {
            engine->stopAll();
            return false;
        } else {
            engine->playAll();
            return true;
        }
    }
    return false;
}

ambientify::SoundEngine::SoundEngine(JNIEnv *jniEnv, JavaVM *javaVm): env(jniEnv), jvm(javaVm) { }

bool ambientify::SoundEngine::init(JNIEnv *jniEnv){
    this->env = jniEnv;
//    result = FMOD::Debug_Initialize(
//            FMOD_DEBUG_LEVEL_LOG |
//            FMOD_DEBUG_TYPE_MEMORY |
//            FMOD_DEBUG_TYPE_FILE |
//            FMOD_DEBUG_TYPE_CODEC |
//            FMOD_DEBUG_TYPE_TRACE |
//            FMOD_DEBUG_DISPLAY_TIMESTAMPS |
//            FMOD_DEBUG_DISPLAY_LINENUMBERS |
//            FMOD_DEBUG_DISPLAY_THREAD  ,FMOD_DEBUG_MODE_TTY,  0, 0);
    try {
        jvm->AttachCurrentThread(&jniEnv, nullptr);
        result = FMOD::System_Create(&system);
        ERRCHECK(result);
        result = system->setSoftwareFormat(24000, FMOD_SPEAKERMODE_STEREO, 2);
        ERRCHECK(result);

        result = system->init(constants::MAX_CHANNELS * 2, FMOD_INIT_NORMAL, nullptr);
        ERRCHECK(result);
        FMOD::ChannelGroup *masterGroup;
        result = system->getMasterChannelGroup(&masterGroup);
        ERRCHECK(result);
        result = masterGroup->setVolume(1.0f);
        ERRCHECK(result);
        std::thread updateThread(&SoundEngine::update, this);
        updateThread.detach();
        LOG_DEBUG("SoundEngine initialized");
        return true;
    } catch (const std::exception &e) {
        LOG_ERR("FMOD error: %s", e.what());
        return false;
    }
}

ambientify::SoundEngine::~SoundEngine() {
    _stopped = true;
    system->release();
}


extern "C"
JNIEXPORT jboolean JNICALL
Java_com_ambientifysoundengine_EngineService_startEngine(JNIEnv *env, jobject thiz) {
    if(ambientify::SoundEngine::isEngineRunning) return true;
    std::shared_ptr<ambientify::SoundEngine> engine = ambientify::SoundEngine::GetInstance(env, nullptr);
    const auto result = engine->init(env);
    if(result) {
        LOG_DEBUG("Notifying java");
        jclass clazz = env->FindClass("com/ambientifysoundengine/EngineService");
        jmethodID methodId = env->GetStaticMethodID(clazz, "bindNotifyJS", "()V");
        if (methodId == nullptr) {
            LOG_ERR("notifyJS native lambda -> methodId is null");
        } else {
            env->CallStaticVoidMethod(clazz, methodId);
        }
    }
    return result;
}
