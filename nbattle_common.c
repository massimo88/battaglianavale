#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>

#include "nbattle_common.h"

//leggi sul fd un buffer di lunghezza len
int read_message(int fd, void* buff,size_t len){
	uint8_t l;
	int n;
	n=read(fd,&l,sizeof(l));
	if(n!=sizeof(l)){
		perror("Errore ricezione lunghezza\n");
		return -1;
	}
	
	if(l>len){
		printf("buffer fornito non è abbastanza grande per %u\n", l);
		return -1;
	}
	
	n=read(fd,buff,l);
	if(n!=l){
		perror("Errore ricezione messaggio\n");
		return -1;
	}
	
	return l;	
}

//spedisce sul fd un buffer di lunghezza len
int send_message(int fd, void* buff,size_t len){
	uint8_t l;
	int n;
	if(len>255){
		printf("Non posso trasmettere messaggi più lunghi di 256 bytes\n");
		return -1;
	}
	l=len;
	printf("About to send %u bytes\n", l);
	n=write(fd,&l,sizeof(l));
	if(n!=sizeof(l)){
		perror("Errore trasmissione lunghezza\n");
		return -1;
	}
	n=write(fd,buff,len);
	if(n!=len){
		perror("Errore trasmissione messaggio\n");
		return -1;
	}
	
	return 0;	
}

int send_string(int fd,char *s){
	return send_message(fd,s,strlen(s)+1);
}