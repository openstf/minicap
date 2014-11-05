this_dir = $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
SOURCES := $(shell find jni -name '*.cpp' -or -name '*.mk')

all: \
	libs/android-10/armeabi-v7a/minicap \
	libs/android-14/armeabi-v7a/minicap \
	libs/android-14/x86/minicap \
	libs/android-15/armeabi-v7a/minicap \
	libs/android-15/x86/minicap \
	libs/android-16/armeabi-v7a/minicap \
	libs/android-16/x86/minicap \
	libs/android-17/armeabi-v7a/minicap \
	libs/android-17/x86/minicap \
	libs/android-18/armeabi-v7a/minicap \
	libs/android-18/x86/minicap \
	libs/android-19/armeabi-v7a/minicap \
	libs/android-19/x86/minicap \
	libs/android-21/armeabi-v7a/minicap \
	libs/android-21/arm64-v8a/minicap \
	libs/android-21/x86/minicap \
	libs/android-21/x86_64/minicap \

libs/android-10/armeabi-v7a/minicap: $(SOURCES)
	mkdir -p $(@D)
	docker run --rm \
		-a stdout -a stderr \
		-v /srv/aosp/android-2.3.3_r1:/aosp \
		-v $(this_dir)jni:/app \
		-v $(this_dir)$(@D):/artifacts \
		sorccu/aosp:jdk6 /aosp.sh build generic-eng minicap

libs/android-14/armeabi-v7a/minicap: $(SOURCES)
	mkdir -p $(@D)
	docker run --rm \
		-a stdout -a stderr \
		-v /srv/aosp/android-4.0.1_r1:/aosp \
		-v $(this_dir)jni:/app \
		-v $(this_dir)$(@D):/artifacts \
		sorccu/aosp:jdk6 /aosp.sh build full-eng minicap

libs/android-14/x86/minicap: $(SOURCES)
	mkdir -p $(@D)
	docker run --rm \
		-a stdout -a stderr \
		-v /srv/aosp/android-4.0.1_r1:/aosp \
		-v $(this_dir)jni:/app \
		-v $(this_dir)$(@D):/artifacts \
		sorccu/aosp:jdk6 /aosp.sh build full_x86-eng minicap

libs/android-15/armeabi-v7a/minicap: $(SOURCES)
	mkdir -p $(@D)
	docker run --rm \
		-a stdout -a stderr \
		-v /srv/aosp/android-4.0.3_r1:/aosp \
		-v $(this_dir)jni:/app \
		-v $(this_dir)$(@D):/artifacts \
		sorccu/aosp:jdk6 /aosp.sh build full-eng minicap

libs/android-15/x86/minicap: $(SOURCES)
	mkdir -p $(@D)
	docker run --rm \
		-a stdout -a stderr \
		-v /srv/aosp/android-4.0.3_r1:/aosp \
		-v $(this_dir)jni:/app \
		-v $(this_dir)$(@D):/artifacts \
		sorccu/aosp:jdk6 /aosp.sh build full_x86-eng minicap

libs/android-16/armeabi-v7a/minicap: $(SOURCES)
	mkdir -p $(@D)
	docker run --rm \
		-a stdout -a stderr \
		-v /srv/aosp/android-4.1.1_r1:/aosp \
		-v $(this_dir)jni:/app \
		-v $(this_dir)$(@D):/artifacts \
		sorccu/aosp:jdk6 /aosp.sh build full-eng minicap

libs/android-16/x86/minicap: $(SOURCES)
	mkdir -p $(@D)
	docker run --rm \
		-a stdout -a stderr \
		-v /srv/aosp/android-4.1.1_r1:/aosp \
		-v $(this_dir)jni:/app \
		-v $(this_dir)$(@D):/artifacts \
		sorccu/aosp:jdk6 /aosp.sh build full_x86-eng minicap

libs/android-17/armeabi-v7a/minicap: $(SOURCES)
	mkdir -p $(@D)
	docker run --rm \
		-a stdout -a stderr \
		-v /srv/aosp/android-4.2_r1:/aosp \
		-v $(this_dir)jni:/app \
		-v $(this_dir)$(@D):/artifacts \
		sorccu/aosp:jdk6 /aosp.sh build full-eng minicap

