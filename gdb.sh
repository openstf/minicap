#!/usr/bin/env bash

# Fail on error, verbose output
set -exo pipefail

# Check NDK setup
if [ -z "$NDK_ROOT" ]; then
  echo 'Environment variable $NDK_ROOT required for operation' >&2
  exit 1
fi

# Figure out which ABIs the device supports
abi=$(adb shell getprop ro.product.cpu.abi | tr -d '\r')
sdk=$(adb shell getprop ro.build.version.sdk | tr -d '\r')

# PIE is only supported since SDK 16
if (($sdk >= 16)); then
  bin=minicap
else
  bin=minicap-nopie
fi

# Check if we're debuggable
gdb_setup=$(make --no-print-dir -f "$NDK_ROOT/build/core/build-local.mk" -C "$PWD" DUMP_NDK_APP_GDBSETUP APP_ABI=$abi)
if [ ! -f "$gdb_setup" ]; then
  echo "Unable to find $gdb_setup, rebuild with 'ndk-build NDK_DEBUG=1'"
  exit 4
fi

# Get output directory
out=$(make --no-print-dir -f "$NDK_ROOT/build/core/build-local.mk" -C "$PWD" DUMP_TARGET_OUT APP_ABI=$abi)

# ABI-specific config
prebabi=$abi
libpath=lib
case $abi in
"armeabi" | "armeabi-v7a")
  prebabi=arm
  ;;
"arm64-v8a")
  libpath=lib64
  prebabi=arm64
  ;;
"x86_64")
  libpath=lib64
  ;;
"mips64")
  libpath=lib64
  ;;
esac

# Validate prebuilt mapping
if [ ! -e "$NDK_ROOT/prebuilt/android-$prebabi" ]; then
  echo "Unable to find prebuilts in $NDK_ROOT/prebuilt/android-$prebabi; incorrect mapping?" >&2
  exit 3
fi

# Find toolchain
toolchain=$(make --no-print-dir -f "$NDK_ROOT/build/core/build-local.mk" -C "$PWD" DUMP_TOOLCHAIN_PREFIX APP_ABI=$abi)
gdbclient="${toolchain}gdb"
gdbserver="$NDK_ROOT/prebuilt/android-$prebabi/gdbserver"

# Create a directory for our resources
dir=/data/local/tmp/minicap-gdb
adb shell "mkdir $dir 2>/dev/null"

# Find pid
pid=$(adb shell ps | tr -d '\r' | awk '$NF ~ /\/minicap$/ {print $2}' | tail -n1)
if [ -z "$pid" ]; then
  echo 'Unable to find minicap pid' >&2
  exit 2
fi

# Upload gdbserver
adb push "$gdbserver" $dir

# Pull libs
adb pull /system/bin/linker $out/linker
adb pull /system/$libpath $out

# Launch gdbserver
adb shell $dir/gdbserver :5039 --attach $pid &
gdbserver_pid=$!
trap "kill $gdbserver_pid 2>/dev/null" EXIT

# Set up the forward
adb forward tcp:5039 tcp:5039

# Find initial gdb.setup
cp -f "$gdb_setup" gdb.setup

# Augment gdb.setup
cat <<EOT >>gdb.setup
file libs/$abi/$bin
target remote :5039
EOT

# Launch gdb
"$gdbclient" -x gdb.setup

# Clean up
adb shell rm -r $dir
