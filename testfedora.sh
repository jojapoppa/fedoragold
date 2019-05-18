#!/bin/sh

make clean

#export CC=gcc-7
#export CXX=g++-7

#export MACOSX_DEPLOYMENT_TARGET=10.11
#export BOOST_ROOT=/Users/jojapoppa/Desktop/FED_BUILD/fedoragold-release/boostfedora_mac
export BOOST_ROOT=/home/jojapoppa/fedora-release/boostfedora

export BOOST_INCLUDEDIR=/home/jojapoppa/fedoragold-release/boostfedora/include

//make test-release
make test-debug
