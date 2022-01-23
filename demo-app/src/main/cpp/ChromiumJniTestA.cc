//
// Created by Xuanyi Huang on 1/22/22.
//

#include <ChromiumJniTestA.h>

jboolean JNI_ChromiumJniTestA_FunctionAA(JNIEnv *env, jboolean param1,
                                         jfloat param2) {
    return true;
}


jlong JNI_ChromiumJniTestA_FunctionAC(JNIEnv *env, const
chromium::android::JavaParamRef<jobject> &jcaller,
                                      const chromium::android::JavaParamRef<jstring> &param1) {
    return 1000.f;
}