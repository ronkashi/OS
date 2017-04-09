#!/bin/bash

export DATA_SIZE=100B
export INPUT_DATA=/dev/urandom
export OUTPUT_DATA=/a/home/cc/students/math/ronkashi/Desktop/OS/b.txt

./data_filter $DATA_SIZE $INPUT_DATA $OUTPUT_DATA
