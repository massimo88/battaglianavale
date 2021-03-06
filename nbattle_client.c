#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include "nbattle_common.h"



struct specifica_navi {
	int num;
	int dimensione;
};

/*
struct specifica_navi specifica[] = {
	{
		.num = 1,
		.dimensione = 4,
	},
	{
		.num = 2,
		.dimensione = 3,
	},
	{
		.num = 3,
		.dimensione = 2,
	},
	{
		.num = 4,
		.dimensione = 1,
	},
};
*/

struct specifica_navi specifica[] = {
	{
		.num = 1,
		.dimensione = 4,
	},
};
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

void print_map(char map[COLS][COLS])
{
	int i,j;

	for (i=COLS-1; i>=0;i--){
		printf("%2d ", i+1);
		for (j=0;j<COLS;j++) {
			printf("%c ", map[i][j]);	
		}
		printf("\n");
	}
	printf("   ");
	for (j=0; j<COLS; j++) {
		printf("%c ", 'A'+j);
	}
	printf("\n");
}

int gioco_finito(char m[COLS][COLS], int num_X)
{
	int i,j;
	int num_H = 0;
	for (i=0;i<COLS;i++){
		for (j=0;j<COLS;j++) {
			if (m[i][j] == 'H')
				num_H++;
		}
	}

	return num_X == num_H;
}

int read_response(int fd, char *buff, int len, char **resp)
{
	char *first_space;
	int n=0;
	int retcode;
	n=read_message(fd,buff,len);
	if(n<=0){
		if (n == 0) {
			printf("Peer ha chiuso connessione\n");
		} else {
			printf("Errore ricezione risposta\n");
		}
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

	*resp = first_space +1;

	return retcode;
}

//funzione comune per spedire un comando
int cmd_common(int fd,char* tokens[],int num_tokens){
	int err;
	char buff[MAX_BUFF_LEN+1];
	char *resp;
	int retcode;

	err = send_tokens(fd, tokens, num_tokens);
	if (err) {
		return err;
	}

	retcode = read_response(fd, buff, MAX_BUFF_LEN, &resp);

	if (retcode == 0) {
		printf("[OK]");
	} else {
		printf("[FAIL]");
	}
	printf("%s\n", resp);

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
		printf("Disconessione avvenuta con successo: TI SEI ARRESO\n");
	}
	return retcode;
}


int leggi_coord(char *s, int *riga, int *colonna)
{
	if (strlen(s) < 2 || s[0] <'A' || s[0] > 'A'+COLS-1) {
		printf("Coordinate non valide\n");
		return -1;
	}
	*colonna = s[0] -'A';
	*riga = atoi(s+1);
	(*riga)--;
	if (*riga < 0 || *riga >= COLS) {
		printf("Coordinate non valide\n");
		return -1;
	}

	return 0;
}

