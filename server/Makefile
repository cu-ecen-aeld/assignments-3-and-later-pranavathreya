default: all

all: clean build

build:
	${CC} -o aesdsocket aesdsocket.c -Wall -Werror -pthread

clean:
	rm -rf *.o aesdsocket

run:
	./aesdsocket

