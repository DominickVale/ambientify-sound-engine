cmake_minimum_required(VERSION 3.18.1)

set (CMAKE_VERBOSE_MAKEFILE ON)
project(ambientify-sound-engine)
set(PACKAGE_NAME "ambientify-sound-engine")
set (BUILD_DIR ${CMAKE_SOURCE_DIR}/build)

if(${REACT_NATIVE_VERSION} LESS 66)
        set (
                INCLUDE_JSI_CPP
                "${NODE_MODULES_DIR}/react-native/ReactCommon/jsi/jsi/jsi.cpp"
        )
        set (
                INCLUDE_JSIDYNAMIC_CPP
                "${NODE_MODULES_DIR}/react-native/ReactCommon/jsi/jsi/JSIDynamic.cpp"
        )
endif()

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

file (GLOB LIBFBJNI_INCLUDE_DIR "${BUILD_DIR}/fbjni-*-headers.jar/")

target_include_directories(
        ${PACKAGE_NAME}
        PRIVATE
        # --- fbjni ---
        "${LIBFBJNI_INCLUDE_DIR}"
        # --- React Native ---
        "${NODE_MODULES_DIR}/react-native/React"
        "${NODE_MODULES_DIR}/react-native/React/Base"
        "${NODE_MODULES_DIR}/react-native/ReactAndroid/src/main/jni"
        "${NODE_MODULES_DIR}/react-native/ReactAndroid/src/main/java/com/facebook/react/turbomodule/core/jni"
        "${NODE_MODULES_DIR}/react-native/ReactCommon"
        "${NODE_MODULES_DIR}/react-native/ReactCommon/callinvoker"
        "${NODE_MODULES_DIR}/react-native/ReactCommon/jsi"
        "${NODE_MODULES_DIR}/react-native/ReactCommon/runtimeexecutor"
        "${NODE_MODULES_DIR}/hermes-engine/android/include/"
        ${INCLUDE_JSI_CPP} # only on older RN versions
        ${INCLUDE_JSIDYNAMIC_CPP} # only on older RN versions
        "./src/main/cpp"
)

file (GLOB LIBRN_DIR "${BUILD_DIR}/react-native-0*/jni/${ANDROID_ABI}")


if(${REACT_NATIVE_VERSION} LESS 69)
        # From `hermes-engine` npm package
        target_include_directories(
                ${PACKAGE_NAME}
                PRIVATE
                "${JS_RUNTIME_DIR}/android/include"
        )
else()
        # Bundled Hermes from module `com.facebook.react:hermes-engine` or project `:ReactAndroid:hermes-engine`
        target_include_directories(
                ${PACKAGE_NAME}
                PRIVATE
                "${JS_RUNTIME_DIR}/API"
                "${JS_RUNTIME_DIR}/public"
        )
endif()

find_library(
        FBJNI_LIB
        fbjni
        PATHS ${LIBRN_DIR}
        NO_CMAKE_FIND_ROOT_PATH
)

find_library(
        REACT_NATIVE_JNI_LIB
        reactnativejni
        PATHS ${LIBRN_DIR}
        NO_CMAKE_FIND_ROOT_PATH
)

find_library(
        JSI_LIB
        jsi
        PATHS ${LIBRN_DIR}
        NO_CMAKE_FIND_ROOT_PATH
)

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

# target_link_libraries(sequel fbjni::fbjni android log)
target_link_libraries(
        ${PACKAGE_NAME}
        ${LOG_LIB}
        ${JSI_LIB}
        ${REACT_NATIVE_JNI_LIB}
        ${FBJNI_LIB}
        fmod
        fmt::fmt-header-only
        android
)

