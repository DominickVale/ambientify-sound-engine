#include "bindings.h"
#include <ReactCommon/CallInvokerHolder.h>
#include <fbjni/fbjni.h>
#include <jni.h>
#include <jsi/jsi.h>
#include <typeinfo>

using namespace facebook;
using namespace std;
namespace jni = ::facebook::jni;

JavaVM *javaVm;

struct EngineBridge : jni::JavaClass<EngineBridge> {
    static constexpr auto kJavaDescriptor =
            "Lcom/ambientifysoundengine/EngineBridge;";

    static void registerNatives() {
        javaClassStatic()->registerNatives(
                {// initialization for JSI
                        makeNativeMethod("installNativeJsi",
                                         EngineBridge::installNativeJsi)});
    }

private:
    static void installNativeJsi(
            jni::alias_ref<jni::JObject> thiz, jlong jsiRuntimePtr,
            jni::alias_ref<react::CallInvokerHolder::javaobject> jsCallInvokerHolder,
            jni::alias_ref<jni::JString> docPath) {
        auto jsiRuntime = reinterpret_cast<jsi::Runtime *>(jsiRuntimePtr);
        auto jsCallInvoker = jsCallInvokerHolder->cthis()->getCallInvoker();

        ambientify::install(javaVm, *jsiRuntime, jsCallInvoker);
    }
};

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *) {
    javaVm = vm;
    return jni::initialize(vm, [] { EngineBridge::registerNatives(); });
}
