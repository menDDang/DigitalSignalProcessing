#include "wave/wave_core.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wave/wave.h"

typedef unsigned char byte;
typedef unsigned short WORD;
typedef unsigned int DWORD;

#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3)                                         \
  ((DWORD)(byte)(ch0) | ((DWORD)(byte)(ch1) << 8) |                            \
   ((DWORD)(byte)(ch2) << 16) | ((DWORD)(byte)(ch3) << 24))
#endif

#ifndef PCM_FILE_FORMAT
#define PCM_FILE_FORMAT 1
#endif

#ifndef RIFF_WAVE_HEADER_SIZE
#define RIFF_WAVE_HEADER_SIZE 44
#endif

#ifndef WAVE_HEADER_SIZE
#define WAVE_HEADER_SIZE (RIFF_WAVE_HEADER_SIZE - 8)
#endif

#ifndef FORMAT_CHUNK_SIZE
#define FORMAT_CHUNK_SIZE 16
#endif

#ifndef CHUNK_NAME_LENGTH
#define CHUNK_NAME_LENGTH 4
#endif

#ifndef BITS_PER_BYTE
#define BITS_PER_BYTE 8
#endif

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

namespace wave {

typedef struct _chunk_info_struct {
  DWORD id;          // chunk id
  DWORD size;        // chunk size
  DWORD fcc_type;    // format type or list type
  DWORD data_offset; // offset of data portion of chunk
} ChunkInfo;

typedef struct _wave_format_struct {
  WORD type;                   // format type
  WORD num_channels;           // number of channels
  DWORD num_samples_per_sec;   // sampling rate
  DWORD num_avg_bytes_per_sec; // for buffer estimation
  WORD num_block_align;        // block size of data
  WORD num_bits_per_sec;       // bit rate
  WORD cb_size; // the count in bytes of the size of extra information (after
                // cbSize)
} WaveFormatEx;

////////////////////////////////
// Prototype of local functions
int find_riff_chunk(FILE *fp, ChunkInfo *chunk);
int find_chunk(FILE *fp, ChunkInfo *chunk, ChunkInfo *riff_chunk);

WaveCore::WaveCore()
    : sampling_rate_(0), bit_rate_(0), num_channels_(0),
      mode_(kWaveCoreModeNotSetted), fp_(NULL), num_bytes_(0), bytes_(NULL),
      is_initialized_(false) {}

WaveCore::~WaveCore() {
  if (fp_ != NULL) {
    fclose(fp_);
    fp_ = NULL;
  }

  if (bytes_ != NULL) {
    delete[] bytes_;
    bytes_ = NULL;
  }
}

int WaveCore::init(const unsigned int sampling_rate,
                   const unsigned int bit_rate, const unsigned int num_channels,
                   const char *file_name, const wave_core_mode_t mode) {

  if (file_name == NULL) {
    fprintf(stderr, "wave::WaveCore::init() - invalid argument `file_name`.\n");
    return WAVE_INVALID_ARG_VALUE;
  }
  if ((mode != kWaveCoreModeReadOnly) && (mode != kWaveCoreModeWriteOnly)) {
    fprintf(stderr, "wave::WaveCore::init() - invalid argument `mode`.\n");
    return WAVE_INVALID_ARG_VALUE;
  }

  clear();

  sampling_rate_ = sampling_rate;
  bit_rate_ = bit_rate;
  num_channels_ = num_channels;
  mode_ = mode;

  if (mode == kWaveCoreModeReadOnly) {
    fp_ = fopen(file_name, "rb");
  } else if (mode == kWaveCoreModeWriteOnly) {
    fp_ = fopen(file_name, "wb");
  }

  if (fp_ == NULL) {
    fprintf(stderr, "wave::WaveCore::init() - failed to open file %s\n",
            file_name);
    return WAVE_FILE_IO_FAILED;
  }

  is_initialized_ = true;

  return WAVE_SUCCESS;
}

void WaveCore::clear() {

  sampling_rate_ = 0;
  bit_rate_ = 0;
  num_channels_ = 0;

  mode_ = kWaveCoreModeNotSetted;

  if (fp_ != NULL) {
    fclose(fp_);
    fp_ = NULL;
  }

  num_bytes_ = 0;
  if (bytes_ != NULL) {
    delete[] bytes_;
    bytes_ = NULL;
  }

  is_initialized_ = false;
}

int WaveCore::readHeader() {
  ChunkInfo riff_chunk = {
      0,
  };
  ChunkInfo chunk = {
      0,
  };
  WaveFormatEx wave_format = {
      0,
  };
  unsigned int num_bytes_to_read;
  int ret;

  // check arguments
  if (mode_ != kWaveCoreModeReadOnly) {
    fprintf(stderr, "WaveCore::readHeader() - Can not use this method in "
                    "WRITE_ONLY mode.\n");
    return WAVE_INVALID_USAGE;
  }
  if (fp_ == NULL) {
    fprintf(stderr, "Wav::readHeader() - fp_ is NULL.\n");
    return WAVE_INVALID_ARG_VALUE;
  }

  // find RIFF chunk
  ret = find_riff_chunk(fp_, &riff_chunk);
  if (ret != WAVE_SUCCESS) {
    fprintf(stderr, "WaveCore::readHeader() - Fail to find RIFF chunk.\n");
    return WAVE_FILE_IO_FAILED;
  }

  // find fmt chunk
  chunk.id = MAKEFOURCC('f', 'm', 't', ' ');
  ret = find_chunk(fp_, &chunk, &riff_chunk);
  if (ret != WAVE_SUCCESS) {
    fprintf(stderr, "WaveCore::readHeader() - Fail to find fmt chunk.\n");
    return WAVE_FILE_IO_FAILED;
  }

  fseek(fp_, chunk.data_offset, SEEK_SET);
  num_bytes_to_read = min(sizeof(wave_format), chunk.size);
  if (1 != fread(&wave_format, num_bytes_to_read, 1, fp_)) {
    fprintf(stderr, "WaveCore::readHeader() - Fail to read format chunk.\n");
    return WAVE_FILE_IO_FAILED;
  }

  if (wave_format.num_channels != num_channels_) {
    fprintf(stderr,
            "WaveCore::readHeader() - Invalid input file. Expected number of "
            "channels : %d, but given %d\n",
            (int)wave_format.num_channels, (int)num_channels_);
    return WAVE_INVALID_FORMAT;
  }

  if (wave_format.num_samples_per_sec != sampling_rate_) {
    fprintf(stderr,
            "WaveCore::readHeader() - Invalid input file. Expected sampling "
            "rate : %d, but given %d\n",
            (int)wave_format.num_samples_per_sec, (int)sampling_rate_);
    return WAVE_INVALID_FORMAT;
  }

  if (wave_format.num_bits_per_sec != bit_rate_) {
    fprintf(stderr,
            "WaveCore::readHeader() - Invalid input file. Expected sampling "
            "rate : %d, but given %d\n",
            (int)wave_format.num_bits_per_sec, (int)bit_rate_);
    return WAVE_INVALID_FORMAT;
  }

  // find data chunk
  chunk.id = MAKEFOURCC('d', 'a', 't', 'a');
  ret = find_chunk(fp_, &chunk, &riff_chunk);
  if (ret != WAVE_SUCCESS) {
    fprintf(stderr, "WaveCore::readHeader() - Fail to find data chunk.\n");
    return WAVE_FILE_IO_FAILED;
  }

  num_bytes_ = chunk.size;
  data_position_ = chunk.data_offset;
  fseek(fp_, data_position_, SEEK_SET);

  return WAVE_SUCCESS;
}

int WaveCore::readData() {

  int error_code;

  if (num_bytes_ == 0) {
    error_code = readHeader();
    if (error_code != WAVE_SUCCESS) {
      return error_code;
    }
  }

  if (bytes_ != NULL) {
    delete[] bytes_;
    bytes_ = NULL;
  }
  bytes_ = new byte[num_bytes_];
  memset((void *)bytes_, 0, sizeof(byte) * num_bytes_);

  if (num_bytes_ != fread(bytes_, sizeof(byte), num_bytes_, fp_)) {
    fprintf(stderr, "WaveCore::readBytes() -  Fail to read data.\n");
    return WAVE_FILE_IO_FAILED;
  }

  return WAVE_SUCCESS;
}

int WaveCore::write() {

  // check format was setted
  if (sampling_rate_ == 0) {
    fprintf(
        stderr,
        "WaveCore::write() - 'sampling_rate_' was not setted. Before call this "
        "method, set sampling rate using init().\n");
    return WAVE_INVALID_USAGE;
  }
  if (bit_rate_ == 0) {
    fprintf(stderr,
            "WaveCore::write() - 'bit_rate_' was not setted. Before call this "
            "method, set bit rate using init().\n");
    return WAVE_INVALID_USAGE;
  }
  if (num_channels_ == 0) {
    fprintf(
        stderr,
        "WaveCore::write() - 'num_channels_' was not setted. Before call this "
        "method, set number of channels using init().\n");
    return WAVE_INVALID_USAGE;
  }
  if (bytes_ == NULL) {
    fprintf(stderr,
            "WaveCore::write() - 'bytes_' was not setted. Before call this "
            "method, set data using setFloatData() or setBytes().\n");
    return WAVE_INVALID_USAGE;
  }

  byte header[RIFF_WAVE_HEADER_SIZE];
  int byteRate = bit_rate_ / BITS_PER_BYTE * num_channels_ * sampling_rate_;
  int blockAlign = bit_rate_ / BITS_PER_BYTE * num_channels_;
  // DWORD totalDataLen = num_bytes_ + FORMAT_CHUNK_SIZE + 20;  // 20 for 'WAVE'
  // chunk and 'fmt ' chunk
  DWORD totalDataLen = WAVE_HEADER_SIZE + num_bytes_;

  // set riff chunk
  header[0] = 'R';
  header[1] = 'I';
  header[2] = 'F';
  header[3] = 'F';

  header[4] = (byte)(totalDataLen & 0xff);
  header[5] = (byte)((totalDataLen >> 8) & 0xff);
  header[6] = (byte)((totalDataLen >> 16) & 0xff);
  header[7] = (byte)((totalDataLen >> 24) & 0xff);

  // set WAVE chunk
  header[8] = 'W';
  header[9] = 'A';
  header[10] = 'V';
  header[11] = 'E';

  // set format chunk
  header[12] = 'f';
  header[13] = 'm';
  header[14] = 't';
  header[15] = ' ';

  header[16] = (byte)FORMAT_CHUNK_SIZE;
  header[17] = 0;
  header[18] = 0;
  header[19] = 0;

  header[20] = (byte)PCM_FILE_FORMAT;
  header[21] = 0;

  header[22] = (byte)num_channels_;
  header[23] = 0;

  header[24] = (byte)(sampling_rate_ & 0xff);
  header[25] = (byte)((sampling_rate_ >> 8) & 0xff);
  header[26] = (byte)((sampling_rate_ >> 16) & 0xff);
  header[27] = (byte)((sampling_rate_ >> 24) & 0xff);

  header[28] = (byte)(byteRate & 0xff);
  header[29] = (byte)((byteRate >> 8) & 0xff);
  header[30] = (byte)((byteRate >> 16) & 0xff);
  header[31] = (byte)((byteRate >> 24) & 0xff);

  header[32] = (byte)(blockAlign & 0xff);
  header[33] = (byte)((blockAlign >> 8) & 0xff);

  header[34] = (byte)bit_rate_;
  header[35] = 0;

  // set data chunk
  header[36] = 'd';
  header[37] = 'a';
  header[38] = 't';
  header[39] = 'a';

  header[40] = (byte)(num_bytes_ & 0xff);
  header[41] = (byte)((num_bytes_ >> 8) & 0xff);
  header[42] = (byte)((num_bytes_ >> 16) & 0xff);
  header[43] = (byte)((num_bytes_ >> 24) & 0xff);

  // reset file pointer
  if (fseek(fp_, 0L, SEEK_SET) != 0) {
    fprintf(stderr, "WaveCore::write() - Fail to set file pointer.\n");
    return WAVE_FILE_IO_FAILED;
  }

  // write header
  if (RIFF_WAVE_HEADER_SIZE !=
      fwrite(header, sizeof(byte), RIFF_WAVE_HEADER_SIZE, fp_)) {
    fprintf(stderr, "WaveCore::write() - Fail to write file.\n");
    return WAVE_FILE_IO_FAILED;
  }

  // write data
  if (fwrite(bytes_, sizeof(byte), num_bytes_, fp_) != num_bytes_) {
    fprintf(stderr, "WaveCore::write() - Fail to write data.\n");
    return WAVE_FILE_IO_FAILED;
  }

  return WAVE_SUCCESS;
}

int WaveCore::setData(const unsigned char *bytes, const unsigned int num_bytes)
{
  if (bytes_ != NULL) {
    delete[] bytes_;
    bytes_ = NULL;
  }
  num_bytes_ = num_bytes;
  bytes_ = new unsigned char[num_bytes_];
  memcpy((void*)bytes_, (const void*)bytes, num_bytes);
  return WAVE_SUCCESS;
}

///// local functions /////

int find_riff_chunk(FILE *fp, ChunkInfo *chunk) {
  DWORD chunk_name;
  char buf[5] = {
      0,
  };

  if (fp == NULL || chunk == NULL) {
    return WAVE_INVALID_ARG_VALUE;
  }

  // re-wind file pointer
  fseek(fp, 0L, SEEK_SET);

  // get "RIFF" chunk name
  if (CHUNK_NAME_LENGTH != fread(buf, sizeof(char), CHUNK_NAME_LENGTH, fp)) {
    fprintf(stderr, "find_riff_chunk() - fail to read chunk_name.\n");
    return WAVE_FILE_IO_FAILED;
  }
  chunk_name = MAKEFOURCC(buf[0], buf[1], buf[2], buf[3]);
  if (chunk_name != MAKEFOURCC('R', 'I', 'F', 'F')) {
    fprintf(stderr, "find_riff_chunk() - invalid format. 'chunk_name' must be "
                    "equal to 'RIFF'.\n");
    return WAVE_UNSUPPORTED_TYPE;
  }
  chunk->id = chunk_name;

  // get chunk size
  if (1 != fread(&(chunk->size), sizeof(DWORD), 1, fp)) {
    fprintf(stderr, "find_riff_chunk() - fail to read chunk size.\n");
    return WAVE_FILE_IO_FAILED;
  }
  if (chunk->size <= 0) {
    fprintf(stderr, "find_riff_chunk() - chunk size must be positive.\n");
    return WAVE_INVALID_ARG_VALUE;
  }

  // get "WAVE" chunk name
  if (CHUNK_NAME_LENGTH != fread(buf, sizeof(char), CHUNK_NAME_LENGTH, fp)) {
    fprintf(stderr, "find_riff_chunk() - fail to read chunk_name.\n");
    return WAVE_FILE_IO_FAILED;
  }
  chunk_name = MAKEFOURCC(buf[0], buf[1], buf[2], buf[3]);
  if (chunk_name != MAKEFOURCC('W', 'A', 'V', 'E')) {
    fprintf(stderr, "find_riff_chunk() - invalid format. 'chunk_name' must be "
                    "equal to 'WAVE'.\n");
    return WAVE_UNSUPPORTED_TYPE;
  }
  chunk->fcc_type = chunk_name;

  // set data offset
  chunk->data_offset = ftell(fp);

  return WAVE_SUCCESS;
}

int find_chunk(FILE *fp, ChunkInfo *chunk, ChunkInfo *riff_chunk) {
  long fpOffset = riff_chunk->data_offset;
  long fpEnd = fpOffset + riff_chunk->size;
  DWORD dword_name = 0;
  char buf[CHUNK_NAME_LENGTH] = {0,};
  int ret;

  if (fp == NULL || chunk == NULL || riff_chunk == NULL) {
    return WAVE_INVALID_ARG_VALUE;
  }

  while (fpOffset < fpEnd) {
    fseek(fp, fpOffset, SEEK_SET);

    if (CHUNK_NAME_LENGTH != fread(buf, sizeof(char), CHUNK_NAME_LENGTH, fp)) {
      fprintf(stderr,
              "find_riff_chunk(find_chunk - fail to read chunk_name.\n");
      ret = WAVE_FILE_IO_FAILED;
      break;
    }
    dword_name = MAKEFOURCC(buf[0], buf[1], buf[2], buf[3]);

    if (1 != fread(&(chunk->size), sizeof(DWORD), 1, fp)) {
      fprintf(stderr,
              "find_riff_chunk(find_chunk - fail to read chunk size.\n");
      ret = WAVE_FILE_IO_FAILED;
      break;
    }

    if (dword_name == chunk->id) {
      chunk->data_offset = ftell(fp);
      ret = WAVE_SUCCESS;
      break;
    }

    fpOffset +=
        (sizeof(char) * CHUNK_NAME_LENGTH) + sizeof(DWORD) + chunk->size;
  }

  if (ret != WAVE_SUCCESS) {
    fprintf(stderr, "find_riff_chunk(find_chunk - fail to find chunk.\n");
  }

  return ret;
}

} // namespace wave