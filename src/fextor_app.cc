#include "fextor_app.h"

#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <vector>

#include "wave/wave.h"
#include "dsp/feature_extractor.h"

Feature::Feature() 
  : num_frame_(0)
  , feat_dim_(0)
  , data_(NULL) {

}

Feature::Feature(unsigned int num_frame, unsigned int feat_dim) 
  : num_frame_(num_frame)
  , feat_dim_(feat_dim)
  , data_(new dsp::float_t[num_frame * feat_dim]) {

}

Feature::~Feature() {
  if (data_ != NULL) {
    delete[] data_;
    data_ = NULL;
  }
}

int Feature::load(const char* input_file_name) {
  if (input_file_name == NULL) {
    fprintf(stderr, "Feature::load() - invalid argument. `input_file_name` must be not NULL.\n");
    return 1;
  }

  FILE *fp_in = fopen(input_file_name, "rb");
  if (fp_in == NULL) {
    fprintf(stderr, "Feature::load() - failed to open file : %s\n", input_file_name);
    return 1;
  }

  // read header
  unsigned long size;
  fscanf(fp_in, "%u%u%lu", &num_frame_, &feat_dim_, &size);
  if (size != sizeof(dsp::float_t)) {
    fprintf(stderr, "Feature::load() - size of data are unmatched.\n");
    fclose(fp_in);
    return 1;
  }

  // read data
  if (data_ != NULL) {
    delete[] data_;
    data_ = NULL;
  }
  data_ = new dsp::float_t[num_frame_ * feat_dim_];
  //data_.reset(new dsp::float_t[num_frame_ * feat_dim_]);
  size_t n = fread(data_, sizeof(dsp::float_t), num_frame_ * feat_dim_, fp_in);
  if (n != num_frame_ * feat_dim_) {
    fprintf(stderr, "Feature::load() - failed to load feature data.\n");
    fclose(fp_in);
    return 1;
  }

  fclose(fp_in);
  //delete[] data_;
  //data_ = NULL;
  return 0;
}

int Feature::save(const char* output_file_name) {
  if (output_file_name == NULL) {
    fprintf(stderr, "Feature::save() - invalid argument. `output_file_name` must be not NULL.\n");
    return 1;
  }

  FILE *fp_out = fopen(output_file_name, "wb");
  if (fp_out == NULL) {
    fprintf(stderr, "Feature::save() - failed to open file : %s\n", output_file_name);
    return 1;
  }

  // write header
  fwrite(&num_frame_, sizeof(unsigned int), 1, fp_out);
  fwrite(&feat_dim_, sizeof(unsigned int), 1, fp_out);
  unsigned long size = sizeof(dsp::float_t);
  fwrite(&size, sizeof(unsigned long), 1, fp_out);
  //fprintf(fp_out, "%u%u%lu", num_frame_, feat_dim_,
  //        sizeof(dsp::float_t));

  // write data
  fwrite(data_, sizeof(dsp::float_t), num_frame_ * feat_dim_, fp_out);

  fclose(fp_out);
  return 0;
}

dsp::float_t* Feature::getPtr(const unsigned int index) {
  if (index >= num_frame_ * feat_dim_) {
    return NULL;
  }
  return data_ + index;
}

int extractOne(const char* input_wav_name, const char* output_feat_name, 
               const dsp::FEInitParam* param, dsp::FeatureExtractor* extractor,
               int target) {
  
  // read wav
  dsp::float_t *wav = NULL;
  unsigned int wav_length;
  wave::WaveReader wav_reader;
  wav_reader.init(FEXTOR_SAMPLING_RATE, FEXTOR_BIT_RATE, FEXTOR_NUM_CHANNELS);
  int error_code = wav_reader.read(input_wav_name, &wav, &wav_length);
  if (error_code != WAVE_SUCCESS) {
    fprintf(stderr, "failed to read file : %s\n", input_wav_name);
    return error_code;
  }

  // get number of frames & dimension of each feature
  unsigned int num_frame = (wav_length - param->window_size) / param->step_size;
  unsigned int feat_dim;
  switch (target)
  {
  case FEXTOR_TARGET_SPECTRUM:
    feat_dim = param->num_fft_point / 2 + 1;
    break;
  case FEXTOR_TARGET_MEL:
    feat_dim = param->num_mels;
    break;
  case FEXTOR_TARGET_MFCC:
    feat_dim = param->num_mfcc;
    break;
  default:
    fprintf(stderr, "invalid target (given : %d)\n", (int)target);
    delete[] wav;
    return 1;
    break;
  }

  // extract feature
  Feature feat(num_frame, feat_dim);
  for (unsigned int n = 0; n < num_frame; n++) {
    dsp::float_t* src = wav + n * param->step_size;
    dsp::float_t* dest = feat.getPtr(n * feat_dim);
    
    switch (target)
    {
    case FEXTOR_TARGET_SPECTRUM:
      error_code = extractor->spectrum(src, dest);
      break;
    case FEXTOR_TARGET_MEL:
      error_code = extractor->melspectrum(src, dest);
      break;
    case FEXTOR_TARGET_MFCC:
      error_code = extractor->mfcc(src, dest);
      break;
    }

    if (error_code != DSP_SUCCESS) {
      fprintf(stderr, "failed to extract feature\n");
      delete[] wav;
      return error_code;
    }
  }
  
  // write feature
  error_code = feat.save(output_feat_name);
  if (error_code != 0) {
    fprintf(stderr, "failed to save feature.\n");
    delete[] wav;
    return error_code;
  }

  delete[] wav;
  return 0;
}