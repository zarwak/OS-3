CC = gcc
CFLAGS = -Wall -g -Iinclude
LDFLAGS = -lreadline

TARGET = bin/myshell
SRCDIR = src

# Explicitly list source files
SOURCES = $(SRCDIR)/builtins.c \
          $(SRCDIR)/execute.c \
          $(SRCDIR)/history.c \
          $(SRCDIR)/main.c \
          $(SRCDIR)/readline_support.c \
          $(SRCDIR)/shell.c \
          $(SRCDIR)/parser.c \
          $(SRCDIR)/redirection.c \
          $(SRCDIR)/jobs.c \
          $(SRCDIR)/control_structures.c \
          $(SRCDIR)/variables.c

OBJECTS = $(SOURCES:.c=.o)

# Default target
all: $(TARGET)

# Create target executable
$(TARGET): $(OBJECTS)
	@mkdir -p bin
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

# Compile source files to object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TARGET)
	find src -name "*.o" -delete
	find . -name "test_*" -delete

# Install dependencies
deps:
	sudo apt update
	sudo apt install -y libreadline-dev build-essential

.PHONY: all clean deps
