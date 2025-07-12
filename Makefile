CC=clang
CFLAGS=-std=c99 -g -Wall -Wextra -Wpedantic
OBJ=json.o example.o

example: $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o example

$(OBJ): *.h

.PHONY: clean
clean:
	rm -f *.o
	rm -f example
