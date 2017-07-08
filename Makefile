
.PHONY all: stream


stream.o: src/stream.c

main.o: src/main.c

stream: stream.o main.o
    cc -o $@ $^


.PHONY clean:
	rm -rf *.o stream
