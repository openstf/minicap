#!/usr/bin/env bash
set -exo pipefail
ndk-build
abi=$(adb shell getprop ro.product.cpu.abi | tr -d '\r')
sdk=$(adb shell getprop ro.build.version.sdk | tr -d '\r')
if (($sdk >= 16)); then
  bin=minicap
else
  bin=minicap-nopie
fi
adb push libs/$abi/$bin /data/local/tmp/
adb push jni/minicap-shared/aosp/libs/android-$sdk/$abi/minicap.so /data/local/tmp/
adb shell LD_LIBRARY_PATH=/data/local/tmp /data/local/tmp/$bin "$@"
