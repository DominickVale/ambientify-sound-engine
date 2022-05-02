/* Copyright (c) Meta Platforms, Inc. and affiliates.
*
* This source code is licensed under the MIT license found in the
* LICENSE file in the root directory of this source tree.
*/

#include <utility>

#include "Promise.h"

namespace a_utils {

    using namespace facebook;

    Promise::Promise(jsi::Runtime &rt, jsi::Function resolve, jsi::Function reject)
            : runtime_(rt), resolve_(std::move(resolve)), reject_(std::move(reject)) {}

    void Promise::resolve(const jsi::Value &result) {
        resolve_.call(runtime_, result);
    }

    void Promise::reject(const std::string &message) {
        jsi::Object error(runtime_);
        error.setProperty(
                runtime_, "message", jsi::String::createFromUtf8(runtime_, message));
        reject_.call(runtime_, error);
    }

    jsi::Value createPromiseAsJSIValue(
            jsi::Runtime &rt,
            const PromiseSetupFunctionType func) {
        jsi::Function JSPromise = rt.global().getPropertyAsFunction(rt, "Promise");
        jsi::Function fn = jsi::Function::createFromHostFunction(
                rt,
                jsi::PropNameID::forAscii(rt, "fn"),
                2,
                [func](
                        jsi::Runtime &rt2,
                        const jsi::Value &thisVal,
                        const jsi::Value *args,
                        size_t count) {
                    jsi::Function resolve = args[0].getObject(rt2).getFunction(rt2);
                    jsi::Function reject = args[1].getObject(rt2).getFunction(rt2);
                    auto wrapper = std::make_shared<Promise>(
                            rt2, std::move(resolve), std::move(reject));
                    func(rt2, wrapper);
                    return jsi::Value::undefined();
                });

        return JSPromise.callAsConstructor(rt, fn);
    }

}
