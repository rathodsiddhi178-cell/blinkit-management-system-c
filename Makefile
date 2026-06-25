CC = gcc
CFLAGS = -Wall -Wextra -g
TARGET = blinkit
SRCS = main.c avl.c operations.c fileio.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c blinkit.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) *.dat

run: all
	./$(TARGET)