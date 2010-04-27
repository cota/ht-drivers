INCDIR := ../include

all: validator.bin calc.bin

.PHONY: all clean

validator.bin: validator.c
	$(CC) -o $@ -I$(INCDIR) -O3 $^

calc.bin: calc.c
	$(CC) -o $@ -O3 $^

clean:
	rm -rf *.bin

