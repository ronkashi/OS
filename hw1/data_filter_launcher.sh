#!/bin/bashbash

export DATA_SIZE=300M
export INPUT_DATA=/dev/urandom
export OUTPUT_DATA=/a/home/cc/students/math/ronkashi/Desktop/OS/b.txt

gcc -o data_filter data_filter.c

./data_filter $DATA_SIZE $INPUT_DATA $OUTPUT_DATA
