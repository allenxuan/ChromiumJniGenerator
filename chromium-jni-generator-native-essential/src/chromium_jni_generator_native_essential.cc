//
// Created by Xuanyi Huang on 1/20/22.
//

#include <chromium_jni_generator_native_essential.h>

namespace chromium {
    namespace android {

        JavaVM *g_jvm = NULL;

        JNIEnv *JavaRef<jobject>::SetNewLocalRef(JNIEnv *env, jobject obj) {
            if (!env) {
                env = AttachCurrentThread();
            } else {
                DDCHECK_EQ(env, AttachCurrentThread());  // Is |env| on correct thread.
            }
            if (obj)
                obj = env->NewLocalRef(obj);
            if (obj_)
                env->DeleteLocalRef(obj_);
            obj_ = obj;
            return env;
        }

        void JavaRef<jobject>::SetNewGlobalRef(JNIEnv *env, jobject obj) {
            if (!env) {
                env = AttachCurrentThread();
            } else {
                DDCHECK_EQ(env, AttachCurrentThread());  // Is |env| on correct thread.
            }
            if (obj)
                obj = env->NewGlobalRef(obj);
            if (obj_)
                env->DeleteGlobalRef(obj_);
            obj_ = obj;
        }

        void JavaRef<jobject>::ResetLocalRef(JNIEnv *env) {
            if (obj_) {
                DDCHECK_EQ(env, AttachCurrentThread());  // Is |env| on correct thread.
                env->DeleteLocalRef(obj_);
                obj_ = nullptr;
            }
        }

        void JavaRef<jobject>::ResetGlobalRef() {
            if (obj_) {
                AttachCurrentThread()->DeleteGlobalRef(obj_);
                obj_ = nullptr;
            }
        }

        jobject JavaRef<jobject>::ReleaseInternal() {
            jobject obj = obj_;
            obj_ = nullptr;
            return obj;
        }

        void DetachFromVM() {
            // Ignore the return value, if the thread is not attached, DetachCurrentThread
            // will fail. But it is ok as the native thread may never be attached.
            if (g_jvm)
                g_jvm->DetachCurrentThread();
        }

        void InitVM(JavaVM *vm) {
            DDCHECK(!g_jvm || g_jvm == vm);
            g_jvm = vm;
        }

        bool IsVMInitialized() {
            return g_jvm != NULL;
        }

        JNIEnv *AttachCurrentThread() {
            DDCHECK(g_jvm);
            JNIEnv *env = nullptr;
            jint ret = g_jvm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_2);
            if (ret == JNI_EDETACHED || !env) {
                JavaVMAttachArgs args;
                args.version = JNI_VERSION_1_2;
                args.group = nullptr;

                // 16 is the maximum size for thread names on Android.
                char thread_name[16];
                int err = prctl(PR_GET_NAME, thread_name);
                if (err < 0) {
                    // DPLOG(ERROR) << "prctl(PR_GET_NAME)";
                    args.name = nullptr;
                } else {
                    args.name = thread_name;
                }

                ret = g_jvm->AttachCurrentThread(&env, &args);
                DDCHECK_EQ(JNI_OK, ret);
            }
            return env;
        }

        JavaRef<jobject>::JavaRef(JNIEnv *env, jobject obj) : obj_(obj) {
            if (obj) {
                DDCHECK(env && env->GetObjectRefType(obj) == JNILocalRefType);
            }
        }

        ScopedJavaLocalRef<jclass> GetClassInternal(JNIEnv *env,
                                                    const char *class_name,
                                                    jobject class_loader) {
            jclass clazz;
            // if (class_loader != nullptr) {
            //     // ClassLoader.loadClass expects a classname with components separated by
            //     // dots instead of the slashes that JNIEnv::FindClass expects. The JNI
            //     // generator generates names with slashes, so we have to replace them here.
            //     // TODO(torne): move to an approach where we always use ClassLoader except
            //     // for the special case of base::android::GetClassLoader(), and change the
            //     // JNI generator to generate dot-separated names. http://crbug.com/461773
            //     size_t bufsize = strlen(class_name) + 1;
            //     char dotted_name[bufsize];
            //     memmove(dotted_name, class_name, bufsize);
            //     for (size_t i = 0; i < bufsize; ++i) {
            //         if (dotted_name[i] == '/') {
            //             dotted_name[i] = '.';
            //         }
            //     }
            //
            //     clazz = static_cast<jclass>(
            //             env->CallObjectMethod(class_loader, g_class_loader_load_class_method_id,
            //                                   ConvertUTF8ToJavaString(env, dotted_name).obj()));
            // } else {
            clazz = env->FindClass(class_name);
            // }
            // if (ClearException(env) || !clazz) {
            //     LOG(FATAL) << "Failed to find class " << class_name;
            // }
            return ScopedJavaLocalRef<jclass>(env, clazz);
        }

        ScopedJavaLocalRef<jclass> GetClass(JNIEnv *env, const char *class_name) {
            // return GetClassInternal(env, class_name, g_class_loader.Get().obj());
            return GetClassInternal(env, class_name, nullptr);
        }

// This is duplicated with LazyGetClass above because these are performance
// sensitive.
        jclass LazyGetClass(JNIEnv *env,
                            const char *class_name,
                            std::atomic<jclass> *atomic_class_id) {
            const jclass value = atomic_class_id->load(std::memory_order_acquire);
            if (value)
                return value;
            ScopedJavaGlobalRef<jclass> clazz;
            clazz.Reset(GetClass(env, class_name));
            jclass cas_result = nullptr;
            if (atomic_class_id->compare_exchange_strong(cas_result, clazz.obj(),
                                                         std::memory_order_acq_rel)) {
                // We intentionally leak the global ref since we now storing it as a raw
                // pointer in |atomic_class_id|.
                return clazz.Release();
            } else {
                return cas_result;
            }
        }

    }
}