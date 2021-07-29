cmake_minimum_required(VERSION 3.10.0)
project(dsp VERSION 1.0)

option(USE_DOUBLE_PRECISION off)

message("USE_DOUBLE_PRECISION : ${USE_DOUBLE_PRECISION}")

add_library(dsp_obj OBJECT fft.cc feature_extractor.cc)
target_compile_definitions(dsp_obj PUBLIC USE_DOUBLE_PRECISION=${USE_DOUBLE_PRECISION})

add_library(dsp_static STATIC $<TARGET_OBJECTS:dsp_obj>)