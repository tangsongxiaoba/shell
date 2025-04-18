CC = gcc

CFLAGS = -Wall -g -std=c99

SOURCES = tash.c

TARGET_DIR = target

EXECUTABLE = tash

TARGET = $(TARGET_DIR)/$(EXECUTABLE)

all: $(TARGET)

$(TARGET): $(SOURCES) | $(TARGET_DIR)
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET)

$(TARGET_DIR):
	mkdir -p $(TARGET_DIR)

clean:
	rm -f $(TARGET)
	rm -rf $(TARGET_DIR)

.PHONY: all clean