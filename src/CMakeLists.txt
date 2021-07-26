cmake_minimum_required(VERSION 3.0.0)
project(fextor VERSION 1.0.0)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

#find_package(gflags REQUIRED)

add_subdirectory(wave)
add_subdirectory(dsp)


add_executable(wave_test wave_test.cc)
target_link_libraries(wave_test PRIVATE wave_obj gflags)

add_executable(dsp_test dsp_test.cc)
target_link_libraries(dsp_test PRIVATE wave_obj dsp_obj gflags)

add_executable(fextor fextor.cc fextor_app.cc)
target_link_libraries(fextor PRIVATE wave_obj dsp_obj gflags)