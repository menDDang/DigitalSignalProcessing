#ifndef DSP_DSP_H
#define DSP_DSP_H

// Error codes in dsp
#define DSP_SUCCESS 0
#define DSP_INVALID_ARG_VALUE -1
#define DSP_INVALID_USAGE -2
#define DSP_FAILED_MALLOC -3

namespace dsp {

#if USE_DOUBLE_PRECISION
typedef double float_t;
#else
typedef float float_t;
#endif

}  // namespace dsp

#endif  // DSP_DSP_H