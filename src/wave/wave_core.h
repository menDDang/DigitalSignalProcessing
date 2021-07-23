#ifndef WAVE_WAVE_CORE_H
#define WAVE_WAVE_CORE_H

#include <stdio.h>

namespace wave {

class WaveCore {
public:
  enum wave_core_mode_t { 
    kWaveCoreModeNotSetted, 
    kWaveCoreModeReadOnly, 
    kWaveCoreModeWriteOnly 
  };

  WaveCore();

  virtual ~WaveCore();

private:
  unsigned int sampling_rate_;
  unsigned int bit_rate_;
  unsigned int num_channels_;

  wave_core_mode_t mode_;

  FILE *fp_;
  size_t data_position_;

  size_t num_bytes_;
  unsigned char *bytes_;

  bool is_initialized_;

public:
  unsigned int getSamplingRate() const { return sampling_rate_; }
  unsigned int getBitRate() const { return bit_rate_; }
  unsigned int getNumChannels() const { return num_channels_; }
  unsigned int getNumBytes() const { return num_bytes_; }
  const unsigned char* getBytePtr() const { return bytes_; }

  // Initialize variables
  int init(const unsigned int sampling_rate, const unsigned int bit_rate,
           const unsigned int num_channels, const char *file_name,
           const wave_core_mode_t mode);

  // Clear all variables in this class, including `mode_`
  // After clear, you must re-initialize this class using init() method
  void clear();

  int readHeader();
  int readData();

  // Write header and data
  int write();

  // Push byte data
  int setData(const unsigned char *bytes, const unsigned int num_bytes);

  // Pull byte data length of `num_bytes_`
  // Before call this function, you must allocate `bytes` first
  // Note that length of bytes can be known by using `getNumBytes()`
  int getData(unsigned char *bytes);

}; // class WaveCore

} // namespace wave

#endif // WAVE_WAVE_CORE_H