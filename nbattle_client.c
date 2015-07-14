#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include "nbattle_common.h"



void usage (void){
	printf("	nbattle_client <host remoto> <porta>\n");
}

void print_help(void)
{
	printf("Sono disponibili i seguenti comandi:\n"
"*!help --> mostra l'elenco dei comandi disponibili\n"
"*!who --> mostra l'elenco dei client connessi al server\n"
"*!create --> crea una nuova partita e attendi un avversario\n"
"*!join --> unisciti ad una partita e inizia a giocare\n"
"*!disconnect --> disconnetti il client dall'attuale partita\n"
"*!quit --> disconnetti il client dal server\n"
"*!show_enemy_map --> mostra la mappa dell'avversario\n"
"*!show_my_map --> mostra la tua mappa\n"
"* !hit coordinates --> colpisci le coordinate coordinates\n");
}

void cmd_help(int fd,char* tokens[],int num_tokens){
	print_help();
}
//funzione comune per spedire un comando
void cmd_common(int fd,char* tokens[],int num_tokens){
	int err;
	int n;
	char buff[MAX_BUFF_LEN+1];
	err=send_string(fd,tokens[0]);
	if(err){
		printf("Errore trasmissione comando %s \n",tokens[0]);
		return;
	}
	n=read_message(fd,buff,sizeof(buff));
	if(n<0){
		printf("Errore ricezione risposta\n");
		return;
	}
	buff[n]='\0';//mette il terminatore alla stringa del msg ricevuto dal server
	printf("%s\n",buff);
}

void cmd_who(int fd,char* tokens[],int num_tokens){
	cmd_common(fd,tokens,num_tokens);
}

void cmd_create(int fd,char* tokens[],int num_tokens){
	printf("Nuova partita creata\n");
	printf("In attesa di un avversario...");
	cmd_common(fd,tokens,num_tokens);
	printf("La partita è iniziata");	
}

void cmd_join(int fd,char* tokens[],int num_tokens){
	char *line = NULL; //stringa che contiene i caratteri della linea
	int n;//conterrà il numero delle lettere inserite da tastiera nella linea
        size_t nchars;
	printf("Inserire lo username dell'utente da sfidare: ");
	n=getline(&line,&nchars,stdin);//stdin file che rappresenta lo standardinput
	if(n<0){
		perror("getline()");
		return;
	}
	
	if(strcmp(line,"\n")==0){
		printf("Il nome non può essere vuoto\n");
		return;
	}
	line[n-1]='\0';
	tokens[num_tokens]= line; //metto l username come argomento della join 
	num_tokens++; //aggiungo un argomento in coda
	cmd_common(fd,tokens,num_tokens);
	printf("La partita è iniziata");	
}



int get_username(int fd){
	size_t nchars = 0; //variabile che fa parte del funzionamento della getline
	char *line = NULL; //stringa che contiene i caratteri della linea
	int n;//conterrà il numero delle lettere inserite da tastiera nella linea
	//inserimento username
	printf("Inserisci il tuo nome:");
	n=getline(&line,&nchars,stdin);//stdin file che rappresenta lo standardinput
	if(n<0){
		perror("getline()");
		return -1;
	}
	
	if(strcmp(line,"\n")==0){
		printf("Il nome non può essere vuoto\n");
		return -1;
	}
	
//-1 cosi nn includo il \n	
	send_message(fd,line,n-1);
	
	return 0;	
}
int main(int argc, char*argv[]){
	
	//struct per il bind 
	struct sockaddr_in server_addr;	
	int fd; // file descriptor per il socket locale che servirà per connettersi e parlare col server.
	int err;
	int port;
	int i;
	int game_running=0;// variabile booleana che indica se siamo nella fase di gioco o di contrattazione
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
		perror("socket()");//mi dà -1 se c'è errore, mi chiama la funzione di libreria del c
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
	
	print_help();			
	
	err=get_username(fd);
	if(err){
		printf("Errore durante inserimento username\n");
		return -1;
	}
//loop ingresso comandidello standard input inviati al server
	for(;;){
		size_t nchars;//conterrà il numero delle lettere inserite da tastiera nella linea
		char *line = NULL; //stringa che contiene i caratteri della linea
		char *tokens[MAX_ARGS+1];//conterrà i token della linea di input, +1 xke l ultima cella deve memorizzare null
		int k=0; //serve per indicizzare l array token
		int n;
		if(game_running)
			printf("# ");
		else 
			printf("> ");
		n=getline(&line,&nchars,stdin);//stdin file che rappresenta lo standardinput
		if(n<=0){
			perror("getline()");
			continue;//xke ci riprova che può essere un errore temporaneo, causato da mancanza di memoria(per esempio)
		}
		
		// sostituisci '\n' con '\0', il \n nn lo voglio e lo voglio eliminare, xke la getline mi restituisce una stringa dove nella posizione n-1 mi restituisce il \n. 
		line[n-1] = '\0';
		
		tokens[k] = strtok(line, " "); //funzione non rientrante
		while( tokens[k] != NULL ) 
   		{
			k++;//memorizzo in ciascuna cella un argomento diverso
			if(k>=MAX_ARGS){
				break;
			}
			tokens[k] = strtok(NULL, " ");
   		}
		
		for(i=0;i<k;i++){
			printf("%s\n",tokens[i]);
		}
		
		if(strcmp(tokens[0],"!help")==0){  //token[0] si riferisce alla prima parola
			cmd_help(fd,tokens,k);			
		}
		else if(strcmp(tokens[0],"!who")==0){
			cmd_who(fd,tokens,k);			
		}
		else if(strcmp(tokens[0],"!create")==0){
			cmd_create(fd,tokens,k);
			
		}
		else if(strcmp(tokens[0],"!join")==0){
			cmd_join(fd,tokens,k);
			
		}
		else if(strcmp(tokens[0],"!disconnect")==0){
			
		}
		else if(strcmp(tokens[0],"!quit")==0){
			break;
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
		free(line);
	}
//anche se nn lo scrivo, la close la fa da solo
	close(fd);

	return 0;
}
