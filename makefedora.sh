#!/bin/sh

# may need brew install cmake on mac...

make clean

# for Alpine Linux
# sudo apk --no-cache add bsd-compat-headers
# apk add --no-cache linux-headers
# sudo apk --no-cache add gcompat (needed for libucontext build) - maybe NOT ... CAUSES ISSUES WITH ELECTRON
# sudo apk add --no-cache --update git bash libffi-dev openssl-dev bzip2-dev readline-dev sqlite-dev build-base python python-dev
# sudo apk add --no-cache --update python3
# wget "http://distfiles.dereferenced.org/libucontext/libucontext-0.1.0.tar.xz" (THIS IS COMPILED IN LOCAL NOW... in src/Platform/Linux/System)
# tar -xf libucontext-0.1.0.tar.xz
# cd libucontext-0.1.0
# ... install gold linker too...
# and ... sudo pip install wllvm
# make
#
# issue where fortify breaks build when compiling on Alpine Linux
#added to /usr/include/fortify/fortify-headers.h
# #define _FORTIFY_FN_NOTALWAYS(fn) _FORTIFY_ORIG(__USER_LABEL_PREFIX__,fn); \
#       extern __inline__ __attribute__((__gnu_inline__,__artificial__))
#then edited line 70 of /usr/include/fortify/stdio.h
# _FORTIFY_FN_NOTALWAYS(vsnprintf) int vsnprintf(char *__s, size_t __n, const char *__f,
#       __builtin_va_list __v)

#wllvm
#wllvm++
#gclang
export CC=clang
export CXX=clang++
AR="llvm-ar" 
NM="llvm-nm" 
RANLIB="llvm-ranlib" 

#export MACOSX_DEPLOYMENT_TARGET=10.11
export BOOST_ROOT=/home/jojapoppa/fedoragold/boostfedora
#export BOOST_ROOT=/Users/jojapoppa/Desktop/FEDG/fedoragold-release/boostfedora_mac

export Boost_INCLUDE_DIR=/home/jojapoppa/fedoragold-release/boostfedora/include

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

export LLVM_COMPILER=clang
make VERBOSE=1 build-release
# make build-release 
