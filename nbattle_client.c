#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

int main(){
	
	//struct per il bind 
	struct sockaddr_in server_addr;	
	int fd; // file descriptor per il socket locale che servirà per connettersi e parlare col server 
	int err;	
	//lo scopo del sk è di ascoltare connessioni dei client
	fd=socket (AF_INET, SOCK_STREAM,0);
	if (fd<0){
		perror("socket()");
		return -1;//xke devo uscire dal programma
	}
	server_addr.sin_addr.s_addr= INADDR_ANY;
	server_addr.sin_family=AF_INET;
	server_addr.sin_port= htons(5555);
		
	err=connect(fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
	if (err){
		perror("connect()");
		return -1;//xke devo uscire dal programma
	}
//anche se nn lo scrivo, la close la fa da solo
	close(fd);

	return 0;
}
