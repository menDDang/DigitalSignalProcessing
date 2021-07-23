cmake_minimum_required(VERSION 3.0.0)
project(dsp VERSION 1.0)

#add_library(fft_obj OBJECT fft.cc)

#add_library(feature_extractor_obj OBJECT feature_extractor.cc)
#target_link_libraries(feature_extractor_obj PUBLIC fft_obj)
#add_dependencies(feature_extractor_obj fft_obj)


add_library(dsp_obj OBJECT fft.cc feature_extractor.cc)