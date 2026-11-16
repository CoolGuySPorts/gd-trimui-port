#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <dlfcn.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <SDL2/SDL.h>

// ============================================================================
// 1. ЭМУЛЯЦИЯ ANDROID ASSETS И ЗВУКА (Экспортируются для libMyGame.so)
// ============================================================================

__attribute__((visibility("default")))
int slCreateEngine(void **pEngine, unsigned int numOptions, const void *pEngineOptions,
                   unsigned int numInterfaces, const void *pInterfaceIds, const void *pInterfaceRequired) {
    printf("[BRIDGE] slCreateEngine вызван -> возвращаем заглушку (без звука)\n");
    return 12; // SL_RESULT_FEATURE_UNSUPPORTED
}

__attribute__((visibility("default")))
void* AAssetManager_fromJava(void* env, void* assetManager) {
    return (void*)0x1;
}

__attribute__((visibility("default")))
void* AAssetManager_open(void* mgr, const char* filename, int mode) {
    if (!filename) return NULL;
    char path[512];
    snprintf(path, sizeof(path), "./assets/%s", filename);
    FILE* f = fopen(path, "rb");
    if (!f) {
        printf("[ASSETS] Не найден файл: %s\n", path);
    }
    return (void*)f;
}

__attribute__((visibility("default")))
int AAsset_read(void* asset, void* buf, size_t count) {
    if (!asset) return -1;
    return (int)fread(buf, 1, count, (FILE*)asset);
}

__attribute__((visibility("default")))
off_t AAsset_getLength(void* asset) {
    if (!asset) return 0;
    FILE* f = (FILE*)asset;
    long cur = ftell(f);
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, cur, SEEK_SET);
    return (off_t)len;
}

__attribute__((visibility("default")))
int AAsset_openFileDescriptor(void* asset, off_t* outStart, off_t* outLength) {
    return -1;
}

__attribute__((visibility("default")))
void AAsset_close(void* asset) {
    if (asset) fclose((FILE*)asset);
}

__attribute__((visibility("default")))
void* AAssetManager_openDir(void* mgr, const char* dirName) {
    char path[512];
    snprintf(path, sizeof(path), "./assets/%s", dirName ? dirName : "");
    return (void*)opendir(path);
}

__attribute__((visibility("default")))
const char* AAssetDir_getNextFileName(void* assetDir) {
    if (!assetDir) return NULL;
    struct dirent* entry;
    while ((entry = readdir((DIR*)assetDir)) != NULL) {
        if (entry->d_name[0] != '.') {
            return entry->d_name;
        }
    }
    return NULL;
}

__attribute__((visibility("default")))
void AAssetDir_close(void* assetDir) {
    if (assetDir) closedir((DIR*)assetDir);
}

// ============================================================================
// 2. ПОДКЛЮЧЕНИЕ ТВОЕГО FAKE_JNI (ИЛИ БАЗОВАЯ ЗАГЛУШКА)
// ============================================================================
#include "fake_jni.h"

// ============================================================================
// 3. ТИПЫ ФУНКЦИЙ ИЗ libMyGame.so
// ============================================================================
typedef int  (*JNI_OnLoad_t)(void* vm, void* reserved);
typedef void (*nativeSetContext_t)(void* env, void* thiz, void* context, void* assetManager);
typedef void (*nativeInit_t)(void* env, void* thiz, int width, int height);
typedef void (*nativeRender_t)(void* env);
typedef void (*nativeTouchesBegin_t)(void* env, void* thiz, int id, float x, float y);
typedef void (*nativeTouchesEnd_t)(void* env, void* thiz, int id, float x, float y);
typedef void (*nativeTouchesMove_t)(void* env, void* thiz, int* ids, float* xs, float* ys, int count);
typedef bool (*nativeKeyEvent_t)(void* env, void* thiz, int keyCode, bool isPressed);

