#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <fstream>

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

typedef struct fextor_arg_t {
  std::string input_file_name_;
  std::string output_file_name_;
  dsp::FEInitParam* param_;
  dsp::FeatureExtractor* extractor_;
  int target_;

  fextor_arg_t() 
    : param_(NULL)
    , extractor_(NULL) {

  }
  ~fextor_arg_t() {}
} FextorArgs;

void worker(void* args) {

  if (args == NULL) {
    fprintf(stderr, "Invalid argument!\n");
    return;
  }
  FextorArgs *fextor_arg = (FextorArgs *)args;
  extractOne(fextor_arg->input_file_name_.c_str(),
             fextor_arg->output_file_name_.c_str(), 
             fextor_arg->param_,
             fextor_arg->extractor_, 
             fextor_arg->target_);
}

std::vector<std::string> readListFile(const char* list_file_name) {
  std::vector<std::string> file_list;

  std::ifstream input(list_file_name);
  for (std::string line; getline(input, line);) {
    file_list.push_back(line);
  }

  return file_list;
}

int main(int argc, char **argv) {

  gflags::SetUsageMessage("fextor");
  gflags::SetVersionString("1.0.0");
  gflags::ParseCommandLineFlags(&argc, &argv, true);

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
    // Read list files
    std::vector<std::string> input_file_list = readListFile(input_file_name);
    std::vector<std::string> output_file_list = readListFile(output_file_name);
    if (input_file_list.size() != output_file_list.size()) {
      fprintf(stderr, "number of files in %s and %s are unmatched.\n", input_file_name, output_file_name);
      return 1;
    }
    const unsigned int num_jobs = (unsigned int)input_file_list.size();
    fprintf(stdout, "%u number of jobs are found.\n", num_jobs);

    // Initialize extractors
    dsp::FeatureExtractor *extractors = new dsp::FeatureExtractor[FLAGS_num_threads];
    for (unsigned int n = 0; n < FLAGS_num_threads; n++) {
      error_code = extractors[n].init(&extractor_param);
      if (error_code != DSP_SUCCESS) {
        fprintf(stderr, "failed to init extractor.\n");
        return error_code;
      }
    }

    FextorArgs *args = new FextorArgs[num_jobs];
    for (unsigned int n = 0; n < num_jobs; n++) {
      std::string input_name = input_file_list[n];
      std::string output_name = output_file_list[n];
      
      args[n].input_file_name_ = input_name;
      args[n].output_file_name_ = output_name;
      args[n].param_ = &extractor_param;
      args[n].extractor_ = &extractors[n % FLAGS_num_threads];
      args[n].target_ = FLAGS_target;
    }
    
    //parallel::threadpool thread_pool = parallel::thpool_init(FLAGS_num_threads);
    //for (unsigned int n = 0; n < num_jobs; n++) {
    //  parallel::thpool_add_work(thread_pool, worker, (void*)&args[n]);
    //}
    //parallel::thpool_wait(thread_pool);
    //parallel::thpool_destroy(thread_pool);

    parallel::ThreadPool thread_pool(FLAGS_num_threads);
    for (unsigned int n = 0; n < num_jobs; n++) {
      thread_pool.addJob(worker, (void*)&args[n]);
    }
    thread_pool.wait();
    delete[] args;
    delete[] extractors;
  } else {
    // Initialize extractor
    dsp::FeatureExtractor extractor;
    error_code = extractor.init(&extractor_param);
    if (error_code != DSP_SUCCESS) {
      fprintf(stderr, "failed to init extractor.\n");
      return error_code;
    }
    
    // Do processing
    error_code = extractOne(input_file_name, output_file_name, 
                            &extractor_param, &extractor, FLAGS_target);
    if (error_code != 0) {
      fprintf(stderr, "Task failed.\n");
    }

  }

  gflags::ShutDownCommandLineFlags();
  return error_code;
}