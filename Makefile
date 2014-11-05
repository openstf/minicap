all:
	rm -rf build
	mkdir -p build/bin build/shared/
	cp -R libs/ build/bin/
	find build/bin -name '*.so' -print0 | xargs -0 rm
	cp -R jni/minicap-shared/aosp/libs/ build/shared/

.PHONY: all
