package org.chromium.chromiumjnigenerator;

import org.chromium.jnigenerator.annotations.CalledByNative;
import org.chromium.jnigenerator.annotations.JNINamespace;

@JNINamespace("")
public class ChromiumJniTestA {
    //Work
    public static native boolean nativeFunctionAA(boolean param1, float param2);

    //Not work. "static" should be put in front of "native".
    public native static boolean nativeFunctionAB(boolean param1, float param2);

    //Work
    public native long nativeFunctionAC(String param1);

    //Not work. The function name should starts with "native".
    public native long functionAD(String param1);

    @CalledByNative
    public static float functionAE(String param1, boolean param2){
        return 9.7f;
    }
}
