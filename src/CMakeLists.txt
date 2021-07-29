cmake_minimum_required(VERSION 3.10.0)
project(fextor VERSION 1.0.0)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

#find_package(gflags REQUIRED)

add_subdirectory(dsp)
add_subdirectory(parallel)
add_subdirectory(wave)

add_executable(wave_test wave_test.cc $<TARGET_OBJECTS:wave_obj>)
add_dependencies(wave_test wave_obj)
set_target_properties(wave_test PROPERTIES ENABLE_EXPORTS on)
target_link_libraries(wave_test PRIVATE gflags)

add_executable(dsp_test dsp_test.cc $<TARGET_OBJECTS:wave_obj> $<TARGET_OBJECTS:dsp_obj>)
add_dependencies(dsp_test wave_obj dsp_obj)
set_target_properties(dsp_test PROPERTIES ENABLE_EXPORTS on)
target_link_libraries(dsp_test PRIVATE gflags)

add_executable(fextor fextor.cc fextor_app.cc $<TARGET_OBJECTS:wave_obj> $<TARGET_OBJECTS:dsp_obj>)
add_dependencies(fextor wave_obj dsp_obj)
set_target_properties(fextor PROPERTIES ENABLE_EXPORTS on)
target_link_libraries(fextor PRIVATE parallel_static gflags)