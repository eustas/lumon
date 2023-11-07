#!/bin/bash

export MY_ROOT=`pwd`
export PICO_SDK_PATH=`realpath ../pico-sdk`
export PICO_BOARD=pico_w

# cd $PICO_SDK_PATH/..
# git clone https://github.com/raspberrypi/pico-sdk.git

cd $PICO_SDK_PATH
git submodule update --init

cd $MY_ROOT
rm -rf ./build
cmake -B build . -DCMAKE_EXPORT_COMPILE_COMMANDS=1
