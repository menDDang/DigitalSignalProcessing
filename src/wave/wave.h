#ifndef WAVE_WAVE_H
#define WAVE_WAVE_H

// Error codes in wave
#define WAVE_SUCCESS 0
#define WAVE_INVALID_ARG_VALUE -1
#define WAVE_INVALID_USAGE -2
#define WAVE_MALLOC_FAILED -3
#define WAVE_FILE_IO_FAILED -4
#define WAVE_UNSUPPORTED_TYPE -5
#define WAVE_INVALID_FORMAT -6

namespace wave {

// Supported sampling rates
enum sampling_rate_t {
  kWaveSamplingRate8K = 8000,
  kWaveSamplingRate16K = 16000,
  kWaveSamplingRate44100 = 44100,
  kWaveSamplingRate44K = 44000
};

// Supported bit rates
enum bit_rate_t {
  kWaveBitRate8 = 8,
  kWaveBitRate16 = 16
};

// Supported channel number
enum channel_num_t {
  kWaveChannelMono = 1,
  kWaveChannelStereo = 2,
  kWaveChannelOcta = 8
};

class WaveCore;

// Convert floating point (single precision) samples into char
// Note that this function is available for not only mono channel, but also
// multi channels. For processing multi channel (N) data, you must set order of data
// as followings :
//     {x[0][t], x[1][t], ... , x[N-1][t]}, {x[0][t+1], x[1][t+1], ... , x[N-1][t+1]} 
//     where x[c][t] denotes data of x in channel c at time t
int convertFloat2Char(const unsigned int bit_rate, const float *src,
                      const unsigned int num_samples, unsigned char **dest,
                      unsigned int *num_bytes);

// Convert floating point (double precision) samples into char
// Note that this function is available for not only mono channel, but also
// multi channels. For processing multi channel (N) data, you must set order of data
// as followings :
//     {x[0][t], x[1][t], ... , x[N-1][t]}, {x[0][t+1], x[1][t+1], ... , x[N-1][t+1]} 
//     where x[c][t] denotes data of x in channel c at time t
int convertFloat2Char(const unsigned int bit_rate, const double *src,
                      const unsigned int num_samples, unsigned char **dest,
                      unsigned int *num_bytes);

// Convert bytes data into floating point (single precision)
// Note that this function is available for not only mono channel, but also
// multi channels. For processing multi channel (N) data, you must set order of data
// as followings :
//     {x[0][t], x[1][t], ... , x[N-1][t]}, {x[0][t+1], x[1][t+1], ... , x[N-1][t+1]} 
//     where x[c][t] denotes data of x in channel c at time t
int convertChar2Float(const unsigned int bit_rate, const unsigned char *src,
                      const unsigned int num_bytes, float **dest,
                      unsigned int *num_samples);

// Convert bytes data into floating point (double precision)
// Note that this function is available for not only mono channel, but also
// multi channels. For processing multi channel (N) data, you must set order of data
// as followings :
//     {x[0][t], x[1][t], ... , x[N-1][t]}, {x[0][t+1], x[1][t+1], ... , x[N-1][t+1]} 
//     where x[c][t] denotes data of x in channel c at time t
int convertChar2Float(const unsigned int bit_rate, const unsigned char *src,
                      const unsigned int num_bytes, double **dest,
                      unsigned int *num_samples);

class WaveReader {
public:
  WaveReader(const unsigned int sampling_rate = 0,
             const unsigned int bit_rate = 0,
             const unsigned int num_channels = 0);

  virtual ~WaveReader();

private:
  unsigned int sampling_rate_;
  unsigned int bit_rate_;
  unsigned int num_channels_;

  WaveCore *core_;

public:
  unsigned int getSamplingRate() const { return sampling_rate_; }
  unsigned int getBitRate() const { return bit_rate_; }
  unsigned int getNumChannels() const { return num_channels_; }

  int init(const unsigned int sampling_rate, const unsigned int bit_rate,
            const unsigned int num_channels);

  int read(const char *file_name, float **dest, unsigned int *dest_size);
  int read(const char *file_name, double **dest, unsigned int *dest_size);
}; // class WaveReader

class WaveWriter {
public:
  WaveWriter(const unsigned int sampling_rate = 0,
             const unsigned int bit_rate = 0,
             const unsigned int num_channels = 0);

  virtual ~WaveWriter();

private:
  unsigned int sampling_rate_;
  unsigned int bit_rate_;
  unsigned int num_channels_;

  WaveCore *core_;

public:
  unsigned int getSamplingRate() const { return sampling_rate_; }
  unsigned int getBitRate() const { return bit_rate_; }
  unsigned int getNumChannels() const { return num_channels_; }

  int init(const unsigned int sampling_rate, const unsigned int bit_rate,
            const unsigned int num_channels);

  int write(const char *file_name, const float *src,
            const unsigned int src_size);
  int write(const char *file_name, const double *src,
            const unsigned int src_size);
}; // class WaveWriter

} // namespace wave

#endif // WAVE_WAVE_H