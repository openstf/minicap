#!/usr/bin/env bash

# Fail on error, verbose output
set -exo pipefail

# Build project
ndk-build 1>&2

# Figure out which ABI and SDK the device has
abi=$(adb shell getprop ro.product.cpu.abi | tr -d '\r')
sdk=$(adb shell getprop ro.build.version.sdk | tr -d '\r')

# PIE is only supported since SDK 16
if (($sdk >= 16)); then
  bin=minicap
else
  bin=minicap-nopie
fi

# Create a directory for our resources
dir=/data/local/tmp/minicap-devel
adb shell mkdir -p $dir

# Upload the binary
adb push libs/$abi/$bin $dir

# Upload the shared library
adb push jni/minicap-shared/aosp/libs/android-$sdk/$abi/minicap.so $dir

# Run!
adb shell LD_LIBRARY_PATH=$dir $dir/$bin "$@"

# Clean up
adb shell rm -r $dir
