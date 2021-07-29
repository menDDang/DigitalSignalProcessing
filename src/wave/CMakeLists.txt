cmake_minimum_required(VERSION 3.10.0)
project(wave VERSION 1.0)

add_library(wave_obj OBJECT wave.cc wave_core.cc wave_gain.cc)
 
add_library(wave_static OBJECT $<TARGET_OBJECTS:wave_obj>)