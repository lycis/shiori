CC := clang
CFLAGS := -std=c23 -Wall -Wextra -Wpedantic
DEBUGFLAGS := -O0 -gdwarf-4

TARGET := scratch.exe
SOURCES := main.c

.PHONY: all debug release clean

all: debug

debug: CFLAGS += $(DEBUGFLAGS)
debug: $(TARGET)

release: CFLAGS += -O2
release: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET)

clean:
	$(RM) $(TARGET)