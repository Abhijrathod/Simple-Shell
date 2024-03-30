all: PA1.o 
	gcc PA1.o -o shell352 -lm

PA1.o: PA1.c 
	gcc -c PA1.c

debug:
	gcc -o shell352 -g PA1.c

clean:
	rm *.o *.h.gch shell352