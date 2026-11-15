cmake_minimum_required(VERSION 3.10)
project(gd_loader C)

set(CMAKE_C_STANDARD 11)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

add_executable(gd_loader main.c fake_jni.c bionic_bridge.c)
target_link_options(gd_loader PRIVATE -rdynamic)
target_link_libraries(gd_loader ${SDL2_LIBRARIES} GLESv2 dl m pthread)
