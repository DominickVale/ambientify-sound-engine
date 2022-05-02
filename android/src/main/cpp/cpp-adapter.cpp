#include <jni.h>
#include <fbjni/fbjni.h>
#include <jsi/jsi.h>
#include <ReactCommon/CallInvoker.h>
#include <ReactCommon/CallInvokerHolder.h>
#include <ReactCommon/RuntimeExecutor.h>
#include <react/jni/JRuntimeExecutor.h>
#include <sys/types.h>
#include "pthread.h"
#include "SoundEngineHostObject.h"

using namespace facebook;
using namespace std;

JavaVM *java_vm;

void installSoundEngineHostObject(jsi::Runtime &jsiRuntime, ambientify::RuntimeExecutor rtEx, std::shared_ptr<facebook::react::CallInvoker> jsCallInvoker) {
    ambientify::SoundEngineHostObject soundEngineObj{std::move(jsCallInvoker), std::move(rtEx)};
    shared_ptr<ambientify::SoundEngineHostObject> binding = make_shared<ambientify::SoundEngineHostObject>(move(soundEngineObj));
    auto object = jsi::Object::createFromHostObject(jsiRuntime, binding);

    jsiRuntime.global().setProperty(jsiRuntime, "_AmbientifySoundEngine", object);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_ambientifysoundengine_ASoundEngineModule_nativeInstall(JNIEnv *env, jclass thiz, jlong jsi,  jobject rtEx, jobject callInvokerHolderImpl) {
    auto runtime = reinterpret_cast<jsi::Runtime *>(jsi);

    auto callInvokerRef = jni::make_local(callInvokerHolderImpl);
    auto callInvokerHolder = jni::dynamic_ref_cast<react::CallInvokerHolder::javaobject>(callInvokerRef);
    auto callInvoker = callInvokerHolder->cthis()->getCallInvoker();

    std::weak_ptr<react::CallInvoker> weakJsCallInvoker = callInvoker;
    auto runtimeExecutor =
            [runtime, weakJsCallInvoker](
                    std::function<void(jsi::Runtime & runtime)>&& callback) {
                if (auto strongJsCallInvoker = weakJsCallInvoker.lock()) {
                    strongJsCallInvoker->invokeAsync(
                            [runtime, callback = std::move(callback)]() {
                                callback(*runtime);
                            });
                }
            };

    if (runtime) {
        installSoundEngineHostObject(*runtime, runtimeExecutor, callInvoker);
    }

    env->GetJavaVM(&java_vm);
}










