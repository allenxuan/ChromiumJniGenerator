cmake_minimum_required(VERSION 3.6)

add_library(chromium_jni_generator_native_essential STATIC
        src/chromium_jni_generator_native_essential.cc
        )

target_include_directories(chromium_jni_generator_native_essential
        PUBLIC include)