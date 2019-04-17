.PHONY: all clean

SRC = read_fat.c 
BIN = $(SRC:.c=)

clean:
	rm -f ./bin/* $(OBJ)

all: 
	gcc read_fat.c -o ./bin/read_fat
