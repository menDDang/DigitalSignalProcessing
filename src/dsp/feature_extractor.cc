#include "dsp/feature_extractor.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dsp/fft.h"

namespace dsp {

typedef struct mel_filter_bank_t {
  unsigned int start;
  unsigned int length;
  float_t *data;

  mel_filter_bank_t() : start(0), length(0), data(0) {}
  ~mel_filter_bank_t() {
    if (data != NULL) {
      delete[] data;
      data = NULL;
    }
  }
} FilterBank;

inline float_t convertMelToHertz(float_t mel) {
  return (float_t)700 * (std::exp(mel / (float_t)1125.0) - 1);
}

inline float_t convertHertzToMel(float_t hertz) {
  return (float_t)1125.0 * std::log((float_t)1.0 + hertz / (float_t)700.0);
}

int setDefaultParam(const unsigned int sampling_rate, FEInitParam* param) {
  if (param == NULL) {
    fprintf(stderr, "dsp::setDefaultParam() - `param` must be not NULL.\n");
    return DSP_INVALID_ARG_VALUE;
  }
  
  param->sampling_rate = sampling_rate;
  param->window_type = kWindowTypeHanning;
  param->window_size = 0.02 * sampling_rate;
  param->step_size = 0.01 * sampling_rate;
  param->is_center = true;
  param->min_hertz = 0;
  param->max_hertz = sampling_rate / 2;
  param->epsilon = DSP_DEFAULT_EPSILON;
  param->ref_level_db = DSP_DEFAULT_REF_LEVEL_DB;
    
  switch (sampling_rate)
  {
  case kSamplingRate16K:
    param->num_fft_point = 512;
    param->num_mels = 80;
    param->num_mfcc = 40;
    break;
  
  default:
    fprintf(stderr, "dsp::setDefaultParam() - invalid sampling rate.\n");
    return DSP_INVALID_ARG_VALUE;
  }

  return DSP_SUCCESS;
}

FeatureExtractor::FeatureExtractor()
    : sampling_rate_(0), window_size_(0), num_fft_point_(0), num_mels_(0),
      num_mfcc_(0), is_center_(false), min_hertz_(0), max_hertz_(0),
      ref_level_db_(0), epsilon_(0), tmp_buffer_(NULL), window_(NULL),
      mel_filter_banks_(NULL), dct_matrix_(NULL) {}

FeatureExtractor::~FeatureExtractor() {
  if (tmp_buffer_ != NULL) {
    delete[] tmp_buffer_;
    tmp_buffer_ = NULL;
  }
  if (window_ != NULL) {
    delete[] window_;
    window_ = NULL;
  }
  if (mel_filter_banks_ != NULL) {
    delete[] mel_filter_banks_;
    mel_filter_banks_ = NULL;
  }
  if (dct_matrix_ != NULL) {
    delete[] dct_matrix_;
    dct_matrix_ = NULL;
  }
}

int FeatureExtractor::init(const FEInitParam *const param) {
  int error_code;

  if (param == NULL) {
    fprintf(stderr,
            "dsp::FeatureExtractor::init() - `param` must be not NULL.\n");
    return DSP_INVALID_ARG_VALUE;
  }

  sampling_rate_ = param->sampling_rate;
  num_fft_point_ = param->num_fft_point;
  epsilon_ = param->epsilon;
  ref_level_db_ = param->ref_level_db;
  is_center_ = param->is_center;

  const unsigned int buffer_size = num_fft_point_ * 2;
  if (tmp_buffer_ != NULL) {
    delete[] tmp_buffer_;
    tmp_buffer_ = NULL;
  }
  tmp_buffer_ = new float_t[buffer_size];

  error_code = initWindow(param->window_type, param->window_size);
  if (error_code != DSP_SUCCESS) {
    return error_code;
  }

  if (param->num_mels != 0) {
    error_code =
        initMelFilters(param->num_mels, param->min_hertz, param->max_hertz);
    if (error_code != DSP_SUCCESS) {
      return error_code;
    }
  }

  if (param->num_mfcc != 0) {
    error_code = initDctMatrix(param->num_mfcc);
    if (error_code != DSP_SUCCESS) {
      return error_code;
    }
  }

  return DSP_SUCCESS;
}

int FeatureExtractor::initWindow(window_type_t window_type,
                                 const unsigned int window_size) {

  if (window_size == 0) {
    fprintf(stderr, "dsp::FeatureExtractor::initWindow() - `window_size` must "
                    "be positive.\n");
    return DSP_INVALID_ARG_VALUE;
  }
  if (window_size > num_fft_point_) {
    fprintf(stderr, "dsp::FeatureExtractor::initWindow() - `window_size` must "
                    "be smaller than `num_fft_point_`.\n");
    return DSP_INVALID_ARG_VALUE;
  }

  window_size_ = window_size;

  if (window_ != NULL) {
    delete[] window_;
    window_ = NULL;
  }
  window_ = new float_t[window_size_];
  if (window_ == NULL) {
    return DSP_FAILED_MALLOC;
  }

  switch (window_type) {
  case kWindowTypeRectangle:
    for (unsigned int i = 0; i < window_size_; i++) {
      window_[i] = (float_t)1.0;
    }
    break;

  case kWindowTypeHanning:
    for (unsigned int i = 0; i < window_size_; i++) {
      window_[i] =
          (float_t)0.54 - (float_t)0.46 * std::cos((float_t)(2.0 * M_PI * i) /
                                                   (float_t)(window_size_ - 1));
    }
    break;

  default:
    break;
  }

  return DSP_SUCCESS;
}

int FeatureExtractor::initMelFilters(const unsigned int num_mels,
                                     const float_t min_hertz,
                                     const float_t max_hertz) {
  if (num_mels == 0) {
    fprintf(stderr, "dsp::FeatureExtractor::initMelFilters() - `num_mels` must "
                    "be positive.\n");
    return DSP_INVALID_ARG_VALUE;
  }
  if (min_hertz < 0) {
    fprintf(stderr, "dsp::FeatureExtractor::initMelFilters() - `min_hertz` "
                    "must be non-negative value.\n");
    return DSP_INVALID_ARG_VALUE;
  }
  if ((max_hertz < 0) || (max_hertz > sampling_rate_ / 2)) {
    fprintf(stderr, "dsp::FeatureExtractor::initMelFilters() - `max_hertz` "
                    "must be in range of (min_hertz, sampling_rate / 2).\n");
    return DSP_INVALID_ARG_VALUE;
  }

  num_mels_ = num_mels;
  min_hertz_ = min_hertz;
  max_hertz_ = max_hertz;
  if (mel_filter_banks_ != NULL) {
    delete[] mel_filter_banks_;
    mel_filter_banks_ = NULL;
  }
  mel_filter_banks_ = new FilterBank[num_mels_];

  float_t min_mel = convertHertzToMel(min_hertz_);
  float_t max_mel = convertHertzToMel(max_hertz_);
  unsigned int step = (max_mel - min_mel) / (num_mels_ + 1);

  for (unsigned int n = 0; n < num_mels_; n++) {
    float_t start_hertz = convertMelToHertz(min_mel + (float_t)(n * step));
    float_t end_hertz = convertMelToHertz(min_mel + (float_t)((n + 2) * step));

    unsigned int start = findFilterBankIndex(start_hertz);
    unsigned int length = findFilterBankIndex(end_hertz) - start;

    mel_filter_banks_[n].start = start;
    mel_filter_banks_[n].length = length;
    mel_filter_banks_[n].data = new float_t[length];

    // left side of filter bank, f(x) = px + q
    float_t p = (float_t)2.0 / (float_t)length;
    float_t q = -p * (float_t)start;
    for (unsigned int i = 0; i < length / 2; i++) {
      mel_filter_banks_[n].data[i] = p * (float_t)(start + i) + q;
    }

    // middle of filter bank
    mel_filter_banks_[n].data[length / 2] = 1;

    // right side of filter bank, f(x) = px + q
    p = (float_t)-2 / (float_t)length;
    q = -p * (float_t)(start + length);
    for (unsigned int i = length / 2 + 1; i < length; i++) {
      mel_filter_banks_[n].data[i] = p * (float_t)(start + i) + q;
    }
  }

  return DSP_SUCCESS;
}

int FeatureExtractor::initDctMatrix(const unsigned int num_mfcc) {

  if (num_mfcc == 0) {
    fprintf(stderr, "dsp::FeatureExtractor::initDctMatric() - `num_mfcc` must "
                    "be positive.\n");
    return DSP_INVALID_ARG_VALUE;
  }

  num_mfcc_ = num_mfcc;
  if (dct_matrix_ != NULL) {
    delete[] dct_matrix_;
    dct_matrix_ = NULL;
  }
  dct_matrix_ = new float_t[num_mels_ * num_mfcc_];
  if (dct_matrix_ == NULL) {
    return DSP_FAILED_MALLOC;
  }

  for (unsigned int i = 0; i < num_mels_; i++) {
    for (unsigned int j = 0; j < num_mfcc_; j++) {
      unsigned int ji = j * num_mels_ + i;
      dct_matrix_[ji] = std::cos((float_t)((i + 0.5) * j * M_PI) / (float_t)num_mels_);
      //dct_matrix_[ji] = std::cos((float_t)(i * (j + 0.5) * M_PI) / (float_t)num_mels_);
      //dct_matrix_[ji] = cos(M_PI / (float_t)(num_mels_ * (j + 0.5) * i));
    }
  }

  return DSP_SUCCESS;
}

int FeatureExtractor::getMagnitude(const float_t *const wave_frame_data,
                                   float_t *magnitude,
                                   float_t *temp_mem) {
  if (wave_frame_data == NULL) {
    fprintf(stderr, "dsp::FeatureExtractor::getMagnitude() - `wave_frame_data` "
                    "must be not NULL.\n");
    return DSP_INVALID_ARG_VALUE;
  }
  if (magnitude == NULL) {
    fprintf(stderr, "dsp::FeatureExtractor::getMagnitude() - `magnitude` must "
                    "be not NULL.\n");
    return DSP_INVALID_ARG_VALUE;
  }
  if (window_ == NULL) {
    fprintf(stderr, "dsp::FeatureExtractor::getMagnitude() - not initialized. "
                    "call init() first.\n");
    return DSP_INVALID_USAGE;
  }
  if (temp_mem == NULL) {
    fprintf(stderr, "dsp::FeatureExtractor::getMagnitude() - `temp_mem` must "
    "be not NULL.\n");
    return DSP_INVALID_ARG_VALUE;
  }

  // Apply windowing function
  memset(temp_mem, 0, sizeof(float_t) * (num_fft_point_ * 2));
  unsigned int start_idx = 0;
  if (is_center_) {
    start_idx = (num_fft_point_ - window_size_) / 2;
  }
  for (unsigned int i = 0; i < window_size_; i++) {
    temp_mem[start_idx + i] = wave_frame_data[i] * window_[i];
  }

  // Short-time Fourier transform
  int error_code = dsp::fft(temp_mem, num_fft_point_);
  if (error_code != DSP_SUCCESS) {
    fprintf(stderr,
            "dsp::FeatureExtractor::getMagnitude() - failed to call fft().\n");
    return error_code;
  }

  // Get magnitudes
  for (unsigned int i = 0; i < num_fft_point_ / 2 + 1; i++) {
    float_t real = temp_mem[i];
    float_t image = temp_mem[i + num_fft_point_];
    magnitude[i] = std::sqrt(real * real + image * image);
  }

  return DSP_SUCCESS;
}

int FeatureExtractor::getMel(const float_t *const magnitude,
                             float_t *mel_filter_bank_output) {

  if (magnitude == NULL) {
    fprintf(
        stderr,
        "dsp::FeatureExtractor::getMel() - `magnitude` must be not NULL.\n");
    return DSP_INVALID_ARG_VALUE;
  }
  if (mel_filter_bank_output == NULL) {
    fprintf(stderr, "dsp::FeatureExtractor::getMel() - "
                    "`mel_filter_bank_output` must be not NULL.\n");
    return DSP_INVALID_ARG_VALUE;
  }

  if (mel_filter_banks_ == NULL) {
    fprintf(stderr, "dsp::FeatureExtractor::getMel() - `mel_filter_banks` is "
                    "not initialized. call init() first.\n");
    return DSP_INVALID_USAGE;
  }

  for (unsigned int i = 0; i < num_mels_; i++) {
    float_t sum = 0;
    for (unsigned int j = 0; j < mel_filter_banks_[i].length; j++) {
      unsigned int idx = mel_filter_banks_[i].start + j;
      sum += magnitude[idx] * mel_filter_banks_[i].data[j];
    }
    mel_filter_bank_output[i] = sum;
  }

  return DSP_SUCCESS;
}

int FeatureExtractor::getMfcc(const float_t *const mel_filter_bank_output,
                              float_t *mfcc_output) {

  if (mel_filter_bank_output == NULL) {
    fprintf(stderr, "dsp::getMfcc::getMel() - `mel_filter_bank_output` must be "
                    "not NULL.\n");
    return DSP_INVALID_ARG_VALUE;
  }
  if (mfcc_output == NULL) {
    fprintf(stderr,
            "dsp::getMfcc::getMel() - `mfcc_output` must be not NULL.\n");
    return DSP_INVALID_ARG_VALUE;
  }

  if (dct_matrix_ == NULL) {
    fprintf(stderr, "dsp::FeatureExtractor::getMfcc() - `dct_matrix` is not "
                    "initialized. call init() first.\n");
    return DSP_INVALID_USAGE;
  }

  for (unsigned int j = 0; j < num_mfcc_; j++) {
    mfcc_output[j] = 0;
    for (unsigned int i = 0; i < num_mels_; i++) {
      unsigned int ji = j * num_mels_ + i;
      mfcc_output[j] += dct_matrix_[ji] * mel_filter_bank_output[i];
    }
  }

  return DSP_SUCCESS;
}

int FeatureExtractor::spectrum(const float_t *const wave_frame_data,
                               float_t *dest,
                               const bool logarize_output,
                               float_t *temp_mem) {
  int error_code;
  float_t *tmp = (temp_mem != NULL) ? temp_mem : tmp_buffer_;

  error_code = getMagnitude(wave_frame_data, dest, tmp);
  if (error_code != DSP_SUCCESS) {
    fprintf(
        stderr,
        "dsp::FeatureExtractor::spectrum() - failed to call getMagnitude().\n");
    return error_code;
  }

  if (logarize_output) {
    for (unsigned int i = 0; i < num_fft_point_ / 2 + 1; i++) {
      dest[i] = logarize(dest[i]);
    }
  }

  return DSP_SUCCESS;
}

int FeatureExtractor::melspectrum(const float_t *const wave_frame_data,
                                  float_t *dest,
                                  const bool logarize_output,
                                  float_t *temp_mem) {
  int error_code;
  float_t *tmp = (temp_mem != NULL) ? temp_mem : tmp_buffer_;

  error_code = getMagnitude(wave_frame_data, tmp, tmp);
  if (error_code != DSP_SUCCESS) {
    fprintf(stderr, "dsp::FeatureExtractor::melspectrum() - failed to call "
                    "getMagnitude().\n");
    return error_code;
  }

  error_code = getMel(tmp, dest);
  if (error_code != DSP_SUCCESS) {
    fprintf(
        stderr,
        "dsp::FeatureExtractor::melspectrum() - failed to call getMel().\n");
    return error_code;
  }

  if (logarize_output) {
    for (unsigned int i = 0; i < num_mels_; i++) {
      dest[i] = logarize(dest[i]);
    }
  }

  return DSP_SUCCESS;
}

int FeatureExtractor::mfcc(const float_t *const wave_frame_data,
                           float_t *dest,
                           float_t *temp_mem) {
  int error_code;
  float_t *tmp = (temp_mem != NULL) ? temp_mem : tmp_buffer_;

  error_code = getMagnitude(wave_frame_data, tmp, tmp);
  if (error_code != DSP_SUCCESS) {
    fprintf(stderr,
            "dsp::FeatureExtractor::mfcc() - failed to call getMagnitude().\n");
    return error_code;
  }

  // if num_fft_point > num_mels, getMel function can be operated in-place.
  error_code = getMel(tmp, tmp);
  if (error_code != DSP_SUCCESS) {
    fprintf(stderr,
            "dsp::FeatureExtractor::mfcc() - failed to call getMel().\n");
    return error_code;
  }

  for (unsigned int i = 0; i < num_mels_; i++) {
    tmp[i] = logarize(tmp[i]);
  }

  error_code = getMfcc(tmp, dest);
  if (error_code != DSP_SUCCESS) {
    fprintf(stderr,
            "dsp::FeatureExtractor::mfcc() - failed to call getMfcc().\n");
    return error_code;
  }

  return DSP_SUCCESS;
}

unsigned int FeatureExtractor::findFilterBankIndex(float_t hertz) {
  return (unsigned int)std::floor((num_fft_point_ + 1) * hertz /
                                  (float_t)sampling_rate_);
}

float_t FeatureExtractor::logarize(float_t x) {
  float_t tmp = (x < epsilon_) ? epsilon_ : x;
  return 20.0 * std::log10(tmp) - ref_level_db_;
}
} // namespace dsp