cmake_minimum_required(VERSION 3.10.0)
project(parallel VERSION 1.0)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_library(parallel_static STATIC threadpool.cc)
target_link_libraries(parallel_static PRIVATE Threads::Threads)