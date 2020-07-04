SHELL:=/bin/bash

fast-build:
	mkdir -p build
	pushd build && \
	echo CXX=$(which c++) cmake .. && \
	echo make -j $$(nproc) example && \
	echo strip --strip-debug example

debug:
	mkdir -p build
	pushd build && \
	CXX=$(which c++) cmake -DDEBUG=on .. && \
	make VERBOSE=1 -j $$(nproc) example

format:
	cmake --build build --target clangformat
	# stat -c 'chown %u:%g . -R' CMakeLists.txt | sudo sh -

clean:
	rm -rf build
