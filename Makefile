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

SOURCES := $(wildcard $(SOURCE_DIR)/*.c)
HEADERS := $(wildcard $(SOURCE_DIR)/*.h)
FORMAT_FILES := $(SOURCES) $(HEADERS)
DEBUG_OBJECTS := $(patsubst $(SOURCE_DIR)/%.c,$(DEBUG_DIR)/%.o,$(SOURCES))
RELEASE_OBJECTS := $(patsubst $(SOURCE_DIR)/%.c,$(RELEASE_DIR)/%.o,$(SOURCES))
SANITIZE_OBJECTS := $(patsubst $(SOURCE_DIR)/%.c,$(SANITIZE_DIR)/%.o,$(SOURCES))
DEPENDENCIES := $(DEBUG_OBJECTS:.o=.d) $(RELEASE_OBJECTS:.o=.d) $(SANITIZE_OBJECTS:.o=.d)

ifeq ($(OS),Windows_NT)
    EXE := .exe
    # Link the MSVC C runtime into shiori.exe. Windows system DLLs such as
    # KERNEL32.dll remain normal operating-system dependencies.
    CFLAGS += -fms-runtime-lib=static
    LDFLAGS += -fms-runtime-lib=static
    SANITIZERS := address
    MKDIR = if not exist "$(subst /,\,$1)" mkdir "$(subst /,\,$1)"
    COPY_TARGET = copy /Y "$(subst /,\,$1)" "$(TARGET)" >NUL
    SANITIZER_RUNTIME := $(shell $(CC) -print-resource-dir)/lib/windows/clang_rt.asan_dynamic-x86_64.dll
    COPY_SANITIZER_RUNTIME = copy /Y "$(subst /,\,$(SANITIZER_RUNTIME))" "$(subst /,\,$(SANITIZE_DIR))" >NUL
    VERIFY_LLVM_VERSION = findstr /C:"version $(LLVM_VERSION)" >NUL
    RM_BUILD = if exist "$(subst /,\,$(BUILD_DIR))" rmdir /S /Q "$(subst /,\,$(BUILD_DIR))"
    RM_TARGET = if exist "$(TARGET)" del /Q "$(TARGET)" & if exist "$(TARGET:.exe=.pdb)" del /Q "$(TARGET:.exe=.pdb)"
else
    EXE :=
    MKDIR = mkdir -p "$1"
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

.PHONY: all debug release sanitize clean run test test-integration test-sanitize format format-check tidy toolchain-check check check-sanitize

all: debug

debug release sanitize format format-check tidy: toolchain-check

toolchain-check:
	@$(CC) --version | $(VERIFY_LLVM_VERSION)
	@$(CLANG_FORMAT) --version | $(VERIFY_LLVM_VERSION)
	@$(CLANG_TIDY) --version | $(VERIFY_LLVM_VERSION)

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
	./$(TARGET)

PYTHON ?= python

test: test-integration

test-integration: debug
	$(PYTHON) -m robot --variable SHIORI_BINARY:$(abspath $(DEBUG_BINARY)) --outputdir $(BUILD_DIR)/test-results tests/integration

test-sanitize: sanitize
	$(PYTHON) -m robot --variable SHIORI_BINARY:$(abspath $(SANITIZE_BINARY)) --outputdir $(BUILD_DIR)/sanitize-test-results tests/integration

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
