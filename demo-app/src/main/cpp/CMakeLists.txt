cmake_minimum_required(VERSION 3.6)
project(ChromiumJniGeneratorTest)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# 只在 Release 下开启包大小优化编译选项
# -Oz 优化包大小
# -flto 启用链接时优化
add_compile_options("$<$<CONFIG:RELEASE>:-Oz;-flto>")
# 使函数和数据分布在独立的section，这可以方便消除没有用到的函数/数据；
add_compile_options("$<$<CONFIG:RELEASE>:-ffunction-sections;-fdata-sections;-fvisibility=hidden;-fomit-frame-pointer>")
# 这些参数可以大大减小文件大小，但是不利于调试，因此不推荐使用
# ;-fno-exceptions;-fno-unwind-tables;-fno-asynchronous-unwind-tables

# -O3 产出的包体积大于等于 -O2，因此这里使用 -O2优化
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -O2 -flto")

if (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    # -Wl,--dead-strip 移除不必要的 section
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-dead_strip")
else ()
    # -Wl,--gc-sections 移除不必要的 section
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--gc-sections")
endif ()


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

