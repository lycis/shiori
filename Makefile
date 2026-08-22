CC := clang
CLANG_FORMAT := clang-format
CLANG_TIDY := clang-tidy
LLVM_VERSION := $(file <.llvm-version)
CPPFLAGS := -Isrc
CFLAGS := -std=c23 -Wall -Wextra -Wpedantic -Werror
DEBUGFLAGS := -O0 -gdwarf-4
DEBUG_LDFLAGS := -Wl,/debug:dwarf
RELEASEFLAGS := -O2
SANITIZEFLAGS := -O1 -g -fno-omit-frame-pointer
LDFLAGS :=

SOURCE_DIR := src
BUILD_DIR := build
DEBUG_DIR := $(BUILD_DIR)/debug
RELEASE_DIR := $(BUILD_DIR)/release
SANITIZE_DIR := $(BUILD_DIR)/sanitize
UNIT_TEST_DIR := $(BUILD_DIR)/unit
TOOLCHAIN_CHECK_DIR := $(BUILD_DIR)/toolchain-check
TOOLCHAIN_CHECK_SOURCE := tests/toolchain/windows_sdk_check.c

SOURCES := $(wildcard $(SOURCE_DIR)/*.c)
HEADERS := $(wildcard $(SOURCE_DIR)/*.h)
FORMAT_FILES := $(SOURCES) $(HEADERS) $(TOOLCHAIN_CHECK_SOURCE)
DEBUG_OBJECTS := $(patsubst $(SOURCE_DIR)/%.c,$(DEBUG_DIR)/%.o,$(SOURCES))
RELEASE_OBJECTS := $(patsubst $(SOURCE_DIR)/%.c,$(RELEASE_DIR)/%.o,$(SOURCES))
SANITIZE_OBJECTS := $(patsubst $(SOURCE_DIR)/%.c,$(SANITIZE_DIR)/%.o,$(SOURCES))
DEPENDENCIES := $(DEBUG_OBJECTS:.o=.d) $(RELEASE_OBJECTS:.o=.d) $(SANITIZE_OBJECTS:.o=.d)

ifeq ($(OS),Windows_NT)
    # GitHub Actions and Git for Windows may export SHELL=/usr/bin/bash.
    # These recipes use cmd.exe syntax, so select the supported shell explicitly.
    SHELL := cmd.exe
    .SHELLFLAGS := /C
    EXE := .exe
    # Link the MSVC C runtime into shiori.exe. Windows system DLLs such as
    # KERNEL32.dll remain normal operating-system dependencies.
    CFLAGS += -fms-runtime-lib=static
    LDFLAGS += -fms-runtime-lib=static
    SANITIZERS := address
    MKDIR = if not exist "$(subst /,\,$1)" mkdir "$(subst /,\,$1)"
    RUN_BINARY = "$(subst /,\,$1)"
    COPY_TARGET = copy /Y "$(subst /,\,$1)" "$(TARGET)" >NUL
    SANITIZER_RUNTIME := $(shell $(CC) -print-resource-dir)/lib/windows/clang_rt.asan_dynamic-x86_64.dll
    COPY_SANITIZER_RUNTIME = copy /Y "$(subst /,\,$(SANITIZER_RUNTIME))" "$(subst /,\,$(SANITIZE_DIR))" >NUL
    VERIFY_LLVM_VERSION = findstr /C:"version $(LLVM_VERSION)" >NUL
    WINDOWS_SDK_CHECK := $(TOOLCHAIN_CHECK_DIR)/windows_sdk_check.exe
    RM_BUILD = if exist "$(subst /,\,$(BUILD_DIR))" rmdir /S /Q "$(subst /,\,$(BUILD_DIR))"
    RM_TARGET = if exist "$(TARGET)" del /Q "$(TARGET)" & if exist "$(TARGET:.exe=.pdb)" del /Q "$(TARGET:.exe=.pdb)"
else
    EXE :=
    MKDIR = mkdir -p "$1"
    RUN_BINARY = "$1"
    COPY_TARGET = cp "$1" "$(TARGET)"
    COPY_SANITIZER_RUNTIME =
    VERIFY_LLVM_VERSION = grep -F "version $(LLVM_VERSION)" >/dev/null
    RM_BUILD = rm -rf "$(BUILD_DIR)"
    RM_TARGET = rm -f "$(TARGET)"
    SANITIZERS := address,undefined
endif

SANITIZEFLAGS += -fsanitize=$(SANITIZERS)
SANITIZE_LDFLAGS := -fsanitize=$(SANITIZERS)

TARGET := shiori$(EXE)
DEBUG_BINARY := $(DEBUG_DIR)/$(TARGET)
RELEASE_BINARY := $(RELEASE_DIR)/$(TARGET)
SANITIZE_BINARY := $(SANITIZE_DIR)/$(TARGET)
CLI_UNIT_TEST := $(UNIT_TEST_DIR)/cli_test$(EXE)
TERMINAL_LIFECYCLE_UNIT_TEST := $(UNIT_TEST_DIR)/terminal_lifecycle_test$(EXE)

.PHONY: all debug release sanitize clean run test test-unit test-integration test-sanitize format format-check tidy toolchain-check check check-sanitize

all: debug

debug release sanitize format format-check tidy test-unit: toolchain-check

toolchain-check:
	@$(CC) --version | $(VERIFY_LLVM_VERSION)
	@$(CLANG_FORMAT) --version | $(VERIFY_LLVM_VERSION)
	@$(CLANG_TIDY) --version | $(VERIFY_LLVM_VERSION)
ifeq ($(OS),Windows_NT)
	@$(call MKDIR,$(TOOLCHAIN_CHECK_DIR))
	@$(CC) $(CFLAGS) $(TOOLCHAIN_CHECK_SOURCE) $(LDFLAGS) -o $(WINDOWS_SDK_CHECK) >NUL 2>&1 || (echo ERROR: Clang cannot compile and link with the MSVC and Windows SDK toolchain. & echo Install Visual Studio Build Tools with the Desktop development with C++ workload, then run this build from an x64 Native Tools Command Prompt. & exit /b 1)
	@del /Q "$(subst /,\,$(WINDOWS_SDK_CHECK))" "$(subst /,\,$(WINDOWS_SDK_CHECK:.exe=.pdb))" 2>NUL
endif

debug: $(DEBUG_BINARY)
	@$(call COPY_TARGET,$<)

release: $(RELEASE_BINARY)
	@$(call COPY_TARGET,$<)

sanitize: $(SANITIZE_BINARY)

$(DEBUG_BINARY): $(DEBUG_OBJECTS)
	$(CC) $(DEBUG_OBJECTS) $(LDFLAGS) $(DEBUG_LDFLAGS) -o $@

$(RELEASE_BINARY): $(RELEASE_OBJECTS)
	$(CC) $(RELEASE_OBJECTS) $(LDFLAGS) -o $@

$(SANITIZE_BINARY): $(SANITIZE_OBJECTS)
	$(CC) $(SANITIZE_OBJECTS) $(LDFLAGS) $(SANITIZE_LDFLAGS) -o $@
	@$(COPY_SANITIZER_RUNTIME)

$(DEBUG_DIR)/%.o: $(SOURCE_DIR)/%.c
	@$(call MKDIR,$(@D))
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEBUGFLAGS) -MMD -MP -c $< -o $@

$(RELEASE_DIR)/%.o: $(SOURCE_DIR)/%.c
	@$(call MKDIR,$(@D))
	$(CC) $(CPPFLAGS) $(CFLAGS) $(RELEASEFLAGS) -MMD -MP -c $< -o $@

$(SANITIZE_DIR)/%.o: $(SOURCE_DIR)/%.c
	@$(call MKDIR,$(@D))
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SANITIZEFLAGS) -MMD -MP -c $< -o $@

run: debug
	$(call RUN_BINARY,$(TARGET))

PYTHON ?= python

test: test-unit test-integration

test-unit: $(CLI_UNIT_TEST) $(TERMINAL_LIFECYCLE_UNIT_TEST)
	$(call RUN_BINARY,$(CLI_UNIT_TEST))
	$(call RUN_BINARY,$(TERMINAL_LIFECYCLE_UNIT_TEST))

$(CLI_UNIT_TEST): tests/unit/cli_test.c $(DEBUG_DIR)/cli.o
	@$(call MKDIR,$(@D))
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEBUGFLAGS) $^ $(LDFLAGS) -o $@

$(TERMINAL_LIFECYCLE_UNIT_TEST): tests/unit/terminal_lifecycle_test.c $(DEBUG_DIR)/terminal_lifecycle.o
	@$(call MKDIR,$(@D))
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEBUGFLAGS) $^ $(LDFLAGS) -o $@

test-integration: debug
	$(PYTHON) -m robot --variable SHIORI_BINARY:$(abspath $(DEBUG_BINARY)) --outputdir $(BUILD_DIR)/test-results --xunit xunit.xml tests/integration

test-sanitize: sanitize
	$(PYTHON) -m robot --variable SHIORI_BINARY:$(abspath $(SANITIZE_BINARY)) --outputdir $(BUILD_DIR)/sanitize-test-results --xunit xunit.xml tests/integration

format:
	$(CLANG_FORMAT) -i $(FORMAT_FILES)

format-check:
	$(CLANG_FORMAT) --dry-run --Werror $(FORMAT_FILES)

tidy:
	$(CLANG_TIDY) --warnings-as-errors=* $(SOURCES) -- $(CPPFLAGS) $(CFLAGS)

check: format-check tidy test

check-sanitize: test-sanitize

clean:
	@$(RM_BUILD)
	@$(RM_TARGET)

-include $(DEPENDENCIES)
