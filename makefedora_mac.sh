#!/bin/sh

# may need brew install cmake on mac...

make clean

export MACOSX_DEPLOYMENT_TARGET=10.12
#export BOOST_ROOT=/home/fork/fedoragold-release/boostfedora
export BOOST_ROOT=/Users/jojapoppa/Desktop/FEDG/fedoragold-release/boostfedora_mac

cp CMakeListsMac.txt CMakeLists.txt

make build-release
