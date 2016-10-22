#!/usr/bin/env bash

set -xueo pipefail

builder=$1

rsync \
  --recursive \
  --copy-links \
  --perms \
  --times \
  -FF ./jni/minicap-shared/aosp/ "$builder":minicap/

ssh -T "$builder" "cd minicap && make -j 1"

rsync \
  --recursive \
  --copy-links \
  --perms \
  --times \
  "$builder":minicap/libs/ ./jni/minicap-shared/aosp/libs/
