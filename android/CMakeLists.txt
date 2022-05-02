cmake_minimum_required(VERSION 3.18.1)

project(rnAmbientifySoundEngine)
set(PACKAGE_NAME "ambientifySoundEngine")
set (BUILD_DIR ${CMAKE_SOURCE_DIR}/build)

file (GLOB LIBFBJNI_INCLUDE_DIR "${BUILD_DIR}/fbjni-*-headers.jar/")
include_directories(
        ${PACKAGE_NAME}
        "${LIBFBJNI_INCLUDE_DIR}"
        "${NODE_MODULES_DIR}/react-native/React"
        "${NODE_MODULES_DIR}/react-native/React/Base"
        "${NODE_MODULES_DIR}/react-native/ReactAndroid/src/main/jni"
        "${NODE_MODULES_DIR}/react-native/ReactAndroid/src/main/java/com/facebook/react/turbomodule/core/jni"
        "${NODE_MODULES_DIR}/react-native/ReactCommon"
        "${NODE_MODULES_DIR}/react-native/ReactCommon/callinvoker"
        "${NODE_MODULES_DIR}/react-native/ReactCommon/jsi"
        "${NODE_MODULES_DIR}/react-native/ReactCommon/runtimeexecutor"
        "./src/main/cpp"
        "."
)

file(GLOB LIBRN_DIR "${BUILD_DIR}/react-native-0*/jni/${ANDROID_ABI}")
message(STATUS "LIBRN_DIR: ${LIBRN_DIR}")
message(STATUS "LIBFBJNI: ${LIBFBJNI_INCLUDE_DIR}")

if (EXISTS "${LIBRN_DIR}/libjsi.so")
  # React Native 0.66.x and above

  find_library(
          JSI_LIB
          jsi
          PATHS ${LIBRN_DIR}
          NO_CMAKE_FIND_ROOT_PATH
  )

  add_library(
          ${PACKAGE_NAME}
          SHARED
          ./src/main/cpp/common.cpp
          ./src/main/cpp/utils/Promise.cpp
          ./src/main/cpp/SoundEngine.cpp
          ./src/main/cpp/EngineChannel.cpp
          ./src/main/cpp/SoundEngineHostObject.cpp
          ./src/main/cpp/cpp-adapter.cpp
  )
else ()
  # React Native 0.65.x and below
  set(JSI_LIB "")
  add_library(
          ${PACKAGE_NAME}
          SHARED
          ../../react-native/ReactCommon/jsi/jsi/jsi.cpp
          ./src/main/cpp/common.cpp
          ./src/main/cpp/utils/Promise.cpp
          ./src/main/cpp/SoundEngine.cpp
          ./src/main/cpp/EngineChannel.cpp
          ./src/main/cpp/SoundEngineHostObject.cpp
          ./src/main/cpp/cpp-adapter.cpp
  )
endif ()

set(LIB_FMOD ${FMOD_API_ROOT}/core/lib/${ANDROID_ABI}/libfmod${FMOD_LIB_SUFFIX}.so)

add_library(fmod SHARED IMPORTED)

add_subdirectory(./src/main/cpp/include/fmt EXCLUDE_FROM_ALL)

set_target_properties(
        fmod PROPERTIES
        IMPORTED_LOCATION ${LIB_FMOD}
        INTERFACE_INCLUDE_DIRECTORIES ${FMOD_API_ROOT}/core/inc
)

find_library(
        LOG_LIB
        log
)

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

set_target_properties(
        ${PACKAGE_NAME} PROPERTIES
        CXX_STANDARD 20
        CXX_EXTENSIONS OFF
        POSITION_INDEPENDENT_CODE ON
)

target_link_libraries(
        ${PACKAGE_NAME}
        PRIVATE
        ${LOG_LIB}
        ${REACT_NATIVE_JNI_LIB}
        ${FBJNI_LIB}
        ${JSI_LIB}
        fmod
        fmt::fmt-header-only
        android
)
