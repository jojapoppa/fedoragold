#define __SYSCALL_LL_E(x) \
((union { long long ll; long l[2]; }){ .ll = x }).l[0], \
((union { long long ll; long l[2]; }){ .ll = x }).l[1]
#define __SYSCALL_LL_O(x) 0, __SYSCALL_LL_E((x))

/*
    used in system/lib/libc/musl/arch/js/syscall_arch.h

    Unfortunatelly we intentionally not move syscalljs() to external function
    in searate file. That's why clang/llvm optimizer actually not even reference
    unused syscalljs_XXX() functions.
*/

#include "syscalljs.h"

static __inline long __syscall0(long n)
{
    return __syscalljs(n, 0, 0, 0, 0, 0, 0);
}

static __inline long __syscall1(long n, long a1)
{
    return __syscalljs(n, a1, 0, 0, 0, 0, 0);
}

static __inline long __syscall2(long n, long a1, long a2)
{
    return __syscalljs(n, a1, a2, 0, 0, 0, 0);
}

static __inline long __syscall3(long n, long a1, long a2, long a3)
{
    return __syscalljs(n, a1, a2, a3, 0, 0, 0);
}

static __inline long __syscall4(long n, long a1, long a2, long a3, long a4)
{
    return __syscalljs(n, a1, a2, a3, a4, 0, 0);
}

static __inline long __syscall5(long n, long a1, long a2, long a3, long a4, long a5)
{
    return __syscalljs(n, a1, a2, a3, a4, a5, 0);
}

static __inline long __syscall6(long n, long a1, long a2, long a3, long a4, long a5, long a6) {
    return __syscalljs(n, a1, a2, a3, a4, a5, a6);
}
