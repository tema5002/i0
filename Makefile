CC=clang
CFLAGS=-O3 -Wall -Wextra -Werror -std=c99 -D_XOPEN_SOURCE=500
OUT=i0
SRC=i0.c

all: $(OUT)

$(OUT): $(SRC) $(wildcard lang/*)
	$(CC) $(SRC) -o $(OUT) $(CFLAGS)

clean:
	rm -rf $(OUT)

.PHONY: all clean
