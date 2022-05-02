//
// Created by dominick on 3/4/22.
//

#include "SoundEngineHostObject.h"
#include "utils/Promise.h"
#include <android/log.h>

namespace ambientify {

    jsi::Value SoundEngineHostObject::get(jsi::Runtime &runtime, const jsi::PropNameID &name) {
        const auto propName = name.utf8(runtime);
        const auto funcName = "_AmbientifySoundEngine." + propName;

        if (soundEngine) {
            if (propName == "isReady") {
                return {soundEngine->isEngineReady};
            }
            if (propName == "setStatusAsync") {
                return a_utils::createFunc(
                        runtime,
                        funcName.c_str(),
                        2,
                        [this](jsi::Runtime &runtime,
                               const jsi::Value &,
                               const jsi::Value *arguments,
                               size_t count) -> jsi::Value {
                            return a_utils::createPromiseAsJSIValue(
                                    runtime, [&](jsi::Runtime &runtime, std::shared_ptr<a_utils::Promise> promise) {
                                        const auto channelId = static_cast<int>(arguments[0].getNumber());
                                        const auto jsiStatus = std::make_shared<jsi::Object>(arguments[1].getObject(runtime));

                                        auto fn = [&, promise = std::move(promise), channelId, jsiStatus]() {
                                            runtimeExecutor([&, promise, channelId, jsiStatus](jsi::Runtime &rt3) {
                                                try {
                                                    auto statusToSet = std::make_shared<std::map<std::string, std::any>>();
                                                    auto jsiStatusPropNames = jsiStatus->getPropertyNames(rt3);
                                                    for (int i = 0; i < jsiStatusPropNames.size(rt3); i++) {
                                                        const auto propNameStr = jsiStatusPropNames.getValueAtIndex(rt3, i).asString(rt3).utf8(rt3);
                                                        if (propNameStr == "volume") {
                                                            statusToSet->insert(
                                                                    {propNameStr, (float) jsiStatus->getProperty(rt3, propNameStr.c_str()).getNumber()});
                                                        } else if (propNameStr == "pan") {
                                                            statusToSet->insert(
                                                                    {propNameStr, (float) jsiStatus->getProperty(rt3, propNameStr.c_str()).getNumber()});
                                                        } else if (propNameStr == "pitch") {
                                                            statusToSet->insert(
                                                                    {propNameStr, (float) jsiStatus->getProperty(rt3, propNameStr.c_str()).getNumber()});
                                                        } else if (propNameStr == "cfPercentageStart") {
                                                            statusToSet->insert({propNameStr, (float) jsiStatus->getProperty(rt3,
                                                                                                                             propNameStr.c_str()).getNumber()});
                                                        } else if (propNameStr == "noUnload") {
                                                            statusToSet->insert({propNameStr, jsiStatus->getProperty(rt3, propNameStr.c_str()).getBool()});
                                                        } else if (propNameStr == "isMuted") {
                                                            statusToSet->insert({propNameStr, jsiStatus->getProperty(rt3, propNameStr.c_str()).getBool()});
                                                        }
                                                    }

                                                    soundEngine->loadChannelStatus(channelId, statusToSet);
                                                    promise->resolve(jsi::Value(true));
                                                } catch (const commons::ASoundEngineException &e) {
                                                    promise->reject(e.what());
                                                }
                                                return jsi::Value::undefined();

                                            });
                                        };
                                        tpool->enqueue(fn);
                                    });
                        });
            }

            if (propName == "createChannelAsync") {
                return a_utils::createFunc(
                        runtime,
                        funcName.c_str(),
                        2,
                        [this](jsi::Runtime &runtime,
                               const jsi::Value &,
                               const jsi::Value *arguments,
                               size_t count) -> jsi::Value {
                            return a_utils::createPromiseAsJSIValue(
                                    runtime, [&](jsi::Runtime &runtime, std::shared_ptr<a_utils::Promise> promise) {
                                        if (count < 1) jsi::detail::throwJSError(runtime, "No valid path specified.");
                                        std::string path;
                                        bool shouldPlay = false;
                                        if (arguments[0].isString()) {
                                            path = static_cast<std::string>(arguments[0].getString(runtime).utf8(runtime));
                                        }
                                        if (arguments[1].isBool()) {
                                            shouldPlay = static_cast<bool>(arguments[1].getBool());
                                        }

                                        auto fn = [&, promise = std::move(promise), path, shouldPlay]() {
                                            runtimeExecutor([&, promise, path, shouldPlay](jsi::Runtime &rt3) {
                                                try {
                                                    if (!path.empty()) {
                                                        int newChannelIdx = soundEngine->createChannel(&path);
                                                        if (shouldPlay) soundEngine->playChannel(newChannelIdx);
                                                        promise->resolve(jsi::Value(newChannelIdx));
                                                    } else {
                                                        promise->resolve(jsi::Value(soundEngine->createChannel()));
                                                    }
                                                } catch (const commons::ASoundEngineException &e) {
                                                    promise->reject(e.what());
                                                }
                                                return jsi::Value::undefined();

                                            });
                                        };
                                        tpool->enqueue(fn);
                                    });
                        });
            }

            if (propName == "loadChannelAsync") {
                return a_utils::createFunc(
                        runtime,
                        funcName.c_str(),
                        2,
                        [this](jsi::Runtime &runtime,
                               const jsi::Value &,
                               const jsi::Value *arguments,
                               size_t count) -> jsi::Value {
                            return a_utils::createPromiseAsJSIValue(
                                    runtime, [&](jsi::Runtime &runtime, std::shared_ptr<a_utils::Promise> promise) {
                                        const auto channelId = static_cast<int>(arguments[0].getNumber());
                                        const auto path = static_cast<std::string>(arguments[1].getString(runtime).utf8(runtime));

                                        auto fn = [&, promise = std::move(promise), channelId, path]() {
                                            runtimeExecutor([&, promise, channelId, path](jsi::Runtime &rt3) {
                                                try {
                                                    soundEngine->loadChannel(channelId, &path);
                                                    promise->resolve(jsi::Value::undefined());
                                                } catch (const commons::ASoundEngineException &e) {
                                                    promise->reject(e.what());
                                                }
                                                return jsi::Value::undefined();
                                            });
                                        };
                                        tpool->enqueue(fn);
                                    });
                        });
            }

            if (propName == "unloadChannel") {
                return a_utils::createFunc(
                        runtime,
                        funcName.c_str(),
                        1,
                        [this](jsi::Runtime &runtime,
                               const jsi::Value &,
                               const jsi::Value *arguments,
                               size_t count) -> jsi::Value {
                            try {
                                const auto channelId = static_cast<int>(arguments[0].getNumber());
                                soundEngine->unloadChannel(channelId);
                            } catch (const commons::ASoundEngineException &e) {
                                jsi::detail::throwJSError(runtime, e.what());
                            }
                            return jsi::Value::undefined();
                        });
            }

            if (propName == "playChannel") {
                return a_utils::createFunc(
                        runtime,
                        funcName.c_str(),
                        1,
                        [this](jsi::Runtime &runtime,
                               const jsi::Value &,
                               const jsi::Value *arguments,
                               size_t count) -> jsi::Value {
                            try {
                                const auto channelId = static_cast<int>(arguments[0].getNumber());
                                soundEngine->playChannel(channelId);
                            } catch (const commons::ASoundEngineException &e) {
                                jsi::detail::throwJSError(runtime, e.what());
                            }
                            return jsi::Value::undefined();
                        });
            }

            if (propName == "stopChannel") {
                return a_utils::createFunc(
                        runtime,
                        funcName.c_str(),
                        1,
                        [this](jsi::Runtime &runtime,
                               const jsi::Value &,
                               const jsi::Value *arguments,
                               size_t count) -> jsi::Value {
                            try {
                                const auto channelId = static_cast<int>(arguments[0].getNumber());
                                soundEngine->stopChannel(channelId);
                            } catch (const commons::ASoundEngineException &e) {
                                jsi::detail::throwJSError(runtime, e.what());
                            }
                            return jsi::Value::undefined();
                        });
            }

            if (propName == "setCrossfadingAsync") {
                return a_utils::createFunc(
                        runtime,
                        funcName.c_str(),
                        2,
                        [this](jsi::Runtime &runtime,
                               const jsi::Value &,
                               const jsi::Value *arguments,
                               size_t count) -> jsi::Value {
                            return a_utils::createPromiseAsJSIValue(
                                    runtime, [&](jsi::Runtime &runtime, std::shared_ptr<a_utils::Promise> promise) {
                                        const auto channelId = static_cast<int>(arguments[0].getNumber());
                                        const auto enabled = static_cast<int>(arguments[1].getBool());

                                        auto fn = [&, promise = std::move(promise), channelId, enabled]() {
                                            runtimeExecutor([&, promise, channelId, enabled](jsi::Runtime &rt3) {
                                                try {
                                                    soundEngine->setChannelCfEnabled(channelId, enabled);
                                                    promise->resolve(jsi::Value(true));
                                                } catch (const commons::ASoundEngineException &e) {
                                                    promise->reject(e.what());
                                                }
                                                return jsi::Value::undefined();
                                            });
                                        };
                                        tpool->enqueue(fn);
                                    });
                        });
            }

            if (propName == "setChannelVolume") {
                return a_utils::createFunc(
                        runtime,
                        funcName.c_str(),
                        1,
                        [this](jsi::Runtime &runtime,
                               const jsi::Value &,
                               const jsi::Value *arguments,
                               size_t count) -> jsi::Value {
                            try {
                                const auto channelId = static_cast<int>(arguments[0].getNumber());
                                const auto volume = static_cast<float>(arguments[1].getNumber());
                                const auto pan = static_cast<float>(arguments[2].getNumber());
                                SoundEngine::setChannelVolume(channelId, volume, pan);
                            } catch (const commons::ASoundEngineException &e) {
                                jsi::detail::throwJSError(runtime, e.what());
                            }
                            return true;
                        });
            }
            if (propName == "toggleChannelPlayback") {
                return a_utils::createFunc(
                        runtime,
                        funcName.c_str(),
                        1,
                        [this](jsi::Runtime &runtime,
                               const jsi::Value &,
                               const jsi::Value *arguments,
                               size_t count) -> jsi::Value {
                            try {
                                const auto channelId = static_cast<int>(arguments[0].getNumber());
                                const bool isNowPlaying = SoundEngine::toggleChannelPlayback(channelId);
                                return jsi::Value(isNowPlaying);
                            } catch (const commons::ASoundEngineException &e) {
                                jsi::detail::throwJSError(runtime, e.what());
                            }
                            return true;
                        });
            }
            if (propName == "getSerializedStateAsync") {
                return a_utils::createFunc(
                        runtime,
                        funcName.c_str(),
                        0,
                        [this](jsi::Runtime &runtime,
                               const jsi::Value &,
                               const jsi::Value *arguments,
                               size_t count) -> jsi::Value {
                            return a_utils::createPromiseAsJSIValue(
                                    runtime, [&](jsi::Runtime &runtime, std::shared_ptr<a_utils::Promise> promise) {
                                        auto fn = [&, promise = std::move(promise)]() {
                                            runtimeExecutor([&, promise](jsi::Runtime &rt3) {
                                                if (soundEngine->isEngineReadyFn()) {
                                                    if (!SoundEngine::channels.empty()) {
                                                        auto array = jsi::Array(rt3, SoundEngine::channels.size());
                                                        try {
                                                            // WARNING: this code makes me vomit my goddamn soul away but i'm too lazy to implement a converting solution
                                                            // right now since it will be available in the next react native versions anyway (ReactCommons bridging),
                                                            // so... ¯\_(ツ)_/¯
                                                            for (size_t i = 0; i < SoundEngine::channels.size(); ++i) {
                                                                auto res = jsi::Object(rt3);
                                                                auto jsiRSettings = jsi::Object(rt3);
                                                                auto channel = SoundEngine::channels.at(i);
                                                                auto status = channel->getSerializedStatus();

                                                                res.setProperty(rt3, "id", jsi::Value(status->id));
                                                                res.setProperty(rt3, "volume", jsi::Value((double) status->volume));
                                                                res.setProperty(rt3, "durationMs", jsi::Value((int) status->durationMs));
                                                                res.setProperty(rt3, "isLoaded", jsi::Value(status->isLoaded));
                                                                res.setProperty(rt3, "isLoading", jsi::Value(status->isLoading));
                                                                res.setProperty(rt3, "isPlaying", jsi::Value(status->isPlaying));
                                                                res.setProperty(rt3, "isMuted", jsi::Value(status->isMuted));
                                                                res.setProperty(rt3, "crossfadeEnabled", jsi::Value(status->crossfadeEnabled));
                                                                res.setProperty(rt3, "didJustFinish", jsi::Value(status->didJustFinish));
                                                                res.setProperty(rt3, "cfPercentageStart", jsi::Value((double) status->cfPercentageStart));
                                                                res.setProperty(rt3, "volume", jsi::Value((double) status->volume));
                                                                res.setProperty(rt3, "pan", jsi::Value((double) status->pan));
                                                                res.setProperty(rt3, "pitch", jsi::Value((double) status->pitch));
                                                                res.setProperty(rt3, "currentFilePath",
                                                                                jsi::Value(rt3, jsi::String::createFromUtf8(rt3, status->currentFilePath)));

                                                                res.setProperty(rt3, "randomizationEnabled", jsi::Value(status->randomizationEnabled));
                                                                jsiRSettings.setProperty(rt3, "times", jsi::Value(status->rSettings.times));
                                                                jsiRSettings.setProperty(rt3, "minutes", jsi::Value(status->rSettings.minutes));
                                                                if (status->rSettings.volumeRange.has_value()) {
                                                                    float v[2] = {status->rSettings.volumeRange->first, status->rSettings.volumeRange->second};
                                                                    auto volR = jsi::Array::createWithElements(rt3, v[0], v[1]);
                                                                    jsiRSettings.setProperty(rt3, "volumeRange", volR);
                                                                }
                                                                if (status->rSettings.panRange.has_value()) {
                                                                    float v[2] = {status->rSettings.panRange->first, status->rSettings.panRange->second};
                                                                    auto panR = jsi::Array::createWithElements(rt3, v[0], v[1]);
                                                                    jsiRSettings.setProperty(rt3, "panRange", panR);
                                                                }
                                                                if (status->rSettings.pitchRange.has_value()) {
                                                                    float v[2] = {status->rSettings.pitchRange->first, status->rSettings.pitchRange->second};
                                                                    auto pitR = jsi::Array::createWithElements(rt3, v[0], v[1]);
                                                                    jsiRSettings.setProperty(rt3, "pitchRange", pitR);
                                                                }
                                                                res.setProperty(rt3, "rSettings", jsiRSettings);

                                                                array.setValueAtIndex(rt3, i, res);
                                                            }
                                                        } catch (const commons::ASoundEngineException &e) {
                                                            promise->reject(e.what());
                                                        }
                                                        promise->resolve(std::move(array));
                                                    }
                                                }
                                            });
                                        };
                                        tpool->enqueue(fn);
                                    });
                        });
            }

            if (propName == "setRandomizationEnabled") {
                return a_utils::createFunc(
                        runtime,
                        funcName.c_str(),
                        2,
                        [this](jsi::Runtime &runtime,
                               const jsi::Value &,
                               const jsi::Value *arguments,
                               size_t count) -> jsi::Value {
                            try {
                                const auto channelId = static_cast<int>(arguments[0].getNumber());
                                const auto shouldRandomize = static_cast<bool>(arguments[1].getBool());
                                soundEngine->setChannelRandomizationEnabled(channelId, shouldRandomize);
                            } catch (const commons::ASoundEngineException &e) {
                                jsi::detail::throwJSError(runtime, e.what());
                            }
                            return jsi::Value::undefined();
                        });
            }

            if (propName == "setRandomizationSettings") {
                return a_utils::createFunc(
                        runtime,
                        funcName.c_str(),
                        2,
                        [this](jsi::Runtime &runtime,
                               const jsi::Value &,
                               const jsi::Value *arguments,
                               size_t count) -> jsi::Value {
                            try {
                                const auto channelId = static_cast<int>(arguments[0].getNumber());
                                const auto jsiStatus = std::make_shared<jsi::Object>(arguments[1].getObject(runtime));

                                auto statusToSet = std::make_shared<ChannelRandomizationDataSettings>();
                                auto jsiStatusPropNames = jsiStatus->getPropertyNames(runtime);
                                for (int i = 0; i < jsiStatusPropNames.size(runtime); i++) {
                                    // @todo: safety checks
                                    const auto propNameStr = jsiStatusPropNames.getValueAtIndex(runtime, i).asString(runtime).utf8(runtime);
                                    if (jsiStatus->getProperty(runtime, propNameStr.c_str()).isNull()) continue;
                                    if (propNameStr == "times") {
                                        statusToSet->times = (int) jsiStatus->getProperty(runtime, propNameStr.c_str()).getNumber();
                                    } else if (propNameStr == "minutes") {
                                        statusToSet->minutes = (int) jsiStatus->getProperty(runtime, propNameStr.c_str()).getNumber();
                                    } else if (propNameStr == "panRange") {
                                        const auto panRange = jsiStatus->getProperty(runtime, propNameStr.c_str()).getObject(runtime).asArray(runtime);
                                        const auto v1 = static_cast<float>(panRange.getValueAtIndex(runtime, 0).getNumber());
                                        const auto v2 = static_cast<float>(panRange.getValueAtIndex(runtime, 1).getNumber());
                                        statusToSet->panRange = std::make_pair(v1, v2);
                                    } else if (propNameStr == "volumeRange") {
                                        const auto volumeRange = jsiStatus->getProperty(runtime, propNameStr.c_str()).getObject(runtime).asArray(runtime);
                                        const auto v1 = static_cast<float>(volumeRange.getValueAtIndex(runtime, 0).getNumber());
                                        const auto v2 = static_cast<float>(volumeRange.getValueAtIndex(runtime, 1).getNumber());
                                        statusToSet->volumeRange = std::make_pair(v1, v2);
                                    } else if (propNameStr == "pitchRange") {
                                        const auto pitchRange = jsiStatus->getProperty(runtime, propNameStr.c_str()).getObject(runtime).asArray(runtime);
                                        const auto v1 = static_cast<float>(pitchRange.getValueAtIndex(runtime, 0).getNumber());
                                        const auto v2 = static_cast<float>(pitchRange.getValueAtIndex(runtime, 1).getNumber());
                                        statusToSet->pitchRange = std::make_pair(v1, v2);
                                    }
                                }

                                soundEngine->setChannelRandomizationSettings(channelId, statusToSet);
                            } catch (const commons::ASoundEngineException &e) {
                                jsi::detail::throwJSError(runtime, e.what());
                            }
                            return jsi::Value::undefined();
                        });
            }

            if (propName == "toggleChannelMuted") {
                return a_utils::createFunc(
                        runtime,
                        funcName.c_str(),
                        1,
                        [this](jsi::Runtime &runtime,
                               const jsi::Value &,
                               const jsi::Value *arguments,
                               size_t count) -> jsi::Value {
                            try {
                                const auto channelId = static_cast<int>(arguments[0].getNumber());
                                const bool isNowMuted = soundEngine->toggleChannelMuted(channelId);
                                return {runtime, isNowMuted};
                            } catch (const commons::ASoundEngineException &e) {
                                jsi::detail::throwJSError(runtime, e.what());
                            }
                            return jsi::Value::undefined();
                        });
            }

            if (propName == "toggleChannelMuted") {
                return a_utils::createFunc(
                        runtime,
                        funcName.c_str(),
                        1,
                        [this](jsi::Runtime &runtime,
                               const jsi::Value &,
                               const jsi::Value *arguments,
                               size_t count) -> jsi::Value {
                            try {
                                const auto channelId = static_cast<int>(arguments[0].getNumber());
                                soundEngine->toggleChannelMuted(channelId);
                            } catch (const commons::ASoundEngineException &e) {
                                jsi::detail::throwJSError(runtime, e.what());
                            }
                            return jsi::Value::undefined();
                        });
            }

            if (propName == "resetChannel") {
                return a_utils::createFunc(
                        runtime,
                        funcName.c_str(),
                        1,
                        [this](jsi::Runtime &runtime,
                               const jsi::Value &,
                               const jsi::Value *arguments,
                               size_t count) -> jsi::Value {
                            try {
                                const auto channelId = static_cast<int>(arguments[0].getNumber());
                                soundEngine->resetChannel(channelId);
                            } catch (const commons::ASoundEngineException &e) {
                                jsi::detail::throwJSError(runtime, e.what());
                            }
                            return jsi::Value::undefined();
                        });
            }
        }

        return

                jsi::Value::undefined();
    }

    void SoundEngineHostObject::set(jsi::Runtime &, const jsi::PropNameID &name, const jsi::Value &value) {
        throw std::runtime_error("SoundEngineHostObject::set not implemented nor necessary");
    }

    std::vector<jsi::PropNameID> SoundEngineHostObject::getPropertyNames(jsi::Runtime &rt) {
        std::vector<jsi::PropNameID> result;
        result.push_back(jsi::PropNameID::forUtf8(rt, std::string("set")));
        result.push_back(jsi::PropNameID::forUtf8(rt, std::string("createChannelAsync")));
        return HostObject::getPropertyNames(rt);
    }

}
