#include <iostream>

#include "gflags/gflags.h"

#include "wave/wave.h"

DEFINE_uint32(sampling_rate, 16000, "sampling rate");
DEFINE_uint32(bit_rate, 16, "bit rate");
DEFINE_uint32(num_channels, 1, "number of channels");

DEFINE_string(input_file_name, "", "name of input file");
DEFINE_string(output_file_name, "", "name of output file");

int main(int argc, char** argv) {

    gflags::SetUsageMessage("wave_test");
    gflags::SetVersionString("1.0.0");        
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    const unsigned int sampling_rate = FLAGS_sampling_rate;
    const unsigned int bit_rate = FLAGS_bit_rate;
    const unsigned int num_channels = FLAGS_num_channels;
    

    wave::WaveReader reader;
    reader.init(sampling_rate, bit_rate, num_channels);

    float *data = NULL;
    unsigned int num_samples;
    reader.read(FLAGS_input_file_name.c_str(), &data, &num_samples);

    wave::WaveWriter writer;
    writer.init(sampling_rate, bit_rate, num_channels);
    writer.write(FLAGS_output_file_name.c_str(), data, num_samples);

    gflags::ShutDownCommandLineFlags();

    delete[] data;
    return 0;
}