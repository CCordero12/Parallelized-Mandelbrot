
all: mandel mandelmovie

mandel: mandel.o bitmap.o 
	gcc mandel.o bitmap.o -o mandel -lpthread

mandel.o: mandel.c
	gcc -Wall -g -c mandel.c -o mandel.o

bitmap.o: bitmap.c
	gcc -Wall -g -c bitmap.c -o bitmap.o

mandelmovie: mandelmovie.c
	gcc -Wall -o mandelmovie mandelmovie.c -lm
	chmod +x mandelmovie

clean:
	rm -f mandel.o bitmap.o mandel
