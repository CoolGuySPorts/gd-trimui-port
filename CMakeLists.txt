cmake_minimum_required(VERSION 3.10)
project(gd_loader C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

file(GLOB SOURCES "*.c")

add_executable(gd_loader ${SOURCES})

target_include_directories(gd_loader PRIVATE ${SDL2_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(gd_loader PRIVATE ${SDL2_LIBRARIES} GLESv2 EGL dl pthread m)
