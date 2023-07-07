cmake_minimum_required(VERSION 3.16)
project(algo1)

if(SHARED_MEMORY_LOG)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSHARED_MEMORY_LOG")
endif(SHARED_MEMORY_LOG)
unset(SHARED_MEMORY_LOG CACHE)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -g")

file(GLOB_RECURSE HEADERS "*.h")

add_library(Scheduler STATIC lib/scheduler.cpp)

target_include_directories(Scheduler PUBLIC include)

add_executable(opt-test lib/main.cpp)
target_link_libraries(opt-test Scheduler)


