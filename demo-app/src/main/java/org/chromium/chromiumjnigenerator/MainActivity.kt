package org.chromium.chromiumjnigenerator

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.util.Log

class MainActivity : AppCompatActivity() {
    companion object {
        init {
            System.loadLibrary("chromium_jni_generator_test_jni")
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
    }

    override fun onStart() {
        super.onStart()

        // use three components:
        // 1. chromium-jni-generator-native-essential
        // 2. chromium-jni-generator-native-gen-script
        // 3. chromium-jni-generator-jvm-annotations
        val testA = ChromiumJniTestA()
        val funAAResult = ChromiumJniTestA.nativeFunctionAA(false, 1f) //static native method
        val funACResult = ChromiumJniTestA().nativeFunctionAC("") // non-static native method

        // use four components:
        // 1. chromium-jni-generator-native-essential
        // 2. chromium-jni-generator-native-gen-script
        // 3. chromium-jni-generator-jvm-annotations
        // 4. chromium-jni-generator-jvm-compiler
        val testBJni = ChromiumJniTestBJni.get()
        val funBAResult = testBJni.nativeFunctionBA(true, 2.3f) //static native method
        val funBBResult = testBJni.functionBB("hello") //static native method

        Log.d("MainActivity", "Happy Ending")
    }
}