int cmd_hit(int fd,char* tokens[],int num_tokens, char peer_map[COLS][COLS]){
	int retcode;
	int riga, colonna;
	char buff[MAX_BUFF_LEN+1];
	char *resp;
	if (num_tokens < 2) {
		printf("Comando !hit richiede due argomenti\n");
		return -1;
	}

	// validiamo le coordinate inserite dall'utente
	retcode = leggi_coord(tokens[1], &riga, &colonna);
	if (retcode) {
		return retcode;
	}

	// manda la mossa al server
	retcode = send_tokens(fd, tokens, 2);
	if (retcode) {
		return retcode;
	}
	// aspetta la risposta dell'avversario inoltrata dal server
	// (COLPITO o MANCATO)
	retcode = read_response(fd, buff, MAX_BUFF_LEN, &resp);

	if (retcode == 0) {
		printf("[OK]");
		printf("AVVERSARIO DICE %s\n", resp);
		if (resp[0] == 'C') {
			peer_map[riga][colonna] = 'H';
		} else if (resp[0] == 'M') {
			peer_map[riga][colonna] = 'o';
		}
	} else {
		printf("[FAIL]");
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
int leggi_mappa(char map[COLS][COLS])
{
	int i, j;
	// numero di celle in array statico = dimensione totale (in bytes) diviso
	// dimensione di una cella
	for (i=0; i<sizeof(specifica)/sizeof(struct specifica_navi);i++) {
		for (j=0;j<specifica[i].num;) {
			size_t nchars = 0;
			char *line = NULL;
			int n;
			int orizzontale;
			int riga;
			int colonna;
			int k;
			int overlap = 0;
			int err;
			printf("Inserisci le coordinate della %da nave di dimensione %d: ",
				j+1, specifica[i].dimensione);
			n=getline(&line,&nchars,stdin);
			if(n<0){
				perror("getline()");
				return -1;
			}
			line[n-1]='\0';
			err = leggi_coord(line, &riga, &colonna);
			free(line);
			if (err) {
				continue;
			}

			line = NULL;
			printf("Inserisci l'orientamento della %da nave di dimensione %d: ",
				j+1, specifica[i].dimensione);
			n=getline(&line,&nchars,stdin);
			if(n<0){
				perror("getline()");
				return -1;
			}
			line[n-1]='\0';

			if (strcmp(line, "O") == 0) {
				orizzontale = 1;
			} else if (strcmp(line, "V") == 0) {
				orizzontale = 0;
			} else {
				printf("Orientamento non valido\n");
				free(line);
				continue;
			}
			free(line);

			if ((orizzontale && colonna + specifica[i].dimensione > COLS) ||
				(!orizzontale && riga + specifica[i].dimensione > COLS)) {
				printf("Nave fuori da mappa! Riprova");
				continue;
			}

			for (k=0; k<specifica[i].dimensione; k++) {
				if ((orizzontale && map[riga][colonna+k] == 'X') ||
						(!orizzontale && map[riga+k][colonna] == 'X')) {
					overlap = 1;
					break;
				}
			}

			if (overlap) {
				printf("Nave sovrapposta ad un'altra nave esistente\n");
				continue;
			}

			for (k=0; k<specifica[i].dimensione; k++) {
				if (orizzontale) {
					map[riga][colonna+k] = 'X';
				} else {
					map[riga+k][colonna] = 'X';
				}
			}

			j++;
		}
	}
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
	int mio_turno=0; // è il mio turno o quello dell'avversario ?
	char map[COLS][COLS];
	char peer_map[COLS][COLS];
	int num_X;
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

	memset(map, '-', COLS * COLS);
	memset(peer_map, '-', COLS * COLS);
	print_map(map);

	// calcola num_X
	num_X = 0;
	{
		int i;
		for (i=0; i<sizeof(specifica)/sizeof(struct specifica_navi);i++) {
			num_X += specifica[i].dimensione * specifica[i].num;
		}
	}
	printf("Numero totale di X = %d\n", num_X);

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
					memset(map, '-', COLS * COLS);
					leggi_mappa(map);
					print_map(map);
					mio_turno = 1;
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
					memset(map, '-', COLS * COLS);
					leggi_mappa(map);
					print_map(map);
					mio_turno = 0;
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
				print_map(peer_map);
			}
		}
		else if(strcmp(tokens[0],"!show_my_map")==0){
			if (!game_running) {
				printf("Non posso, non sto giocando!\n");
			} else {
				print_map(map);
			}
			
		}
		else if(strcmp(tokens[0],"!hit")==0){
			if (!game_running) {
				printf("Non posso, non sto giocando!\n");
			} else {
				cmd_hit(fd,tokens,k, peer_map);
				mio_turno = 0;		
				if (gioco_finito(peer_map, num_X)) {
					printf("Hai vinto!\n");
					game_running = 0;
				}
			}
			
		}
		else{
			printf("comando %s non esistente\n",tokens[0]);
		}
		free(line);

		if (game_running && !mio_turno) {
			// siamo in fase di gioco, ma non è il mio turno, quindi
			// devo aspettare che il server mi inoltri il la mossa
			// dell'avversario
			char buff[MAX_BUFF_LEN+1];
			int retcode;
			char *resp;
			int riga, colonna;
			char *tokens[2];

			retcode = read_response(fd, buff, MAX_BUFF_LEN, &resp);
			if (retcode) {
				printf("Errore durante ricezione mossa avversaria\n");
				break;
			}

			if (strncmp(resp, "DD", 2) == 0) {
				printf("L'avversario si è arreso! Hai vinto\n");
				game_running = 0;
			} else {
				printf("AVVERSARIO SPARA SU '%s'\n", resp);

				tokens[0] = "!resp";
				tokens[1] = "M";

				retcode = leggi_coord(resp, &riga, &colonna);
				if (retcode) {
					printf("NOn può succedere\n");
				} else {
					if (map[riga][colonna] == 'X') {
						tokens[1] = "C";
						map[riga][colonna] = 'H';
					} else {
						map[riga][colonna] = 'o';
					}
				}

				retcode = send_tokens(fd, tokens, 2);
				if (retcode) {
					printf("Errore durante invio responso\n");
					break;
				}

				mio_turno = 1;

				if (gioco_finito(map, num_X)) {
					printf("Hai perso!\n");
					game_running = 0;
				}
			}
		}
	}
//anche se nn lo scrivo, la close la fa da solo
	close(fd);

	return 0;
}
