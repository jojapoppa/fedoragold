cmake_minimum_required(VERSION 2.8.6)

set(VERSION "0.2")
# $Format:Packaged from commit %H%nset(COMMIT %h)%nset(REFS "%d")$

MESSAGE("${INCLUDE_DIR}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s USE_PTHREADS=1") 
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_PTHREADS=1")

# --target=wasm32-wasi --sysroot=/home/jojapoppa/wasi-sysroot -I/home/jojapoppa/wasi-sysroot/include -v")
#include_directories(SYSTEM /home/jojapoppa/wasi-sysroot/include)
#set(CMAKE_REQUIRED_INCLUDES /home/jojapoppa/wasi-sysroot/include)
#set(CMAKE_THREAD_LIBS_INIT "-L/home/jojapoppa/wasi-sysroot/lib/wasm32-wasi/libpthread.a")

#set(CMAKE_THREAD_LIBS_INIT "-L/home/jojapoppa/muslclib/lib/libpthread.a")

enable_language(CXX)
set(CMAKE_HAVE_THREADS_LIBRARY ON)
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME ON)
set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})

# use this to generate documentation for our REST api's
# set(LLVM_ENABLE_SPHINX "ON")
# LLVM_BUILD_DOCS
# LLVM_ENABLE_DOXYGEN

#set(CMAKE_SYSTEM_NAME "Linux")
#set(CMAKE_CROSSCOMPILING "ON")

#set(LLVM_TARGETS_TO_BUILD "x86")
#set(LLVM_TARGET_ARCH "x86_64")
#set(LLVM_DEFAULT_TARGET_TRIPLE "x86_64-pc-win64-musl")
#set(LLVM_DEFAULT_TARGET_TRIPLE "x86_64-pc-linux-musl")
#set(LIBCXX_TARGET_TRIPLE "x86_64-pc-linux-musl")
#set(LIBCXXABI_TARGET_TRIPLE "x86_64-pc-linux-musl")
#set(LIBUNWIND_TARGET_TRIPLE "x86_64-pc-linux-musl")

#set(LIBCXXABI_USE_COMPILER_RT "ON")
#set(LIBCXXABI_USE_LLVM_UNWINDER "ON")

#set(LIBCXX_HAS_MUSL_LIBC "ON")
#set(LLVM_ENABLE_LIBCXX "ON")
#set(LLVM_ENABLE_ASSERTIONS "ON")
#set(LLVM_ENABLE_THREADS "ON") 
#set(LLVM_ENABLE_LTO "ON")
#set(LLVM_OPTIMIZED_TABLEGEN "ON")
#set(LLVM_USE_OPROFILE "ON")

#set(CMAKE_EXE_LINKER_FLAGS "-Wl,-stack_size,0x10000000,-stack_addr,0xc0000000")

#set(CMAKE_EXE_LINKER_FLAGS "-static -Wl,-Bstatic -L/usr/lib/llvm-8/lib/libc++.bca -L/usr/lib/x86_64-linux-musl/libpthread.a -Wl,-Bstatic -L/usr/lib/x86_64-linux-musl/libc.bca -Wl,-Bstatic -L/usr/lib/x86_64-linux-musl/libm.a")

#-fuse-ld=gold -use-llvm-plugin       -Wl,-plugin-opt=save-temps or emit-llvm
#                                      (creates both llvm and exe ... good)

#trying emscriptem
#set(CMAKE_EXE_LINKER_FLAGS "-flto -fuse-ld=gold -Wl,-plugin-opt=emit-llvm -static -Wl,-Bstatic -L/home/jojapoppa/muslclib/lib/libc.a -Wl,-Bstatic -L/home/jojapoppa/muslclib/lib/libm.a -Wl,-Bstatic -L/home/jojapoppa/libc++/lib/libc++.a -Wl,-Bstatic -L/home/jojapoppa/muslclib/lib/libpthread.a")

set(BOOST_HAS_THREADS ON)
set(BOOST_HAS_PTHREADS ON) 
set(BUILD_SHARED_LIBS OFF)
set(STATIC ON CACHE BOOL "Link libraries statically")

#set(BOOST_LIBRARYDIR /home/jojapoppa/fedoragold/boostfedora/lib)
#set(BOOST_THREAD_LIBRARY libboost_thread-mt.a) 
add_library(c STATIC IMPORTED)

#set(CMAKE_CXX_STANDARD 11)
#set(CMAKE_CXX_STANDARD_REQUIRED on)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_CONFIGURATION_TYPES "Debug RelWithDebInfo Release CACHE TYPE INTERNAL")

#set(CMAKE_SKIP_INSTALL_RULES ON)
#set(CMAKE_SKIP_PACKAGE_ALL_DEPENDENCY ON)
#set(CMAKE_SUPPRESS_REGENERATION ON)

enable_testing()
# copy CTestCustom.cmake to build dir to disable long running tests in 'make test'
configure_file(${CMAKE_SOURCE_DIR}/CTestCustom.cmake ${CMAKE_BINARY_DIR})

