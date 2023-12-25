cmake_minimum_required(VERSION 3.18.1)

set (CMAKE_VERBOSE_MAKEFILE ON)
project(ambientify-sound-engine)
set(PACKAGE_NAME "ambientify-sound-engine")
set (BUILD_DIR ${CMAKE_SOURCE_DIR}/build)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
set (CMAKE_CXX_STANDARD 20)

include_directories(
        "./src/main/cpp"
)

add_library(
        ${PACKAGE_NAME}
        SHARED
        ./src/main/cpp/common.cpp
        ./src/main/cpp/utils/Promise.cpp
        ./src/main/cpp/SoundEngine.cpp
        ./src/main/cpp/EngineChannel.cpp
        ./src/main/cpp/bindings.cpp
        ./src/main/cpp/cpp-adapter.cpp
)

find_package(ReactAndroid REQUIRED CONFIG)
find_package(fbjni REQUIRED CONFIG)

find_library(
        LOG_LIB
        log
)

set(LIB_FMOD ${FMOD_API_ROOT}/core/lib/${ANDROID_ABI}/libfmod${FMOD_LIB_SUFFIX}.so)

add_library(fmod SHARED IMPORTED)

add_subdirectory(./src/main/cpp/include/fmt EXCLUDE_FROM_ALL)

set_target_properties(
        fmod PROPERTIES
        IMPORTED_LOCATION ${LIB_FMOD}
        INTERFACE_INCLUDE_DIRECTORIES ${FMOD_API_ROOT}/core/inc
)

set_target_properties(
        ${PACKAGE_NAME} PROPERTIES
        CXX_STANDARD 20
        CXX_EXTENSIONS OFF
        POSITION_INDEPENDENT_CODE ON
)

target_link_libraries(
        ${PACKAGE_NAME}
        ${LOG_LIB}
        fbjni::fbjni
        ReactAndroid::jsi
        ReactAndroid::turbomodulejsijni
        ReactAndroid::react_nativemodule_core
        ReactAndroid::hermes_executor
        fmod
        fmt::fmt-header-only
        android
)

