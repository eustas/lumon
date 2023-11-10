#!/bin/bash

unamestr=$(uname)
if [[ "$unamestr" == 'Darwin' ]]; then
  export TARGET=/Volumes/RPI-RP2/
else
  export TARGET=/media/$USER/RPI-RP2
fi

cmake --build build -j 16 && cp -v ./build/lumon.uf2 $TARGET
