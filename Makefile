SHELL:=/bin/bash

fast-build:
	mkdir -p build
	pushd build && \
	CXX=$(which c++) cmake .. && \
	make -j $$(nproc)

debug:
	mkdir -p build
	pushd build && \
	CXX=$(which c++) cmake -DDEBUG=on .. && \
	make VERBOSE=1 -j $$(nproc)

format:
	cmake --build build --target clangformat
	# stat -c 'chown %u:%g . -R' CMakeLists.txt | sudo sh -

clean:
	rm -rf build
