CC := clang
CFLAGS := -std=c23 -Wall -Wextra -Wpedantic
DEBUGFLAGS := -O0 -gdwarf-4
RELEASEFLAGS := -O2

SOURCES := main.c

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