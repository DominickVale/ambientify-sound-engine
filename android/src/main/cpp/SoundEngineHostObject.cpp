//
// Created by dominick on 3/4/22.
//

#include "SoundEngineHostObject.h"
#include "utils/Promise.h"
#include <android/log.h>

namespace ambientify {
    jsi::Object channelStatusToJSI(jsi::Runtime &rt, const std::shared_ptr<EngineChannel> &channel) {
        auto res = jsi::Object(rt);
        auto jsiRSettings = jsi::Object(rt);
        auto status = channel->getSerializedStatus();

        res.setProperty(rt, "id", jsi::Value(status->id));
        res.setProperty(rt, "volume", jsi::Value((double) status->volume));
        res.setProperty(rt, "durationMs", jsi::Value((int) status->durationMs));
        res.setProperty(rt, "isLoaded", jsi::Value(status->isLoaded));
        res.setProperty(rt, "isLoading", jsi::Value(status->isLoading));
        res.setProperty(rt, "isPlaying", jsi::Value(status->isPlaying));
        res.setProperty(rt, "isMuted", jsi::Value(status->isMuted));
        res.setProperty(rt, "crossfadeEnabled", jsi::Value(status->crossfadeEnabled));
        res.setProperty(rt, "didJustFinish", jsi::Value(status->didJustFinish));
        res.setProperty(rt, "cfPercentageStart", jsi::Value((double) status->cfPercentageStart));
        res.setProperty(rt, "volume", jsi::Value((double) status->volume));
        res.setProperty(rt, "pan", jsi::Value((double) status->pan));
        res.setProperty(rt, "pitch", jsi::Value((double) status->pitch));
        res.setProperty(rt, "currentFilePath",
                        jsi::Value(rt, jsi::String::createFromUtf8(rt, status->currentFilePath)));

        res.setProperty(rt, "randomizationEnabled", jsi::Value(status->randomizationEnabled));
        jsiRSettings.setProperty(rt, "times", jsi::Value(status->rSettings.times));
        jsiRSettings.setProperty(rt, "minutes", jsi::Value(status->rSettings.minutes));
        if (status->rSettings.volumeRange.has_value()) {
            float v[2] = {status->rSettings.volumeRange->first, status->rSettings.volumeRange->second};
            auto volR = jsi::Array::createWithElements(rt, v[0], v[1]);
            jsiRSettings.setProperty(rt, "volumeRange", volR);
        }
        if (status->rSettings.panRange.has_value()) {
            float v[2] = {status->rSettings.panRange->first, status->rSettings.panRange->second};
            auto panR = jsi::Array::createWithElements(rt, v[0], v[1]);
            jsiRSettings.setProperty(rt, "panRange", panR);
        }
        if (status->rSettings.pitchRange.has_value()) {
            float v[2] = {status->rSettings.pitchRange->first, status->rSettings.pitchRange->second};
            auto pitR = jsi::Array::createWithElements(rt, v[0], v[1]);
            jsiRSettings.setProperty(rt, "pitchRange", pitR);
        }
        res.setProperty(rt, "rSettings", jsiRSettings);
        return res;
    }

