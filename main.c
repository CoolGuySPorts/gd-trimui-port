#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <dlfcn.h>
#include <SDL.h>
#include <GLES2/gl2.h>
#include "fake_jni.h"

typedef jint (*JNI_OnLoad_t)(JavaVM *vm, void *reserved);
typedef void (*nativeSetApkPath_t)(JNIEnv *env, jobject thiz, jstring path);
typedef void (*nativeInit_t)(JNIEnv *env, jobject thiz, jint w, jint h);
typedef void (*nativeRender_t)(JNIEnv *env, jobject thiz);
typedef void (*nativeTouchesBegin_t)(JNIEnv *env, jobject thiz, jint id, jfloat x, jfloat y);
typedef void (*nativeTouchesEnd_t)(JNIEnv *env, jobject thiz, jint id, jfloat x, jfloat y);
typedef jboolean (*nativeKeyDown_t)(JNIEnv *env, jobject thiz, jint keyCode);

static void* resolve_sym(void *h, const char *n1, const char *n2) {
    void *s = dlsym(h, n1);
    if (!s && n2) s = dlsym(h, n2);
    if (!s) fprintf(stderr, "[WARN] Missing: %s\n", n1);
    return s;
}

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    printf("[STEP 1] SDL_Init...\n");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS) < 0) return 1;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    int w = 1280, h = 720;
    printf("[STEP 2] Creating window...\n");
    SDL_Window *win = SDL_CreateWindow("GD", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN);
    if (!win) return 1;

    SDL_GLContext ctx = SDL_GL_CreateContext(win);
    if (!ctx) return 1;
    SDL_GL_MakeCurrent(win, ctx);
    SDL_GL_SetSwapInterval(1);

    SDL_GameController *pad = NULL;
    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i) && (pad = SDL_GameControllerOpen(i))) break;
    }

    printf("[STEP 3] Init fake JNI...\n");
    JavaVM *vm = NULL;
    JNIEnv *env = NULL;
    init_fake_jni(&vm, &env);

    printf("[STEP 4] Loading libfmod.so...\n");
    void *fmod_h = dlopen("./libfmod.so", RTLD_LAZY | RTLD_GLOBAL);
    if (!fmod_h) fprintf(stderr, "fmod warn: %s\n", dlerror());
    else printf("[STEP 4 OK] libfmod.so loaded!\n");

    printf("[STEP 5] Loading libcocos2dcpp.so...\n");
    void *gd_h = dlopen("./libcocos2dcpp.so", RTLD_LAZY | RTLD_GLOBAL);
    if (!gd_h) {
        fprintf(stderr, "cocos err: %s\n", dlerror());
        return 1;
    }
    printf("[STEP 5 OK] libcocos2dcpp.so loaded!\n");

    printf("[STEP 6] Calling JNI_OnLoad...\n");
    JNI_OnLoad_t on_load = (JNI_OnLoad_t)dlsym(gd_h, "JNI_OnLoad");
    if (on_load) on_load(vm, NULL);
    printf("[STEP 6 OK] JNI_OnLoad done!\n");

    nativeSetApkPath_t nSetApk = (nativeSetApkPath_t)resolve_sym(gd_h, "Java_org_cocos2dx_lib_Cocos2dxHelper_nativeSetApkPath", NULL);
    if (nSetApk) {
        printf("[STEP 6.5] Setting APK path...\n");
        nSetApk(env, NULL, (jstring)"./game.apk");
    }

    nativeInit_t nInit = (nativeInit_t)resolve_sym(gd_h, "Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeInit", NULL);
    nativeRender_t nRender = (nativeRender_t)resolve_sym(gd_h, "Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeRender", NULL);
    nativeTouchesBegin_t nDown = (nativeTouchesBegin_t)resolve_sym(gd_h, "Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeTouchesBegin", "Java_org_cocos2dx_lib_Cocos2dxGLSurfaceView_nativeTouchesBegin");
    nativeTouchesEnd_t nUp = (nativeTouchesEnd_t)resolve_sym(gd_h, "Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeTouchesEnd", "Java_org_cocos2dx_lib_Cocos2dxGLSurfaceView_nativeTouchesEnd");
    nativeKeyDown_t nKey = (nativeKeyDown_t)resolve_sym(gd_h, "Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeKeyDown", "Java_org_cocos2dx_lib_Cocos2dxGLSurfaceView_nativeKeyDown");

    if (!nInit || !nRender) return 1;

    printf("[STEP 7] Calling nativeInit...\n");
    nInit(env, NULL, w, h);
    printf("[STEP 7 OK] nativeInit done! Entering main loop...\n");

    bool run = true;
    SDL_Event e;
    while (run) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) run = false;
            else if (e.type == SDL_CONTROLLERBUTTONDOWN) {
                if (e.cbutton.button == SDL_CONTROLLER_BUTTON_A || e.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_UP) {
                    if (nDown) nDown(env, NULL, 0, w / 2.0f, h / 2.0f);
                } else if (e.cbutton.button == SDL_CONTROLLER_BUTTON_B) {
                    if (nKey) nKey(env, NULL, 4);
                } else if (e.cbutton.button == SDL_CONTROLLER_BUTTON_START && pad && SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_BACK)) {
                    run = false;
                }
            } else if (e.type == SDL_CONTROLLERBUTTONUP) {
                if (e.cbutton.button == SDL_CONTROLLER_BUTTON_A || e.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_UP) {
                    if (nUp) nUp(env, NULL, 0, w / 2.0f, h / 2.0f);
                }
            }
        }
        nRender(env, NULL);
        SDL_GL_SwapWindow(win);
    }

    if (pad) SDL_GameControllerClose(pad);
    if (gd_h) dlclose(gd_h);
    if (fmod_h) dlclose(fmod_h);
    SDL_GL_DeleteContext(ctx);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
