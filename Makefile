all: libreman.a myapp

libreman.a: reman.c
	gcc -Wall -c reman.c
	ar -cvq libreman.a reman.o
	ranlib libreman.a

myapp: myapp.c
	gcc -Wall -o myapp myapp.c -L. -lreman -lpthread

clean:
	rm -fr *.o *.a *~ a.out myapp reman.o libreman.a