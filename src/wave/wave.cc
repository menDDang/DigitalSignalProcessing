#include "wave/wave.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wave/wave_core.h"

#ifndef BITS_PER_BYTE
#define BITS_PER_BYTE 8
#endif

namespace wave {

template <typename T>
void convert_float_to_char(const unsigned int bit_rate, const T *src,
                           unsigned int num_samples, unsigned char **dest,
                           unsigned int *num_bytes) {

  unsigned int _num_bytes = num_samples * (bit_rate / BITS_PER_BYTE);
  unsigned char *bytes = new unsigned char[_num_bytes];
  memset((void *)bytes, 0, sizeof(unsigned char) * _num_bytes);

  T scale = std::pow(2.0, (T)(bit_rate - 1));

  switch (bit_rate) {
  case 8: {
    // 8 bits for one sample
    for (unsigned int i = 0; i < num_samples; i++) {
      int tmp = (int)(src[i] * scale) + 128;
      bytes[i] = (unsigned char)tmp;
    }
    break;
  }
  case 16: {
    // 16 bits for one sample
    short *dest_16 = (short *)bytes;
    for (unsigned int i = 0; i < num_samples; i++) {
      short tmp = (short)(src[i] * scale);
      dest_16[i] = tmp;
    }
    break;
  }

  default:
    break;
  }

  (*dest) = bytes;
  (*num_bytes) = _num_bytes;
  bytes = NULL;
}

template <typename T>
void convert_char_to_float(const unsigned int bit_rate,
                           const unsigned char *src,
                           const unsigned int num_bytes, T **dest,
                           unsigned int *num_samples) {

  unsigned int _num_samples = num_bytes / (bit_rate / BITS_PER_BYTE);
  T *samples = new T[_num_samples];
  memset((void *)samples, 0, sizeof(T) * _num_samples);

  T scale = std::pow(2.0, (T)(bit_rate - 1));

  switch (bit_rate) {
  case 8: {
    for (unsigned int i = 0; i < _num_samples; i++) {
      int tmp = (int)src[i] - 128;
      samples[i] = (T)tmp / scale;
    }
    break;
  }

  case 16: {
    short *src_16 = (short *)src;
    for (unsigned int i = 0; i < _num_samples; i++) {
      int tmp = src_16[i];
      samples[i] = (T)tmp / scale;
    }
    break;
  }
  default:
    break;
  }

  (*dest) = samples;
  (*num_samples) = _num_samples;
  samples = NULL;
}

int convertFloat2Char(const unsigned int bit_rate, const float *src,
                      const unsigned int num_samples, unsigned char **dest,
                      unsigned int *num_bytes) {

  // Check arguments
  if ((bit_rate == 0) || (src == NULL) || (num_samples == 0) ||
      (dest == NULL) || (num_bytes == NULL)) {
    return WAVE_INVALID_ARG_VALUE;
  }

  // Release pre-allocated memory
  if ((*dest) != NULL) {
    delete[](*dest);
    (*dest) = NULL;
  }

  convert_float_to_char<float>(bit_rate, src, num_samples, dest, num_bytes);
  return WAVE_SUCCESS;
}

int convertFloat2Char(const unsigned int bit_rate, const double *src,
                      const unsigned int num_samples, unsigned char **dest,
                      unsigned int *num_bytes) {

  // Check arguments
  if ((bit_rate == 0) || (src == NULL) || (num_samples == 0) ||
      (dest == NULL) || (num_bytes == NULL)) {
    return WAVE_INVALID_ARG_VALUE;
  }

  // Release pre-allocated memory
  if ((*dest) != NULL) {
    delete[](*dest);
    (*dest) = NULL;
  }

  convert_float_to_char<double>(bit_rate, src, num_samples, dest, num_bytes);
  return WAVE_SUCCESS;
}

int convertChar2Float(const unsigned int bit_rate, const unsigned char *src,
                      const unsigned int num_bytes, float **dest,
                      unsigned int *num_samples) {

  // Check arguments
  if ((bit_rate == 0) || (src == NULL) || (num_bytes == 0) || (dest == NULL) ||
      (num_samples == NULL)) {
    return WAVE_INVALID_ARG_VALUE;
  }

  // Release pre-allocated memory
  if ((*dest) != NULL) {
    delete[](*dest);
    (*dest) = NULL;
  }

  convert_char_to_float<float>(bit_rate, src, num_bytes, dest, num_samples);
  return WAVE_SUCCESS;
}

int convertChar2Float(const unsigned int bit_rate, const unsigned char *src,
                      const unsigned int num_bytes, double **dest,
                      unsigned int *num_samples) {

  // Check arguments
  if ((bit_rate == 0) || (src == NULL) || (num_bytes == 0) || (dest == NULL) ||
      (num_samples == NULL)) {
    return WAVE_INVALID_ARG_VALUE;
  }

  // Release pre-allocated memory
  if ((*dest) != NULL) {
    delete[](*dest);
    (*dest) = NULL;
  }

  convert_char_to_float<double>(bit_rate, src, num_bytes, dest, num_samples);
  return WAVE_SUCCESS;
}

WaveReader::WaveReader(const unsigned int sampling_rate,
                       const unsigned int bit_rate,
                       const unsigned int num_channels)
    : sampling_rate_(sampling_rate), bit_rate_(bit_rate),
      num_channels_(num_channels), core_(NULL) {
  if (sampling_rate_ != 0 && bit_rate_ != 0 && num_channels_ != 0) {
    init(sampling_rate_, bit_rate_, num_channels_);
  }
}

WaveReader::~WaveReader() {
  if (core_ != NULL) {
    delete core_;
    core_ = NULL;
  }
}

int WaveReader::init(const unsigned int sampling_rate,
                     const unsigned int bit_rate,
                     const unsigned int num_channels) {

  switch (sampling_rate) {
  case kWaveSamplingRate8K:
  case kWaveSamplingRate16K:
  case kWaveSamplingRate44100:
  case kWaveSamplingRate44K:
    break;

  default:
    return WAVE_UNSUPPORTED_TYPE;
  }

  switch (bit_rate) {
  case kWaveBitRate8:
  case kWaveBitRate16:
    break;

  default:
    return WAVE_UNSUPPORTED_TYPE;
  }

  switch (num_channels) {
  case kWaveChannelMono:
  case kWaveChannelStereo:
  case kWaveChannelOcta:
    break;

  default:
    return WAVE_UNSUPPORTED_TYPE;
  }

  sampling_rate_ = sampling_rate;
  bit_rate_ = bit_rate;
  num_channels_ = num_channels;

  if (core_ == NULL) {
    core_ = new WaveCore();
  }

  return WAVE_SUCCESS;
}

int WaveReader::read(const char *file_name, float **dest,
                     unsigned int *dest_size) {
  int error_code;

  if ((file_name == NULL) || (dest == NULL) || (dest_size == NULL)) {
    return WAVE_INVALID_ARG_VALUE;
  }

  if (core_ == NULL) {
    error_code = init(sampling_rate_, bit_rate_, num_channels_);
    if (error_code != WAVE_SUCCESS) {
      return error_code;
    }
  }

  error_code = core_->init(sampling_rate_, bit_rate_, num_channels_, file_name,
                           WaveCore::kWaveCoreModeReadOnly);
  if (error_code != WAVE_SUCCESS) {
    return error_code;
  }

  error_code = core_->readHeader();
  if (error_code != WAVE_SUCCESS) {
    return error_code;
  }

  error_code = core_->readData();
  if (error_code != WAVE_SUCCESS) {
    return error_code;
  }

  error_code = convertChar2Float(bit_rate_, core_->getBytePtr(),
                                 core_->getNumBytes(), dest, dest_size);
  if (error_code != WAVE_SUCCESS) {
    return error_code;
  }

  return WAVE_SUCCESS;
}

int WaveReader::read(const char *file_name, double **dest,
                     unsigned int *dest_size) {
  int error_code;

  if ((file_name == NULL) || (dest == NULL) || (dest_size == NULL)) {
    return WAVE_INVALID_ARG_VALUE;
  }

  if (core_ == NULL) {
    error_code = init(sampling_rate_, bit_rate_, num_channels_);
    if (error_code != WAVE_SUCCESS) {
      return error_code;
    }
  }

  error_code = core_->init(sampling_rate_, bit_rate_, num_channels_, file_name,
                           WaveCore::kWaveCoreModeReadOnly);
  if (error_code != WAVE_SUCCESS) {
    return error_code;
  }

  error_code = core_->readHeader();
  if (error_code != WAVE_SUCCESS) {
    return error_code;
  }

  error_code = core_->readData();
  if (error_code != WAVE_SUCCESS) {
    return error_code;
  }

  error_code = convertChar2Float(bit_rate_, core_->getBytePtr(),
                                 core_->getNumBytes(), dest, dest_size);
  if (error_code != WAVE_SUCCESS) {
    return error_code;
  }

  return WAVE_SUCCESS;
}

WaveWriter::WaveWriter(const unsigned int sampling_rate,
                       const unsigned int bit_rate,
                       const unsigned int num_channels)
    : sampling_rate_(sampling_rate), bit_rate_(bit_rate),
      num_channels_(num_channels), core_(NULL) {
  if (sampling_rate_ != 0 && bit_rate_ != 0 && num_channels_ != 0) {
    init(sampling_rate_, bit_rate_, num_channels_);
  }
}

WaveWriter::~WaveWriter() {
  if (core_ != NULL) {
    delete core_;
    core_ = NULL;
  }
}

int WaveWriter::init(const unsigned int sampling_rate,
                     const unsigned int bit_rate,
                     const unsigned int num_channels) {

  switch (sampling_rate) {
  case kWaveSamplingRate8K:
  case kWaveSamplingRate16K:
  case kWaveSamplingRate44100:
  case kWaveSamplingRate44K:
    break;

  default:
    return WAVE_UNSUPPORTED_TYPE;
  }

  switch (bit_rate) {
  case kWaveBitRate8:
  case kWaveBitRate16:
    break;

  default:
    return WAVE_UNSUPPORTED_TYPE;
  }

  switch (num_channels) {
  case kWaveChannelMono:
  case kWaveChannelStereo:
  case kWaveChannelOcta:
    break;

  default:
    return WAVE_UNSUPPORTED_TYPE;
  }

  sampling_rate_ = sampling_rate;
  bit_rate_ = bit_rate;
  num_channels_ = num_channels;

  if (core_ == NULL) {
    core_ = new WaveCore();
  }

  return WAVE_SUCCESS;
}

int WaveWriter::write(const char *file_name, const float *src,
                      const unsigned int src_size) {

  int error_code;

  if ((file_name == NULL) || (src == NULL) || (src_size == 0)) {
    return WAVE_INVALID_ARG_VALUE;
  }

  if (core_ == NULL) {
    error_code = init(sampling_rate_, bit_rate_, num_channels_);
    if (error_code != WAVE_SUCCESS) {
      return error_code;
    }
  }

  error_code = core_->init(sampling_rate_, bit_rate_, num_channels_, file_name,
                           WaveCore::kWaveCoreModeWriteOnly);
  if (error_code != WAVE_SUCCESS) {
    return error_code;
  }

  unsigned int num_bytes = 0;
  unsigned char *bytes = NULL;
  error_code = convertFloat2Char(bit_rate_, src, src_size, &bytes, &num_bytes);
  if (error_code != WAVE_SUCCESS) {
    if (bytes != NULL) {
      delete[] bytes;
    }
    return error_code;
  }

  error_code = core_->setData(bytes, num_bytes);
  if (error_code != WAVE_SUCCESS) {
    if (bytes != NULL) {
      delete[] bytes;
    }
    return error_code;
  }

  error_code = core_->write();
  if (error_code != WAVE_SUCCESS) {
    if (bytes != NULL) {
      delete[] bytes;
    }
    return error_code;
  }

  if (bytes != NULL) {
    delete[] bytes;
  }

  return WAVE_SUCCESS;
}

int WaveWriter::write(const char *file_name, const double *src,
                      const unsigned int src_size) {

  int error_code;

  if ((file_name == NULL) || (src == NULL) || (src_size == 0)) {
    return WAVE_INVALID_ARG_VALUE;
  }

  if (core_ == NULL) {
    error_code = init(sampling_rate_, bit_rate_, num_channels_);
    if (error_code != WAVE_SUCCESS) {
      return error_code;
    }
  }

  error_code = core_->init(sampling_rate_, bit_rate_, num_channels_, file_name,
                           WaveCore::kWaveCoreModeWriteOnly);
  if (error_code != WAVE_SUCCESS) {
    return error_code;
  }

  unsigned int num_bytes = 0;
  unsigned char *bytes = NULL;
  error_code = convertFloat2Char(bit_rate_, src, src_size, &bytes, &num_bytes);
  if (error_code != WAVE_SUCCESS) {
    if (bytes != NULL) {
      delete[] bytes;
    }
    return error_code;
  }

  error_code = core_->setData(bytes, num_bytes);
  if (error_code != WAVE_SUCCESS) {
    if (bytes != NULL) {
      delete[] bytes;
    }
    return error_code;
  }

  error_code = core_->write();
  if (error_code != WAVE_SUCCESS) {
    if (bytes != NULL) {
      delete[] bytes;
    }
    return error_code;
  }

  if (bytes != NULL) {
    delete[] bytes;
  }

  return WAVE_SUCCESS;
}
} // namespace wave