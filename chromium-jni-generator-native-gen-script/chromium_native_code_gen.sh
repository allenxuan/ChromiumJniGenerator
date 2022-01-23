#!/bin/bash

#create code generation directories
rm -rf ./gen
mkdir -p gen/cpp
mkdir -p gen/java

#generate native code
jni_generator.py --input_file XXXXA.java --input_file XXXXB.java --output_file XXXXA.cc --output_file XXXXB.cc