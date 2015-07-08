all: nbattle_client nbattle_server

nbattle_client: nbattle_client.o nbattle_common.o
	gcc -g -Wall -o nbattle_client nbattle_common.o nbattle_client.o

nbattle_server: nbattle_server.o nbattle_common.o
	gcc -g -Wall -o nbattle_server nbattle_server.o nbattle_common.o -lpthread

nbattle_client.o: nbattle_client.c
	gcc -g -Wall -c nbattle_client.c

nbattle_server.o: nbattle_server.c
	gcc -g -Wall -c nbattle_server.c

nbattle_common.o: nbattle_common.c
	gcc -g -Wall -c nbattle_common.c

clean:
	-rm *.o nbattle_client nbattle_server
