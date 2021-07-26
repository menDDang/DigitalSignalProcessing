#include <stdio.h>
#include <stdlib.h>

#include <iostream>

#include "gflags/gflags.h"

#include "parallel/threadpool.h"
#include "fextor_app.h"

DEFINE_double(step_duration, 0.01, "size of step in seconds");

DEFINE_string(input, "", "path of input file");
DEFINE_string(output, "", "path of output file");

DEFINE_int32(target, FEXTOR_TARGET_MFCC, "target to extract, "
              "(0: spectrum, 1: mel, 2: mfcc");

DEFINE_bool(list, false, "set fextor to process multi number of files");
DEFINE_uint32(num_threads, 4, "number of threads for parallel");


int main(int argc, char **argv) {

  gflags::SetUsageMessage("fextor");
  gflags::SetVersionString("1.0.0");
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  const unsigned int sampling_rate = 16000;
  const unsigned int bit_rate = 16;
  const unsigned int num_channels = 1;

  // Check arguments
  const char *input_file_name = FLAGS_input.c_str();
  if (input_file_name[0] == '\0') {
    fprintf(stderr, "Invalid argument - `input` argument is must be given.\n");
    return 1;
  }
  const char *output_file_name = FLAGS_output.c_str();
  if (output_file_name[0] == '\0') {
    fprintf(stderr, "Invalid argument - `output` argument is must be given.\n");
    return 1;
  }

  // Parse parameters for extractor
  dsp::FEInitParam extractor_param;
  int error_code = dsp::setDefaultParam(FEXTOR_SAMPLING_RATE, &extractor_param);
  if (error_code != DSP_SUCCESS) {
    fprintf(stderr, "failed to init parameters for extractor.\n");
    return error_code;
  }

  if (FLAGS_list) {
    fprintf(stderr, "not implemented yet.\n");
    return 1;
  } else {

    dsp::FeatureExtractor extractor;
    extractor.init(&extractor_param);

    error_code = extractOne(input_file_name, output_file_name, 
                            extractor_param, extractor, FLAGS_target);
    if (error_code != 0) {
      fprintf(stderr, "Task failed.\n");
    }

    return error_code;
  }
}