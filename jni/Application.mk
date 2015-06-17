NDK_TOOLCHAIN_VERSION := 4.9

APP_ABI := armeabi-v7a arm64-v8a x86 x86_64

# Get C++11 working
APP_CPPFLAGS += -std=c++11 -fexceptions
APP_STL := gnustl_static

# Disable PIE for SDK <16 support. Enable manually for >=5.0
# where necessary.
APP_PIE := false

APP_CFLAGS += \
	-Ofast \
	-funroll-loops \
	-fno-strict-aliasing \

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
# http://community.arm.com/groups/tools/blog/2013/04/15/arm-cortex-a-processors-and-gcc-command-lines
APP_CFLAGS += \
	-march=armv7-a \
	-mfpu=neon \
	-mfloat-abi=softfp \
	-marm \
	-fprefetch-loop-arrays \
	-DHAVE_NEON=1 \

else ifeq ($(TARGET_ARCH_ABI),armeabi-v7a-hard)
# http://community.arm.com/groups/tools/blog/2013/04/15/arm-cortex-a-processors-and-gcc-command-lines
APP_CFLAGS += \
	-march=armv7-a \
	-mfpu=neon \
	-mfloat-abi=hard \
	-marm \
	-fprefetch-loop-arrays \
	-DHAVE_NEON=1 \
	-mhard-float \
	-D_NDK_MATH_NO_SOFTFP=1 \

APP_LDFLAGS += -lm_hard

else ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
# http://community.arm.com/groups/tools/blog/2013/04/15/arm-cortex-a-processors-and-gcc-command-lines
APP_CFLAGS += \
	-mcpu=cortex-a15 \
	-mfpu=neon-vfpv4 \
	-mfloat-abi=softfp \
	-DHAVE_NEON=1 \

else ifeq ($(TARGET_ARCH_ABI),x86)
# From Android on x86: An Introduction to Optimizing for Intel® Architecture
# Chapter 12
APP_CFLAGS += \
	-fprefetch-loop-arrays \
	-fno-short-enums \
	-finline-limit=300 \
	-fomit-frame-pointer \
	-mssse3 \
	-mfpmath=sse \
	-masm=intel \
	-DHAVE_NEON=1 \

endif
