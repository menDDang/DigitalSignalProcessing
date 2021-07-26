#ifndef WAVE_WAVE_GAIN_H
#define WAVE_WAVE_GAIN_H

namespace wave {

// Compute Root Mean Square of input wav  
float computeRms(const float* const wav, const unsigned int wav_length);
double computeRms(const double* const wav, const unsigned int wav_length);

// Compute energy of input wav in DB scale
float computeDecibel(const float* const wav, const unsigned int wav_length);
double computeDecibel(const double* const wav, const unsigned int wav_length);

// Normalize input wav in given DB
int normalizeDb(float* wav, const unsigned int wav_length, const float db);
int normalizeDb(double* wav, const unsigned int wav_length, const double db);

}  // namespace wave

#endif  // WAVE_WAVE_GAIN_H