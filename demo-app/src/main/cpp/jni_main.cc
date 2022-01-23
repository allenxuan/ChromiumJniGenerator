//
// Created by Xuanyi Huang on 1/21/22.
//

#include <jni.h>
#include <jni_main.h>
#include <chromium_jni_generator_native_essential.h>

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    chromium::android::InitVM(vm);
    return JNI_VERSION_1_4;
}