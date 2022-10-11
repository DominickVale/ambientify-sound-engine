package com.ambientifysoundengine;

import android.util.Log;

import androidx.annotation.NonNull;

import com.facebook.react.bridge.JavaScriptContextHolder;
import com.facebook.react.bridge.LifecycleEventListener;
import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.bridge.ReactContextBaseJavaModule;
import com.facebook.react.bridge.ReactMethod;
import com.facebook.react.bridge.RuntimeExecutor;
import com.facebook.react.module.annotations.ReactModule;
import com.facebook.react.turbomodule.core.CallInvokerHolderImpl;

@ReactModule(name = ASoundEngineModule.NAME)
public class ASoundEngineModule extends ReactContextBaseJavaModule implements LifecycleEventListener {
  public static final String NAME = "ASoundEngine";
  private static native void nativeInstall(long jsiPtr, RuntimeExecutor runtimeExecutor, CallInvokerHolderImpl callInvoker);

  public ASoundEngineModule(ReactApplicationContext reactContext) {
    super(reactContext);
    reactContext.addLifecycleEventListener(this);
  }

  @Override
  @NonNull
  public String getName() {
    return NAME;
  }

  @ReactMethod(isBlockingSynchronousMethod = true)
  public boolean install() {
    try {
      ReactApplicationContext context = getReactApplicationContext();

      for (String lib: BuildConfig.FMOD_LIBS)
      {
        System.loadLibrary(lib);
      }
      System.loadLibrary("ambientifySoundEngine");
      org.fmod.FMOD.init(context);

      JavaScriptContextHolder jsContext = context.getJavaScriptContextHolder();
      if (jsContext.get() != 0) {
        RuntimeExecutor runtimeExecutor =
          getReactApplicationContext().getCatalystInstance().getRuntimeExecutor();
        CallInvokerHolderImpl jsCallInvokerHolder =
          (CallInvokerHolderImpl)
            getReactApplicationContext().getCatalystInstance().getJSCallInvokerHolder();
        nativeInstall(jsContext.get(), runtimeExecutor, jsCallInvokerHolder);
      } else {
        Log.e(NAME, "JSI Runtime is not available in debug mode");
      }
      return true;
    } catch (Exception exception) {
      return false;
    }
  }

  @Override
  public void onHostResume() {
    //noop
  }

  @Override
  public void onHostPause() {
    //noop
  }

  @Override
  public void onHostDestroy() {
    Log.i(NAME, "Unloading FMOD...");
    org.fmod.FMOD.close();
  }
}
