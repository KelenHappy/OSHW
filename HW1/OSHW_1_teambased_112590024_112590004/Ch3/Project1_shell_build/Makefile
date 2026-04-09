CC = gcc
CFLAGS = -Wall -std=c99 -g
TARGET = shell
SRC = shell.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET) .shell_history

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run