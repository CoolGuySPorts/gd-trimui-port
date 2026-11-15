#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <errno.h>
#include <math.h>
#include <ctype.h>

// 1. Логирование Android
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

// 2. Ошибки Bionic -> Glibc
int* __errno(void) {
    return __errno_location();
}

// 3. Потоки ввода-вывода Android (__sF)
FILE __sF[3];
__attribute__((constructor)) static void init_sf(void) {
    __sF[0] = *stdin;
    __sF[1] = *stdout;
    __sF[2] = *stderr;
}

// 4. Аварийные остановки и защита стека
void android_set_abort_message(const char* msg) {
    fprintf(stderr, "[ABORT] %s\n", msg ? msg : "unknown");
}

uintptr_t __stack_chk_guard = 0x595e9fbd94fda766ULL;
void __stack_chk_fail(void) {
    fprintf(stderr, "[FATAL] Stack smashing detected!\n");
    abort();
}

// 5. Математика и локаль из sys_imports.txt
int __isnanf(float x) {
    return isnan(x);
}

int __signbit(double x) {
    return signbit(x);
}

size_t __ctype_get_mb_cur_max(void) {
    return MB_CUR_MAX;
}

// 6. C++ ABI функции (из sys_imports.txt и ошибки libfmod.so)
void __cxa_pure_virtual(void) {
    fprintf(stderr, "[FATAL] Pure virtual function called!\n");
    abort();
}

void __cxa_finalize(void *d) {
    // Безопасная заглушка при выгрузке .so
}

extern int __cxa_atexit(void (*func)(void *), void *arg, void *dso_handle);
int __cxa_thread_atexit_impl(void (*func)(void *), void *arg, void *dso_handle) {
    return __cxa_atexit(func, arg, dso_handle);
}
