#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <errno.h>
#include <math.h>
#include <ctype.h>
#include <dlfcn.h>

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

// 2. Ошибки и стандартные потоки Bionic (__sF из 3 элементов!)
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

// 3. C++ ABI (LIBC_O и LIBC)
void __cxa_pure_virtual_impl(void) {
    fprintf(stderr, "[FATAL] Pure virtual call!\n");
    abort();
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

// 4. Универсальный ARM64-трамплин: перенаправляет любой вызов @LIBC в системную glibc
void* resolve_glibc_sym(const char *name) {
    void *sym = dlsym(RTLD_NEXT, name);
    if (!sym) sym = dlsym(RTLD_DEFAULT, name);
    if (!sym) fprintf(stderr, "[WARN] Unresolved glibc sym: %s\n", name);
    return sym;
}

#define BRIDGE_SYM(sym) \
    static void *ptr_##sym = NULL; \
    static const char name_##sym[] = #sym; \
    __attribute__((naked)) void sym(void) { \
        __asm__ volatile ( \
            "adrp x16, %0\n\t" \
            "ldr x16, [x16, :lo12:%0]\n\t" \
            "cbz x16, 1f\n\t" \
            "br x16\n\t" \
            "1:\n\t" \
            "stp x29, x30, [sp, #-112]!\n\t" \
            "mov x29, sp\n\t" \
            "stp x0, x1, [sp, #16]\n\t" \
            "stp x2, x3, [sp, #32]\n\t" \
            "stp x4, x5, [sp, #48]\n\t" \
            "stp x6, x7, [sp, #64]\n\t" \
            "str x8, [sp, #80]\n\t" \
            "stp d0, d1, [sp, #96]\n\t" \
            "adrp x0, %1\n\t" \
            "add x0, x0, :lo12:%1\n\t" \
            "bl resolve_glibc_sym\n\t" \
            "mov x16, x0\n\t" \
            "adrp x17, %0\n\t" \
            "str x16, [x17, :lo12:%0]\n\t" \
            "ldp x0, x1, [sp, #16]\n\t" \
            "ldp x2, x3, [sp, #32]\n\t" \
            "ldp x4, x5, [sp, #48]\n\t" \
            "ldp x6, x7, [sp, #64]\n\t" \
            "ldr x8, [sp, #80]\n\t" \
            "ldp d0, d1, [sp, #96]\n\t" \
            "ldp x29, x30, [sp], #112\n\t" \
            "cbz x16, 2f\n\t" \
            "br x16\n\t" \
            "2: ret\n\t" \
            : : "S"(ptr_##sym), "S"(name_##sym) \
        ); \
    }

BRIDGE_SYM(abort) BRIDGE_SYM(abs) BRIDGE_SYM(accept) BRIDGE_SYM(access)
BRIDGE_SYM(acos) BRIDGE_SYM(acosf) BRIDGE_SYM(alarm) BRIDGE_SYM(asinf)
BRIDGE_SYM(atan2) BRIDGE_SYM(atan2f) BRIDGE_SYM(atanf) BRIDGE_SYM(atof)
BRIDGE_SYM(atoi) BRIDGE_SYM(atoll) BRIDGE_SYM(basename) BRIDGE_SYM(bind)
BRIDGE_SYM(bsearch) BRIDGE_SYM(btowc) BRIDGE_SYM(calloc) BRIDGE_SYM(chmod)
BRIDGE_SYM(clock) BRIDGE_SYM(clock_gettime) BRIDGE_SYM(close) BRIDGE_SYM(closedir)
BRIDGE_SYM(closelog) BRIDGE_SYM(connect) BRIDGE_SYM(cos) BRIDGE_SYM(cosf)
BRIDGE_SYM(difftime) BRIDGE_SYM(dl_iterate_phdr) BRIDGE_SYM(dlclose) BRIDGE_SYM(dlerror)
BRIDGE_SYM(dlopen) BRIDGE_SYM(dlsym) BRIDGE_SYM(dup2) BRIDGE_SYM(execl)
BRIDGE_SYM(exit) BRIDGE_SYM(exp) BRIDGE_SYM(expf) BRIDGE_SYM(fclose)
BRIDGE_SYM(fcntl) BRIDGE_SYM(fdopen) BRIDGE_SYM(feof) BRIDGE_SYM(ferror)
BRIDGE_SYM(fflush) BRIDGE_SYM(fgets) BRIDGE_SYM(fileno) BRIDGE_SYM(fmax)
BRIDGE_SYM(fmin) BRIDGE_SYM(fmod) BRIDGE_SYM(fmodf) BRIDGE_SYM(fopen)
BRIDGE_SYM(fork) BRIDGE_SYM(fprintf) BRIDGE_SYM(fputc) BRIDGE_SYM(fputs)
BRIDGE_SYM(fread) BRIDGE_SYM(free) BRIDGE_SYM(freeaddrinfo) BRIDGE_SYM(freelocale)
BRIDGE_SYM(frexp) BRIDGE_SYM(fseek) BRIDGE_SYM(fseeko) BRIDGE_SYM(fstat)
BRIDGE_SYM(ftell) BRIDGE_SYM(ftello) BRIDGE_SYM(fwrite) BRIDGE_SYM(gai_strerror)
BRIDGE_SYM(getaddrinfo) BRIDGE_SYM(getauxval) BRIDGE_SYM(getc) BRIDGE_SYM(getegid)
BRIDGE_SYM(getenv) BRIDGE_SYM(geteuid) BRIDGE_SYM(getgid) BRIDGE_SYM(gethostbyname)
BRIDGE_SYM(gethostname) BRIDGE_SYM(getnameinfo) BRIDGE_SYM(getpeername) BRIDGE_SYM(getpid)
BRIDGE_SYM(getpwuid) BRIDGE_SYM(getpwuid_r) BRIDGE_SYM(getsockname) BRIDGE_SYM(getsockopt)
BRIDGE_SYM(gettimeofday) BRIDGE_SYM(getuid) BRIDGE_SYM(gmtime) BRIDGE_SYM(gmtime_r)
BRIDGE_SYM(if_nametoindex) BRIDGE_SYM(inet_ntop) BRIDGE_SYM(inet_pton) BRIDGE_SYM(initgroups)
BRIDGE_SYM(ioctl) BRIDGE_SYM(isalnum) BRIDGE_SYM(isalpha) BRIDGE_SYM(isascii)
BRIDGE_SYM(isdigit_l) BRIDGE_SYM(isgraph) BRIDGE_SYM(islower) BRIDGE_SYM(islower_l)
BRIDGE_SYM(isprint) BRIDGE_SYM(isspace) BRIDGE_SYM(isupper) BRIDGE_SYM(isupper_l)
BRIDGE_SYM(iswalpha_l) BRIDGE_SYM(iswblank_l) BRIDGE_SYM(iswcntrl_l) BRIDGE_SYM(iswdigit_l)
BRIDGE_SYM(iswlower_l) BRIDGE_SYM(iswprint_l) BRIDGE_SYM(iswpunct_l) BRIDGE_SYM(iswspace_l)
BRIDGE_SYM(iswupper_l) BRIDGE_SYM(iswxdigit_l) BRIDGE_SYM(isxdigit) BRIDGE_SYM(isxdigit_l)
BRIDGE_SYM(kill) BRIDGE_SYM(listen) BRIDGE_SYM(localeconv) BRIDGE_SYM(localtime)
BRIDGE_SYM(log10) BRIDGE_SYM(log) BRIDGE_SYM(logf) BRIDGE_SYM(longjmp)
BRIDGE_SYM(lround) BRIDGE_SYM(lseek) BRIDGE_SYM(madvise) BRIDGE_SYM(malloc)
BRIDGE_SYM(mbrlen) BRIDGE_SYM(mbrtowc) BRIDGE_SYM(mbsnrtowcs) BRIDGE_SYM(mbsrtowcs)
BRIDGE_SYM(mbtowc) BRIDGE_SYM(memchr) BRIDGE_SYM(memcmp) BRIDGE_SYM(memcpy)
BRIDGE_SYM(memmove) BRIDGE_SYM(memrchr) BRIDGE_SYM(memset) BRIDGE_SYM(mkdir)
BRIDGE_SYM(mlock) BRIDGE_SYM(mmap) BRIDGE_SYM(modf) BRIDGE_SYM(mprotect)
BRIDGE_SYM(munmap) BRIDGE_SYM(nanosleep) BRIDGE_SYM(newlocale) BRIDGE_SYM(open)
BRIDGE_SYM(opendir) BRIDGE_SYM(openlog) BRIDGE_SYM(pipe) BRIDGE_SYM(poll)
BRIDGE_SYM(pow) BRIDGE_SYM(powf) BRIDGE_SYM(printf) BRIDGE_SYM(pthread_attr_init)
BRIDGE_SYM(pthread_cond_broadcast) BRIDGE_SYM(pthread_cond_destroy) BRIDGE_SYM(pthread_cond_init)
BRIDGE_SYM(pthread_cond_signal) BRIDGE_SYM(pthread_cond_timedwait) BRIDGE_SYM(pthread_cond_wait)
BRIDGE_SYM(pthread_create) BRIDGE_SYM(pthread_detach) BRIDGE_SYM(pthread_equal)
BRIDGE_SYM(pthread_getspecific) BRIDGE_SYM(pthread_join) BRIDGE_SYM(pthread_key_create)
BRIDGE_SYM(pthread_key_delete) BRIDGE_SYM(pthread_mutex_destroy) BRIDGE_SYM(pthread_mutex_init)
BRIDGE_SYM(pthread_mutex_lock) BRIDGE_SYM(pthread_mutex_trylock) BRIDGE_SYM(pthread_mutex_unlock)
BRIDGE_SYM(pthread_mutexattr_destroy) BRIDGE_SYM(pthread_mutexattr_init) BRIDGE_SYM(pthread_mutexattr_settype)
BRIDGE_SYM(pthread_once) BRIDGE_SYM(pthread_rwlock_destroy) BRIDGE_SYM(pthread_rwlock_init)
BRIDGE_SYM(pthread_rwlock_rdlock) BRIDGE_SYM(pthread_rwlock_unlock) BRIDGE_SYM(pthread_rwlock_wrlock)
BRIDGE_SYM(pthread_self) BRIDGE_SYM(pthread_setspecific) BRIDGE_SYM(qsort) BRIDGE_SYM(rand)
BRIDGE_SYM(read) BRIDGE_SYM(readdir) BRIDGE_SYM(realloc) BRIDGE_SYM(recv)
BRIDGE_SYM(recvfrom) BRIDGE_SYM(remove) BRIDGE_SYM(rename) BRIDGE_SYM(sched_yield)
BRIDGE_SYM(send) BRIDGE_SYM(sendto) BRIDGE_SYM(setbuf) BRIDGE_SYM(setgid)
BRIDGE_SYM(setjmp) BRIDGE_SYM(setlocale) BRIDGE_SYM(setsockopt) BRIDGE_SYM(setuid)
BRIDGE_SYM(shutdown) BRIDGE_SYM(sigaction) BRIDGE_SYM(sigaddset) BRIDGE_SYM(sigdelset)
BRIDGE_SYM(sigemptyset) BRIDGE_SYM(sigfillset) BRIDGE_SYM(siglongjmp) BRIDGE_SYM(signal)
BRIDGE_SYM(sigprocmask) BRIDGE_SYM(sigsetjmp) BRIDGE_SYM(sinf) BRIDGE_SYM(snprintf)
BRIDGE_SYM(socket) BRIDGE_SYM(socketpair) BRIDGE_SYM(sprintf) BRIDGE_SYM(sqrt)
BRIDGE_SYM(sqrtf) BRIDGE_SYM(srand) BRIDGE_SYM(sscanf) BRIDGE_SYM(stat)
BRIDGE_SYM(strcasecmp) BRIDGE_SYM(strcat) BRIDGE_SYM(strchr) BRIDGE_SYM(strcmp)
BRIDGE_SYM(strcoll_l) BRIDGE_SYM(strcpy) BRIDGE_SYM(strcspn) BRIDGE_SYM(strdup)
BRIDGE_SYM(strerror) BRIDGE_SYM(strerror_r) BRIDGE_SYM(strftime) BRIDGE_SYM(strftime_l)
BRIDGE_SYM(strlen) BRIDGE_SYM(strncasecmp) BRIDGE_SYM(strncmp) BRIDGE_SYM(strncpy)
BRIDGE_SYM(strpbrk) BRIDGE_SYM(strrchr) BRIDGE_SYM(strspn) BRIDGE_SYM(strstr)
BRIDGE_SYM(strtod) BRIDGE_SYM(strtof) BRIDGE_SYM(strtok) BRIDGE_SYM(strtok_r)
BRIDGE_SYM(strtol) BRIDGE_SYM(strtold) BRIDGE_SYM(strtold_l) BRIDGE_SYM(strtoll)
BRIDGE_SYM(strtoll_l) BRIDGE_SYM(strtoul) BRIDGE_SYM(strtoull) BRIDGE_SYM(strtoull_l)
BRIDGE_SYM(strxfrm_l) BRIDGE_SYM(swprintf) BRIDGE_SYM(sysconf) BRIDGE_SYM(syslog)
BRIDGE_SYM(tanf) BRIDGE_SYM(tcgetattr) BRIDGE_SYM(tcsetattr) BRIDGE_SYM(time)
BRIDGE_SYM(tolower) BRIDGE_SYM(tolower_l) BRIDGE_SYM(toupper) BRIDGE_SYM(toupper_l)
BRIDGE_SYM(towlower_l) BRIDGE_SYM(towupper_l) BRIDGE_SYM(umask) BRIDGE_SYM(ungetc)
BRIDGE_SYM(uselocale) BRIDGE_SYM(usleep) BRIDGE_SYM(vasprintf) BRIDGE_SYM(vfprintf)
BRIDGE_SYM(vsnprintf) BRIDGE_SYM(vsscanf) BRIDGE_SYM(waitpid) BRIDGE_SYM(wcrtomb)
BRIDGE_SYM(wcscoll_l) BRIDGE_SYM(wcslen) BRIDGE_SYM(wcsnrtombs) BRIDGE_SYM(wcstod)
BRIDGE_SYM(wcstof) BRIDGE_SYM(wcstol) BRIDGE_SYM(wcstold) BRIDGE_SYM(wcstoll)
BRIDGE_SYM(wcstoul) BRIDGE_SYM(wcstoull) BRIDGE_SYM(wcsxfrm_l) BRIDGE_SYM(wctob)
BRIDGE_SYM(wmemchr) BRIDGE_SYM(wmemcmp) BRIDGE_SYM(wmemcpy) BRIDGE_SYM(wmemmove)
BRIDGE_SYM(wmemset) BRIDGE_SYM(write)
