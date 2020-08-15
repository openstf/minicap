#!/bin/bash

# Download and install the NDK
wget http://dl.google.com/android/ndk/android-ndk-${ANDROID_NDK_VERSION}-linux-x86_64.bin -O travis/android-ndk-${ANDROID_NDK_VERSION}-linux-x86_64.bin
chmod +x travis/android-ndk-${ANDROID_NDK_VERSION}-linux-x86_64.bin
./travis/android-ndk-${ANDROID_NDK_VERSION}-linux-x86_64.bin -otravis/ -y > /dev/null

# Dump the environment variables
echo "ANDROID_NDK_HOME set to $ANDROID_NDK_HOME"
echo "PATH set to $PATH"

