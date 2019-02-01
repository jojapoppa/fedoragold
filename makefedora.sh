#!/bin/sh

# may need brew install cmake on mac...

make clean
#export BOOST_ROOT=/home/fork/fedoragold-release/boostfedora
export BOOST_ROOT=/Users/jojapoppa/Desktop/FEDG/fedoragold-release/boostfedora_mac

# on Windows
# need to manually alter flag for #define SPH_AMD64_MSVC 1 (and turn off the GCC one...)
# ... at src/crypto/sph_types.h
# also in src/Common/Base58.cpp ... need to comment out [[fallthrough]]; on MSVC because
# C++11 throws an error if that is present (it's C++17 syntax)... there is a compiler fix
# for this however, as c++ is supposed to ignore undefined directives like that... so I may
# not need to worry about it later
#
# set BOOST_ROOT=/Users/JP/Desktop/FEDG_BUILD/fedoragold-release/boostfedora_win
# and use cmake on Windows
# remove CMakeCache.txt
# mkdir build
# cd build
# cmake -G "Visual Studio 15 2017 Win64" ..
# cmake --build . --config Release

make build-release
