all: deliver 

server: deliver.o
	gcc -o $@ $^

%.o: %.c
	gcc -c $<

clean:
	rm -f *.o deliver 
