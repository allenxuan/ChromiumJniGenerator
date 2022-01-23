//
// Created by Xuanyi Huang on 1/22/22.
//

#include <ChromiumJniTestB.h>

jboolean JNI_ChromiumJniTestB_NativeFunctionBA(JNIEnv* env, jboolean param1,
                                                      jfloat param2){
    return true;
}

jfloat JNI_ChromiumJniTestB_FunctionBB(JNIEnv* env, const
chromium::android::JavaParamRef<jstring>& param1){
    return 3888.2f;
}