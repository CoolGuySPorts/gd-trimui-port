#define _GNU_SOURCE
#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>

#define RTLD_DEFAULT ((void *) 0)

extern void *real_dlsym(void *handle, const char *name);
__asm__(".symver real_dlsym, dlsym@GLIBC_2.17");

void* resolve_glibc_sym(const char *name) {
    return real_dlsym(RTLD_DEFAULT, name);
}

int __android_log_print(int prio, const char *tag, const char *fmt, ...) {
    typedef int (*vprintf_t)(const char*, va_list);
    typedef int (*printf_t)(const char*, ...);
    static vprintf_t fn_vp = NULL;
    static printf_t fn_p = NULL;
    if (!fn_vp) fn_vp = (vprintf_t)resolve_glibc_sym("vprintf");
    if (!fn_p) fn_p = (printf_t)resolve_glibc_sym("printf");
    if (fn_p) fn_p("[%s] ", tag ? tag : "GD");
    va_list ap;
    va_start(ap, fmt);
    int r = fn_vp ? fn_vp(fmt, ap) : 0;
    va_end(ap);
    if (fn_p) fn_p("\n");
    return r;
}

int __android_log_write(int prio, const char *tag, const char *text) {
    return __android_log_print(prio, tag, "%s", text ? text : "");
}

int* __errno(void) {
    typedef int* (*fn_t)(void);
    static fn_t fn = NULL;
    if (!fn) fn = (fn_t)resolve_glibc_sym("__errno_location");
    return fn();
}

int gettid(void) {
    typedef long (*sys_t)(long, ...);
    static sys_t fn = NULL;
    if (!fn) fn = (sys_t)resolve_glibc_sym("syscall");
    return fn ? (int)fn(178) : 1000;
}

void __FD_SET_chk(int fd, void *set, size_t set_size) {
    if (fd >= 0 && (size_t)(fd / 8) < set_size) {
        ((uint8_t*)set)[fd / 8] |= (1U << (fd % 8));
    }
}

void __assert2(const char *file, int line, const char *func, const char *expr) {
    __android_log_print(6, "ASSERT", "%s:%d %s: %s", file ? file : "?", line, func ? func : "?", expr ? expr : "?");
    typedef void (*fn_t)(void);
    fn_t fn = (fn_t)resolve_glibc_sym("abort");
    if (fn) fn();
    while(1);
}

int cxa_guard_acq_impl(uint64_t *gv) {
    if (*(uint8_t*)gv != 0) return 0;
    return 1;
}
__asm__(".symver cxa_guard_acq_impl, __cxa_guard_acquire@@LIBC_O");

void cxa_guard_rel_impl(uint64_t *gv) {
    *(uint8_t*)gv = 1;
}
__asm__(".symver cxa_guard_rel_impl, __cxa_guard_release@@LIBC_O");

__attribute__((used)) char __sF = {0};
uintptr_t __stack_chk_guard = 0x595e9fbd94fda766ULL;

void __stack_chk_fail(void) {
    typedef void (*fn_t)(void);
    fn_t fn = (fn_t)resolve_glibc_sym("abort");
    if (fn) fn();
    while(1);
}

void android_set_abort_message(const char* msg) {
    __android_log_print(6, "ABORT", "%s", msg ? msg : "unknown");
}

int __isnanf(float x) { return __builtin_isnan(x); }
int __signbit(double x) { return __builtin_signbit(x); }
size_t __ctype_get_mb_cur_max(void) { return 1; }

void __cxa_pure_virtual_impl(void) { __stack_chk_fail(); }
__asm__(".symver __cxa_pure_virtual_impl, __cxa_pure_virtual@@LIBC_O");

void* op_new_impl(size_t sz) {
    typedef void* (*malloc_t)(size_t);
    static malloc_t fn = NULL;
    if (!fn) fn = (malloc_t)resolve_glibc_sym("malloc");
    return fn ? fn(sz ? sz : 1) : NULL;
}
__asm__(".symver op_new_impl, _Znwm@@LIBC_O");

void* op_new_arr_impl(size_t sz) { return op_new_impl(sz); }
__asm__(".symver op_new_arr_impl, _Znam@@LIBC_O");

void op_del_impl(void *p) {
    typedef void (*free_t)(void*);
    static free_t fn = NULL;
    if (!fn) fn = (free_t)resolve_glibc_sym("free");
    if (fn && p) fn(p);
}
__asm__(".symver op_del_impl, _ZdlPv@@LIBC_O");

void op_del_arr_impl(void *p) { op_del_impl(p); }
__asm__(".symver op_del_arr_impl, _ZdaPv@@LIBC_O");

void __cxa_finalize(void *d) {
    typedef void (*fn_t)(void*);
    static fn_t fn = NULL;
    if (!fn) fn = (fn_t)resolve_glibc_sym("__cxa_finalize");
    if (fn) fn(d);
}

int __cxa_atexit(void (*func)(void *), void *arg, void *dso_handle) {
    typedef int (*fn_t)(void (*)(void *), void *, void *);
    static fn_t fn = NULL;
    if (!fn) fn = (fn_t)resolve_glibc_sym("__cxa_atexit");
    return fn ? fn(func, arg, dso_handle) : 0;
}

int __cxa_thread_atexit_impl(void (*func)(void *), void *arg, void *dso_handle) {
    return __cxa_atexit(func, arg, dso_handle);
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
