#define _GNU_SOURCE
#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>

#define RTLD_LAZY 1
#define RTLD_GLOBAL 0x00100

extern void *real_dlsym(void *handle, const char *name);
__asm__(".symver real_dlsym, dlsym@GLIBC_2.17");

extern void *real_dlopen(const char *filename, int flags);
__asm__(".symver real_dlopen, dlopen@GLIBC_2.17");

static void *h_libc = NULL;
static void *h_libm = NULL;
static void *h_libpthread = NULL;
static void *h_libdl = NULL;
static void *h_librt = NULL;

void* resolve_glibc_sym(const char *name) {
    if (!h_libc) {
        h_libc = real_dlopen("libc.so.6", RTLD_LAZY | RTLD_GLOBAL);
        h_libm = real_dlopen("libm.so.6", RTLD_LAZY | RTLD_GLOBAL);
        h_libpthread = real_dlopen("libpthread.so.0", RTLD_LAZY | RTLD_GLOBAL);
        h_libdl = real_dlopen("libdl.so.2", RTLD_LAZY | RTLD_GLOBAL);
        h_librt = real_dlopen("librt.so.1", RTLD_LAZY | RTLD_GLOBAL);
    }
    void *sym = NULL;
    if (h_libc) sym = real_dlsym(h_libc, name);
    if (!sym && h_libm) sym = real_dlsym(h_libm, name);
    if (!sym && h_libpthread) sym = real_dlsym(h_libpthread, name);
    if (!sym && h_libdl) sym = real_dlsym(h_libdl, name);
    if (!sym && h_librt) sym = real_dlsym(h_librt, name);

    if (!sym && h_libc) {
        typedef int (*printf_t)(const char*, ...);
        typedef int (*fflush_t)(void*);
        static printf_t fn_p = NULL;
        static fflush_t fn_fl = NULL;
        if (!fn_p) fn_p = (printf_t)real_dlsym(h_libc, "printf");
        if (!fn_fl) fn_fl = (fflush_t)real_dlsym(h_libc, "fflush");
        if (fn_p) fn_p("[MISSING SYM] %s\n", name ? name : "null");
        if (fn_fl) fn_fl(NULL);
    }
    return sym;
}

int __android_log_vprint(int prio, const char *tag, const char *fmt, va_list ap) {
    typedef int (*vprintf_t)(const char*, va_list);
    typedef int (*printf_t)(const char*, ...);
    typedef int (*fflush_t)(void*);
    static vprintf_t fn_vp = NULL;
    static printf_t fn_p = NULL;
    static fflush_t fn_fl = NULL;
    if (!fn_vp) fn_vp = (vprintf_t)resolve_glibc_sym("vprintf");
    if (!fn_p) fn_p = (printf_t)resolve_glibc_sym("printf");
    if (!fn_fl) fn_fl = (fflush_t)resolve_glibc_sym("fflush");
    if (fn_p) fn_p("[%s] ", tag ? tag : "GD");
    int r = fn_vp ? fn_vp(fmt, ap) : 0;
    if (fn_p) fn_p("\n");
    if (fn_fl) fn_fl(NULL);
    return r;
}

int __android_log_print(int prio, const char *tag, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int r = __android_log_vprint(prio, tag, fmt, ap);
    va_end(ap);
    return r;
}

int __android_log_write(int prio, const char *tag, const char *text) {
    return __android_log_print(prio, tag, "%s", text ? text : "");
}

