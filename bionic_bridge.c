#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <errno.h>
#include <math.h>
#include <ctype.h>
#include <dlfcn.h>

int __android_log_print(int prio, const char *tag, const char *fmt, ...) {
    va_list ap;
    printf("[%s] ", tag ? tag : "GD");
    va_start(ap, fmt);
    int r = vprintf(fmt, ap);
    va_end(ap);
    printf("\n");
    return r;
}

int __android_log_write(int prio, const char *tag, const char *text) {
    printf("[%s] %s\n", tag ? tag : "GD", text ? text : "");
    return 0;
}

int* __errno(void) { return __errno_location(); }

struct { FILE f0, f1, f2; } __sF;
__attribute__((constructor)) static void init_sf(void) {
    __sF.f0 = *stdin;
    __sF.f1 = *stdout;
    __sF.f2 = *stderr;
}

void android_set_abort_message(const char* msg) {
    fprintf(stderr, "[ABORT] %s\n", msg ? msg : "unknown");
}

uintptr_t __stack_chk_guard = 0x595e9fbd94fda766ULL;
void __stack_chk_fail(void) {
    typedef void (*fn_t)(void);
    fn_t fn = (fn_t)dlsym(RTLD_NEXT, "abort");
    if (fn) fn();
    while(1);
}

int __isnanf(float x) { return isnan(x); }
int __signbit(double x) { return signbit(x); }
size_t __ctype_get_mb_cur_max(void) { return MB_CUR_MAX; }

void __cxa_pure_virtual_impl(void) {
    fprintf(stderr, "[FATAL] Pure virtual call!\n");
    __stack_chk_fail();
}
__asm__(".symver __cxa_pure_virtual_impl, __cxa_pure_virtual@@LIBC_O");

void __cxa_finalize(void *d) {
    typedef void (*fn_t)(void*);
    static fn_t fn = NULL;
    if (!fn) fn = (fn_t)dlsym(RTLD_NEXT, "__cxa_finalize");
    if (fn) fn(d);
}

int __cxa_atexit(void (*func)(void *), void *arg, void *dso_handle) {
    typedef int (*fn_t)(void (*)(void *), void *, void *);
    static fn_t fn = NULL;
    if (!fn) fn = (fn_t)dlsym(RTLD_NEXT, "__cxa_atexit");
    return fn ? fn(func, arg, dso_handle) : 0;
}

int __cxa_thread_atexit_impl(void (*func)(void *), void *arg, void *dso_handle) {
    return __cxa_atexit(func, arg, dso_handle);
}

void* resolve_glibc_sym(const char *name) {
    void *sym = dlsym(RTLD_NEXT, name);
    if (!sym) sym = dlsym(RTLD_DEFAULT, name);
    if (!sym) fprintf(stderr, "[WARN] Unresolved glibc sym: %s\n", name);
    return sym;
}

#define X(sym) \
    __asm__( \
        ".pushsection .bss\n\t.align 3\n\t.Lptr_" #sym ": .quad 0\n\t.popsection\n\t" \
        ".pushsection .rodata\n\t.Lstr_" #sym ": .asciz \"" #sym "\"\n\t.popsection\n\t" \
        ".pushsection .text\n\t.global " #sym "\n\t.type " #sym ", %function\n\t" \
        #sym ":\n\t" \
        "adrp x16, .Lptr_" #sym "\n\tldr x16, [x16, :lo12:.Lptr_" #sym "]\n\tcbz x16, 1f\n\tbr x16\n\t" \
        "1:\n\tstp x29, x30, [sp, #-176]!\n\tmov x29, sp\n\t" \
        "stp x0, x1, [sp, #16]\n\tstp x2, x3, [sp, #32]\n\tstp x4, x5, [sp, #48]\n\tstp x6, x7, [sp, #64]\n\tstr x8, [sp, #80]\n\t" \
        "stp d0, d1, [sp, #96]\n\tstp d2, d3, [sp, #112]\n\tstp d4, d5, [sp, #128]\n\tstp d6, d7, [sp, #144]\n\t" \
        "adrp x0, .Lstr_" #sym "\n\tadd x0, x0, :lo12:.Lstr_" #sym "\n\t" \
        "bl resolve_glibc_sym\n\tmov x16, x0\n\t" \
        "adrp x17, .Lptr_" #sym "\n\tstr x16, [x17, :lo12:.Lptr_" #sym "]\n\t" \
        "ldp x0, x1, [sp, #16]\n\tldp x2, x3, [sp, #32]\n\tldp x4, x5, [sp, #48]\n\tldp x6, x7, [sp, #64]\n\tldr x8, [sp, #80]\n\t" \
        "ldp d0, d1, [sp, #96]\n\tldp d2, d3, [sp, #112]\n\tldp d4, d5, [sp, #128]\n\tldp d6, d7, [sp, #144]\n\t" \
        "ldp x29, x30, [sp], #176\n\tcbz x16, 2f\n\tbr x16\n\t2: ret\n\t.popsection\n\t" \
    );

#include "syms.def"
#undef X
