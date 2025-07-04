all: src/*.c
	clang src/main.c -o i0 -Wall -Wextra -std=c99 -O3 -D_XOPEN_SOURCE=700