libs/android-17/x86/minicap: $(SOURCES)
	mkdir -p $(@D)
	docker run --rm \
		-a stdout -a stderr \
		-v /srv/aosp/android-4.2_r1:/aosp \
		-v $(this_dir)jni:/app \
		-v $(this_dir)$(@D):/artifacts \
		sorccu/aosp:jdk6 /aosp.sh build full_x86-eng minicap

libs/android-18/armeabi-v7a/minicap: $(SOURCES)
	mkdir -p $(@D)
	docker run --rm \
		-a stdout -a stderr \
		-v /srv/aosp/android-4.3_r1:/aosp \
		-v $(this_dir)jni:/app \
		-v $(this_dir)$(@D):/artifacts \
		sorccu/aosp:jdk6 /aosp.sh build aosp_arm-eng minicap

libs/android-18/x86/minicap: $(SOURCES)
	mkdir -p $(@D)
	docker run --rm \
		-a stdout -a stderr \
		-v /srv/aosp/android-4.3_r1:/aosp \
		-v $(this_dir)jni:/app \
		-v $(this_dir)$(@D):/artifacts \
		sorccu/aosp:jdk6 /aosp.sh build aosp_x86-eng minicap

libs/android-19/armeabi-v7a/minicap: $(SOURCES)
	mkdir -p $(@D)
	docker run --rm \
		-a stdout -a stderr \
		-v /srv/aosp/android-4.4_r1:/aosp \
		-v $(this_dir)jni:/app \
		-v $(this_dir)$(@D):/artifacts \
		sorccu/aosp:jdk6 /aosp.sh build aosp_arm-eng minicap

libs/android-19/x86/minicap: $(SOURCES)
	mkdir -p $(@D)
	docker run --rm \
		-a stdout -a stderr \
		-v /srv/aosp/android-4.4_r1:/aosp \
		-v $(this_dir)jni:/app \
		-v $(this_dir)$(@D):/artifacts \
		sorccu/aosp:jdk6 /aosp.sh build aosp_x86-eng minicap

libs/android-21/armeabi-v7a/minicap: $(SOURCES)
	mkdir -p $(@D)
	docker run --rm \
		-a stdout -a stderr \
		-v /srv/aosp/android-5.0.0_r2:/aosp \
		-v $(this_dir)jni:/app \
		-v $(this_dir)$(@D):/artifacts \
		sorccu/aosp:jdk6 /aosp.sh build aosp_arm-eng minicap

libs/android-21/arm64-v8a/minicap: $(SOURCES)
	mkdir -p $(@D)
	docker run --rm \
		-a stdout -a stderr \
		-v /srv/aosp/android-5.0.0_r2:/aosp \
		-v $(this_dir)jni:/app \
		-v $(this_dir)$(@D):/artifacts \
		sorccu/aosp:jdk6 /aosp.sh build aosp_arm64-eng minicap

libs/android-21/x86/minicap: $(SOURCES)
	mkdir -p $(@D)
	docker run --rm \
		-a stdout -a stderr \
		-v /srv/aosp/android-5.0.0_r2:/aosp \
		-v $(this_dir)jni:/app \
		-v $(this_dir)$(@D):/artifacts \
		sorccu/aosp:jdk6 /aosp.sh build aosp_x86-eng minicap

libs/android-21/x86_64/minicap: $(SOURCES)
	mkdir -p $(@D)
	docker run --rm \
		-a stdout -a stderr \
		-v /srv/aosp/android-5.0.0_r2:/aosp \
		-v $(this_dir)jni:/app \
		-v $(this_dir)$(@D):/artifacts \
		sorccu/aosp:jdk6 /aosp.sh build aosp_x86_64-eng minicap

libs/android-21/mips/minicap: $(SOURCES)
	mkdir -p $(@D)
	docker run --rm \
		-a stdout -a stderr \
		-v /srv/aosp/android-5.0.0_r2:/aosp \
		-v $(this_dir)jni:/app \
		-v $(this_dir)$(@D):/artifacts \
		sorccu/aosp:jdk6 /aosp.sh build aosp_mips-eng minicap

libs/android-21/mips64/minicap: $(SOURCES)
	mkdir -p $(@D)
	docker run --rm \
		-a stdout -a stderr \
		-v /srv/aosp/android-5.0.0_r2:/aosp \
		-v $(this_dir)jni:/app \
		-v $(this_dir)$(@D):/artifacts \
		sorccu/aosp:jdk6 /aosp.sh build aosp_mips64-eng minicap
