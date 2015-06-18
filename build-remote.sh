#!/usr/bin/env bash

set -xueo pipefail

TARGET=/tmp/minicap

rsync \
  --rsync-path='nice rsync' \
  --recursive \
  --copy-links \
  --perms \
  --times \
  -FF ./jni/minicap-shared/aosp/ "$BUILD_HOST":$TARGET

ssh -T "$BUILD_HOST" "docker run --rm \
  -a stdout -a stderr \
  -v $TARGET:$TARGET \
  -v \$(which docker):\$(which docker) \
  -v /usr/lib:/usr/lib \
  -v /var/run/docker.sock:/var/run/docker.sock \
  openstf/aosp:jdk7 bash -c 'cd $TARGET && make -j 1'"

rsync \
  --rsync-path='nice rsync' \
  --recursive \
  --copy-links \
  --perms \
  --times \
  "$BUILD_HOST":$TARGET/libs/ ./jni/minicap-shared/aosp/libs/
