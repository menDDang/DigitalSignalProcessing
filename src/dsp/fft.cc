#include "dsp/fft.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>

namespace complex {

template <typename T>
void add(const T x_real, const T x_image, const T y_real, const T y_image,
         T &out_real, T &out_image) {
  out_real = x_real + y_real;
  out_image = x_image + y_image;
}

template <typename T>
void subtract(const T x_real, const T x_image, const T y_real, const T y_image,
              T &out_real, T &out_image) {
  out_real = x_real - y_real;
  out_image = x_image - y_image;
}

template <typename T>
void multiply(const T x_real, const T x_image, const T y_real, const T y_image,
              T &out_real, T &out_image) {
  out_real = x_real * y_real - x_image * y_image;
  out_image = x_real * y_image + x_image * y_real;
}

/*
template <typename T>
void devide(const T x_real, const T x_image, const T y_real, const T y_image,
            T &out_real, T &out_image) {
  const T denominator = y_real * y_real + x_image * x_image;
  out_real = (x_real * y_real + x_image * y_image) / denominator;
  out_image = (x_image * y_real - x_real * y_image) / denominator;
}
*/

// Convert complex number expressed in polar coordinate
// into Cartesian coordinate system
template <typename T>
void convertPolarToCartesian(const T radius, const T angle, T &out_real,
                             T &out_image) {
  out_real = radius * std::cos(angle);
  out_image = radius * std::sin(angle);
}

} // namespace complex

namespace dsp {

int getBitReversal(const int bit, int num_bits) {

  int reversed_bit, i;
  for (reversed_bit = 0, i = 0; i < num_bits; ++i) {
    reversed_bit |= ((bit >> i) & 1) << (num_bits - i - 1);
  }
  return reversed_bit;
}

// In-place fast Fourier transform.
// Assume that length of `x` is 2*`num_fft_point`, where range of [0 ~
// `num_fft_point`) are filled with real part of x and [`num_fft_point`,
// 2*`num_fft_point`) are filled with imagenary part of x. And `num_fft_point`
// must be power of 2. Note that this function has O(N log(N)) time complextity.
template <typename T>
int fft_fn(T *x, unsigned int num_fft_point, bool inverse = false) {

  if ((x == NULL) || (num_fft_point == 0)) {
    return DSP_INVALID_ARG_VALUE;
  }

  // Permutate x using bit reversal
  unsigned int fft_order = (unsigned int)log2(num_fft_point);
  for (unsigned int i = 0; i < num_fft_point; i++) {
    unsigned int reversed_i = getBitReversal(i, fft_order);
    if (i > reversed_i) {
      T tmp = x[i];
      x[i] = x[reversed_i];
      x[reversed_i] = tmp;
    }
  }

  T pi = inverse ? (T)M_PI : (T)-M_PI;
  for (unsigned int block_len = 2; block_len <= num_fft_point;
       block_len = (block_len << 1)) {
    unsigned int block_num = num_fft_point / block_len;
    for (unsigned int n = 0; n < block_len / 2; n++) {
      T w_real, w_image;
      complex::convertPolarToCartesian(
          (T)1.0, (T)(-2.0 * pi * n) / (T)block_len, w_real, w_image);
      for (unsigned int k = 0; k < block_num; k++) {
        unsigned int idx = k * block_len + n; // k-th block, n-th element
        T w_odd_real, w_odd_image;
        complex::multiply(x[idx + block_len / 2],
                          x[idx + block_len / 2 + num_fft_point], w_real,
                          w_image, w_odd_real, w_odd_image);
        complex::subtract(x[idx], x[idx + num_fft_point], w_odd_real,
                          w_odd_image, x[idx + block_len / 2],
                          x[idx + block_len / 2 + num_fft_point]);
        complex::add(x[idx], x[idx + num_fft_point], w_odd_real, w_odd_image,
                     x[idx], x[idx + num_fft_point]);
      }
    }
  }

  if (inverse) {
    for (unsigned int i = 0; i < num_fft_point; i++) {
      x[i] /= (T)num_fft_point;
    }
  }

  return DSP_SUCCESS;
}

int fft(float *x, unsigned int num_fft_point) {
  return fft_fn<float>(x, num_fft_point, false);
}

int fft(double *x, unsigned int num_fft_point) {
  return fft_fn<double>(x, num_fft_point, false);
}

int ifft(float *x, unsigned int num_fft_point) {
  return fft_fn<float>(x, num_fft_point, true);
}
int ifft(double *x, unsigned int num_fft_point) {
  return fft_fn<double>(x, num_fft_point, true);
}

} // namespace dsp
