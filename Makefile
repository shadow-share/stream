
.PHONY all: stream

stream: stream.c main.c
	$(CC) -Wall -std=c99 -o stream $^

.PHONY clean:
	rm -rf stream

