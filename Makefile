mem: mem.c
	gcc -c -fpic mem.c -Wall -Werror
	gcc -shared -o libmem.so mem.o
clean:
	rm mem.o
