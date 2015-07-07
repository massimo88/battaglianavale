#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

void usage (void){
	printf("	nbattle_client <host remoto> <porta>\n");
}

int main(int argc, char*argv[]){
	
	//struct per il bind 
	struct sockaddr_in server_addr;	
	int fd; // file descriptor per il socket locale che servirà per connettersi e parlare col server.
	int err;
	int port;
	//gestione argomenti linea di comando
	if (argc!=3){
		printf("Sono richiesti due argomenti\n");
		usage();
		return -1;
	}
	port=atoi(argv[2]);
	if (port<=0 || port>65535) {
		printf ("porta non valida, 1<=port<=65535\n");
		usage();
		return -1;	
}
		
	if (inet_pton(AF_INET,argv[1],&server_addr.sin_addr)!=1){
		printf ("Indirizzo Host remoto non valido\n");
		usage();
		return -1;	
	}
	
	//lo scopo del sk è di ascoltare connessioni dei client
	fd=socket (AF_INET, SOCK_STREAM,0);
	if (fd<0){
		perror("socket()");//mi dà -1 se c'è errore
		return -1;//xke devo uscire dal programma xke non ha senso coninuare
	}
	server_addr.sin_addr.s_addr= INADDR_ANY;
	server_addr.sin_family=AF_INET;
//dichiaro la porta dove devo collegarmi
	server_addr.sin_port= htons(port);
		
	err=connect(fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
	if (err){
		perror("connect()");
		return -1;//xke devo uscire dal programma
	}
//anche se nn lo scrivo, la close la fa da solo
	close(fd);

	return 0;
}
