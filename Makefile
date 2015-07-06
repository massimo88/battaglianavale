all: nbattle_client nbattle_server

nbattle_client: nbattle_client.o
	gcc -Wall -o nbattle_client nbattle_client.o

nbattle_server: nbattle_server.o
	gcc -Wall -o nbattle_server nbattle_server.o -lpthread

nbattle_client.o: nbattle_client.c
	gcc -Wall -c nbattle_client.c

nbattle_server.o: nbattle_server.c
	gcc -Wall -c nbattle_server.c

clean:
	-rm *.o nbattle_client nbattle_server
