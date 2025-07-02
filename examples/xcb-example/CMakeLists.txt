cmake_minimum_required(VERSION 3.27)
project(xcb-example C)

find_package(PkgConfig)
pkg_check_modules(XCB REQUIRED xcb)

add_executable(xcb-example main.c)
set_target_properties(xcb-example PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON)

target_link_libraries(xcb-example PUBLIC ${XCB_LIBRARIES})

target_include_directories(xcb-example PUBLIC .)
target_include_directories(xcb-example PUBLIC ${XCB_INCLUDE_DIRS})
