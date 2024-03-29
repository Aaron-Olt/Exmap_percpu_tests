#!/bin/bash

cd exmap-dev
./load.sh
cd ../


for th in 1 4 8 16 32 64 128; do
exmap-dev/eval/test-bench-alloc $th >> tests/per$th.txt
done

rmmod exmap

cd exmap-master
./load.sh
cd ../

for th in 1 4 8 16 32 64 128; do
exmap-dev/eval/test-bench-alloc $th >> tests/def$th.txt
done
