#!/bin/bash
source ../../../toolchains/darwin-x86_64/android-18-x86-4.8.sh

LIB_DIR=$PWD/libjpeg-turbo-1.3.90
OUT_DIR=$PWD/build/x86

set -e

mkdir -p $OUT_DIR

cd $LIB_DIR

./configure \
  --host=i686-linux-android \
  --enable-shared=no \
  --prefix=$OUT_DIR \
  --with-simd

make clean \
  all \
  install-nodist_includeHEADERS \
  install-includeHEADERS \
  install-libLTLIBRARIES

rm -f $OUT_DIR/lib/*.la
