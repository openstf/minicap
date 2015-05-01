# Example: minicap over WebSockets

A quick and dirty example to show how minicap might be used as part of an application. Also useful for testing.

## Requirements

* [Node.js](https://nodejs.org/) >= 0.12 (for this example only)
* [ADB](http://developer.android.com/intl/ja/tools/help/adb.html)
* An Android device with USB debugging enabled.

## Running

1. Check that your device is connected and ADB is running with `adb devices`. The following steps may not work properly if you don't.
```
adb devices
```
2. Set up a forward for the server we'll soon have running inside the device. Note that due to laziness the port is currently fixed to 1717.
```
adb forward tcp:1717 localabstract:minicap
```
3. Get information about your display. Unfortunately the easy API methods we could use for automatic detection segfault on some Samsung devices, presumably due to maker customizations. You'll need to know the display width and height in pixels. Here are some ways to do it:
```
adb shell wm size
adb shell dumpsys display
```
4. Start the minicap server. The most convenient way is to use the helper script at the root of this repo.
```
# Try ./run.sh -h for help
./run.sh -P 720x1280@720x1280/0
```
The first set is the true size of your display, and the second set is the size of the desired projection. Larger projections require more processing power and bandwidth. The final argument is the rotation of the display. Note that this is not the rotation you want it to have, it simply specifies the display's current rotation, which is used to normalize the output frames between Android versions. If the rotation changes you have to restart the server.
5. Start the example app.
```
PORT=9002 node app.js
```
6. Open http://localhost:9002 in your browser.
