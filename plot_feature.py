import argparse
import struct

import numpy as np
import matplotlib.pyplot as plt

def load_feature(filename):
  feats = []
  with open(filename, "rb") as f:
    num_frame = struct.unpack('I', f.read(4))[0]
    feat_dim = struct.unpack('I', f.read(4))[0]
    size = struct.unpack('L', f.read(8))[0]
    
    print("num_frame : {}, feat_dim : {}, size : {}".format(num_frame, feat_dim, size))

    feats = []
    for n in range(num_frame):
      if size == 4:
        x = [struct.unpack('f', f.read(4))[0] for i in range(feat_dim)]
      elif size == 8:
        x = [struct.unpack('d', f.read(8))[0] for i in range(feat_dim)]
      else:
        raise ValueError("load_feature() - size must be 4 or 8.")
      feats.append(x)
  return np.array(feats)


if __name__ == '__main__':
  parser = argparse.ArgumentParser()
  parser.add_argument('-i', '--input_file_name', type=str, required=True)
  parser.add_argument('-o', '--output_file_name', type=str, required=True)
  args = parser.parse_args()

  # parsing arguments
  input_file_name = args.input_file_name
  output_file_name = args.output_file_name

  # load features
  feats = load_feature(input_file_name)
  feats[:,0] = 0
  fig, ax = plt.subplots()
  cax = ax.imshow(feats.T, origin='lower')
  fig.savefig(output_file_name)
    
