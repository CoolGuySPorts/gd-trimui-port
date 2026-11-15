#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <errno.h>
#include <math.h>
#include <ctype.h>
#include <dlfcn.h>

// Логирование Android
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

// Ошибки и потоки Bionic
int* __errno(void) { return __errno_location(); }

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
void __stack_chk_fail(void) { abort(); }

int __isnanf(float x) { return isnan(x); }
int __signbit(double x) { return signbit(x); }
size_t __ctype_get_mb_cur_max(void) { return MB_CUR_MAX; }

// C++ ABI с точной привязкой к LIBC_O и LIBC
void __cxa_pure_virtual_impl(void) {
    fprintf(stderr, "[FATAL] Pure virtual call!\n");
    abort();
}
__asm__(".symver __cxa_pure_virtual_impl, __cxa_pure_virtual@@LIBC_O");

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

// Проброс математических функций libm.so под версией LIBC
float acosf(float x) { return __builtin_acosf(x); }
float asinf(float x) { return __builtin_asinf(x); }
float atanf(float x) { return __builtin_atanf(x); }
float atan2f(float y, float x) { return __builtin_atan2f(y, x); }
float cosf(float x) { return __builtin_cosf(x); }
float sinf(float x) { return __builtin_sinf(x); }
float tanf(float x) { return __builtin_tanf(x); }
float sqrtf(float x) { return __builtin_sqrtf(x); }
float powf(float x, float y) { return __builtin_powf(x, y); }
float expf(float x) { return __builtin_expf(x); }
float logf(float x) { return __builtin_logf(x); }
float log10f(float x) { return __builtin_log10f(x); }
float floorf(float x) { return __builtin_floorf(x); }
float ceilf(float x) { return __builtin_ceilf(x); }
float roundf(float x) { return __builtin_roundf(x); }
float fmodf(float x, float y) { return __builtin_fmodf(x, y); }
float hypotf(float x, float y) { return __builtin_hypotf(x, y); }

double acos(double x) { return __builtin_acos(x); }
double asin(double x) { return __builtin_asin(x); }
double atan(double x) { return __builtin_atan(x); }
double atan2(double y, double x) { return __builtin_atan2(y, x); }
double cos(double x) { return __builtin_cos(x); }
double sin(double x) { return __builtin_sin(x); }
double tan(double x) { return __builtin_tan(x); }
double sqrt(double x) { return __builtin_sqrt(x); }
double pow(double x, double y) { return __builtin_pow(x, y); }
double exp(double x) { return __builtin_exp(x); }
double log(double x) { return __builtin_log(x); }
double log10(double x) { return __builtin_log10(x); }
double floor(double x) { return __builtin_floor(x); }
double ceil(double x) { return __builtin_ceil(x); }
double round(double x) { return __builtin_round(x); }
double fmod(double x, double y) { return __builtin_fmod(x, y); }
