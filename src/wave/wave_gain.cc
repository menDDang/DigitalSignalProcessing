#include "wave/wave_gain.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>


namespace wave {

template <typename T>
T _computeRms(const T* const wav, const unsigned int wav_length) {
  T rms = 0;
  for (unsigned int i = 0; i < wav_length; i++) {
    rms += wav[i] * wav[i] / (T)wav_length;
  }
  rms = std::sqrt(rms);
  return rms;
}

template <typename T>
T _computeDecibel(const T* const wav, const unsigned int wav_length) {
  T rms = computeRms(wav, wav_length);
  return 20.0 * std::log10(rms);
}

template <typename T>
int _normalizeDbScale(T* wav, const unsigned int wav_length, const T db) {

  if ((wav == NULL) || (wav_length == 0) || (db > 0)){ 
    return 1;
  }

  T signal_level = _computeDecibel(wav, wav_length);
  T scalor = std::pow((T)10.0, (db - signal_level) / (T)20.0);

  for (unsigned int i = 0; i < wav_length; i++) {
    wav[i] *= scalor;
  }
  
  return 0;
}

float computeRms(const float* const wav, const unsigned int wav_length) {
  return _computeRms<float>(wav, wav_length);
}

float computeDecibel(const float* const wav, const unsigned int wav_length) {
  return _computeDecibel<float>(wav, wav_length);
}


double computeRms(const double* const wav, const unsigned int wav_length) {
  return _computeRms<double>(wav, wav_length);
}

double computeDecibel(const double* const wav, const unsigned int wav_length) {
  return _computeDecibel<double>(wav, wav_length);
}

int normalizeDb(float* wav, const unsigned int wav_length, const float db) {
  return _normalizeDbScale<float>(wav, wav_length, db);
}

int normalizeDb(double* wav, const unsigned int wav_length, const double db) {
  return _normalizeDbScale<double>(wav, wav_length, db);
}


}  // namespace wave


