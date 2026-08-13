CC := clang
CPPFLAGS := -Isrc
CFLAGS := -std=c23 -Wall -Wextra -Wpedantic
DEBUGFLAGS := -O0 -gdwarf-4
RELEASEFLAGS := -O2
LDFLAGS :=

SOURCE_DIR := src
BUILD_DIR := build
DEBUG_DIR := $(BUILD_DIR)/debug
RELEASE_DIR := $(BUILD_DIR)/release

SOURCES := $(wildcard $(SOURCE_DIR)/*.c)
DEBUG_OBJECTS := $(patsubst $(SOURCE_DIR)/%.c,$(DEBUG_DIR)/%.o,$(SOURCES))
RELEASE_OBJECTS := $(patsubst $(SOURCE_DIR)/%.c,$(RELEASE_DIR)/%.o,$(SOURCES))
DEPENDENCIES := $(DEBUG_OBJECTS:.o=.d) $(RELEASE_OBJECTS:.o=.d)

ifeq ($(OS),Windows_NT)
    EXE := .exe
    # Link the MSVC C runtime into shiori.exe. Windows system DLLs such as
    # KERNEL32.dll remain normal operating-system dependencies.
    CFLAGS += -fms-runtime-lib=static
    LDFLAGS += -fms-runtime-lib=static
    MKDIR = if not exist "$(subst /,\,$1)" mkdir "$(subst /,\,$1)"
    COPY_TARGET = copy /Y "$(subst /,\,$1)" "$(TARGET)" >NUL
    RM_BUILD = if exist "$(subst /,\,$(BUILD_DIR))" rmdir /S /Q "$(subst /,\,$(BUILD_DIR))"
    RM_TARGET = if exist "$(TARGET)" del /Q "$(TARGET)" & if exist "$(TARGET:.exe=.pdb)" del /Q "$(TARGET:.exe=.pdb)"
else
    EXE :=
    MKDIR = mkdir -p "$1"
    COPY_TARGET = cp "$1" "$(TARGET)"
    RM_BUILD = rm -rf "$(BUILD_DIR)"
    RM_TARGET = rm -f "$(TARGET)"
endif

TARGET := shiori$(EXE)
DEBUG_BINARY := $(DEBUG_DIR)/$(TARGET)
RELEASE_BINARY := $(RELEASE_DIR)/$(TARGET)

.PHONY: all debug release clean run

all: debug

debug: $(DEBUG_BINARY)
	@$(call COPY_TARGET,$<)

release: $(RELEASE_BINARY)
	@$(call COPY_TARGET,$<)

$(DEBUG_BINARY): $(DEBUG_OBJECTS)
	$(CC) $(DEBUG_OBJECTS) $(LDFLAGS) -o $@

$(RELEASE_BINARY): $(RELEASE_OBJECTS)
	$(CC) $(RELEASE_OBJECTS) $(LDFLAGS) -o $@

$(DEBUG_DIR)/%.o: $(SOURCE_DIR)/%.c
	@$(call MKDIR,$(@D))
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEBUGFLAGS) -MMD -MP -c $< -o $@

$(RELEASE_DIR)/%.o: $(SOURCE_DIR)/%.c
	@$(call MKDIR,$(@D))
	$(CC) $(CPPFLAGS) $(CFLAGS) $(RELEASEFLAGS) -MMD -MP -c $< -o $@

run: debug
	./$(TARGET)

clean:
	@$(RM_BUILD)
	@$(RM_TARGET)

-include $(DEPENDENCIES)
