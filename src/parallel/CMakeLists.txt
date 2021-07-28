cmake_minimum_required(VERSION 3.0.0)
project(parallel VERSION 1.0)

add_library(parallel_obj OBJECT threadpool.cc)
target_link_libraries(parallel_obj PUBLIC pthread)