void __android_log_assert(const char *cond, const char *tag, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    __android_log_vprint(6, tag ? tag : "ASSERT", fmt ? fmt : (cond ? cond : "assert"), ap);
    va_end(ap);
    typedef void (*fn_t)(void);
    fn_t fn = (fn_t)resolve_glibc_sym("abort");
    if (fn) fn();
    while(1);
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

/* Потокобезопасные атомарные гарды C++ без зависимости от pthread */
int __cxa_guard_acquire(uint64_t *gv) {
    uint8_t *b = (uint8_t*)gv;
    if (__atomic_load_n(b, __ATOMIC_ACQUIRE) == 1) return 0;
    uint8_t expected = 0;
    while (!__atomic_compare_exchange_n(b, &expected, 2, 0, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
        if (expected == 1) return 0;
        expected = 0;
        __asm__ volatile("yield" ::: "memory");
    }
    return 1;
}

void __cxa_guard_release(uint64_t *gv) {
    __atomic_store_n((uint8_t*)gv, 1, __ATOMIC_RELEASE);
}

void __cxa_guard_abort(uint64_t *gv) {
    __atomic_store_n((uint8_t*)gv, 0, __ATOMIC_RELEASE);
}

__attribute__((used, aligned(16))) char __sF = {0};
uintptr_t __stack_chk_guard = 0x595e9fbd94fda766ULL;

void __stack_chk_fail(void) {
    __android_log_print(6, "STACK", "Stack smashing detected!");
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

void __cxa_pure_virtual(void) { __stack_chk_fail(); }

/* Полный набор C++ operator new / delete */
void* _Znwm(size_t sz) {
    typedef void* (*malloc_t)(size_t);
    static malloc_t fn = NULL;
    if (!fn) fn = (malloc_t)resolve_glibc_sym("malloc");
    return fn ? fn(sz ? sz : 1) : NULL;
}

void* _Znam(size_t sz) { return _Znwm(sz); }
void* _ZnwmRKSt9nothrow_t(size_t sz, const void *nt) { (void)nt; return _Znwm(sz); }
void* _ZnamRKSt9nothrow_t(size_t sz, const void *nt) { (void)nt; return _Znwm(sz); }

void _ZdlPv(void *p) {
    typedef void (*free_t)(void*);
    static free_t fn = NULL;
    if (!fn) fn = (free_t)resolve_glibc_sym("free");
    if (fn && p) fn(p);
}

void _ZdaPv(void *p) { _ZdlPv(p); }
void _ZdlPvm(void *p, size_t sz) { (void)sz; _ZdlPv(p); }
void _ZdaPvm(void *p, size_t sz) { (void)sz; _ZdlPv(p); }

/* Реализации BSD/Bionic функций, которых может не быть в glibc 2.27-2.31 */
size_t strlcpy(char *dst, const char *src, size_t dsize) {
    const char *osrc = src;
    size_t nleft = dsize;
    if (nleft != 0) {
        while (--nleft != 0) {
            if ((*dst++ = *src++) == '\0') break;
        }
    }
    if (nleft == 0) {
        if (dsize != 0) *dst = '\0';
        while (*src++);
    }
    return (src - osrc - 1);
}

size_t strlcat(char *dst, const char *src, size_t dsize) {
    const char *odst = dst;
    const char *osrc = src;
    size_t n = dsize;
    while (n-- != 0 && *dst != '\0') dst++;
    size_t dlen = dst - odst;
    n = dsize - dlen;
    if (n-- == 0) return (dlen + strlcpy((char*)"", src, 0));
    while (*src != '\0') {
        if (n != 0) { *dst++ = *src; n--; }
        src++;
    }
    *dst = '\0';
    return (dlen + (src - osrc));
}

static uint64_t rng_state = 0x123456789abcdef0ULL;
uint32_t arc4random(void) {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 7;
    rng_state ^= rng_state << 17;
    return (uint32_t)(rng_state >> 32);
}

void arc4random_buf(void *buf, size_t nbytes) {
    uint8_t *p = (uint8_t*)buf;
    for (size_t i = 0; i < nbytes; i++) p[i] = (uint8_t)arc4random();
}

uint32_t arc4random_uniform(uint32_t upper_bound) {
    if (upper_bound < 2) return 0;
    return arc4random() % upper_bound;
}

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
        "ldp x29, x30, [sp], #176\n\tcbz x16, 2f\n\tbr x16\n\t2: mov x0, #0\n\tret\n\t.popsection\n\t" \
    );

#include "syms.def"
#undef X
