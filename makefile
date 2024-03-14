# Define the compiler
CC=gcc

# Compiler flags including debug information
CFLAGS=-Wall -g

# Linker flags
LDFLAGS=-lpthread

# Source files
SRCS=main.c

# Precompiled object files
OBJS=list.o

# Target executable
TARGET=a

# Default target
all: $(TARGET)

# Compile and link in one step
$(TARGET):
	$(CC) $(CFLAGS) $(SRCS) $(OBJS) -o $(TARGET) $(LDFLAGS)

# Clean target for removing the executable
clean:
	rm -f $(TARGET)

.PHONY: all clean
