//
// Created by dominick on 12/8/22.
//
#include "bindings.h"
#include "SoundEngine.h"
#include <utility>
#include <vector>
#include <string>
#include <iostream>

using namespace std;
using namespace facebook;


#define HOSTFN(name, basecount) \
jsi::Function::createFromHostFunction( \
rt, \
jsi::PropNameID::forAscii(rt, name), \
basecount, \
[=](jsi::Runtime &rt, const jsi::Value &thisValue, const jsi::Value *args, size_t count) -> jsi::Value

#define HOSTFN_ASYNC(name, basecount) \
a_utils::createFunc( \
rt, \
name, \
basecount, \
[&](jsi::Runtime &rt, \
const jsi::Value &, \
const jsi::Value *arguments, \
        size_t count) -> jsi::Value { \
return a_utils::createPromiseAsJSIValue( \
        rt, [&](jsi::Runtime &rt, std::shared_ptr<a_utils::Promise> promise) -> void

namespace ambientify {
    static constexpr auto TAG = "AmbientifySoundEngine";
    std::shared_ptr<react::CallInvoker> invoker;
    std::shared_ptr<SoundEngine> soundEngine;
	std::shared_ptr<ThreadPool> tpool;

    jsi::Object channelStatusToJSI(jsi::Runtime &rt, int chId) {
        auto channel = SoundEngine::channels.at(chId);
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
        jsiRSettings.setProperty(rt, "volumeRange", jsi::Value::null());
        jsiRSettings.setProperty(rt, "panRange", jsi::Value::null());
        jsiRSettings.setProperty(rt, "pitchRange", jsi::Value::null());
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

    void install(JavaVM *jvm, jsi::Runtime &rt, std::shared_ptr<react::CallInvoker> jsCallInvoker) {
        invoker = std::move(jsCallInvoker);

        JNIEnv *jniEnv = a_utils::attachCurrThread(jvm);
        tpool = std::make_shared<ThreadPool>(1);
        soundEngine = SoundEngine::GetInstance(jniEnv, jvm);


        auto isReady = HOSTFN("isReady", 0) {
            return {soundEngine->isEngineReady};
          });

        auto nChannels = HOSTFN("nChannels", 0) {
            return { (int) SoundEngine::channels.size() };
        });

		auto createChannel = HOSTFN_ASYNC("createChannelAsync", 2) {
                   if (count < 1) throw jsi::JSError(rt, "No valid path specified.");
                   std::string path;
                   bool shouldPlay = false;
                   if (arguments[0].isString()) {
                       path = static_cast<std::string>(arguments[0].getString(rt).utf8(rt));
                   }
                   if (arguments[1].isBool()) {
                       shouldPlay = static_cast<bool>(arguments[1].getBool());
                   }

                   auto fn = [&, promise = std::move(promise), path, shouldPlay]() {
                       invoker->invokeAsync([&, promise, path, shouldPlay]() {
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
                    return {};
                });
        });

        auto setStatus = HOSTFN_ASYNC("setStatusAsync", 2) {
                const auto channelId = static_cast<int>(arguments[0].getNumber());
                const auto jsiStatus = std::make_shared<jsi::Object>(arguments[1].getObject(rt));

                auto fn = [&, promise , channelId, jsiStatus]() {
                    invoker->invokeAsync([&, promise, channelId, jsiStatus] {
                        try {
                            auto statusToSet = std::make_shared<std::map<std::string, std::any>>();
                            auto jsiStatusPropNames = jsiStatus->getPropertyNames(rt);
                            for (int i = 0; i < jsiStatusPropNames.size(rt); i++) {
                                const auto propNameStr = jsiStatusPropNames.getValueAtIndex(rt, i).asString(rt).utf8(rt);
                                if (propNameStr == "volume") {
                                    statusToSet->insert(
                                            {propNameStr, (float) jsiStatus->getProperty(rt, propNameStr.c_str()).getNumber()});
                                } else if (propNameStr == "pan") {
                                    statusToSet->insert(
                                            {propNameStr, (float) jsiStatus->getProperty(rt, propNameStr.c_str()).getNumber()});
                                } else if (propNameStr == "pitch") {
                                    statusToSet->insert(
                                            {propNameStr, (float) jsiStatus->getProperty(rt, propNameStr.c_str()).getNumber()});
                                } else if (propNameStr == "cfPercentageStart") {
                                    statusToSet->insert({propNameStr, (float) jsiStatus->getProperty(rt,
                                                                                                     propNameStr.c_str()).getNumber()});
                                } else if (propNameStr == "noUnload") {
                                    statusToSet->insert({propNameStr, jsiStatus->getProperty(rt, propNameStr.c_str()).getBool()});
                                } else if (propNameStr == "isMuted") {
                                    statusToSet->insert({propNameStr, jsiStatus->getProperty(rt, propNameStr.c_str()).getBool()});
                                }
                            }

                            soundEngine->loadChannelStatus(channelId, statusToSet);
                            promise->resolve(jsi::Value(channelStatusToJSI(rt, channelId)));
                        } catch (const commons::ASoundEngineException &e) {
                            promise->reject(e.what());
                        }
                        return jsi::Value::undefined();

                    });
                };
                tpool->enqueue(fn);
            });
        });

        auto loadChannel = HOSTFN_ASYNC("loadChannelAsync", 2) {
               const auto channelId = static_cast<int>(arguments[0].getNumber());
               const auto path = static_cast<std::string>(arguments[1].getString(rt).utf8(rt));

               auto fn = [&, promise = std::move(promise), channelId, path]() {
                   LOG_DEBUG("Attempting to load channel");
                   invoker->invokeAsync([&, promise, channelId, path]() {
                       try {
                           LOG_DEBUG("Loading channel");
                           soundEngine->loadChannel(channelId, &path);

                           auto res = channelStatusToJSI(rt, channelId);
                           promise->resolve(jsi::Value(rt, res));
                       } catch (const commons::ASoundEngineException &e) {
                           promise->reject(e.what());
                       }
                   });
               };
               tpool->enqueue(fn);
           });
        });

        auto unloadChannel = HOSTFN_ASYNC("unloadChannelAsync", 1) {
               const auto channelId = static_cast<int>(arguments[0].getNumber());

               auto fn = [&, promise = std::move(promise), channelId]() {
                   invoker->invokeAsync([&, promise, channelId]() {
                       try {
                           soundEngine->unloadChannel(channelId);
                           auto res = channelStatusToJSI(rt, channelId);
                           promise->resolve(jsi::Value(rt, res));
                       } catch (const commons::ASoundEngineException &e) {
                           promise->reject(e.what());
                       }
                   });
               };
               tpool->enqueue(fn);
           });
        });

        auto playChannel = HOSTFN_ASYNC("playChannelAsync", 1) {
               const auto channelId = static_cast<int>(arguments[0].getNumber());
               auto fn = [&, promise = std::move(promise), channelId]() {
                   invoker->invokeAsync([&, promise, channelId]() {
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

        auto playAll = HOSTFN_ASYNC("playAllAsync", 0) {
               auto fn = [&, promise = std::move(promise) ]() {
                   invoker->invokeAsync([&, promise ]() {
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

        auto stopAll = HOSTFN_ASYNC("stopAllAsync", 0) {
               auto fn = [&, promise = std::move(promise)]() {
                   invoker->invokeAsync([&, promise]() {
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

        auto stopChannel = HOSTFN_ASYNC("stopChannelAsync", 1) {
               const auto channelId = static_cast<int>(arguments[0].getNumber());
               auto fn = [&, promise = std::move(promise), channelId]() {
                   invoker->invokeAsync([&, promise, channelId]() {
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

        auto setCrossfading = HOSTFN_ASYNC("setCrossfadingAsync", 2) {
               const auto channelId = static_cast<int>(arguments[0].getNumber());
               const auto enabled = static_cast<int>(arguments[1].getBool());

               auto fn = [&, promise = std::move(promise), channelId, enabled]() {
                   invoker->invokeAsync([&, promise, channelId, enabled]() {
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

        auto setChannelVolume = HOSTFN_ASYNC("setChannelVolumeAsync", 3) {
                 const auto channelId = static_cast<int>(arguments[0].getNumber());
                 const auto volume = static_cast<float>(arguments[1].getNumber());
                 const auto pan = static_cast<float>(arguments[2].getNumber());

                 auto fn = [&, promise = std::move(promise), channelId, volume, pan]() {
                     invoker->invokeAsync([&, promise, channelId, volume, pan]() {
                         try {
                             soundEngine->setChannelVolume(channelId, volume, pan);
                             auto res = channelStatusToJSI(rt, channelId);
                             promise->resolve(jsi::Value(rt, res));
                         } catch (const commons::ASoundEngineException &e) {
                             promise->reject(e.what());
                         }
                         return jsi::Value::undefined();
                     });
                 };
                 tpool->enqueue(fn);
             });
        });

        auto setMasterVolume = HOSTFN_ASYNC("setMasterVolumeAsync", 1) {
               const auto volume = static_cast<float>(arguments[0].getNumber());

               auto fn = [&, promise = std::move(promise), volume]() {
                   invoker->invokeAsync([&, promise, volume]() {
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

        auto toggleChannelPlayback = HOSTFN_ASYNC("toggleChannelPlaybackAsync", 1) {
               const auto channelId = static_cast<int>(arguments[0].getNumber());
               auto fn = [&, promise = std::move(promise), channelId]() {
                   invoker->invokeAsync([&, promise, channelId]() {
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

        auto getSerializedState = HOSTFN_ASYNC("getSerializedStateAsync", 0) {
             auto fn = [&, promise = std::move(promise)]() {
                 invoker->invokeAsync([&, promise]() {
                     if (SoundEngine::isEngineReady) {
                         if (!SoundEngine::channels.empty()) {
                             auto jsiState = jsi::Object(rt);
                             auto array = jsi::Array(rt, SoundEngine::channels.size());
                             try {
                                 for (size_t i = 0; i < SoundEngine::channels.size(); ++i) {
                                     auto res = channelStatusToJSI(rt, i);
                                     array.setValueAtIndex(rt, i, res);
                                 }
                             } catch (const commons::ASoundEngineException &e) {
                                 promise->reject(e.what());
                             }
                             jsiState.setProperty(rt, "sounds", array);
                             jsiState.setProperty(rt, "masterVolume", soundEngine->getMasterVolume());
                             promise->resolve(std::move(jsiState));
                         }
                     }
                 });
             };
             tpool->enqueue(fn);
           });
        });

        auto setRandomizationEnabled = HOSTFN_ASYNC("setRandomizationEnabledAsync", 2) {
               const auto channelId = static_cast<int>(arguments[0].getNumber());
               const auto shouldRandomize = static_cast<bool>(arguments[1].getBool());

               auto fn = [&, promise = std::move(promise), channelId, shouldRandomize]() {
                   invoker->invokeAsync([&, promise, channelId, shouldRandomize]() {
                       try {
                           soundEngine->setChannelRandomizationEnabled(channelId, shouldRandomize);
                           auto res = channelStatusToJSI(rt, channelId);
                           promise->resolve(std::move(res));
                       } catch (const commons::ASoundEngineException &e) {
                           promise->reject(e.what());
                       }
                   });
               };
               tpool->enqueue(fn);
           });
       });

        auto setRandomizationSettings = HOSTFN_ASYNC("setRandomizationSettingsAsync", 2) {
               const auto channelId = static_cast<int>(arguments[0].getNumber());
               const auto jsiStatus = std::make_shared<jsi::Object>(arguments[1].getObject(rt));
               try {
                   auto statusToSet = std::make_shared<ChannelRandomizationDataSettings>();
                   auto jsiStatusPropNames = jsiStatus->getPropertyNames(rt);
                   for (int i = 0; i < jsiStatusPropNames.size(rt); i++) {
                       // @todo: safety checks
                       const auto propNameStr = jsiStatusPropNames.getValueAtIndex(rt, i).asString(rt).utf8(rt);
                       if (jsiStatus->getProperty(rt, propNameStr.c_str()).isNull()) continue;
                       if (propNameStr == "times") {
                           statusToSet->times = (int) jsiStatus->getProperty(rt, propNameStr.c_str()).getNumber();
                       } else if (propNameStr == "minutes") {
                           statusToSet->minutes = (int) jsiStatus->getProperty(rt, propNameStr.c_str()).getNumber();
                       } else if (propNameStr == "panRange") {
                           const auto panRange = jsiStatus->getProperty(rt, propNameStr.c_str()).getObject(rt).asArray(rt);
                           const auto v1 = static_cast<float>(panRange.getValueAtIndex(rt, 0).getNumber());
                           const auto v2 = static_cast<float>(panRange.getValueAtIndex(rt, 1).getNumber());
                           statusToSet->panRange = std::make_pair(v1, v2);
                       } else if (propNameStr == "volumeRange") {
                           const auto volumeRange = jsiStatus->getProperty(rt, propNameStr.c_str()).getObject(rt).asArray(rt);
                           const auto v1 = static_cast<float>(volumeRange.getValueAtIndex(rt, 0).getNumber());
                           const auto v2 = static_cast<float>(volumeRange.getValueAtIndex(rt, 1).getNumber());
                           statusToSet->volumeRange = std::make_pair(v1, v2);
                       } else if (propNameStr == "pitchRange") {
                           const auto pitchRange = jsiStatus->getProperty(rt, propNameStr.c_str()).getObject(rt).asArray(rt);
                           const auto v1 = static_cast<float>(pitchRange.getValueAtIndex(rt, 0).getNumber());
                           const auto v2 = static_cast<float>(pitchRange.getValueAtIndex(rt, 1).getNumber());
                           statusToSet->pitchRange = std::make_pair(v1, v2);
                       }
                   }

                   auto fn = [&, promise = std::move(promise), channelId, statusToSet]() {
                       invoker->invokeAsync([&, promise, channelId, statusToSet]() {
                           try {
                               soundEngine->setChannelRandomizationSettings(channelId, statusToSet);
                               auto res = channelStatusToJSI(rt, channelId);
                               promise->resolve(jsi::Value(rt, res));
                           } catch (const commons::ASoundEngineException &e) {
                               promise->reject(e.what());
                           }
                       });
                   };
                   tpool->enqueue(fn);

               } catch (const commons::ASoundEngineException &e) {
                   throw jsi::JSError(rt, e.what());
               }
           });
        });

        auto toggleChannelMuted = HOSTFN_ASYNC("toggleChannelMutedAsync", 1) {
               const auto channelId = static_cast<int>(arguments[0].getNumber());
               auto fn = [&, promise = std::move(promise), channelId]() {
                   invoker->invokeAsync([&, promise, channelId]() {
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

        auto resetChannel = HOSTFN_ASYNC("resetChannelAsync", 1) {
             const auto channelId = static_cast<int>(arguments[0].getNumber());

             auto fn = [&, promise = std::move(promise), channelId]() {
                 invoker->invokeAsync([&, promise, channelId]() {
                     try {
                         soundEngine->resetChannel(channelId);
                         auto res = channelStatusToJSI(rt, channelId);
                         promise->resolve(jsi::Value(rt, res));
                     } catch (const commons::ASoundEngineException &e) {
                         promise->reject(e.what());
                     }
                 });
             };
             tpool->enqueue(fn);
          });
        });

        jsi::Object module = jsi::Object(rt);
        module.setProperty(rt, "isReady", std::move(isReady));
        module.setProperty(rt, "nChannels", std::move(nChannels));
		module.setProperty(rt, "createChannelAsync", std::move(createChannel));
        module.setProperty(rt, "setStatusAsync", std::move(setStatus));
        module.setProperty(rt, "loadChannelAsync", std::move(loadChannel));
        module.setProperty(rt, "unloadChannelAsync", std::move(unloadChannel));
        module.setProperty(rt, "playChannelAsync", std::move(playChannel));
        module.setProperty(rt, "playAllAsync", std::move(playAll));
        module.setProperty(rt, "stopAllAsync", std::move(stopAll));
        module.setProperty(rt, "stopChannelAsync", std::move(stopChannel));
        module.setProperty(rt, "setCrossfadingAsync", std::move(setCrossfading));
        module.setProperty(rt, "setChannelVolumeAsync", std::move(setChannelVolume));
        module.setProperty(rt, "setMasterVolumeAsync", std::move(setMasterVolume));
        module.setProperty(rt, "toggleChannelPlaybackAsync", std::move(toggleChannelPlayback));
        module.setProperty(rt, "getSerializedStateAsync", std::move(getSerializedState));
        module.setProperty(rt, "setRandomizationEnabledAsync", std::move(setRandomizationEnabled));
        module.setProperty(rt, "setRandomizationSettingsAsync", std::move(setRandomizationSettings));
        module.setProperty(rt, "toggleChannelMutedAsync", std::move(toggleChannelMuted));
        module.setProperty(rt, "resetChannelAsync", std::move(resetChannel));

		rt.global().setProperty(rt, "__AmbientifySoundEngine", std::move(module));

        LOG_DEBUG("_AmbientifySoundEngine jsi bindings installed, notifying JS...");
        jclass clazz = jniEnv->FindClass("com/ambientifysoundengine/EngineService");
        jmethodID methodId = jniEnv->GetStaticMethodID(clazz, "bindNotifyJS", "()V");
        if (methodId == nullptr) {
            LOG_ERR("notifyJS native lambda -> methodId is null");
        } else {
            jniEnv->CallStaticVoidMethod(clazz, methodId);
        }
    }

}

