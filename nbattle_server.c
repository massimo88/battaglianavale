#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

#define NUM_THREADS 5
//struttura informazioni relative ad un thread nel pool
struct gestore {
	pthread_t th;	
	pthread_cond_t newrequest;
	pthread_mutex_t lock;
};

void * gestisci_client(void *arg){
	
	struct gestore* g=arg;
	for(;;){
//prendo il lock per utilizzare la variabile codition
		pthread_mutex_lock(&g->lock);
//aspetta che arrivi l segnale nel frattempo si blocca rilasciano il lock
		pthread_cond_wait(&g->newrequest,&g->lock);
		pthread_mutex_unlock(&g->lock);
		printf("Ho gestito la connessione\n");	
	};	
	return NULL;
}

int main(){
	//array di varibaili pthread_t di tipo opaco(non sappiamo come è fatto dentro
	struct gestore pool[NUM_THREADS];	
	//struct per il bind 
	struct sockaddr_in server_addr;	
	int lfd; // listening file descriptor per il socket 
	int err;
	int i;
	int curr_thread=0; //thread corrente	
	
	for(i=0; i< NUM_THREADS; i++) {
		err=pthread_create(&pool[i].th, NULL, gestisci_client, &pool[i]);
		if (err){
			perror("phtread_create()");
			return -1;//xke devo uscire dal programma
		}
		pthread_cond_init(&pool[i].newrequest, NULL);
		pthread_mutex_init(&pool[i].lock, NULL);
	}
	
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
//sveglio il thread i esimo che si era bloccato nella wait
		pthread_mutex_lock(&pool[curr_thread].lock);
		pthread_cond_signal (&pool[curr_thread].newrequest);		
		pthread_mutex_unlock(&pool[curr_thread].lock);
		curr_thread=(curr_thread+1)%NUM_THREADS;
	}
	
	

	return 0;
}
