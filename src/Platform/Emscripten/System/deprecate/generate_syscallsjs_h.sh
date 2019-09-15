#!/bin/sh

#    runs from...
#    system/lib/libc/musl/arch/js/

#    The main purpose of such obscure thing - is to allow
#    linker not to include unused syscall implementations code.
#
#    If we define single entry point for all syscalls,
#    linker will unable to throw some parts of that big function
#    even if we split it to subfunctions.
#
#    Next obscure thing - pass extra zeroes for unused arguments.
#    Since we have no table that specify how many arguments should be
#    passed to each syscall number... We use fact that we may call JS
#    function with more arguments than actually defined.
#
#    As an alternative, we may define 6 very big switch..case blocks
#    but what is the profit?
{
    echo "// Please don't change this file. It's autogoenrated"
    echo
    echo '#include <emscripten.h>'
    echo
    for i in `seq 0 400`; do
        echo "extern long syscalljs_$i(long a1, long a2, long a3, long a4, long a5, long a6);"
    done
    echo
    echo 'static __inline __syscalljs(long n, long a1, long a2, long a3, long a4, long a5, long a6) {'
    echo '  switch(n) {'
    for i in `seq 0 400`; do
        echo "    case $i: return syscalljs_$i(a1, a2, a3, a4, a5, a6);"
    done
    echo '  }'
    echo "  EM_ASM({Module.printErr('Someone try to call unknown syscall number', \$0); abort(-1); }, n);"
    echo '  return -1; // Will never happen'
    echo '}'
} > syscalljs.h
