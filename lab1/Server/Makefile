all: server 

server: server.o
	gcc -o $@ $^

%.o: %.c
	gcc -c $<

clean:
	rm -f *.o server 
