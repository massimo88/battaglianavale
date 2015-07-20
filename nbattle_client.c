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

int cmd_help(int fd,char* tokens[],int num_tokens){
	print_help();
        return 0;
}

//funzione comune per spedire un comando
int cmd_common(int fd,char* tokens[],int num_tokens){
	int err;
	int n=0;
	char buff[MAX_BUFF_LEN+1];
	char *first_space;
	int retcode;

	err = send_tokens(fd, tokens, num_tokens);
	if (err) {
		return err;
	}

	n=read_message(fd,buff,sizeof(buff));
	if(n<0){
		printf("Errore ricezione risposta\n");
		return -1;
	}
	buff[n]='\0';//mette il terminatore alla stringa del msg ricevuto dal server

	first_space = strchr(buff, ' ');
	if (first_space == NULL || first_space - buff != 2 ||
		 (strncmp(buff, "OK", 2) && strncmp(buff, "KO", 2))) {
		printf("Formato risposta server non valido");
		return -1;
	}

	// 0 significa OK, 1 significa KO

	retcode = (strncmp(buff, "OK", 2) == 0) ? 0 : 1;

	if (retcode == 0) {
		printf("[OK]");
	} else {
		printf("[FAIL]");
	}
	printf("%s\n", first_space + 1);

	return retcode;
}

int cmd_who(int fd,char* tokens[],int num_tokens){
	return cmd_common(fd,tokens,num_tokens);
}

int cmd_create(int fd,char* tokens[],int num_tokens){
	int retcode;
	printf("Nuova partita creata\n");
	printf("In attesa di un avversario...\n");
	retcode = cmd_common(fd,tokens,num_tokens);
	if (retcode == 0) {
		printf("La partita è iniziata\n");
	}
	return retcode;
}

int cmd_join(int fd,char* tokens[],int num_tokens){
	char *line = NULL; //stringa che contiene i caratteri della linea
	int n;//conterrà il numero delle lettere inserite da tastiera nella linea
        size_t nchars;//scritto su manuale della getline
	int retcode;
	printf("Inserire lo username dell'utente da sfidare: ");
	n=getline(&line,&nchars,stdin);//stdin file che rappresenta lo standardinput
	if(n<0){
		perror("getline()");
		return -1;
	}

	if(strcmp(line,"\n")==0){
		printf("Il nome non può essere vuoto\n");
		return -1;
	}
	line[n-1]='\0';
	tokens[1]= line; //metto l username come argomento della join 
	retcode = cmd_common(fd,tokens,2);//ignora gli argomenti dopo il "!join"
	free(line);// xke la getline mi alloca il buffer e quindi è compito del chiamante liberarla
	printf("La partita è iniziata\n");
	return retcode;
}

int cmd_disconnect(int fd,char* tokens[],int num_tokens){
	int retcode;
	retcode = cmd_common(fd,tokens,1);//ignora gli argomenti dopo il "!join"
	if (retcode == 0) {
		printf("Disconesso dalla partita\n");
	}
	return retcode;
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
		int retcode;
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
		
		if (k == 0) {
			free(line);
			continue;
		}

		if(strcmp(tokens[0],"!help")==0){  //token[0] si riferisce alla prima parola
			cmd_help(fd,tokens,k);			
		}
		else if(strcmp(tokens[0],"!who")==0){
			cmd_who(fd,tokens,k);			
		}
		else if(strcmp(tokens[0],"!create")==0){
			if (game_running) {
				// faccio il controllo lato client (qui) e non lo faccio
				// sul server, ma andrebbe fatto anche lì per evitare
				// attacchi informatici da parte di client maliziosi (modificati)
				printf("Non posso creare la partita non sto già giocando\n");
			} else {
				retcode = cmd_create(fd,tokens,k);
				if (retcode == 0) {
					// passo in modalità di gioco
					game_running = 1;
				}
			}
		}
		else if(strcmp(tokens[0],"!join")==0){
			if (game_running) {
				// faccio il controllo lato client (qui) e non lo faccio
				// sul server, ma andrebbe fatto anche lì per evitare
				// attacchi informatici da parte di client maliziosi (modificati)
				printf("Non posso unirmi alla partita non sto già giocando\n");
			} else {
				retcode = cmd_join(fd,tokens,k);
				if (retcode == 0) {
					// passo in modalità di gioco
					game_running = 1;
				}
			}
		}
		else if(strcmp(tokens[0],"!disconnect")==0){
			if (!game_running) {
				printf("Non posso disconnettermi, non sto giocando\n");
			} else {
				retcode = cmd_disconnect(fd, tokens,k);
				if (retcode == 0) {
					// rimetto a zero perchè ritorno in
					// modalità configurazione
					game_running = 0;
				}
			}
		}
		else if(strcmp(tokens[0],"!quit")==0){
			break;
		}
		else if(strcmp(tokens[0],"!show_enemy_map")==0){
			if (!game_running) {
				printf("Non posso, non sto giocando!\n");
			} else {
			}
		}
		else if(strcmp(tokens[0],"!show_my_map")==0){
			if (!game_running) {
				printf("Non posso, non sto giocando!\n");
			} else {
			}
			
		}
		else if(strcmp(tokens[0],"!hit")==0){
			if (!game_running) {
				printf("Non posso, non sto giocando!\n");
			} else {
			}
			
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
