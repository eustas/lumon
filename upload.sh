#!/bin/bash

#export TARGET=/Volumes/RPI-RP2/
export TARGET=/media/$USER/RPI-RP2

cmake --build build -j 16 && cp -v ./build/lumon.uf2 $TARGET
