cmake_minimum_required(VERSION 2.8)
project(wxserver)


set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -Wall")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2 -Wall")

add_subdirectory(src)
add_subdirectory(tests)

add_executable(wxserver tests/wxserver.c src/wxsrv.h src/wxsignal.h)

target_link_libraries(wxserver wxsrv)