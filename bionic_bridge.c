#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <math.h>

int __android_log_print(int prio, const char *tag, const char *fmt, ...) {
    va_list ap;
    printf("[%s] ", tag ? tag : "GD");
    va_start(ap, fmt);
    int r = vprintf(fmt, ap);
    va_end(ap);
    printf("\n");
    return r;
}

int* __errno(void) {
    return __errno_location();
}

FILE __sF[3];
__attribute__((constructor)) static void init_sf(void) {
    __sF[0] = *stdin;
    __sF[1] = *stdout;
    __sF[2] = *stderr;
}

void android_set_abort_message(const char* msg) {
    fprintf(stderr, "[ABORT] %s\n", msg ? msg : "unknown");
}

int __isnanf(float x) {
    return isnan(x);
}

int __signbit(double x) {
    return signbit(x);
}
