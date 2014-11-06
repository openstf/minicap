APP_ABI := armeabi-v7a x86

# Get C++11 working
APP_CPPFLAGS += -std=c++11
APP_STL := gnustl_static

# Disable PIE for SDK <16 support. Enable manually for >=5.0
# where necessary.
APP_PIE := false
