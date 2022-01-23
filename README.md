# How to use the ChromiumJniGenerator
## 1 Dependencies
### 1.1 C ++ dependencies
Copy two directories: chromium-jni-generator-native-essential and chromium-jni-generator-native-gen-script to your project
![pic1](/screenshot/1.png)
The chromium-jni-generator-neutral-essential directory contains the chromium_jni_generator_native_essential .h and chromium_jni_generator_native_essential.cc
![pic2](/screenshot/2.png)
The chromium-jni-generator-native-gen-script directory contains python scripts that generate native code
![pic3](/screenshot/3.png)
chromium_native_code_gen.sh is an example of using python scripts to generate native code
```shell
#!/bin/bash

#create code generation directories
rm -rf ./gen
mkdir -p gen/cpp
mkdir -p gen/java

#generate native code
./jni_generator.py --input_file XXXXA.java --input_file XXXXB.java --output_file gen/cpp/XXXXA.h --output_file gen/cpp/XXXXB.h
```
### 1.2 Java dependencies
```groovy
allprojects {
    repositories {
        ...
        maven { url 'https://jitpack.io' }
    }
}
```
```groovy
dependencies {
    compileOnly 'com.github.allenxuan.ChromiumJniGenerator:chromium-jni-generator-jvm-annotations:v1.0.0'
    kapt 'com.github.allenxuan.ChromiumJniGenerator:chromium-jni-generator-jvm-compiler:v1.0.0'
}
```
## 2 Java Layer Access C/C ++ Layer ( JNI )
Define native functions in the Java layer
```java
`ChromiumJniTestA.java`

@JNINamespace("test::your::name::space")
public class ChromiumJniTestA {
//Work
public static native boolean nativeFunctionAA(boolean param1, float param2);

    //Not work. "static" should be put in front of "native".
    public native static boolean nativeFunctionAB(boolean param1, float param2);

    //Work
    public native long nativeFunctionAC(String param1);

    //Not work. The function name should starts with "native".
    public native long functionAD(String param1);
}
```
Or define native methods with @NativeMethods annotations
```java
`ChromiumJniTestB.java`

@JNINamespace("test::your::name::space")
public class ChromiumJniTestB {
@NativeMethods
interface Natives {
boolean nativeFunctionBA(boolean param1, float param2);

        float functionBB(String param1);
    }
}
```
Java Annotation Processor generates auxiliary classes ChromiumJniTestBJni.java and GEN_JNI.java from @NativeMethods
```java
`ChromiumJniTestBJni.java`

class ChromiumJniTestBJni implements ChromiumJniTestB.Natives {
  @Override
  public boolean nativeFunctionBA(boolean param1, float param2) {
    return (boolean)GEN_JNI.org_chromium_chromiumjnigenerator_ChromiumJniTestB_nativeFunctionBA(param1, param2);
  }

  @Override
  public float functionBB(String param1) {
    return (float)GEN_JNI.org_chromium_chromiumjnigenerator_ChromiumJniTestB_functionBB(param1);
  }

  public static ChromiumJniTestB.Natives get() {
    return new ChromiumJniTestBJni();
  }
}
```
```java
`GEN_JNI.java`

public final class GEN_JNI {

  /**
   * org.chromium.chromiumjnigenerator.ChromiumJniTestB.functionBB
   * @param param1 (java.lang.String)
   * @return (float)
   */
  public static final native float org_chromium_chromiumjnigenerator_ChromiumJniTestB_functionBB(
      String param1);

  /**
   * org.chromium.chromiumjnigenerator.ChromiumJniTestB.nativeFunctionBA
   * @param param1 (boolean)
   * @param param2 (float)
   * @return (boolean)
   */
  public static final native boolean org_chromium_chromiumjnigenerator_ChromiumJniTestB_nativeFunctionBA(
      boolean param1, float param2);
}
```
Generate JNI functions through jni_generator.py
```shell
./jni_generator.py --input_file ../java/org/chromium/chromiumjnigenerator/ChromiumJniTestA.java --output_file ../cpp/gen/ChromiumJniTestA.h --input_file ../java/org/chromium/chromiumjnigenerator/ChromiumJniTestB.java --output_file ../cpp/gen/ChromiumJniTestB.h
```
```c++
`ChromiumJniTestA.h`

static jboolean JNI_ChromiumJniTestA_FunctionAA(JNIEnv* env, jboolean param1,
    jfloat param2);

JNI_GENERATOR_EXPORT jboolean
    Java_org_chromium_chromiumjnigenerator_ChromiumJniTestA_nativeFunctionAA(
    JNIEnv* env,
    jclass jcaller,
    jboolean param1,
    jfloat param2) {
  return JNI_ChromiumJniTestA_FunctionAA(env, param1, param2);
}

static jlong JNI_ChromiumJniTestA_FunctionAC(JNIEnv* env, const
    chromium::android::JavaParamRef<jobject>& jcaller,
    const chromium::android::JavaParamRef<jstring>& param1);

JNI_GENERATOR_EXPORT jlong Java_org_chromium_chromiumjnigenerator_ChromiumJniTestA_nativeFunctionAC(
    JNIEnv* env,
    jobject jcaller,
    jstring param1) {
  return JNI_ChromiumJniTestA_FunctionAC(env, chromium::android::JavaParamRef<jobject>(env,
      jcaller), chromium::android::JavaParamRef<jstring>(env, param1));
}
```
```c++
`ChromiumJniTestB.h`

static jboolean JNI_ChromiumJniTestB_NativeFunctionBA(JNIEnv* env, jboolean param1,
    jfloat param2);

JNI_GENERATOR_EXPORT jboolean
    Java_org_chromium_base_natives_GEN_1JNI_org_1chromium_1chromiumjnigenerator_1ChromiumJniTestB_1nativeFunctionBA(
    JNIEnv* env,
    jclass jcaller,
    jboolean param1,
    jfloat param2) {
  return JNI_ChromiumJniTestB_NativeFunctionBA(env, param1, param2);
}

static jfloat JNI_ChromiumJniTestB_FunctionBB(JNIEnv* env, const
    chromium::android::JavaParamRef<jstring>& param1);

JNI_GENERATOR_EXPORT jfloat
    Java_org_chromium_base_natives_GEN_1JNI_org_1chromium_1chromiumjnigenerator_1ChromiumJniTestB_1functionBB(
    JNIEnv* env,
    jclass jcaller,
    jstring param1) {
  return JNI_ChromiumJniTestB_FunctionBB(env, chromium::android::JavaParamRef<jstring>(env,
      param1));
}
```
Create ChromiumJniTestA.cc and ChromiumJniTestB.cc to implement ChromiumJniTestA.h and ChromiumJniTestB.h
```c++
`ChromiumJniTestA.cc`

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
```
```c++
`ChromiumJniTestB.cc`

#include <ChromiumJniTestB.h>

jboolean JNI_ChromiumJniTestB_NativeFunctionBA(JNIEnv* env, jboolean param1,
                                                      jfloat param2){
    return true;
}

jfloat JNI_ChromiumJniTestB_FunctionBB(JNIEnv* env, const
chromium::android::JavaParamRef<jstring>& param1){
    return 3888.2f;
}
```
Remember to add the chromium-jni-generator-native-essential directory to the compilation process.
```cmake
add_library(chromium_jni_generator_test_jni SHARED
        jni_main.cc
        ChromiumJniTestA.cc
        ChromiumJniTestB.cc)
target_include_directories(chromium_jni_generator_test_jni
        PRIVATE ./
        PRIVATE ./gen)

add_subdirectory(../chromium-jni-generator-native-essential chromium_jni_generator_native_essential)

target_link_libraries(chromium_jni_generator_test_jni
        PRIVATE chromium_jni_generator_native_essential)
```
## 3 C/C ++ Layer Access to Java Layer
Add @CalledByNative annotations to Java methods
```java
`ChromiumJniTestA.java`

@JNINamespace("test::your::name::space")
public class ChromiumJniTestA {
    ...
    ...

    @CalledByNative
    public static float functionAE(String param1, boolean param2){
        return 9.7f;
    }
}
```
Generate C/C ++ code through jni_generator.py
```c++
`ChromiumJniTestA.h`

JNI_REGISTRATION_EXPORT extern const char
    kClassPath_org_chromium_chromiumjnigenerator_ChromiumJniTestA[];
const char kClassPath_org_chromium_chromiumjnigenerator_ChromiumJniTestA[] =
    "org/chromium/chromiumjnigenerator/ChromiumJniTestA";
// Leaking this jclass as we cannot use LazyInstance from some threads.
JNI_REGISTRATION_EXPORT std::atomic<jclass>
    g_org_chromium_chromiumjnigenerator_ChromiumJniTestA_clazz(nullptr);
#ifndef org_chromium_chromiumjnigenerator_ChromiumJniTestA_clazz_defined
#define org_chromium_chromiumjnigenerator_ChromiumJniTestA_clazz_defined
inline jclass org_chromium_chromiumjnigenerator_ChromiumJniTestA_clazz(JNIEnv* env) {
  return chromium::android::LazyGetClass(env,
      kClassPath_org_chromium_chromiumjnigenerator_ChromiumJniTestA,
      &g_org_chromium_chromiumjnigenerator_ChromiumJniTestA_clazz);
}
#endif

static std::atomic<jmethodID>
    g_org_chromium_chromiumjnigenerator_ChromiumJniTestA_functionAE(nullptr);
static jfloat Java_ChromiumJniTestA_functionAE(JNIEnv* env, const
    chromium::android::JavaRef<jstring>& param1,
    jboolean param2) {
  jclass clazz = org_chromium_chromiumjnigenerator_ChromiumJniTestA_clazz(env);
  CHECK_CLAZZ(env, clazz,
      org_chromium_chromiumjnigenerator_ChromiumJniTestA_clazz(env), 0);

  chromium::android::JniJavaCallContextChecked call_context;
  call_context.Init<
      chromium::android::MethodID::TYPE_STATIC>(
          env,
          clazz,
          "functionAE",
          "(Ljava/lang/String;Z)F",
          &g_org_chromium_chromiumjnigenerator_ChromiumJniTestA_functionAE);

  jfloat ret =
      env->CallStaticFloatMethod(clazz,
          call_context.base.method_id, param1.obj(), param2);
  return ret;
}
```
