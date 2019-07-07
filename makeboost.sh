#!/bin/sh

#export MACOSX_DEPLOYMENT_TARGET=10.11

# First, from home folder do...
# note: but first on mac ONLY... ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)" ... then brew install wget --with-libressl ... then rename the download file to boost_1_65_0.tar.gz and unzip that in your home folder
# wget "https://sourceforge.net/projects/boost/files/boost/1.65.0/boost_1_65_0.tar.gz/download"
# then... tar -xvzf boost_1_65_0.tar.gz
# just put boost source 1 folder up

# to redo boost build...
# b2 --clean-all -n

#cd /Users/jojapoppa/Desktop/FEDG_BUILD/fedoragold

# for Windows
# from the boost folder "call bootstrap.bat"
# edit project-config.jam and put this in there:
# using msvc : 14.1 : "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Tools\MSVC\14.16.27023\bin\Hostx64\x64\cl.exe" ;
# path changes with updates... will need to manually track
#
# b2.exe toolset=msvc-14.1 address-model=64 architecture=x86 runtime-link=static,shared link=static threading=multi --build-dir=.x64 --stagedir=stage_x64
# b2.exe toolset=msvc-14.1 address-model=64 architecture=x86 runtime-link=shared link=shared threading=multi --with-thread --build-dir=.x64 --stagedir=stage_x64
#
# and run those from the Visual Studio 2017 x64 Native Tools Command Prompt !!!!!!!!!!!!!!!!!!
#
# on Windows, these files compile to >50mb and so can't be checked in
# perhaps i can optimize the build a little and get throught that?
# libboost_test_exec_monitor-vc141-mt-sgd-1_65.lib
# libboost_log_setup-vc141-mt-gd-1_65.lib
# libboost_log_setup-vc141-mt-sgd-1_65.lib
# libboost_unit_test_framework-vc141-mt-sgd-1_65.lib
# libboost_log-vc141-mt-gd-1_65.lib
# libboost_log-vc141-mt-sgd-1_65.lib
# libboost_wave-vc141-mt-sgd-1_65.lib
# libboost_wave-vc141-mt-gd-1_65.lib
# so... just remove them for now... perhaps they are not getting used at all anyways

cd ../boost_1_65_0
export BOOST_HAS_THREADS ON
export BOOST_HAS_PTHREADS ON
export Boost_USE_MULTITHREADED ON

#the boost jam file will have ... clang : emscripten ;

# using full path here... just edit on each platform...
#./bootstrap.sh --with-toolset=clang --prefix=/home/jojapoppa/fedoragold/boostfedora
./bootstrap.sh --with-toolset=emscripten --prefix=/home/jojapoppa/fedoragold/boostfedora
./b2 clean
#./bootstrap.sh --prefix=/Users/jojapoppa/Desktop/FEDG/boost_1_65_0 macos-version=10.11

#./b2 toolset=clang cxxflags="-emit-llvm" install --prefix=/home/jojapoppa/fedoragold/boostfedora --layout=tagged --threading=multi --without-mpi --build-type=complete

./b2 toolset=clang install --prefix=/home/jojapoppa/fedoragold/boostfedora --layout=tagged --threading=multi --without-mpi --build-type=complete

#./b2 install --prefix=/Users/jojapoppa/Desktop/FEDG/boost_1_65_0 --layout=tagged --threading=multi --without-mpi --build-type=complete -std=libc++ -a macosx-version-min=10.11 cxxflags="-stdlib=libc++ -std=c++11 -mmacosx-version-min=10.11 -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk"

cd ..
ls
