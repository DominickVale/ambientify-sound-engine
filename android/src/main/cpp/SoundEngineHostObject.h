//
// Created by dominick on 3/4/22.
//
#pragma once

#include <jsi/jsi.h>
#include <jni.h>
#include <string>
#include "SoundEngine.h"

using namespace facebook;

namespace ambientify {
    class JSI_EXPORT SoundEngineHostObject : public jsi::HostObject {
    public:

        explicit SoundEngineHostObject(std::shared_ptr<react::CallInvoker> callInvoker, RuntimeExecutor rtEx) : _callInvoker(callInvoker),
                                                                                                                runtimeExecutor(rtEx) {
            soundEngine = SoundEngine::GetInstance();
            tpool = std::make_shared<ThreadPool>(1);
            __android_log_print(ANDROID_LOG_DEBUG, TAG, "SoundEngineHostObject() created.");
        }

        ~SoundEngineHostObject() {
            __android_log_print(ANDROID_LOG_DEBUG, TAG, "SoundEngineHostObject() destroyed");
        }


        jsi::Value get(jsi::Runtime &, const jsi::PropNameID &name) override;

        void set(jsi::Runtime &, const jsi::PropNameID &name, const jsi::Value &value) override;

        std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime &rt) override;

    private:
        std::shared_ptr<react::CallInvoker> _callInvoker;
        RuntimeExecutor runtimeExecutor;
        static constexpr auto TAG = "SoundEngineHostObject";
        std::shared_ptr<SoundEngine> soundEngine;

        std::shared_ptr<ThreadPool> tpool;
    };
}
