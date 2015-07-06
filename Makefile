all: nbattle_client nbattle_server

nbattle_client: nbattle_client.o
	gcc -o nbattle_client nbattle_client.o

nbattle_server: nbattle_server.o
	gcc -o nbattle_server nbattle_server.o

nbattle_client.o: nbattle_client.c
	gcc -c nbattle_client.c

nbattle_server.o: nbattle_server.c
	gcc -c nbattle_server.c

clean:
	-rm *.o nbattle_client nbattle_server
