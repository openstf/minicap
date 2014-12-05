#!/usr/bin/env bash

# Fail on error, verbose output
set -exo pipefail

# Build project
ndk-build

# Figure out which ABI and SDK the device has
abi=$(adb shell getprop ro.product.cpu.abi | tr -d '\r')
sdk=$(adb shell getprop ro.build.version.sdk | tr -d '\r')

# PIE is only supported since SDK 16
if (($sdk >= 16)); then
  bin=minicap
else
  bin=minicap-nopie
fi

# Upload the binary
adb push libs/$abi/$bin /data/local/tmp/

# Upload the shared library
adb push jni/minicap-shared/aosp/libs/android-$sdk/$abi/minicap.so /data/local/tmp/

# Run!
adb shell LD_LIBRARY_PATH=/data/local/tmp /data/local/tmp/$bin "$@"
