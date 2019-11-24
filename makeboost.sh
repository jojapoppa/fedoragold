#!/bin/sh

#export MACOSX_DEPLOYMENT_TARGET=10.11

# First, from home folder do...
# note: but first on mac ONLY... ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)" ... then brew install wget --with-libressl ... then rename the download file to boost_1_65_0.tar.gz and unzip that in your home folder
# wget "https://sourceforge.net/projects/boost/files/boost/1.65.0/boost_1_65_0.tar.gz/download"
# then... tar -xvzf boost_1_65_0.tar.gz
# just put boost source 1 folder up

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
# also, edit C:\Users\JP\Desktop\FEDG_BUILD\fedoragold-release\boostfedora_win\include\boost\config\compiler>vim visualc.hpp and remove the compiler warning "Unknown compiler version - please run the configure tests and report the results"  ... it's annoying

# on Windows, these files compile to >50mb and so can't be checked in
# libboost_test_exec_monitor-vc141-mt-sgd-1_65.lib
# libboost_log_setup-vc141-mt-gd-1_65.lib
# libboost_log_setup-vc141-mt-sgd-1_65.lib
# libboost_unit_test_framework-vc141-mt-sgd-1_65.lib
# libboost_log-vc141-mt-gd-1_65.lib
# libboost_log-vc141-mt-sgd-1_65.lib
# libboost_wave-vc141-mt-sgd-1_65.lib
# libboost_wave-vc141-mt-gd-1_65.lib
# so... just remove them ... as they are not getting used at all anyways

cd ../boost_1_65_0
export BOOST_HAS_THREADS ON
export BOOST_HAS_PTHREADS ON
export Boost_USE_MULTITHREADED ON

#the boost jam file will have ... clang : emscripten ; like this...

emcc --clear-cache --clear-ports
export NO_BZIP2=1  # supplied by emscripten

then...

#address-model=64 architecture=x86

rm -rf bin.v2
sudo updatedb

# only done the first time... for emscripten
./bootstrap.sh address-model=64 --with-toolset=clang-emscripten --without-icu --prefix=/home/jojapoppa/fedoragold/boostfedora_emscripten

# non-emscripten bootstrap...
./bootstrap.sh address-model=64 --with-toolset=gcc --without-icu --prefix=/home/jojapoppa/fedoragold/boostfedora

./b2 --clean-all -n

then edit project-config.jam ... for Emscripten
# Compiler configuration. This definition will be used unless
# you already have defined some toolsets in your user-config.jam
# file.
if ! clang-emscripten in [ feature.values <toolset> ]
{
    using clang : emscripten
      : emcc -s USE_ZLIB=1 -s USE_PTHREADS=1 -s WASM=1 -s EMIT_EMSCRIPTEN_METADATA=1 -s DEMANGLE_SUPPORT=1 -s SIMD=1 -s BINARYEN=1 --memory-init-file 1
      :   <root>${EMSCRIPTEN_PATH}
          <archiver>${EMSCRIPTEN_PATH}/emar
          <ranlib>${EMSCRIPTEN_PATH}/emranlib
          <linker>${EMSCRIPTEN_PATH}/emlink
          <cxxflags>-std=c++11 -Wno-null-character -emit-llvm
    ;
}

# mac build stuff...
#./bootstrap.sh --prefix=/Users/jojapoppa/Desktop/FEDG/boost_1_65_0 macos-version=10.11

#./b2 toolset=clang cxxflags="-emit-llvm" install --prefix=/home/jojapoppa/fedoragold/boostfedora --layout=tagged --threading=multi --without-mpi --build-type=complete
#address-model=64 architecture=x86

export EMSCRIPTEN_PATH=/home/jojapoppa/emsdk/upstream/emscripten

#non emscripten does this...
./b2 toolset=gcc address-model=64 link=static variant=release boost.locale.icu=off install --prefix=/home/jojapoppa/fedoragold/boostfedora --layout=tagged --threading=multi --without-mpi --without-python --disable-icu filesystem program_options

#debian does this...
#./b2 toolset=gcc address-model=64 variant=release boost.locale.icu=off install --prefix=/home/jojapoppa/fedoragold/boostfedora --layout=tagged --threading=multi --without-mpi --without-python --disable-icu filesystem program_options

