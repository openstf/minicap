# minicap

Minicap provides a socket interface for streaming realtime screen capture data out of Android devices. It is meant to be used as a component in a larger program and is therefore not immensely useful just by itself. For example, it is being used in [STF](https://github.com/openstf/stf) for remote control.

Minicap works without root if started via [ADB](http://developer.android.com/tools/help/adb.html) on SDK 22 and lower, as well as on Android M Developer Preview 3. The lowest SDK level we build for is 9 (i.e. Android 2.3). Minicap also works on Android Wear. Emulators, however, are not supported.

To capture the screen we currently use two methods. For older Android versions we use the ScreenshotClient, a private API in AOSP. For newer versions we use a virtual display, which also requires access to private APIs. The frames are then encoded using SIMD-enabled [libjpeg-turbo](http://libjpeg-turbo.virtualgl.org/) and sent over a socket interface. A planned future improvement to allow for even higher FPS is to use MediaRecorder and friends to take advantage of hardware encoding.

Since minicap relies on private APIs, some devices may not work. At the time of writing, we have tested it on approximately 160 devices (incl. a few duplicates), and have so far found three models that segfault. They are Xiaomi "HM NOTE 1W" (Redmi Note 1W), Huawei "G750-U10" (Honor 3X) and Lenovo "B6000-F" (Yoga Tablet 8). We will continue to look for solutions for these devices when there's time.

The project consists of two parts. There's the main binary that can be built using NDK alone. The other part is a shared library that's built for each SDK level and each architecture inside the AOSP source tree. We ship precompiled libraries in this repo, but any modifications to the code used by these shared libraries require a recompile against the corresponding AOSP branches. This can be a major pain, but we have several utilities to help with the ordeal. If you're interested in that, [read the build instructions here](jni/minicap-shared/README.md).

## Features

* Usable to very smooth FPS depending on device. Older, weaker devices running an old version of Android can reach 10-20 FPS. Newer devices running recent versions of Android can usually reach 30-40 FPS fairly easily, but there are some exceptions. For maximum FPS we recommend running minicap at half the real vertical and horizontal resolution.
* Decent and usable but non-zero latency. Depending on encoding performance and USB transfer speed it may be one to a few frames behind the physical screen.
* On Android 4.2+, frames are only sent when something changes on the screen. On older versions frames are sent as a constant stream, whether there are changes or not.
* Easy socket interface.

## Requirements

* [NDK](https://developer.android.com/tools/sdk/ndk/index.html), Revision 10e (May 2015)
* [make](http://www.gnu.org/software/make/)

## Building

We include [libjpeg-turbo as  a Git submodule](https://github.com/openstf/android-libjpeg-turbo), so first make sure you've fetched it.

```
git submodule init
git submodule update
```

You're now ready to proceed.

Building requires [NDK](https://developer.android.com/tools/sdk/ndk/index.html), and is known to work with at least with NDK Revision 10e (May 2015). Older versions do not work due to the lack of `.asm` file support for x86_64.

Then it's simply a matter of invoking `ndk-build`.

```
ndk-build
```

You should now have the binaries available in `./libs`.

If you've modified the shared library, you'll also need to [build that](jni/minicap-shared/README.md).

## Running

You'll need to [build](#building) first.

### The easy way

You can then use the included [run.sh](run.sh) script to run the right binary on your device. It will make sure the correct binary and shared library get copied to your device. If you have multiple devices connected, set `ANDROID_SERIAL` before running the script.

```bash
# Run a preliminary check to see whether your device will work
./run.sh autosize -t
# Check help
./run.sh autosize -h
# Start minicap
./run.sh autosize
```

_The `autosize` command is for selecting the correct screen size automatically. This is done by the script instead of the binary itself. To understand why this is necessary, read the manual instructions below._

Finally we simply need to create a local forward so that we can connect to the socket.

```bash
adb forward tcp:1313 localabstract:minicap
```

Now you'll be able to connect to the socket locally on port 1313.

Then just see [usage](#usage) below.

### The hard way

To run manually, you have to first figure out which ABI your device supports:

```bash
ABI=$(adb shell getprop ro.product.cpu.abi | tr -d '\r')
```

_Note that as Android shell always ends lines with CRLF, you'll have to remove the CR like above or the rest of the commands will not work properly._

_Also note that if you've got multiple devices connected, setting `ANDROID_SERIAL` will make things quite a bit easier as you won't have to specify the `-s <serial>` option every time._

Now, push the appropriate binary to the device:

```bash
adb push libs/$ABI/minicap /data/local/tmp/
```

Note that for SDK <16, you will have to use the `minicap-nopie` executable which comes without [PIE](http://en.wikipedia.org/wiki/Position-independent_code#Position-independent_executables) support. Check [run.sh](run.sh) for a scripting example.

The binary enough is not enough. We'll also need to select and push the correct shared library to the device. This can be done as follows.

```bash
SDK=$(adb shell getprop ro.build.version.sdk | tr -d '\r')
adb push jni/minicap-shared/aosp/libs/android-$SDK/$ABI/minicap.so /data/local/tmp/
```

At this point it might be useful to check the usage:

```bash
adb shell LD_LIBRARY_PATH=/data/local/tmp /data/local/tmp/minicap -h
```

Note that you'll need to set `LD_LIBRARY_PATH` every time you call minicap or it won't find the shared library.

Also, you'll need to specify the size of the display and the projection every time you use minicap. This is because the private APIs we would have to use to access that information segfault on many Samsung devices (whereas minicap itself runs fine). The [run.sh](run.sh) helper script provides the `autosize` helper as mentioned above.

So, let's assume that your device has a 1080x1920 screen. First, let's run a quick check to see if your device is able to run the current version of minicap:

```bash
adb shell LD_LIBRARY_PATH=/data/local/tmp /data/local/tmp/minicap -P 1080x1920@1080x1920/0 -t
```

_The format of the -P argument is: {RealWidth}x{RealHeight}@{VirtualWidth}x{VirtualHeight}/{Orientation}. The "virtual" size is the size of the desired projection. The orientation argument tells minicap what the current orientation of the device is (in degrees), which is required so that we can report the correct orientation over the socket interface to the frame consumer. One way to get the current orientation (or rotation) is [RotationWatcher.apk](https://github.com/openstf/RotationWatcher.apk)._

If the command outputs "OK", then everything should be fine. If instead it segfaults (possibly after hanging for a while), your device is not supported and [we'd like to know about it](https://github.com/openstf/minicap/issues).

Finally, let's start minicap. It will start listening on an abstract unix domain socket.

```bash
adb shell LD_LIBRARY_PATH=/data/local/tmp /data/local/tmp/minicap -P 1080x1920@1080x1920/0
```

Now we simply need to create a local forward so that we can connect to the socket.

```bash
adb forward tcp:1313 localabstract:minicap
```

Now you can connect to the socket using the local port. Note that currently **only one connection at a time is supported.** It doesn't really make sense to have more than one connection anyway, as the USB bus would get saturated very quickly. So, let's connect.

```bash
nc localhost 1313
```

This will give you binary output that will be explained in the next section.

## Usage

It is assumed that you now have an open connection to the minicap socket. If not, follow the [instructions](#running) above.

The minicap protocol is a simple push-based binary protocol. When you first connect to the socket, you get a global header followed by the first frame. The global header will not appear again. More frames keep getting sent until you stop minicap.

### Global header binary format

Appears once.

| Bytes | Length | Type | Explanation |
|-------|--------|------|-------------|
| 0     | 1 | unsigned char | Version (currently 1) |
| 1     | 1 | unsigned char | Size of the header (from byte 0) |
| 2-5   | 4 | uint32 (low endian) | Pid of the process |
| 6-9   | 4 | uint32 (low endian) | Real display width in pixels |
| 10-13 | 4 | uint32 (low endian) | Real display height in pixels |
| 14-17 | 4 | uint32 (low endian) | Virtual display width in pixels |
| 18-21 | 4 | uint32 (low endian) | Virtual display height in pixels |
| 22    | 1 | unsigned char | Display orientation |
| 23    | 1 | unsigned char | Quirk bitflags (see below) |

#### Quirk bitflags

Currently, the following quirks may be reported:

| Value | Name | Explanation |
|-------|------|-------------|
| 1     | QUIRK_DUMB | Frames will get sent even if there are no changes from the previous frame. Informative, doesn't require any actions on your part. You can limit the capture rate by reading frame data slower in your own code if you wish. |
| 2     | QUIRK_ALWAYS_UPRIGHT | The frame will always be in upright orientation regardless of the device orientation. This needs to be taken into account when rendering the image. |
| 4     | QUIRK_TEAR | Frame tear might be visible. Informative, no action required. Neither of our current two methods exhibit this behavior. |

### Frame binary format

Appears a potentially unlimited number of times.

| Bytes | Length | Type | Explanation |
|-------|--------|------|-------------|
| 0-3   | 4 | uint32 (low endian) | Frame size in bytes (=n) |
| 4-(n+4) | n | unsigned char[] | Frame in JPG format |

## Contributing

_As a small disclaimer, minicap was the first time the author used C++, so even any non-functional changes to make the code more idiomatic (preferably without introducing new dependencies) are also very welcome, in addition to bug fixes and new features._

See [CONTRIBUTING.md](CONTRIBUTING.md).

## License

See [LICENSE](LICENSE).

Copyright © CyberAgent, Inc. All Rights Reserved.
