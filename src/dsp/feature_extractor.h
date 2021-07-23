#ifndef DSP_FEATURE_EXTRACTOR_H
#define DSP_FEATURE_EXTRACTOR_H

#include <stdio.h>

#include "dsp/dsp.h"

#ifndef DSP_DEFAULT_EPSILON
#define DSP_DEFAULT_EPSILON 1e-4
#endif

#ifndef DSP_DEFAULT_REF_LEVEL_DB
#define DSP_DEFAULT_REF_LEVEL_DB 20
#endif

namespace dsp {

enum available_sampling_rate_t {
  kSamplingRate16K=16000
};

enum window_type_t {
  kWindowTypeRectangle,
  kWindowTypeHanning,
};

typedef struct feature_extractor_init_param_t {
  unsigned int sampling_rate;
  window_type_t window_type;
  unsigned int window_size;   // Number of samples in one frame, 
                              // usually 0.02 seconds.
  unsigned int num_fft_point; // Number of fft points, must be power of 2 and
                              // larger than `window_size`.
  unsigned int num_mels;      // Dimension of mel filter bank outputs, 
                              // number of filter bank = `num_mels` + 2
  unsigned int num_mfcc;      // Dimension of mfcc.
  bool is_center;             // If true, input data are placed in center of 
                              // window and padded with 0, 
                              // else placed at start of window.
  float_t min_hertz;          // Minimum frequency in hertz scale used in compute mel filter banks.
                              // must be non-negative.
  float_t max_hertz;          // Maximum frequency in hertz scale used in compute mel filter banks.
                              // smaller than `sampling_rate` / 2.                  
  float_t epsilon;            // Small positive real number for 
                              // determining energy floor.
  float_t ref_level_db;       // Reference level usied in conversion from amplitude to decibel.                              
} FEInitParam;

int setDefaultParam(const unsigned int sampling_rate, FEInitParam* param);

typedef struct mel_filter_bank_t FilterBank;

class FeatureExtractor {
public:
  FeatureExtractor();
  virtual ~FeatureExtractor();

private:
  unsigned int sampling_rate_;
  unsigned int window_size_;
  unsigned int num_fft_point_;
  unsigned int num_mels_;
  unsigned int num_mfcc_;
  bool is_center_;
  float_t min_hertz_;
  float_t max_hertz_;
  float_t ref_level_db_;
  float_t epsilon_;
  
  float_t *tmp_buffer_;
  float_t *window_;
  FilterBank *mel_filter_banks_;
  float_t *dct_matrix_;

  int initWindow(const window_type_t window_type, const unsigned int window_size);
  int initMelFilters(const unsigned int num_mels, const float_t min_hertz, 
                     const float_t max_hertz);
  int initDctMatrix(const unsigned int num_mfcc);

  int getMagnitude(const float_t *const wave_frame_data, float_t *magnitude);
  int getMel(const float_t *const magnitude,
                          float_t *mel_filter_bank_output);
  int getMfcc(const float_t *const mel_filter_bank_output,
              float_t *mfcc_output);


  unsigned int findFilterBankIndex(float_t hertz);

  // Convert amplitude into decibel scale
  float_t logarize(float_t x);

public:
  int init(const FEInitParam *const param);

  // Convert frame data into magnitudes.
  // Length of frame data = `window_size_`, dimension of magnitudes =
  // `num_fft_point_` / 2 + 1.
  // If multi threading environment, multi number of threads can access
  // this function. In this case, since temporary memory is neccessary,
  // `temp_mem` must be given. 
  int spectrum(const float_t *const wave_frame_data, float_t *dest,
               const bool logarize_output = false, float_t *temp_mem=NULL);

  // Convert frame data into mel-spectrum
  // Length of frame data = `window_size_`, dimension of magnitudes =
  // `num_mels_`.
  // If multi threading environment, multi number of threads can access
  // this function. In this case, since temporary memory is neccessary,
  // `temp_mem` must be given. 
  int melspectrum(const float_t *const wave_frame_data, float_t *dest,
                  const bool logarize_output = false, float_t *temp_mem=NULL);
  
  // Convert frame data into Mel-Filter bank Ceptral Coefficients.
  // Length of frame data = `window_size_`, dimension of mfcc =
  // `num_mfcc_`.
  // If multi threading environment, multi number of threads can access
  // this function. In this case, since temporary memory is neccessary,
  // `temp_mem` must be given. 
  int mfcc(const float_t *const wave_frame_data, float_t *dest, float_t *temp_mem=NULL);

}; // class FeatureExtractor

} // namespace dsp

#endif // DSP_FEATURE_EXTRACTOR_H