// ============================================================================
// 4. ГЛАВНАЯ ФУНКЦИЯ ЗАПУСКА ДЛЯ TRIMUI SMART PRO (1280x720, GLES 1.1)
// ============================================================================
int main(int argc, char* argv[]) {
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    printf("[PORT] Запуск Incredible Jack для Trimui Smart Pro...\n");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_TIMER) < 0) {
        printf("[ERROR] Ошибка SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    // Настройка контекста OpenGL ES 1.1 (требуется libGLESv1_CM.so)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);

    int width = 1280;
    int height = 720;

    SDL_Window* window = SDL_CreateWindow(
        "Incredible Jack",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width, height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN
    );

    if (!window) {
        printf("[ERROR] Ошибка SDL_CreateWindow: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        printf("[ERROR] Ошибка SDL_GL_CreateContext: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_GL_SetSwapInterval(1); // VSync 60 FPS

    // Открытие геймпада Trimui Smart Pro
    SDL_GameController* pad = NULL;
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
            pad = SDL_GameControllerOpen(i);
            printf("[PORT] Подключен геймпад: %s\n", SDL_GameControllerName(pad));
            break;
        }
    }

    // Загрузка библиотеки игры
    printf("[PORT] Загружаем ./libMyGame.so...\n");
    void* handle = dlopen("./libMyGame.so", RTLD_NOW | RTLD_GLOBAL);
    if (!handle) {
        printf("[ERROR] Ошибка dlopen: %s\n", dlerror());
        return 1;
    }

    JNI_OnLoad_t JNI_OnLoad = (JNI_OnLoad_t)dlsym(handle, "JNI_OnLoad");
    nativeSetContext_t nativeSetContext = (nativeSetContext_t)dlsym(handle, "Java_org_cocos2dx_lib_Cocos2dxHelper_nativeSetContext");
    nativeInit_t nativeInit = (nativeInit_t)dlsym(handle, "Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeInit");
    nativeRender_t nativeRender = (nativeRender_t)dlsym(handle, "Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeRender");
    nativeTouchesBegin_t nativeTouchesBegin = (nativeTouchesBegin_t)dlsym(handle, "Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeTouchesBegin");
    nativeTouchesEnd_t nativeTouchesEnd = (nativeTouchesEnd_t)dlsym(handle, "Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeTouchesEnd");
    nativeKeyEvent_t nativeKeyEvent = (nativeKeyEvent_t)dlsym(handle, "Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeKeyEvent");

    if (!nativeInit || !nativeRender) {
        printf("[ERROR] Не найдены функции nativeInit или nativeRender в libMyGame.so!\n");
        return 1;
    }

    // Инициализация фейковой JavaVM и JNIEnv из твоего fake_jni.h
    JavaVM* vm = create_fake_jvm();
    JNIEnv* env = create_fake_jnienv();

    if (JNI_OnLoad) {
        printf("[PORT] Вызов JNI_OnLoad...\n");
        JNI_OnLoad(vm, NULL);
    }

    if (nativeSetContext) {
        printf("[PORT] Вызов nativeSetContext...\n");
        nativeSetContext(env, NULL, (void*)0x1, (void*)0x1);
    }

    printf("[PORT] Вызов nativeInit(%d, %d)...\n", width, height);
    nativeInit(env, NULL, width, height);

    printf("[PORT] Входим в главный цикл рендера!\n");
    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            // Обработка кнопок Trimui Smart Pro -> эмуляция нажатий на экранные кнопки
            else if (event.type == SDL_CONTROLLERBUTTONDOWN || event.type == SDL_CONTROLLERBUTTONUP) {
                bool pressed = (event.type == SDL_CONTROLLERBUTTONDOWN);
                float touchX = 0.0f;
                float touchY = 0.0f;
                int fingerId = 0;
                bool isTouch = false;

                switch (event.cbutton.button) {
                    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
                        touchX = 120.0f; touchY = 600.0f; fingerId = 1; isTouch = true;
                        break;
                    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                        touchX = 280.0f; touchY = 600.0f; fingerId = 1; isTouch = true;
                        break;
                    case SDL_CONTROLLER_BUTTON_A: // Прыжок
                        touchX = 1140.0f; touchY = 600.0f; fingerId = 2; isTouch = true;
                        break;
                    case SDL_CONTROLLER_BUTTON_B: // Назад / Выход в меню
                        if (nativeKeyEvent) nativeKeyEvent(env, NULL, 4, pressed); // KEYCODE_BACK = 4
                        break;
                    case SDL_CONTROLLER_BUTTON_START: // Старт игры (тап по центру экрана)
                        touchX = 640.0f; touchY = 450.0f; fingerId = 3; isTouch = true;
                        break;
                    case SDL_CONTROLLER_BUTTON_GUIDE: // Кнопка Menu/Home для выхода из игры
                        if (pressed) running = false;
                        break;
                }

                if (isTouch && nativeTouchesBegin && nativeTouchesEnd) {
                    if (pressed) {
                        nativeTouchesBegin(env, NULL, fingerId, touchX, touchY);
                    } else {
                        nativeTouchesEnd(env, NULL, fingerId, touchX, touchY);
                    }
                }
            }
        }

        nativeRender(env);
        SDL_GL_SwapWindow(window);
    }

    if (pad) SDL_GameControllerClose(pad);
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
