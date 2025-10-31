CC	:= gcc
CFLAGS  := -O2 -Wall -Wextra -std=c11
LDFLAGS := -lm

TARGET  := xor_nn
SRCS    := nn.c xor_main.c
OBJS    := $(SRCS:.c=.o)

.PHONY: all clean run run_args

all: $(TARGET)

$(TARGET): $(OBJS)
		$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

%.o: %.c nn.h
		$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
		./$(TARGET)

run_args: $(TARGET)
		./$(TARGET) 20000 0.3 42 xnor

clean:
		$(RM) $(OBJS) $(TARGET)

