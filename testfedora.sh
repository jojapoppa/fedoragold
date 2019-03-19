#!/bin/sh

export MACOSX_DEPLOYMENT_TARGET=10.11
export BOOST_ROOT=/Users/jojapoppa/Desktop/FED_BUILD/fedoragold-release/boostfedora_mac
#export BOOST_ROOT=/home/fork/fedora-release/boostfedora

make test-release
