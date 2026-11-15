#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>

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

// 4. Сообщение об аварийной остановке
void android_set_abort_message(const char* msg) {
    fprintf(stderr, "[ABORT] %s\n", msg ? msg : "unknown");
}
