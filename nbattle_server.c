#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>


void gestisci_client (int cfd) {
		
	printf("Nuova Connessione\n");		
}

int main(){
	
	//struct per il bind 
	struct sockaddr_in server_addr;	
	int lfd; // listening file descriptor per il socket 
	int err;	
	//lo scopo del sk è di ascoltare connessioni dei client
	lfd=socket (AF_INET, SOCK_STREAM,0);
	if (lfd<0){
		perror("socket()");
		return -1;//xke devo uscire dal programma
	}
	server_addr.sin_addr.s_addr= INADDR_ANY;
	server_addr.sin_family=AF_INET;
	server_addr.sin_port= htons(5555);
	err=bind(lfd,(struct sockaddr*)&server_addr, sizeof(server_addr));
	
	if(err) {
		perror("bind()");
		return -1;//xke devo uscire dal programma
	}
	//una volta che err è 0, la posso riusare
	err=listen(lfd,10);
	if(err) {
		perror("listen()");
		return -1;//xke devo uscire dal programma
	}
	//creo un for per accettare molte connessioni che mi vengono richeste
	for(;;){
		int cfd;//client file descriptor
		socklen_t addrlen;
		struct sockaddr_in client_addr;
//ritorna un file descriptor, associato alla connessione delclient
		cfd=accept(lfd,(struct sockaddr*)&client_addr, &addrlen);
		if(cfd<0) {
			perror("accept()");
			break;
		}
		gestisci_client(cfd);
		close(cfd);
	}
	
	

	return 0;
}
