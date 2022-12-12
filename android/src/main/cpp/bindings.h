//
// Created by dominick on 12/8/22.
//
#include <jsi/jsilib.h>
#include <jni.h>
#include <jsi/jsi.h>
#include <ReactCommon/CallInvoker.h>

using namespace facebook;

namespace ambientify {
    void install(JavaVM *jvm, jsi::Runtime &rt, std::shared_ptr<react::CallInvoker> jsCallInvoker);
}
