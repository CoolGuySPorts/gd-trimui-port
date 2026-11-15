#ifndef FAKE_JNI_H
#define FAKE_JNI_H

#include <stdint.h>
#include <stddef.h>

typedef void* JNIEnv;
typedef void* JavaVM;
typedef void* jobject;
typedef void* jclass;
typedef void* jstring;
typedef void* jmethodID;
typedef void* jfieldID;
typedef uint8_t jboolean;
typedef int32_t jint;
typedef int64_t jlong;
typedef float jfloat;

void init_fake_jni(JavaVM **out_vm, JNIEnv **out_env);

#endif
