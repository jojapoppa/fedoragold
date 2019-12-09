#!/bin/sh

#make clean

set CC=CL.exe
set CXX=CL.exe
set BOOST_ROOT=/Users/JP/Desktop/FEDG_BUILD/fedoragold-release/boostfedora_win

#using cmake
rm CMakeCache.txt
cp CMakeListsWindows.txt CMakeLists.txt

mkdir build
cd build

echo cmake -G "Visual Studio 15 2017 Win64" ..
echo cmake --build . --config Release

