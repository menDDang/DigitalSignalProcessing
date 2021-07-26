#ifndef FEXTOR_APP_H
#define FEXTOR_APP_H

#define FEXTOR_SAMPLING_RATE 16000
#define FEXTOR_BIT_RATE 16
#define FEXTOR_NUM_CHANNELS 1

#define FEXTOR_TARGET_SPECTRUM  0
#define FEXTOR_TARGET_MEL       1
#define FEXTOR_TARGET_MFCC      2

#include <memory>

#include "dsp/feature_extractor.h"

class Feature {
public:
  Feature();
  Feature(unsigned int num_frame, unsigned int feat_dim);
  ~Feature();

private:
  unsigned int num_frame_;
  unsigned int feat_dim_;
  std::unique_ptr<dsp::float_t> data_;

public:
  int load(const char* input_file_name);
  int save(const char* output_file_name);

  dsp::float_t* getPtr(const unsigned int index);
};  // Feature

int extractOne(const char* input_wav_name, const char* output_feat_name, 
               const dsp::FEInitParam& param, dsp::FeatureExtractor& extractor,
               int target);

#endif // FEXTOR_APP_H