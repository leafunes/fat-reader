.PHONY: all clean

SRC = read_fat.c 
BIN = $(SRC:.c=)

clean:
	rm -f ./bin/* $(OBJ)

all: 
	mkdir -p bin
	gcc read_fat.c -o ./bin/read_fat
