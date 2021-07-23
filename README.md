
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