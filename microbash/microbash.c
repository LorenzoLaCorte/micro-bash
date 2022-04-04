/*	Autore: Lorenzo La Corte
	Titolo: Micro-bash
	Data: Novembre/Dicembre 2020 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <readline/readline.h> //readline
#include <readline/history.h> //readline
#include <fcntl.h> // open
#include <errno.h> // errno

#define SEP_PIPE '|'
#define SEP_WORDS ' '
#define BLUE "\x1b[94m"
#define RESET "\x1b[0m"

// ritorna una stringa contenente la linea inserita dall'utente
char* print_prompt()
{
	// acquisisco e stampo la directory corrente
	printf(BLUE "%s" RESET, getcwd(NULL, 0));
			
	// stampo il prompt e chiedo un comando all'utente (mettendolo in user_input)
    	char *input = readline(BLUE "$ " RESET);
		
	// ritorno il nome del file
	return input;
}

// esegue cd
void exec_builtin(char *argument)
{
	// eseguo il vero e proprio cd
	if(chdir(argument) == -1)
		perror("Error");
}

// ritorna la lunghezza di un array di stringhe
int sizeArrayStrings(char** args)
{
	int count = 0;	

	for(;;)	
	{
		if(args[count] == NULL)		
			break;
		count++;
	}

	return count;
}

// ritorna la lunghezza di un array di array di stringhe
int sizeArrayArrayStrings(char*** args)
{
	int count = 0;	

	for(;;)	
	{
		if(args[count] == NULL)		
			break;
		count++;
	}

	return count; 
}

// uso tante realloc quindi gestisco il loro fallimento in una funzione ausiliaria
void checkAlloc(void * fToCheck)
{
	if(fToCheck == NULL) // se malloc o realloc restituiscono NULL sono fallite
	{
		printf("Allocazione di memoria fallita.\n");
		exit(EXIT_FAILURE);	
	}
}

// imposto flag per verificare che non ci siano piú redirezioni
int r_in = 0;
int r_out = 0;

// esegue la redirezione di input o output a seconda di cosa mettiamo in select_IO 
void red_IO(char * fToRed, int select_IO)
{
	if(select_IO != 0 && select_IO != 1)
	{
		printf("File descriptor errato.\n");
		exit(EXIT_FAILURE);
	}	

	int fd;

	if(select_IO == 0) // gestisco la redirezione dell'input
	{
		fd = open(fToRed, O_RDONLY, S_IRUSR | S_IWUSR);
		if(fd == -1)
		{		
			printf("Errore nella open: %d\n", errno);		
			exit(EXIT_FAILURE);		
		}	
	}

	if(select_IO == 1) // gestisco la redirezione dell'output
	{
		fd = open(fToRed, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);	
		if(fd == -1)
		{
			printf("Errore nella open: %d\n", errno);		
			exit(EXIT_FAILURE);		
		}		
	}

	// redirigo 
	if(dup2(fd, select_IO) == -1)
		printf("Errore nella dup: %d\n", errno);

	close(fd);

}

// gestisce la redirezione eventuale di I/0
void handle_red_IO(char **argms)
{
	// gestisco la redirezione eventuale di input
	if(r_in != 0)
	{
		red_IO(argms[r_in], 0);
		// levo il file dagli argomenti
		argms[r_in] = NULL;	
	}

	// gestisco la redirezione eventuale di output
	if(r_out != 0)
	{
		red_IO(argms[r_out], 1);
		// levo il file dagli argomenti
		argms[r_out] = NULL;	
	}
}

// per ogni argomento dell'istruzione controlla se ci sono espansioni o redirezioni
void checkArgs(char** arguments)
{
	for(int i=1; i<sizeArrayStrings(arguments); i++)
	{
		if(arguments[i][0] == '$') // gestisco l'espansione
		{
			// levo il dollaro
			memmove(&arguments[i][0], &arguments[i][1], strlen(arguments[i]));

			// espando
			arguments[i] = getenv(arguments[i]);
			
			if(arguments[i] == NULL){
				printf("Stai provando a espandere una variabile che non esiste.\n");
				exit(EXIT_FAILURE);
			}
		}

		else if(arguments[i][0] == '<') // gestisco la redirezione dell'input
		{
			if(arguments[i][1] == '\0') 
			{
				printf("Non puoi mettere spazi dopo il simbolo di redirezione.\n");
				exit(EXIT_FAILURE);
			}
		
			if(r_in != 0) // controllo che non ci siano piú redirezioni
			{
				printf("Non puoi effettuare piú di una redirezione di input.\n");
				exit(EXIT_FAILURE);
			}		
			
			// levo il minore
			memmove(&arguments[i][0], &arguments[i][1], strlen(arguments[i]));

			// attivo il flag e lo imposto a i per poi ricordarmi la posizione
			r_in = i;
		}

		if(arguments[i][0] == '>') // gestisco la redirezione dell'output
		{
			if(arguments[i][1] == '\0') 
			{
				printf("Non puoi mettere spazi dopo il simbolo di redirezione.\n");
				exit(EXIT_FAILURE);
			}

			if(r_out != 0)
			{
				printf("Non puoi effettuare piú di una redirezione di output.\n");
				exit(EXIT_FAILURE);
			}

			// levo il maggiore
			memmove(&arguments[i][0], &arguments[i][1], strlen(arguments[i]));

			// attivo il flag e lo imposto a i per poi ricordarmi la posizione
			r_out = i;
		}
	}
}

// controlla che dopo il primo comando non ci siano redirezioni dell’input
void checkRedI(char ***argms)
{
	// scorro i comandi
	for(int i=1; i<sizeArrayArrayStrings(argms); i++)
	{
		// scorro gli argomenti dei comandi
		for(int j=0; j<sizeArrayStrings(argms[i]); j++)
		{
			if(argms[i][j][0] == '<') 
			{
				printf("Solo il primo comando pu`o avere la redirezione dell’input.\n");
				exit(EXIT_FAILURE);
			}
		}
	}
}

// controlla che dopo il primo comando non ci siano redirezioni dell’input
void checkCommands(char **commands)
{
	for(int i=0; i<sizeArrayStrings(commands); i++)
	{
		if(strcmp(commands[i], " ") == 0 )
		{
			printf("Comando ”vuoto” (doppia | senza niente in mezzo).\n");
			exit(EXIT_FAILURE);
		}
	}
}

// esegue in caso di un solo comando senza pipe
void exec_frombin(char **argms)
{
	pid_t PID = fork();
			
	//figlio
	if(PID == 0)
	{
		// gestisco la redirezione eventuale di I/0, in base ai flags		
		handle_red_IO(argms);

		execvp(argms[0],argms);
		perror("Error");
	}

	else // padre
	{	
		wait(NULL);
	}
}

// gestisce le pipe
void exec_frombin_pipe(char*** all_argms)
{
	int n_pipe = sizeArrayArrayStrings(all_argms);
	int status;

	// apro le pipe 2 alla volta (2 per ogni comando), mettendole tutte nel solo array fds, ordinate 2 per comando
	int fds[2*n_pipe];

	for(int i=0; i<n_pipe; i++)
	{
		if(pipe(fds + i*2) < 0) // e controllo se la loro apertura va a buon fine
		{	
			printf("pipe Fallita. \n");
			exit(EXIT_FAILURE);
		}
	}

	// scorro tra le varie pipe
	for(int p_cnt=0; p_cnt<n_pipe; p_cnt++) 
	{
		pid_t pid = fork();
		
		if(pid == -1)
			printf("Errore nella fork: %d\n", errno);

		else if(pid == 0) // figlio
		{
			checkArgs(all_argms[p_cnt]);

			// gestisco la redirezione eventuale di I/0, in base ai flags
			handle_red_IO(all_argms[p_cnt]);

			// redirigo l'output sulla pipe, a meno che non sia all'ultimo comando
			if(p_cnt != n_pipe-1)
			{
				if(dup2(fds[p_cnt*2+1], 1) < 0) // il file descriptor 1 della pipe numero p_cnt
				{
					printf("dup2 Fallita. \n");
					exit(EXIT_FAILURE);
				}
			}

			// redirigo l'input sulla pipe, a meno che non sia al primo comando
			if(p_cnt != 0)
			{
				if(dup2(fds[(p_cnt-1)*2], 0) < 0) // il file descriptor 0 della pipe numero p_cnt-1
				{
					printf("dup2 Fallita. \n");
					exit(EXIT_FAILURE);
				}
			}

			// chiudo tutte le pipe
			for(int i=0; i<2*n_pipe; i++) 
			{
				    close(fds[i]);
			}

			execvp(all_argms[p_cnt][0],all_argms[p_cnt]);
			perror("Error");
		} 
	}

	// Il padre chiude le pipe e aspetta i figli
	for(int i=0; i<2*n_pipe; i++)
		close(fds[i]);

	for(int i=0; i<n_pipe+1; i++)
		wait(&status);
}


// separa una stringa in un array a seconda del carattere divisorio che gli passo
char** separateStrings(char * fn, char op)
{
	// faccio un piccolo magheggio per convertire-fsanitize=address -static-libasan -g il tipo di op
	char oper[1]; 
	oper[0] = op;
	char *operator = oper;

	// mi creo un buffer di appoggio
	char *str1 = malloc(strlen(fn)+1);
	checkAlloc((void*)str1);

	// copio fn dentro l'array di appoggio
	strcpy(str1, fn);

	char ** res  = NULL;
	char * p = strtok(str1, operator);
	int n_spaces = 0;

	// metto i token in res
	while(p) 
	{
 		res = realloc(res, sizeof (char*) * ++n_spaces);
		checkAlloc((void*)res);

  		res[n_spaces-1] = p;

  		p = strtok(NULL, operator);
	}

	// alloco spazio per il terminatore
	res = realloc (res, sizeof (char*) * (n_spaces+1));
	checkAlloc((void*)res);

	res[n_spaces] = NULL;

	return res;
}

// crea un array tridimensionale che contiene tutta la linea di comando, strutturata però in array
char*** create_cmd_line(char * user_input, char ** commands)
{
	char*** cmd_line = malloc(sizeof(char***));
	checkAlloc((void*)cmd_line);

	int i = 0;

	// per ogni comando
	for(i=0; i<sizeArrayStrings(commands); i++)
	{
		char* cmnd = commands[i];

		// lo divido in comando e argomenti
		cmd_line = realloc(cmd_line, sizeof(cmd_line)+strlen(user_input));
		checkAlloc((void*)cmd_line);

		cmd_line[i] = separateStrings(cmnd, SEP_WORDS);

		// evito dangling pointer
		cmnd = (char *)NULL;
	}

	// inserisco un terminatore che poi posso riconoscere per accedere alla size dell'array
	cmd_line = realloc(cmd_line, sizeof (char*) * (i+1));
	checkAlloc((void*)cmd_line);

	cmd_line[sizeArrayStrings(commands)] = NULL;

	return cmd_line;
}

// esegue tutti i tipi di comandi
void execute_cmd_line(char * user_input, char ** commands, char *** cmd_line)
{
	if(sizeArrayStrings(commands) == 1) // comando singolo
	{
		checkArgs(cmd_line[0]); // espansioni e redirezioni

		if(strcmp(cmd_line[0][0], "cd") == 0 ) 	// se è built-in (solo cd)
		{	
			if(r_in != 0 || r_out != 0) // verifico che cd non abbia redirezioni
			{
				printf("cd non può contenere redirezioni.\n");
				return;
			}		

			if(sizeArrayStrings(cmd_line[0]) > 2) // verifico che cd abbia un solo argomento
			{
				printf("cd non può avere piú di un argomento.\n");
				return;
			}		

			exec_builtin(cmd_line[0][1]); // se tutto va bene eseguo cd
		}

		else // se non è built-in
		{
			exec_frombin(cmd_line[0]);
		}
	}

	else // se c'è una pipe 
	{
		// controlla che dopo il primo comando non ci siano redirezioni dell’input
		checkRedI(cmd_line);

		// faccio un controllo per vedere se c'è cd in una pipe
		for(int i=0; i<sizeArrayStrings(commands); i++)
		{
			if(strcmp(cmd_line[i][0], "cd") == 0 )
			{
				printf("Non puoi eseguire cd in una pipe.\n");
				exit(EXIT_FAILURE);
			}
		}

		exec_frombin_pipe(cmd_line); // esegue le pipe 
	}	
}

int main()
{
	for(;;)
	{
		// stampo il prompt e prendo il comando mettendolo nel buffer
		char *user_input = print_prompt();   

		// se l’utente inserisce exit o EOF (premendo ctrl-D all’inizio di una nuova linea),
		if(user_input == NULL)
		{
			// esce con exit status EXIT_SUCCESS
			printf("\n");
			exit(EXIT_SUCCESS);
		}

		// divido la stringa in comandi (divisi dalla pipe)
		char** commands = separateStrings(user_input, SEP_PIPE);
	
		// controllo che dopo il primo comando non ci siano redirezioni dell’input
		checkCommands(commands);

		// creo un array tridimensionale che contiene tutta la linea di comando, strutturata però in array
		char*** cmd_line = create_cmd_line(user_input, commands);
	
		// eseguo tutti i tipi di comandi
		execute_cmd_line(user_input, commands, cmd_line);

		// reimposto flag per verificare che non ci siano piú redirezioni
		r_in = 0;
		r_out = 0;

		// libero user_input, commands e cmd_line	
		free(user_input);			
		free(commands);
		free(cmd_line);
	}

}