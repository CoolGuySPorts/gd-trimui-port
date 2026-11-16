cmake_minimum_required(VERSION 3.10)
project(jack_trimui_port C)

set(CMAKE_C_STANDARD 11)

find_package(SDL2 REQUIRED)

add_executable(jack_port
    main.c
    fake_jni.c
    bionic_bridge.c
)

target_include_directories(jack_port PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${SDL2_INCLUDE_DIRS}
)

# Флаг -rdynamic (-Wl,--export-dynamic) обязателен, чтобы libMyGame.so видела AAsset* и slCreateEngine!
target_link_options(jack_port PRIVATE -Wl,--export-dynamic)

target_link_libraries(jack_port
    ${SDL2_LIBRARIES}
    dl
    m
    pthread
)
