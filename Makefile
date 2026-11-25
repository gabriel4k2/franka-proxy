ifeq ($(OS),Windows_NT)
BUILD_DIR ?= build
STREAMING_BUILD_DIR ?= build-streaming
else
BUILD_DIR ?= out/build/linux-release
STREAMING_BUILD_DIR ?= out/build/streaming
endif

.DEFAULT_GOAL := build
.PHONY: build test streaming-configure streaming-build streaming-test

# Rebuild the configured source tree, including the proxy and Python binding.
build:
	cmake --build "$(BUILD_DIR)" --config Release --parallel 2

# Hardware-free timing checks only; never starts the proxy or remote controller.
test:
	cmake --build "$(BUILD_DIR)" --config Release --target franka_sequence_timing_test --parallel 2
	ctest --test-dir "$(BUILD_DIR)" -C Release -R "^sequence_timing_offline$$" --output-on-failure

# Separate backend; supply the existing vcpkg toolchain path once at configuration.
streaming-configure:
	$(if $(strip $(VCPKG_TOOLCHAIN)),,$(error Set VCPKG_TOOLCHAIN to the existing vcpkg toolchain path))
	cmake -S streaming -B "$(STREAMING_BUILD_DIR)" -DCMAKE_TOOLCHAIN_FILE="$(VCPKG_TOOLCHAIN)" -DVCPKG_MANIFEST_MODE=OFF -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON

streaming-build:
	cmake --build "$(STREAMING_BUILD_DIR)" --config Release --parallel 2

# Only offline checks. Does not launch the server or connect to any robot.
streaming-test: streaming-build
	ctest --test-dir "$(STREAMING_BUILD_DIR)" -C Release --output-on-failure --no-tests=error
