# CMakeLists.txt

cmake_minimum_required(VERSION 3.15)

project(memtool CXX)

set(MEMTOOL_SRCS "src/memtool.cpp")

add_subdirectory(../_Deps/wdl "./lib")

add_executable(${PROJECT_NAME} ${MEMTOOL_SRCS})
target_link_libraries(${PROJECT_NAME} PRIVATE wdl)