CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -g
TARGET := simple_shell

SRC := $(wildcard *.c)
OBJ := $(SRC:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET) 
