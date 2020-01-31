.PHONY: default clean prebuilt

NDKBUILT := \
	libs/arm64-v8a/minicap \
	libs/arm64-v8a/minicap-nopie \
	libs/armeabi-v7a/minicap \
	libs/armeabi-v7a/minicap-nopie \
	libs/x86/minicap \
	libs/x86/minicap-nopie \
	libs/x86_64/minicap \
	libs/x86_64/minicap-nopie \

default: prebuilt

clean:
	ndk-build clean
	rm -rf prebuilt

$(NDKBUILT):
	ndk-build

# It may feel a bit redundant to list everything here. However it also
# acts as a safeguard to make sure that we really are including everything
# that is supposed to be there.
prebuilt: \
	prebuilt/armeabi-v7a/bin/minicap \
	prebuilt/armeabi-v7a/bin/minicap-nopie \
	prebuilt/armeabi-v7a/lib/android-9/minicap.so \
	prebuilt/armeabi-v7a/lib/android-10/minicap.so \
	prebuilt/armeabi-v7a/lib/android-14/minicap.so \
	prebuilt/armeabi-v7a/lib/android-15/minicap.so \
	prebuilt/armeabi-v7a/lib/android-16/minicap.so \
	prebuilt/armeabi-v7a/lib/android-17/minicap.so \
	prebuilt/armeabi-v7a/lib/android-18/minicap.so \
	prebuilt/armeabi-v7a/lib/android-19/minicap.so \
	prebuilt/armeabi-v7a/lib/android-21/minicap.so \
	prebuilt/armeabi-v7a/lib/android-22/minicap.so \
	prebuilt/armeabi-v7a/lib/android-23/minicap.so \
	prebuilt/armeabi-v7a/lib/android-24/minicap.so \
	prebuilt/armeabi-v7a/lib/android-25/minicap.so \
	prebuilt/armeabi-v7a/lib/android-26/minicap.so \
	prebuilt/armeabi-v7a/lib/android-27/minicap.so \
	prebuilt/armeabi-v7a/lib/android-28/minicap.so \
	prebuilt/armeabi-v7a/lib/android-29/minicap.so \
	prebuilt/arm64-v8a/bin/minicap \
	prebuilt/arm64-v8a/bin/minicap-nopie \
	prebuilt/arm64-v8a/lib/android-21/minicap.so \
	prebuilt/arm64-v8a/lib/android-22/minicap.so \
	prebuilt/arm64-v8a/lib/android-23/minicap.so \
	prebuilt/arm64-v8a/lib/android-24/minicap.so \
	prebuilt/arm64-v8a/lib/android-25/minicap.so \
	prebuilt/arm64-v8a/lib/android-26/minicap.so \
	prebuilt/arm64-v8a/lib/android-27/minicap.so \
	prebuilt/arm64-v8a/lib/android-28/minicap.so \
	prebuilt/arm64-v8a/lib/android-29/minicap.so \
	prebuilt/x86/bin/minicap \
	prebuilt/x86/bin/minicap-nopie \
	prebuilt/x86/lib/android-14/minicap.so \
	prebuilt/x86/lib/android-15/minicap.so \
	prebuilt/x86/lib/android-16/minicap.so \
	prebuilt/x86/lib/android-17/minicap.so \
	prebuilt/x86/lib/android-18/minicap.so \
	prebuilt/x86/lib/android-19/minicap.so \
	prebuilt/x86/lib/android-21/minicap.so \
	prebuilt/x86/lib/android-22/minicap.so \
	prebuilt/x86/lib/android-23/minicap.so \
	prebuilt/x86/lib/android-24/minicap.so \
	prebuilt/x86/lib/android-25/minicap.so \
	prebuilt/x86/lib/android-26/minicap.so \
	prebuilt/x86/lib/android-27/minicap.so \
	prebuilt/x86/lib/android-28/minicap.so \
	prebuilt/x86/lib/android-29/minicap.so \
	prebuilt/x86_64/bin/minicap \
	prebuilt/x86_64/bin/minicap-nopie \
	prebuilt/x86_64/lib/android-21/minicap.so \
	prebuilt/x86_64/lib/android-22/minicap.so \
	prebuilt/x86_64/lib/android-23/minicap.so \
	prebuilt/x86_64/lib/android-24/minicap.so \
	prebuilt/x86_64/lib/android-25/minicap.so \
	prebuilt/x86_64/lib/android-26/minicap.so \
	prebuilt/x86_64/lib/android-27/minicap.so \
	prebuilt/x86_64/lib/android-28/minicap.so \
	prebuilt/x86_64/lib/android-29/minicap.so \

prebuilt/%/bin/minicap: libs/%/minicap
	mkdir -p $(@D)
	cp $^ $@

prebuilt/%/bin/minicap-nopie: libs/%/minicap-nopie
	mkdir -p $(@D)
	cp $^ $@

prebuilt/armeabi-v7a/lib/%/minicap.so: jni/minicap-shared/aosp/libs/%/armeabi-v7a/minicap.so
	mkdir -p $(@D)
	cp $^ $@

prebuilt/arm64-v8a/lib/%/minicap.so: jni/minicap-shared/aosp/libs/%/arm64-v8a/minicap.so
	mkdir -p $(@D)
	cp $^ $@

prebuilt/x86/lib/%/minicap.so: jni/minicap-shared/aosp/libs/%/x86/minicap.so
	mkdir -p $(@D)
	cp $^ $@

prebuilt/x86_64/lib/%/minicap.so: jni/minicap-shared/aosp/libs/%/x86_64/minicap.so
	mkdir -p $(@D)
	cp $^ $@
