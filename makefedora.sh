#!/bin/sh

# may need brew install cmake on mac...

make clean
#export BOOST_ROOT=/home/fork/fedoragold-release/boostfedora
export BOOST_ROOT=/Users/jojapoppa/Desktop/FEDG_BUILD/fedoragold-release/boostfedora_mac

make build-release
