#ifndef DSP_FFT_H
#define DSP_FFT_H

#include "dsp/dsp.h"

namespace dsp {

// In-place fast Fourier transform.
// Assume that length of `x` is 2*`num_fft_point`, where range of [0 ~
// `num_fft_point`) are filled with real part of x and [`num_fft_point`,
// 2*`num_fft_point`) are filled with imagenary part of x. And `num_fft_point`
// must be power of 2. Note that this function has O(N log(N)) time complextity.
int fft(float *x, unsigned int num_fft_point);
int fft(double *x, unsigned int num_fft_point);

int ifft(float *x, unsigned int num_fft_point);
int ifft(double *x, unsigned int num_fft_point);

} // namespace dsp

#endif // DSP_FFT_H