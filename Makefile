# SPDX-License-Identifier: MIT
# Copyright (c) 2026 Cortex Forge Contributors
#
# Makefile - Top-level build orchestrator for cortex-forge-server

BUILD_DIR := build
BIN_DIR   := bin
DOCS_DIR  := docs/html
CMAKE     := cmake
DOXYGEN   := doxygen
NINJA     := ninja

.PHONY: all cmake build test run doc doc-open lint format check clean distclean

all: cmake build test

cmake:
	@mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && $(CMAKE) .. -G Ninja -DCMAKE_BUILD_TYPE=Debug

build:
	cd $(BUILD_DIR) && $(NINJA)

test:
	cd $(BUILD_DIR) && ctest --output-on-failure

run:
	cd $(BUILD_DIR) && ./cortex-forge-server

doc:
	@mkdir -p $(DOCS_DIR)
	cd $(BUILD_DIR) && $(DOXYGEN) docs/Doxyfile

doc-open: doc
	@xdg-open $(DOCS_DIR)/index.html 2>/dev/null || open $(DOCS_DIR)/index.html 2>/dev/null || true

lint:
	@cpplint --recursive src/ 2>/dev/null || true

format:
	@clang-format -i src/*.cpp src/*/*.cpp src/*/*.hpp test/*.cpp 2>/dev/null || true

check: lint test

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

distclean: clean
	rm -rf $(DOCS_DIR)

help:
	@echo "Cortex Forge Server"
	@echo ""
	@echo "Targets:"
	@echo "  all       Configure + build + test (default)"
	@echo "  cmake     CMake configure (Ninja, Debug)"
	@echo "  build     Build the project"
	@echo "  test      Run CTest"
	@echo "  run       Run the server binary"
	@echo "  doc       Generate Doxygen documentation"
	@echo "  lint      Run cpplint"
	@echo "  format    Format with clang-format"
	@echo "  check     Run tests + lint"
	@echo "  clean     Remove build directory"
	@echo "  distclean Remove build, bin, docs"
