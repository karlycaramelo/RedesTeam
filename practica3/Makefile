
cc = gcc
OBJ = main.o
HEADER = include/header.h
CFLAGS = -c -Wall -Iinclude
BIN = bin

main: $(OBJ)
	$(CC) $(OBJ) -o $(BIN)/$@

main.o: src/main.c $(HEADER)
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf *.o ./bin/*
