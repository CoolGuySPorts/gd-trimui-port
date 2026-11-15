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

int* __errno(void) {
    return __errno_location();
}

FILE __sF;
__attribute__((constructor)) static void init_sf(void) {
    __sF = *stdin;
    __sF = *stdout;
    __sF = *stderr;
}

void android_set_abort_message(const char* msg) {
    fprintf(stderr, "[ABORT] %s\n", msg ? msg : "unknown");
}

uintptr_t __stack_chk_guard = 0x595e9fbd94fda766ULL;
void __stack_chk_fail(void) {
    fprintf(stderr, "[FATAL] Stack smashing detected!\n");
    abort();
}

int __isnanf(float x) {
    return isnan(x);
}

int __signbit(double x) {
    return signbit(x);
}

size_t __ctype_get_mb_cur_max(void) {
    return MB_CUR_MAX;
}

// Собственные определения C++ ABI с привязкой к версиям LIBC / LIBC_O
void __cxa_pure_virtual(void) {
    fprintf(stderr, "[FATAL] Pure virtual function called!\n");
    abort();
}

void __cxa_finalize(void *d) {
    typedef void (*fn_t)(void*);
    static fn_t real_fn = NULL;
    if (!real_fn) real_fn = (fn_t)dlsym(RTLD_NEXT, "__cxa_finalize");
    if (real_fn) real_fn(d);
}

int __cxa_atexit(void (*func)(void *), void *arg, void *dso_handle) {
    typedef int (*fn_t)(void (*)(void *), void *, void *);
    static fn_t real_fn = NULL;
    if (!real_fn) real_fn = (fn_t)dlsym(RTLD_NEXT, "__cxa_atexit");
    return real_fn ? real_fn(func, arg, dso_handle) : 0;
}

int __cxa_thread_atexit_impl(void (*func)(void *), void *arg, void *dso_handle) {
    return __cxa_atexit(func, arg, dso_handle);
}
