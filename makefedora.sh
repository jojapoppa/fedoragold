#!/bin/sh

# option -a (android) or -i (ios)
options=$(getopt -o ai: -- "$@")
eval set -- "$options"
echo $1

#make clean

# for Alpine Linux
# sudo apk --no-cache add bsd-compat-headers
# apk add --no-cache linux-headers
# sudo apk --no-cache add libucontext
# sudo apk --no-cache add gcompat (needed for libucontext build)
# sudo apk add --no-cache --update git bash libffi-dev openssl-dev bzip2-dev readline-dev sqlite-dev build-base python python-dev
# sudo apk add --no-cache --update python3
# wget "http://distfiles.dereferenced.org/libucontext/libucontext-0.1.0.tar.xz"
# tar -xf libucontext-0.1.0.tar.xz
# cd libucontext-0.1.0
# make
#
# issue where fortify breaks build when compiling on Alpine Linux
#added to /usr/include/fortify/fortify-headers.h
# #define _FORTIFY_FN_NOTALWAYS(fn) _FORTIFY_ORIG(__USER_LABEL_PREFIX__,fn); \
#       extern __inline__ __attribute__((__gnu_inline__,__artificial__))
#then edited line 70 of /usr/include/fortify/stdio.h
# _FORTIFY_FN_NOTALWAYS(vsnprintf) int vsnprintf(char *__s, size_t __n, const char *__f,
#       __builtin_va_list __v)

if python -m platform | grep debian > /dev/null 
then
  echo debian linux build platform...
  export CC=gcc-6
  export CXX=g++-6
  export BOOST_ROOT=/home/jojapoppa/Desktop/FedDev/fedoragold/boostfedora
  export Boost_INCLUDE_DIR=/home/jojapoppa/Desktop/FedDev/fedoragold/boostfedora/include
  cp CMakeListsLinux.txt CMakeLists.txt
  make build-release
  exit
elif python -m platform | grep amzn2 > /dev/null
then
  echo centos on amazon aws platform...
  export CC=gcc
  export CXX=g++
  export BOOST_ROOT=/home/ec2-user/fedoragold/boostfedora
  export Boost_INCLUDE_DIR=/home/ec2-user/fedoragold/boostfedaora/include
  cp CMakeListsLinux.txt CMakeLists.txt
  make build-release
elif python -m platform | grep Darwin > /dev/null
then
  echo Mac OSX linux build platform...
  export MACOSX_DEPLOYMENT_TARGET=10.11
  #export BOOST_ROOT=/home/fork/fedoragold-release/boostfedora
  export BOOST_ROOT=/Users/jojapoppa/Desktop/FEDG/fedoragold-release/boostfedora_mac
  make clean
  cp CMakeListsMac.txt CMakeLists.txt
  make build-release
  exit
elif python -m platform | grep Windows > /dev/null
then
  echo Windows build platform...
  echo   TO USE TYPE: bash ..then.. source ./makefedora.sh
  rm -r build
  set CC=CL.exe
  set CXX=CL.exe
  set BOOST_ROOT=C:\Users\JP\Desktop\FEDG_BUILD\fedoragold-release\boostfedora_win
  set BOOST_LIBRARYDIR=C:\Users\JP\Desktop\FEDG_BUILD\fedoragold-release\boostfedora_win\lib
  rm CMakeCache.txt
  cp CMakeListsWindows.txt CMakeLists.txt
  mkdir build
  cd build
  cmake -G "Visual Studio 15 2017 Win64" -DBoost_INCLUDE_DIR=/Users/JP/Desktop/FEDG_BUILD/fedoragold-release/boostfedora_win/include ..
  cmake --build . --config Release
  cd ..
  pause
  exit
elif [ $1 = "-a" ];
then
  echo Android/Arm build platform...
  rm -r build
  export CC=arm-linux-gnueabi-gcc
  export CXX=arm-linux-gnueabi-g++
  export BOOST_ROOT=/home/jojapoppa/fedoragold/boostfedora_android
  export Boost_INCLUDE_DIR=/home/jojapoppa/fedoragold/boostfedora_android/include
  rm CMakeCache.txt
  cp CMakeListsAndroid.txt CMakeLists.txt
  make build-release
  exit
else
  echo non-debian platform...
  export CC=gcc
  export CXX=g++
  export BOOST_ROOT=/root/fedoragold/boostfedora
  export Boost_INCLUDE_DIR=/root/fedoragold/boostfedora/include
  cp CMakeListsLinux.txt CMakeLists.txt
  make build-release
  exit
fi

# on Windows
# can deal with this next bit using a typedef check later...
# also in src/Common/Base58.cpp ... need to comment out [[fallthrough]]; on MSVC because
# C++11 throws an error if that is present (it's C++17 syntax)... there is a compiler fix
# for this however, as c++ is supposed to ignore undefined directives like that... so I may
# not need to worry about it later
#
# set CC=CL.exe
# set CXX=CL.exe
# set BOOST_ROOT=/Users/JP/Desktop/FEDG_BUILD/fedoragold-release/boostfedora_win
# and use cmake on Windows
# remove CMakeCache.txt
# mkdir build
# cd build
# cmake -G "Visual Studio 15 2017 Win64" ..
# cmake --build . --config Release

#cp CMakeListsLinux.txt CMakeLists.txt

#built with gcc 8 (https://linuxize.com/post/how-to-install-gcc-compiler-on-ubuntu-18-04/)

#make build-debug
#make test-debug
#make test-release
#make build-release

