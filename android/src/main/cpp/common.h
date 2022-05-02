//
// Created by dominick on 3/3/22.
//
#pragma once

#include <android/log.h>
#include <android/asset_manager.h>
#include <string>
#include <string_view>
#include <map>
#include <future>
#include <functional>
#include <chrono>
#include <thread>
#include <mutex>
#include <vector>
#include <any>
#include <iomanip>
#include <random>
#include <ReactCommon/CallInvoker.h>
#include <jsi/jsi.h>
#include <fmt/format.h>
#include "fmod.h"

#include "utils/ThreadPool.h"
#include "utils/Promise.h"

namespace ambientify::constants {
    constexpr auto LOG_ID = "AmbientifySoundEngine";
    constexpr auto MAX_CHANNELS = 15;
}

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

void ERRCHECK_fn(FMOD_RESULT result, const char *file, int line);

#define ERRCHECK(_result) ERRCHECK_fn(_result, __FILENAME__, __LINE__)
#define LOG_DEBUG(...) __android_log_print(ANDROID_LOG_DEBUG, ambientify::constants::LOG_ID, __VA_ARGS__)
#define LOG_ERR(...) __android_log_print(ANDROID_LOG_ERROR, ambientify::constants::LOG_ID, __VA_ARGS__)

// @todo: expand namespace and rename a_utils to ambientify::utils
namespace ambientify {
    using RuntimeExecutor = std::function<void(std::function<void(facebook::jsi::Runtime &runtime)> &&callback)>;
}

namespace ambientify::commons {
    class ASoundEngineException : public std::exception {
    private:
        std::string m_error{};
    public:
        ASoundEngineException(std::string_view error) : m_error{error} {}

        const char *what() const noexcept override { return m_error.c_str(); }
    };
}

namespace a_utils {
    using namespace facebook::jsi;

    template<typename NativeFunc>
    static Function createFunc(Runtime &jsiRuntime, const char *prop, int paramCount, NativeFunc &&func) {
        return Function::createFromHostFunction(
                jsiRuntime,
                PropNameID::forAscii(jsiRuntime, prop),
                paramCount,
                std::forward<NativeFunc>(func));
    }

    static float generateRandomFloat (float min, float max) {
        std::random_device rd;
        std::mt19937 e2(rd());
        std::uniform_real_distribution<> dist(min, max);
        return dist(e2);
    }
}

