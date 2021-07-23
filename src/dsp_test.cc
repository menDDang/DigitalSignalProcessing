#include <iostream>

#include "gflags/gflags.h"

#include "dsp/feature_extractor.h"
#include "wave/wave.h"

DEFINE_uint32(sampling_rate, 16000, "sampling rate");
DEFINE_uint32(bit_rate, 16, "bit rate");
DEFINE_uint32(num_channels, 1, "number of channels");

DEFINE_string(input_file_name, "", "name of input file");
DEFINE_string(output_file_name, "", "name of output file");

int main(int argc, char **argv) {

  gflags::SetUsageMessage("NrFile");
  gflags::SetVersionString("1.0.0");
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  const unsigned int sampling_rate = FLAGS_sampling_rate;
  const unsigned int bit_rate = FLAGS_bit_rate;
  const unsigned int num_channels = FLAGS_num_channels;

  int error_code;

  wave::WaveReader reader;
  error_code = reader.init(sampling_rate, bit_rate, num_channels);
  if (error_code != WAVE_SUCCESS) {
    return error_code;
  }

  unsigned int num_samples;
  dsp::float_t *data = NULL;
  error_code = reader.read(FLAGS_input_file_name.c_str(), &data, &num_samples);
  if (error_code != WAVE_SUCCESS) {
    return error_code;
  }

  dsp::FEInitParam param;
  dsp::setDefaultParam(sampling_rate, &param);
  
  dsp::FeatureExtractor extractor;
  error_code = extractor.init(&param);
  if (error_code != DSP_SUCCESS) {
    return error_code;
  }

  //const unsigned int feat_dim = param.num_fft_point / 2 + 1;
  //const unsigned int feat_dim = param.num_mels;
  const unsigned int feat_dim = param.num_mfcc;
  const unsigned int step_size = (unsigned int)(0.01 * (float)sampling_rate);
  const unsigned int num_frame = (num_samples - param.window_size) / step_size;

  FILE* fp_out = fopen(FLAGS_output_file_name.c_str(), "wb");
  //fprintf(fp_out, "%d %d ", num_frame, feat_dim);
  fwrite(&num_frame, sizeof(const unsigned int), 1, fp_out);
  fwrite(&feat_dim, sizeof(const unsigned int), 1, fp_out);
  dsp::float_t *mfcc = new dsp::float_t[feat_dim];
  for (unsigned int i = 0; i < num_frame; i++) {
    error_code = extractor.mfcc(data + i * step_size, mfcc);
    if (error_code != DSP_SUCCESS) {
      return error_code;
    }

    fwrite(mfcc, sizeof(dsp::float_t), feat_dim, fp_out);
    //for (unsigned int j = 0; j < feat_dim; j++) {
    //  fprintf(fp_out, "%f ", mfcc[j]);
    //}
  }
  gflags::ShutDownCommandLineFlags();

  delete[] data;
  delete[] mfcc;
  fclose(fp_out);
  return 0;
}