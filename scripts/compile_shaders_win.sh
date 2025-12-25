#!/bin/bash

# compile shaders

mkdir -p shader/build

# simple shader
shaderc \
-f shader/v_simple.sc -o shader/build/v_simple.bin \
--platform windows --type vertex --verbose -i ./ -p s_5_0

shaderc \
-f shader/f_simple.sc -o shader/build/f_simple.bin \
--platform windows --type fragment --verbose -i ./ -p s_5_0