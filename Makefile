# Makefile — short commands for everyday tasks.
# CMake does the real build; this file just wraps it.
#
#   make            show available tasks
#   make run ARGS="~ -n 5"
#   make debug BUILD_TYPE=Debug

PROJECT    := dirscope
BUILD_DIR  ?= build
BUILD_TYPE ?= Release
PREFIX     ?= /usr/local
ARGS       ?= .
JOBS       ?= $(shell nproc 2>/dev/null || echo 4)
GENERATOR  ?= $(if $(shell command -v ninja 2>/dev/null),Ninja,Unix Makefiles)
CMAKE_ARGS ?=

BIN     := $(BUILD_DIR)/$(PROJECT)
SOURCES := $(wildcard src/*.cpp include/*.hpp)

.DEFAULT_GOAL := help
.PHONY: help build release debug asan run install uninstall clean distclean rebuild format tidy test windows

help: ## Show this help
	@echo "Usage: make <task> [VAR=value]"
	@echo
	@grep -E '^[a-zA-Z_-]+:.*## ' $(MAKEFILE_LIST) | \
	    awk 'BEGIN {FS = ":.*## "}; {printf "  \033[36m%-10s\033[0m %s\n", $$1, $$2}'
	@echo
	@echo "Variables: BUILD_DIR=$(BUILD_DIR) BUILD_TYPE=$(BUILD_TYPE) PREFIX=$(PREFIX) ARGS=\"$(ARGS)\""

# Configure only when the build folder is new or CMakeLists.txt changed.
$(BUILD_DIR)/CMakeCache.txt: CMakeLists.txt
	cmake -S . -B $(BUILD_DIR) -G "$(GENERATOR)" \
	    -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
	    -DCMAKE_INSTALL_PREFIX=$(PREFIX) \
	    $(CMAKE_ARGS)

build: $(BUILD_DIR)/CMakeCache.txt ## Build (Release by default)
	cmake --build $(BUILD_DIR) -j $(JOBS)

release: ## Optimized build in build/
	@$(MAKE) --no-print-directory build BUILD_DIR=build BUILD_TYPE=Release

debug: ## Debug build with symbols in build-debug/
	@$(MAKE) --no-print-directory build BUILD_DIR=build-debug BUILD_TYPE=Debug

asan: ## Debug build with Address/UB sanitizers in build-asan/
	@$(MAKE) --no-print-directory build BUILD_DIR=build-asan BUILD_TYPE=Debug \
	    CMAKE_ARGS="-DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer'"

run: build ## Build, then run with ARGS (e.g. make run ARGS="~ -n 5")
	./$(BIN) $(ARGS)

test: build ## Build, then run the CTest suite
	ctest --test-dir $(BUILD_DIR) --output-on-failure

install: ## Install to PREFIX (run 'make build' first, then 'sudo make install')
	@test -x $(BIN) || { echo "No binary at $(BIN). Run 'make build' first."; exit 1; }
	cmake --install $(BUILD_DIR) --prefix $(PREFIX)

uninstall: ## Remove files installed by 'make install'
	@test -f $(BUILD_DIR)/install_manifest.txt || { echo "Nothing to uninstall."; exit 0; }
	xargs rm -vf < $(BUILD_DIR)/install_manifest.txt
	rm -f $(BUILD_DIR)/install_manifest.txt

clean: ## Remove compiled files, keep CMake configuration
	@test -d $(BUILD_DIR) && cmake --build $(BUILD_DIR) --target clean || true

distclean: ## Delete all build folders
	rm -rf build build-debug build-asan build-windows

rebuild: ## Delete everything and build from scratch
	@$(MAKE) --no-print-directory distclean
	@$(MAKE) --no-print-directory build

windows: ## Cross-compile dirscope.exe for Windows (needs: sudo apt install mingw-w64)
	@command -v x86_64-w64-mingw32-g++-posix >/dev/null || { echo "Install MinGW-w64: sudo apt install mingw-w64"; exit 1; }
	cmake -S . -B build-windows -G "$(GENERATOR)" -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64.cmake -DCMAKE_BUILD_TYPE=Release
	cmake --build build-windows -j $(JOBS)
	@echo "Built: build-windows/dirscope.exe"

format: ## Format sources with clang-format (uses .clang-format)
	@command -v clang-format >/dev/null || { echo "Install clang-format: sudo apt install clang-format"; exit 1; }
	clang-format -i $(SOURCES)

tidy: $(BUILD_DIR)/CMakeCache.txt ## Run clang-tidy static analysis
	@command -v clang-tidy >/dev/null || { echo "Install clang-tidy: sudo apt install clang-tidy"; exit 1; }
	clang-tidy -p $(BUILD_DIR) $(wildcard src/*.cpp)
