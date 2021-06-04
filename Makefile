CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -O3 -std=c99

MAIN = bin/main.o
OBJS = bin/mutator.o bin/rng.o bin/strategy.o
LIB = libcmutator.a

.PHONY: clean

all: mutator

bin/%.o: src/%.c
	$(CC) $(CFLAGS) -c $? -o $@

$(LIB): $(OBJS)
	ar rcs $(LIB) $^

mutator: $(MAIN) $(LIB)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f $(OBJS) $(MAIN)
	rm -f $(LIB)
	rm -f mutator