project(CryptoNote)
include_directories(include src external "${CMAKE_BINARY_DIR}/version")
enable_language(ASM)

#include_directories(SYSTEM /usr/include/x86_64-linux-musl)
include_directories(src/Platform/Linux)
#include_directories(SYSTEM /usr/include/c++/8/bits)
#include_directories(SYSTEM /usr/include)

#  if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
#    # This option has no effect in glibc version less than 2.20. 
#    # Since glibc 2.20 _BSD_SOURCE is deprecated, this macro is recomended instead
#    add_definitions("-D_DEFAULT_SOURCE -D_GNU_SOURCE")
#  endif()

#  set(ARCH native CACHE STRING "CPU to build for: -march value or default")
#  if("${ARCH}" STREQUAL "default")
#    set(ARCH_FLAG "")
#  else()
#    set(ARCH_FLAG "-march=${ARCH}")
#  endif()

  set(WARNINGS "-Wall -Wno-unused-private-field -Wno-shift-op-parentheses -Wno-bitwise-op-parentheses -Wextra -Wpointer-arith -Wvla -Wwrite-strings -Werror -Wno-error=extra -Wno-error=unused-function -Wno-error=deprecated-declarations -Wno-error=sign-compare -Wno-error=strict-aliasing -Wno-error=type-limits -Wno-unused-parameter -Wno-error=unused-variable -Wno-error=undef -Wno-error=uninitialized -Wno-error=unused-result -Wno-error=shift-count-overflow -Wno-error=unused-function -Wno-error=odr")

#  set(MINGW_FLAG "")
  set(C_WARNINGS "-Waggregate-return -Wnested-externs -Wold-style-definition -Wstrict-prototypes")
  set(CXX_WARNINGS "-Wno-reorder -Wno-missing-field-initializers")

  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 ${MINGW_FLAG} ${WARNINGS} ${C_WARNINGS} ${ARCH_FLAG}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 ${MINGW_FLAG} ${WARNINGS} ${CXX_WARNINGS} ${ARCH_FLAG}")
  set(DEBUG_FLAGS "-g3 -O0")
  set(RELEASE_FLAGS "-DNDEBUG -Wno-unused-variable") # -Ofast

    # There is a clang bug that does not allow to compile code that uses AES-NI intrinsics if -flto is enabled
    if (CMAKE_C_COMPILER_ID STREQUAL "GNU" AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_BUILD_TYPE STREQUAL "Release" AND ((CMAKE_C_COMPILER_VERSION VERSION_GREATER 4.9) OR (CMAKE_C_COMPILER_VERSION VERSION_EQUAL 4.9)))
      # On linux, to build in lto mode, check that ld.gold linker is used: 'update-alternatives --install /usr/bin/ld ld /usr/bin/ld.gold HIGHEST_PRIORITY'
      set(RELEASE_FLAGS "${RELEASE_FLAGS} -flto")
    endif()

  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${DEBUG_FLAGS}")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${DEBUG_FLAGS}")
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${RELEASE_FLAGS}")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${RELEASE_FLAGS}")

if(STATIC)
  set(Boost_USE_STATIC_LIBS ON)
  set(Boost_USE_STATIC_RUNTIME ON)
endif()

# this just shows what libraries it's looking for during find_package
set(Boost_DEBUG on)
find_package(Boost 1.65 REQUIRED COMPONENTS system filesystem thread date_time chrono regex serialization program_options)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
#  set(Boost_LIBRARIES "${Boost_LIBRARIES};rt")

set(COMMIT_ID_IN_VERSION ON CACHE BOOL "Include commit ID in version")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/version")
if (NOT COMMIT_ID_IN_VERSION)
  set(VERSION "${VERSION}-unknown")
  configure_file("src/version.h.in" "version/version.h")
  add_custom_target(version ALL)
elseif(DEFINED COMMIT)
  string(REPLACE "." "\\." VERSION_RE "${VERSION}")
  if(NOT REFS MATCHES "(\\(|, )tag: v${VERSION_RE}(\\)|, )")
    set(VERSION "${VERSION}-g${COMMIT}")
  endif()
  configure_file("src/version.h.in" "version/version.h")
  add_custom_target(version ALL)
else()
  find_package(Git QUIET)
  if(Git_FOUND OR GIT_FOUND)
    message(STATUS "Found Git: ${GIT_EXECUTABLE}")
    add_custom_target(version ALL "${CMAKE_COMMAND}" "-D" "VERSION=${VERSION}" "-D" "GIT=${GIT_EXECUTABLE}" "-D" "TO=${CMAKE_BINARY_DIR}/version/version.h" "-P" "src/version.cmake" WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
  else()
    message(STATUS "WARNING: Git was not found!")
    set(VERSION "${VERSION}-unknown")
    configure_file("src/version.h.in" "version/version.h")
    add_custom_target(version ALL)
  endif()
endif()

add_subdirectory(external)
add_subdirectory(src)
add_subdirectory(tests)
