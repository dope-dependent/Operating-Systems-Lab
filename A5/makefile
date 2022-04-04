all: demo1 demo2 demo3

demo1: memlab.o demo1.o
	gcc -o demo1 demo1.o memlab.o -lm -pthread

demo1.o: demo1.c
	gcc -c demo1.c

demo2: memlab.o demo2.o
	gcc -o demo2 demo2.o memlab.o -lm -pthread

demo2.o: demo2.c
	gcc -c demo2.c

demo3: memlab.o demo3.o
	gcc -o demo3 demo3.o memlab.o -lm -pthread

demo3.o: demo3.c
	gcc -c demo3.c

memlab.o: memlab.h memlab.c
	gcc -c memlab.c

clean: 
	rm -rf memlab.o demo2.o demo1.o demo2 demo1