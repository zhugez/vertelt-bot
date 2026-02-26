# GNUmakefile — thin wrapper that delegates to CMake.
# Mirrors the Crypto++ Makefile UX:  make && sudo make install
#
# Usage:
#   make              # configure + build (Release)
#   make test         # run CTest
#   make install      # install to /usr/local (or PREFIX)
#   make clean        # remove build artifacts
#   make distclean    # remove entire build directory
#   make asan         # AddressSanitizer build
#   make ubsan        # UndefinedBehaviorSanitizer build
#   make coverage     # gcov coverage build
#   make lean         # build without tests/examples

PREFIX    ?= /usr/local
BUILD_DIR ?= build
BUILD_TYPE ?= Release
JOBS      ?= $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

CMAKE_FLAGS ?=

.PHONY: all configure build test install clean distclean asan ubsan coverage lean

all: build

configure:
	@cmake -S . -B $(BUILD_DIR) \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
		-DCMAKE_INSTALL_PREFIX=$(PREFIX) \
		$(CMAKE_FLAGS)

build: configure
	@cmake --build $(BUILD_DIR) -j $(JOBS)

test: build
	@ctest --test-dir $(BUILD_DIR) --output-on-failure

install: build
	@cmake --install $(BUILD_DIR) --prefix $(PREFIX)

clean:
	@if [ -d "$(BUILD_DIR)" ]; then cmake --build $(BUILD_DIR) --target clean; fi

distclean:
	@rm -rf $(BUILD_DIR)

# --- Sanitizer / special builds ---

asan: distclean
	@$(MAKE) build \
		BUILD_TYPE=Debug \
		CMAKE_FLAGS="-DCMAKE_CXX_FLAGS=-fsanitize=address\ -fno-omit-frame-pointer"

ubsan: distclean
	@$(MAKE) build \
		BUILD_TYPE=Debug \
		CMAKE_FLAGS="-DCMAKE_CXX_FLAGS=-fsanitize=undefined"

coverage: distclean
	@$(MAKE) build \
		BUILD_TYPE=Debug \
		CMAKE_FLAGS="-DCMAKE_CXX_FLAGS=--coverage -DCMAKE_EXE_LINKER_FLAGS=--coverage"
	@$(MAKE) test

lean: distclean
	@$(MAKE) build \
		CMAKE_FLAGS="-DVERTEL_BUILD_TESTS=OFF -DVERTEL_BUILD_EXAMPLES=OFF"
