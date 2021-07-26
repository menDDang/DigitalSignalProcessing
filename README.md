
**작성자**
=====
윤기무

**내용**
=====
C++로 제작한 Digital Signal Processing 라이브러리 및 binary 실행 파일들의 code 
입니다.


**Version 1.0.0**
-----
* [x] spectrum 
* [x] mel-spectrum
* [x] mfcc 
* [x] set 16kHz default parameter 
* [ ] multi threading (추후 업데이트 예정)

**Example**
=====
*fextor*
-----
get helping messages

`$ fextor --helpshort`

simple example

`$ input_file_name=input_16k_16bit.wav` \
`$ output_file_name=sample_mfcc.feat` \
`$ fextor --input ${input_file_name} --output ${output_file_name}`

*python*
-----
fextor를 통해 추출된 파일을 python에서 load 및 plot 할 수 있습니다.

`$ input_file_name=sample_mfcc.feat` \
`$ output_file_name=sample_plot.png` \
`$ python plot_feature.py -i ${input_file_name} -o ${output_file_name}`

**Build**
=====
mkdir build && cd build \
cmake .. \
make

**Test**
=====
build/bin/dsp_test --input_file_name ${input_file_name}

**Availabe cmake options**

| options | description | default |
| ------ | ------ | ----- |
| USE_DOUBLE_PRECISION | using `double` type instead of `float` | OFF |