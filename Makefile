CC := clang
CFLAGS := -std=c23 -Wall -Wextra -Wpedantic
DEBUGFLAGS := -O0 -gdwarf-4
RELEASEFLAGS := -O2

SOURCES := src/main.c src/logging.c src/platform.c src/common.c src/config.c src/cli.c src/cmd_init.c src/cmd_config.c src/cmd_todo.c src/cmd_shared.c src/cmd_add.c

ifeq ($(OS),Windows_NT)
    EXE := .exe
else
    EXE :=
endif

TARGET := shiori$(EXE)

ifeq ($(OS),Windows_NT)
    RM_CMD := cmd /C if exist "$(TARGET)" del /Q "$(TARGET)"
else
    RM_CMD := rm -f "$(TARGET)"
endif

.PHONY: all debug release clean run

all: debug

debug: CFLAGS += $(DEBUGFLAGS)
debug: $(TARGET)

release: CFLAGS += $(RELEASEFLAGS)
release: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	$(RM_CMD)