# emscripten does this...
USE_ASM=0 NO_BZIP2=1 ./b2 toolset=clang-emscripten address-model=64 link=static variant=release runtime-link=static boost.locale.icu=off install --prefix=/home/jojapoppa/fedoragold/boostfedora_emscripten --layout=tagged --threading=multi --without-mpi --without-python --disable-icu filesystem program_options

THEN for emscripten... when you're done run:
/home/jojapoppa/emsdk/upstream/bin/llvm-ranlib libboost_*.a
(replacing * with the name of each of your linked boost archives, they need to be run 1-at-a-time...)
Like this:
/home/jojapoppa/emsdk/upstream/bin/llvm-ranlib libboost_atomic-mt-s.a
/home/jojapoppa/emsdk/upstream/bin/llvm-ranlib libboost_iostreams-mt-s.a
/home/jojapoppa/emsdk/upstream/bin/llvm-ranlib libboost_stacktrace_basic-mt-s.a
/home/jojapoppa/emsdk/upstream/bin/llvm-ranlib libboost_chrono-mt-s.a
/home/jojapoppa/emsdk/upstream/bin/llvm-ranlib libboost_locale-mt-s.a
/home/jojapoppa/emsdk/upstream/bin/llvm-ranlib libboost_stacktrace_noop-mt-s.a
/home/jojapoppa/emsdk/upstream/bin/llvm-ranlib libboost_container-mt-s.a
/home/jojapoppa/emsdk/upstream/bin/llvm-ranlib libboost_log-mt-s.a
/home/jojapoppa/emsdk/upstream/bin/llvm-ranlib libboost_system-mt-s.a
/home/jojapoppa/emsdk/upstream/bin/llvm-ranlib libboost_context-mt-s.a
/home/jojapoppa/emsdk/upstream/bin/llvm-ranlib libboost_log_setup-mt-s.a
/home/jojapoppa/emsdk/upstream/bin/llvm-ranlib libboost_test_exec_monitor-mt-s.a
/home/jojapoppa/emsdk/upstream/bin/llvm-ranlib libboost_coroutine-mt-s.a
/home/jojapoppa/emsdk/upstream/bin/llvm-ranlib libboost_prg_exec_monitor-mt-s.a
/home/jojapoppa/emsdk/upstream/bin/llvm-ranlib libboost_thread-mt-s.a
/home/jojapoppa/emsdk/upstream/bin/llvm-ranlib libboost_date_time-mt-s.a
...and like this...
/home/jojapoppa/emsdk/upstream/bin/llvm-ranlib libboost_program_options-mt-s.a
/home/jojapoppa/emsdk/upstream/bin/llvm-ranlib libboost_timer-mt-s.a
/home/jojapoppa/emsdk/upstream/bin/llvm-ranlib libboost_exception-mt-s.a
/home/jojapoppa/emsdk/upstream/bin/llvm-ranlib libboost_random-mt-s.a
/home/jojapoppa/emsdk/upstream/bin/llvm-ranlib libboost_type_erasure-mt-s.a
/home/jojapoppa/emsdk/upstream/bin/llvm-ranlib libboost_fiber-mt-s.a
/home/jojapoppa/emsdk/upstream/bin/llvm-ranlib libboost_regex-mt-s.a
/home/jojapoppa/emsdk/upstream/bin/llvm-ranlib libboost_unit_test_framework-mt-s.a
/home/jojapoppa/emsdk/upstream/bin/llvm-ranlib libboost_filesystem-mt-s.a
/home/jojapoppa/emsdk/upstream/bin/llvm-ranlib libboost_serialization-mt-s.a
/home/jojapoppa/emsdk/upstream/bin/llvm-ranlib libboost_wave-mt-s.a
/home/jojapoppa/emsdk/upstream/bin/llvm-ranlib libboost_graph-mt-s.a
/home/jojapoppa/emsdk/upstream/bin/llvm-ranlib libboost_signals-mt-s.a
/home/jojapoppa/emsdk/upstream/bin/llvm-ranlib libboost_wserialization-mt-s.a

# Mac stuff...
#./b2 install --prefix=/Users/jojapoppa/Desktop/FEDG/boost_1_65_0 --layout=tagged --threading=multi --without-mpi --build-type=complete -std=libc++ -a macosx-version-min=10.11 cxxflags="-stdlib=libc++ -std=c++11 -mmacosx-version-min=10.11 -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk"

cd ..
ls
