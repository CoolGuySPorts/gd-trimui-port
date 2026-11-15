cmake_minimum_required(VERSION 3.10)
project(gd_loader C CXX)

set(CMAKE_C_STANDARD 11)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

add_library(bionic_bridge SHARED bionic_bridge.c)
target_link_options(bionic_bridge PRIVATE "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/bionic.map" "-Wl,--no-as-needed")
target_link_libraries(bionic_bridge PRIVATE c m dl z pthread)

add_executable(gd_loader main.c fake_jni.c)
target_include_directories(gd_loader PRIVATE ${SDL2_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR})
target_link_options(gd_loader PRIVATE -rdynamic)
target_link_libraries(gd_loader PRIVATE bionic_bridge ${SDL2_LIBRARIES} GLESv2 EGL dl pthread m)
