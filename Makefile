
raidsim: main.o disk-array.o disk.o
	gcc main.o disk-array.o disk.o -o raidsim

main.o: main.c
	gcc -Wall -g -c main.c -o main.o

disk-array.o: disk-array.c
	gcc -Wall -g -c disk-array.c -o disk-array.o

disk.o: disk.c
	gcc -Wall -g -c disk.c -o disk.o

test: test.o disk-array.o disk.o
	gcc test.o disk-array.o disk.o -o raidsim

test.o: test.c
	gcc -Wall -g -c test.c -o test.o

clean:
	rm -f *.o raidsim
	rm -f *.o test
