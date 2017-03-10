cmake_minimum_required(VERSION 2.8)
project(testserver)


set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -Wall")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2 -Wall")

add_subdirectory(src)
add_subdirectory(tests)

add_executable(testserver tests/testserver.c)
add_executable(testmaster tests/testmaster.c)
add_executable(testworker tests/testworker.c)

target_link_libraries(testserver wxserver)
target_link_libraries(testmaster wxserver)