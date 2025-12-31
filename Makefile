all: example

example: example.c
	gcc -Wall -Wextra -std=c99 -o example example.c

clean:
	rm -f example

.PHONY: all clean
