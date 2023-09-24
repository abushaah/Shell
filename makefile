myShell: myShell.o
	gcc -Wpedantic -g -std=gnu99 myShell.o -o myShell
myShell.o: myShell.c myShell.h
	gcc -Wpedantic -g -std=gnu99 -c myShell.c
clean:
	rm *.o
