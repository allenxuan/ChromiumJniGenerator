#!/bin/bash

#create code generation directories
rm -rf ../cpp/gen
rm -rf ../java/gen
mkdir -p ../cpp/gen
mkdir -p ../java/gen

#create code generation directories
./jni_generator.py --input_file ../java/org/chromium/chromiumjnigenerator/ChromiumJniTestA.java --output_file ../cpp/gen/ChromiumJniTestA.h --input_file ../java/org/chromium/chromiumjnigenerator/ChromiumJniTestB.java --output_file ../cpp/gen/ChromiumJniTestB.h