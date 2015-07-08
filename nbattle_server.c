#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include "nbattle_common.h"

#define NUM_THREADS 5
#define MAX_USERNAME_LEN 32


//struttura informazioni relative ad un thread nel s.pool
struct gestore {
	char username[MAX_USERNAME_LEN+1];//+1 per dare spazio al terminatore
	int fd;
	pthread_t th;	
	pthread_cond_t newrequest;
	int occupato;
	pthread_mutex_t lock;
};


struct server {
	//array di varibaili pthread_t di tipo opaco(non sappiamo come è fatto dentro
	struct gestore pool[NUM_THREADS];	
	pthread_cond_t gestore_libero; //per aspettare che un thread si liberi
	pthread_mutex_t main_lock;	
	};

struct server s; //variabili globale

//implementare la funzione dei client connessi
void server_who(int fd,char* tokens[],int num_tokens,struct gestore*g){
	int i;
	int n;
	int err;
	char buf[MAX_BUFF_LEN];
	int left=sizeof(buf);//variabili che tiene i bytes disponibili nel buffer
	char *cur = buf;

	n=snprintf(cur,left,"Clienti connessi al server:");
	cur+=n;
	left-=n;
//scorri i pool di thread per raccogliere i nomi(username)
	for(i=0;i<NUM_THREADS && left>0;i++){
		pthread_mutex_lock (&s.pool[i].lock);
		n=snprintf(cur,left,"%s ", s.pool[i].username);
		cur+=n;
		left-=n;
		pthread_mutex_unlock (&s.pool[i].lock);
	}
//invio la risposta
	err=send_message(g->fd,buf,MAX_BUFF_LEN-left);
	if(err){
		printf("Errore di trasmissione risposta\n");
		return;
	}
}

void gestisci_client_2(struct gestore*g){
	int n;
	//legge l'username dal client
	pthread_mutex_lock (&g->lock);
	n=read_message(g->fd,g->username,MAX_USERNAME_LEN);		
	if(n<=0){
		send_string(g->fd,"Errore durante lettura username");
		return;
	}
	g->username[n]='\0';//metto il terminatore alla stringa username
	pthread_mutex_unlock (&g->lock);
	printf("Gestisco la connessione con %s\n",g->username);
	
	for(;;){
		int err;
		char cmd_buf[MAX_BUFF_LEN];
		char *tokens[MAX_ARGS+1];//conterrà i token della linea di input, +1 xke l ultima cella deve memorizzare null
		int k=0; //serve per indicizzare l array token
		int i;
		n=read_message(g->fd,cmd_buf,MAX_BUFF_LEN);
		if(n<=0){
			send_string(g->fd,"Errore durante lettura username");
			return;
		}
		cmd_buf[n]='\0';
		
		tokens[k] = strtok(cmd_buf, " ");
		while( tokens[k] != NULL ) 
   		{
			k++;//memorizzo in ciascuna cella un argomento diverso
			if(k>=MAX_ARGS){
				break;
			}
			tokens[k] = strtok(NULL, " ");
   		}
		
		printf("letti %d\n", k);
		for(i=0;i<k;i++){
			printf("%s\n",tokens[i]);
		}
		
		if(strcmp(tokens[0],"!who")==0){
			server_who(g->fd,tokens,k,g);			
		}
		else if(strcmp(tokens[0],"!create")==0){
			
		}
		else if(strcmp(tokens[0],"!join")==0){
			
		}
		else if(strcmp(tokens[0],"!disconnect")==0){
			
		}
		else if(strcmp(tokens[0],"!quit")==0){
			
		}
		else if(strcmp(tokens[0],"!show_enemy_map")==0){
			
		}
		else if(strcmp(tokens[0],"!show_my_map")==0){
			
		}
		else if(strcmp(tokens[0],"!hit")==0){
			
		}
		else{
			printf("comando %s non esistente\n",tokens[0]);
		}
		
		printf("%s\n",cmd_buf);
		err=send_string(g->fd,"xdgdg");
		if(err){
			printf("Errore trasmissione risposta\n");
			return;
		}
	}
	printf("Ho gestito la connessione con %s\n",g->username);
	usleep(5000000);
} 

void * gestisci_client(void *arg){
	
	struct gestore* g=arg;
	for(;;){
		

//prendo il lock per utilizzare la variabile codition
		pthread_mutex_lock(&s.main_lock);
//aspetta che arrivi l segnale nel frattempo si blocca rilasciano il lock
		if(!g->occupato){
			pthread_cond_wait(&g->newrequest,&s.main_lock);
		}
		pthread_mutex_unlock(&s.main_lock);

		gestisci_client_2(g);
		
		pthread_mutex_lock(&s.main_lock);
		g->occupato=0;
	//sveglio, dato che nn sono piu occupato, il main
		pthread_cond_signal(&s.gestore_libero);
		pthread_mutex_unlock(&s.main_lock);	
	};	
	return NULL;
}

void usage (void){
	printf("	nbattle_client <host remoto> <porta>\n");
}
int main(int argc, char* argv[]){
	//struct per il bind 
	struct sockaddr_in server_addr;	
	int lfd; // listening file descriptor per il socket 
	int err;
	int i;
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



	pthread_cond_init(&s.gestore_libero, NULL);	
	pthread_mutex_init(&s.main_lock, NULL);//inizializzo il lock
	for(i=0; i< NUM_THREADS; i++) {
		s.pool[i].occupato=0;
		err=pthread_create(&s.pool[i].th, NULL, gestisci_client, &s.pool[i]);
		if (err){
			perror("phtread_create()");
			return -1;//xke devo uscire dal programma
		}
		pthread_cond_init(&s.pool[i].newrequest, NULL);
		pthread_mutex_init(&s.pool[i].lock, NULL);//inizializzo il lock
	}
	
	//lo scopo del sk è di ascoltare connessioni dei client
	lfd=socket (AF_INET, SOCK_STREAM,0);
	if (lfd<0){
		perror("socket()");
		return -1;//xke devo uscire dal programma
	}
	server_addr.sin_addr.s_addr= INADDR_ANY;
	server_addr.sin_family=AF_INET;
	server_addr.sin_port= htons(port);
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

		pthread_mutex_lock(&s.main_lock);
//ciclo per il controllo 		
		for(;;){
			for(i=0;i<NUM_THREADS;i++){
				if(!s.pool[i].occupato)
					break;			
			}
			if(i==NUM_THREADS){
				printf("Tutto occupato...\n");
				pthread_cond_wait(&s.gestore_libero, &s.main_lock);	
			}
			else
				break;
		}
		printf("Selezionato thread %d\n", i);
		//la variabile i puo valere da 0 a numthreads-1
		s.pool[i].occupato=1;
		s.pool[i].fd= cfd;
  	//sveglio il thread i esimo che si era bloccato nella wait		
		pthread_cond_signal (&s.pool[i].newrequest);
		pthread_mutex_unlock(&s.main_lock);
	}

	return 0;
}
