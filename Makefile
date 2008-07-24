CFLAGS=-Wall -ansi -O2 -g -lm

all: bwt mtf huffman entropy

clean:
	rm -f bwt mtf huffman entropy
