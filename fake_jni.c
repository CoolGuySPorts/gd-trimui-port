#include "fake_jni.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>

static void* fake_jni_table;
static void* fake_vm_table;
static JNIEnv global_env = (JNIEnv)fake_jni_table;
static JavaVM global_vm = (JavaVM)fake_vm_table;

static uintptr_t jni_GenericZero(JNIEnv *env, ...) {
    return 0;
}

static jint jni_GetVersion(JNIEnv *env) { return 0x00010006; }

static jclass jni_FindClass(JNIEnv *env, const char *name) {
    printf("[JNI] FindClass: %s\n", name ? name : "null");
    return (jclass)0x1001;
}

static jclass jni_GetObjectClass(JNIEnv *env, jobject obj) {
    return (jclass)0x1002;
}

static jint jni_PushLocalFrame(JNIEnv *env, jint capacity) {
    return 0;
}

static jobject jni_PopLocalFrame(JNIEnv *env, jobject result) {
    return result;
}

static jmethodID jni_GetStaticMethodID(JNIEnv *env, jclass clazz, const char *name, const char *sig) {
    printf("[JNI] GetStaticMethodID: %s %s\n", name ? name : "", sig ? sig : "");
    return (jmethodID)(name ? strdup(name) : strdup("unknown"));
}

static jmethodID jni_GetMethodID(JNIEnv *env, jclass clazz, const char *name, const char *sig) {
    return (jmethodID)(name ? strdup(name) : strdup("unknown"));
}

static jstring jni_NewStringUTF(JNIEnv *env, const char *bytes) {
    return (jstring)(bytes ? strdup(bytes) : strdup(""));
}

static const char* jni_GetStringUTFChars(JNIEnv *env, jstring str, jboolean *isCopy) {
    if (isCopy) *isCopy = 0;
    return str ? (const char*)str : "";
}

static jint jni_GetStringUTFLength(JNIEnv *env, jstring str) {
    return str ? (jint)strlen((const char*)str) : 0;
}

static void jni_ReleaseStringUTFChars(JNIEnv *env, jstring str, const char *utf) {}

static jobject jni_CallStaticObjectMethodV(JNIEnv *env, jclass clazz, jmethodID methodID, va_list args) {
    const char *mname = (const char*)methodID;
    if (!mname) return (jobject)strdup("");
    printf("[JNI] CallStaticObjectMethod: %s\n", mname);

    if (strcmp(mname, "getCocos2dxWritablePath") == 0) {
        mkdir("./saves", 0755);
        return (jobject)strdup("./saves/");
    }
    if (strcmp(mname, "getCurrentLanguage") == 0) {
        return (jobject)strdup("en");
    }
    if (strcmp(mname, "getPackageName") == 0 || strcmp(mname, "getCocos2dxPackageName") == 0) {
        return (jobject)strdup("com.robtopx.geometryjump");
    }
    if (strcmp(mname, "getApkPath") == 0) {
        return (jobject)strdup("./game.apk");
    }
    return (jobject)strdup("");
}

static jobject jni_CallStaticObjectMethod(JNIEnv *env, jclass clazz, jmethodID methodID, ...) {
    va_list args;
    va_start(args, methodID);
    jobject res = jni_CallStaticObjectMethodV(env, clazz, methodID, args);
    va_end(args);
    return res;
}

static jboolean jni_CallStaticBooleanMethod(JNIEnv *env, jclass clazz, jmethodID methodID, ...) {
    return 0;
}

static jint jni_CallStaticIntMethod(JNIEnv *env, jclass clazz, jmethodID methodID, ...) {
    return 0;
}

static void jni_CallStaticVoidMethod(JNIEnv *env, jclass clazz, jmethodID methodID, ...) {}
static void jni_CallVoidMethod(JNIEnv *env, jobject obj, jmethodID methodID, ...) {}
static jboolean jni_ExceptionCheck(JNIEnv *env) { return 0; }
static void jni_ExceptionClear(JNIEnv *env) {}
static void jni_DeleteLocalRef(JNIEnv *env, jobject obj) {}
static void jni_DeleteGlobalRef(JNIEnv *env, jobject obj) {}
static jobject jni_NewGlobalRef(JNIEnv *env, jobject obj) { return obj; }
static jobject jni_NewLocalRef(JNIEnv *env, jobject obj) { return obj; }
static jint jni_EnsureLocalCapacity(JNIEnv *env, jint capacity) { return 0; }

static jint vm_GetEnv(JavaVM *vm, void **penv, jint version) {
    *penv = global_env;
    return 0;
}
static jint vm_AttachCurrentThread(JavaVM *vm, JNIEnv **p_env, void *thr_args) {
    *p_env = global_env;
    return 0;
}
static jint vm_DetachCurrentThread(JavaVM *vm) { return 0; }

void init_fake_jni(JavaVM **out_vm, JNIEnv **out_env) {
    for (int i = 0; i < 300; i++) fake_jni_table[i] = (void*)jni_GenericZero;
    fake_jni_table = (void*)jni_GetVersion;
    fake_jni_table = (void*)jni_FindClass;
    fake_jni_table = (void*)jni_ExceptionCheck;
    fake_jni_table = (void*)jni_ExceptionClear;
    fake_jni_table = (void*)jni_PushLocalFrame;
    fake_jni_table = (void*)jni_PopLocalFrame;
    fake_jni_table = (void*)jni_NewGlobalRef;
    fake_jni_table = (void*)jni_DeleteGlobalRef;
    fake_jni_table = (void*)jni_DeleteLocalRef;
    fake_jni_table = (void*)jni_NewLocalRef;
    fake_jni_table = (void*)jni_EnsureLocalCapacity;
    fake_jni_table = (void*)jni_GetObjectClass;
    fake_jni_table = (void*)jni_GetMethodID;
    fake_jni_table = (void*)jni_CallVoidMethod;
    fake_jni_table = (void*)jni_GetStaticMethodID;
    fake_jni_table = (void*)jni_CallStaticObjectMethod;
    fake_jni_table = (void*)jni_CallStaticObjectMethodV;
    fake_jni_table = (void*)jni_CallStaticBooleanMethod;
    fake_jni_table = (void*)jni_CallStaticIntMethod;
    fake_jni_table = (void*)jni_CallStaticVoidMethod;
    fake_jni_table = (void*)jni_NewStringUTF;
    fake_jni_table = (void*)jni_GetStringUTFLength;
    fake_jni_table = (void*)jni_GetStringUTFChars;
    fake_jni_table = (void*)jni_ReleaseStringUTFChars;

    for (int i = 0; i < 16; i++) fake_vm_table[i] = (void*)jni_GenericZero;
    fake_vm_table = (void*)vm_AttachCurrentThread;
    fake_vm_table = (void*)vm_DetachCurrentThread;
    fake_vm_table = (void*)vm_GetEnv;

    *out_vm = &global_vm;
    *out_env = &global_env;
}
