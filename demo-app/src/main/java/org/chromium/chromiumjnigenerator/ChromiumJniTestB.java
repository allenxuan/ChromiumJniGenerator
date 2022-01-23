package org.chromium.chromiumjnigenerator;

import org.chromium.jnigenerator.annotations.JNINamespace;
import org.chromium.jnigenerator.annotations.NativeMethods;


@JNINamespace("")
public class ChromiumJniTestB {
    @NativeMethods
    interface Natives {
        boolean nativeFunctionBA(boolean param1, float param2);

        float functionBB(String param1);
    }
}