    jsi::Value SoundEngineHostObject::get(jsi::Runtime &runtime, const jsi::PropNameID &name) {
        const auto propName = name.utf8(runtime);
        const auto funcName = "_AmbientifySoundEngine." + propName;

        if (soundEngine && soundEngine->isRunning) {
            if (propName == "isRunning") {
                return {soundEngine->isRunning};
            }

            if (propName == "shutDown") {
                soundEngine->shutDown();
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
                                                    promise->resolve(jsi::Value(channelStatusToJSI(rt3, SoundEngine::channels.at(channelId))));
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

                                                    auto res = channelStatusToJSI(rt3, SoundEngine::channels.at(channelId));
                                                    promise->resolve(jsi::Value(rt3, res));
                                                } catch (const commons::ASoundEngineException &e) {
                                                    promise->reject(e.what());
                                                }
                                            });
                                        };
                                        tpool->enqueue(fn);
                                    });
                        });
            }

            if (propName == "unloadChannelAsync") {
                return a_utils::createFunc(
                        runtime,
                        funcName.c_str(),
                        1,
                        [this](jsi::Runtime &runtime,
                               const jsi::Value &,
                               const jsi::Value *arguments,
                               size_t count) -> jsi::Value {
                            return a_utils::createPromiseAsJSIValue(
                                    runtime, [&](jsi::Runtime &runtime, std::shared_ptr<a_utils::Promise> promise) {
                                        const auto channelId = static_cast<int>(arguments[0].getNumber());

                                        auto fn = [&, promise = std::move(promise), channelId]() {
                                            runtimeExecutor([&, promise, channelId](jsi::Runtime &rt3) {
                                                try {
                                                    soundEngine->unloadChannel(channelId);
                                                    auto res = channelStatusToJSI(rt3, SoundEngine::channels.at(channelId));
                                                    promise->resolve(jsi::Value(rt3, res));
                                                } catch (const commons::ASoundEngineException &e) {
                                                    promise->reject(e.what());
                                                }
                                            });
                                        };
                                        tpool->enqueue(fn);
                                    });
                        });
            }

            if (propName == "playChannelAsync") {
                return a_utils::createFunc(
                        runtime,
                        funcName.c_str(),
                        1,
                        [this](jsi::Runtime &runtime,
                               const jsi::Value &,
                               const jsi::Value *arguments,
                               size_t count) -> jsi::Value {
                            return a_utils::createPromiseAsJSIValue(
                                    runtime, [&](jsi::Runtime &runtime, std::shared_ptr<a_utils::Promise> promise) {
                                        const auto channelId = static_cast<int>(arguments[0].getNumber());
                                        auto fn = [&, promise = std::move(promise), channelId]() {
                                            runtimeExecutor([&, promise, channelId](jsi::Runtime &rt3) {
                                                try {
                                                    promise->resolve(jsi::Value(soundEngine->playChannel(channelId)));
                                                } catch (const commons::ASoundEngineException &e) {
                                                    promise->reject(e.what());
                                                }
                                            });
                                        };
                                        tpool->enqueue(fn);
                                    });
                        });
            }

            if (propName == "playAllAsync") {
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
                                        auto fn = [&, promise = std::move(promise) ]() {
                                            runtimeExecutor([&, promise ](jsi::Runtime &rt3) {
                                                try {
                                                    soundEngine->playAll();
                                                    promise->resolve(jsi::Value::undefined());
                                                } catch (const commons::ASoundEngineException &e) {
                                                    promise->reject(e.what());
                                                }
                                            });
                                        };
                                        tpool->enqueue(fn);
                                    });
                        });
            }

            if (propName == "stopAllAsync") {
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
                                                try {
                                                    soundEngine->stopAll();
                                                    promise->resolve(jsi::Value::undefined());
                                                } catch (const commons::ASoundEngineException &e) {
                                                    promise->reject(e.what());
                                                }
                                            });
                                        };
                                        tpool->enqueue(fn);
                                    });
                        });
            }

            if (propName == "stopChannelAsync") {
                return a_utils::createFunc(
                        runtime,
                        funcName.c_str(),
                        1,
                        [this](jsi::Runtime &runtime,
                               const jsi::Value &,
                               const jsi::Value *arguments,
                               size_t count) -> jsi::Value {
                            return a_utils::createPromiseAsJSIValue(
                                    runtime, [&](jsi::Runtime &runtime, std::shared_ptr<a_utils::Promise> promise) {
                                        const auto channelId = static_cast<int>(arguments[0].getNumber());
                                        auto fn = [&, promise = std::move(promise), channelId]() {
                                            runtimeExecutor([&, promise, channelId](jsi::Runtime &rt3) {
                                                try {
                                                    promise->resolve(jsi::Value(soundEngine->stopChannel(channelId)));
                                                } catch (const commons::ASoundEngineException &e) {
                                                    promise->reject(e.what());
                                                }
                                            });
                                        };
                                        tpool->enqueue(fn);
                                    });
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
                                                    promise->resolve(jsi::Value(soundEngine->setChannelCfEnabled(channelId, enabled)));
                                                } catch (const commons::ASoundEngineException &e) {
                                                    promise->reject(e.what());
                                                }
                                            });
                                        };
                                        tpool->enqueue(fn);
                                    });
                        });
            }

            if (propName == "setChannelVolumeAsync") {
                return a_utils::createFunc(
                        runtime,
                        funcName.c_str(),
                        3,
                        [this](jsi::Runtime &runtime,
                               const jsi::Value &,
                               const jsi::Value *arguments,
                               size_t count) -> jsi::Value {
                            return a_utils::createPromiseAsJSIValue(
                                    runtime, [&](jsi::Runtime &runtime, std::shared_ptr<a_utils::Promise> promise) {
                                        const auto channelId = static_cast<int>(arguments[0].getNumber());
                                        const auto volume = static_cast<float>(arguments[1].getNumber());
                                        const auto pan = static_cast<float>(arguments[2].getNumber());

                                        auto fn = [&, promise = std::move(promise), channelId, volume, pan]() {
                                            runtimeExecutor([&, promise, channelId, volume, pan](jsi::Runtime &rt3) {
                                                try {
                                                    soundEngine->setChannelVolume(channelId, volume, pan);
                                                    auto res = channelStatusToJSI(rt3, SoundEngine::channels.at(channelId));
                                                    promise->resolve(jsi::Value(rt3, res));
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

            if (propName == "setMasterVolumeAsync") {
                return a_utils::createFunc(
                        runtime,
                        funcName.c_str(),
                        1,
                        [this](jsi::Runtime &runtime,
                               const jsi::Value &,
                               const jsi::Value *arguments,
                               size_t count) -> jsi::Value {
                                return a_utils::createPromiseAsJSIValue(
                                        runtime, [&](jsi::Runtime &runtime, std::shared_ptr<a_utils::Promise> promise) {
                                            const auto volume = static_cast<float>(arguments[0].getNumber());

                                            auto fn = [&, promise = std::move(promise), volume]() {
                                                runtimeExecutor([&, promise, volume](jsi::Runtime &rt3) {
                                                    try {
                                                        soundEngine->setMasterVolume(volume);
                                                        promise->resolve(jsi::Value(soundEngine->getMasterVolume()));
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

            if (propName == "toggleChannelPlaybackAsync") {
                return a_utils::createFunc(
                        runtime,
                        funcName.c_str(),
                        1,
                        [this](jsi::Runtime &runtime,
                               const jsi::Value &,
                               const jsi::Value *arguments,
                               size_t count) -> jsi::Value {
                                return a_utils::createPromiseAsJSIValue(
                                        runtime, [&](jsi::Runtime &runtime, std::shared_ptr<a_utils::Promise> promise) {
                                            const auto channelId = static_cast<int>(arguments[0].getNumber());
                                            auto fn = [&, promise = std::move(promise), channelId]() {
                                                runtimeExecutor([&, promise, channelId](jsi::Runtime &rt3) {
                                                    try {
                                                        promise->resolve(jsi::Value(soundEngine->toggleChannelPlayback(channelId)));
                                                    } catch (const commons::ASoundEngineException &e) {
                                                        promise->reject(e.what());
                                                    }
                                                });
                                            };
                                            tpool->enqueue(fn);
                                        });

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
                                                if (!SoundEngine::channels.empty()) {
                                                    auto jsiState = jsi::Object(rt3);
                                                    auto array = jsi::Array(rt3, SoundEngine::channels.size());
                                                    try {
                                                        for (size_t i = 0; i < SoundEngine::channels.size(); ++i) {
                                                            auto res = channelStatusToJSI(rt3, SoundEngine::channels.at( i));
                                                            array.setValueAtIndex(rt3, i, res);
                                                        }
                                                    } catch (const commons::ASoundEngineException &e) {
                                                        promise->reject(e.what());
                                                    }
                                                    jsiState.setProperty(rt3, "sounds", array);
                                                    jsiState.setProperty(rt3, "masterVolume", soundEngine->getMasterVolume());
                                                    promise->resolve(std::move(jsiState));
                                                }
                                            });
                                        };
                                        tpool->enqueue(fn);
                                    });
                        });
            }

            if (propName == "setRandomizationEnabledAsync") {
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
                                            const auto shouldRandomize = static_cast<bool>(arguments[1].getBool());

                                            auto fn = [&, promise = std::move(promise), channelId, shouldRandomize]() {
                                                runtimeExecutor([&, promise, channelId, shouldRandomize](jsi::Runtime &rt3) {
                                                    try {
                                                        promise->resolve(jsi::Value(soundEngine->setChannelRandomizationEnabled(channelId, shouldRandomize)));
                                                    } catch (const commons::ASoundEngineException &e) {
                                                        promise->reject(e.what());
                                                    }
                                                });
                                            };
                                            tpool->enqueue(fn);
                                        });
                        });
            }

            if (propName == "setRandomizationSettingsAsync") {
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
                                        try {
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

                                            auto fn = [&, promise = std::move(promise), channelId, statusToSet]() {
                                                runtimeExecutor([&, promise, channelId, statusToSet](jsi::Runtime &rt3) {
                                                    try {
                                                        soundEngine->setChannelRandomizationSettings(channelId, statusToSet);
                                                        auto res = channelStatusToJSI(rt3, SoundEngine::channels.at(channelId));
                                                        promise->resolve(jsi::Value(rt3, res));
                                                    } catch (const commons::ASoundEngineException &e) {
                                                        promise->reject(e.what());
                                                    }
                                                });
                                            };
                                            tpool->enqueue(fn);

                                        } catch (const commons::ASoundEngineException &e) {
                                            jsi::detail::throwJSError(runtime, e.what());
                                        }
                                    });
                        });
            }

            if (propName == "toggleChannelMutedAsync") {
                return a_utils::createFunc(
                        runtime,
                        funcName.c_str(),
                        1,
                        [this](jsi::Runtime &runtime,
                               const jsi::Value &,
                               const jsi::Value *arguments,
                               size_t count) -> jsi::Value {
                                return a_utils::createPromiseAsJSIValue(
                                        runtime, [&](jsi::Runtime &runtime, std::shared_ptr<a_utils::Promise> promise) {
                                            const auto channelId = static_cast<int>(arguments[0].getNumber());
                                            auto fn = [&, promise = std::move(promise), channelId]() {
                                                runtimeExecutor([&, promise, channelId](jsi::Runtime &rt3) {
                                                    try {
                                                        promise->resolve(jsi::Value(soundEngine->toggleChannelMuted(channelId)));
                                                    } catch (const commons::ASoundEngineException &e) {
                                                        promise->reject(e.what());
                                                    }
                                                });
                                            };
                                            tpool->enqueue(fn);
                                        });
                        });
            }

            if (propName == "resetChannelAsync") {
                return a_utils::createFunc(
                        runtime,
                        funcName.c_str(),
                        1,
                        [this](jsi::Runtime &runtime,
                               const jsi::Value &,
                               const jsi::Value *arguments,
                               size_t count) -> jsi::Value {
                            return a_utils::createPromiseAsJSIValue(
                                    runtime, [&](jsi::Runtime &runtime, std::shared_ptr<a_utils::Promise> promise) {
                                        const auto channelId = static_cast<int>(arguments[0].getNumber());

                                        auto fn = [&, promise = std::move(promise), channelId]() {
                                            runtimeExecutor([&, promise, channelId](jsi::Runtime &rt3) {
                                                try {
                                                    soundEngine->resetChannel(channelId);
                                                    auto res = channelStatusToJSI(rt3, SoundEngine::channels.at(channelId));
                                                    promise->resolve(jsi::Value(rt3, res));
                                                } catch (const commons::ASoundEngineException &e) {
                                                    promise->reject(e.what());
                                                }
                                            });
                                        };
                                        tpool->enqueue(fn);
                                    });
                        });
            }
        }

        return jsi::Value::undefined();
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
