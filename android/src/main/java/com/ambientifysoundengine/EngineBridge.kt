package com.ambientifysoundengine

import com.facebook.react.bridge.ReactContext
import com.facebook.react.turbomodule.core.CallInvokerHolderImpl

class EngineBridge {
    private external fun installNativeJsi(
        jsContextNativePointer: Long,
        jsCallInvokerHolder: CallInvokerHolderImpl,
        docPath: String
    )

    fun install(context: ReactContext) {
        val jsContextPointer = context.javaScriptContextHolder.get()
        val jsCallInvokerHolder = context.catalystInstance.jsCallInvokerHolder as CallInvokerHolderImpl
        val path = context.filesDir.absolutePath
        installNativeJsi(
            jsContextPointer,
            jsCallInvokerHolder,
            path
        )
    }

    companion object {
        val instance = EngineBridge()
    }
}
