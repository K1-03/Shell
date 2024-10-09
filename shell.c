#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include<sys/wait.h>
#include <fcntl.h>

#define BUFSIZE 256
#define MAX_TOKENS 128
#define NUM_OF_BUILT_INS 1 

int tokenize(char *command, char *tokens[], int max);
int builtIn(char *argv0);
int emptyLine(char *command);
int execute_cd(int argc, char *argv[]);
int noRedirection(char *command);
int containsPipe(char *command);
void execHelper(char *command);
char *ioRedirection(char *command, char *token);
void exec(char *command); 
void handlePiping(char *commands);

typedef struct {
	char *commandName;
	int (*function)(int argc, char *argv[]);
} BuiltIn;

BuiltIn builtInCommands[] = {
                {"cd", &execute_cd}
};

int
main(){ 
	char *command, *temp;
	char *n;
	char buf[BUFSIZE];	
	char *argv[MAX_TOKENS];
	int argc, i, j, l, length, hasPipe;
	pid_t pid;

	while(1){
		getcwd(buf, BUFSIZE);	
	        command = malloc(2097152);
		temp = malloc(2097152);		
		printf("%s$ ", buf);

		n = fgets(command, 2097152, stdin);	
		length = strlen(command);
		if (n == NULL){
			free(command);
		       	free(temp);	
			break;	
		}	       
		
		if (sscanf(command, "%*[^\n]%*1[\n]") == EOF){
			free(command);
		       	free(temp);	
			break;
		}
			
		if (!emptyLine(command)){
			strcpy(temp, command);	
			argc = tokenize(temp, argv, MAX_TOKENS);
			
			if ((j = builtIn(argv[0])) != -1){		
				(*builtInCommands[j].function)(argc, argv);	
			}	
			
			else  {	
			 	if (hasPipe = containsPipe(command)){
				       handlePiping(command);	
				}	
						
					
				else {	
					exec(command);	
				}
				free(command);	
				free(temp);	
			}		
		}

	}

}

int
tokenize(char *command, char *tokens[], int max){	
		int i;

		i = 1;
		tokens[0] = strtok(command, " \t\n");
		
		while(1){
			tokens[i] = strtok(NULL, " \t\n");	
			if (tokens[i] == NULL){
				return i;	
			}
		        
		 	++i;	
			if (i >= max){
				tokens[max - 1] = NULL;
				return max - 1;	
			}
		}

}

int execute_cd(int argc, char *argv[]){
	char buf[BUFSIZE];

	getcwd(buf, BUFSIZE);
	
	if(argc == 1){
		chdir(getenv("HOME"));
	} 
	
	if (argc == 2){
		if (chdir(argv[1]) < 0){
			perror(buf);
		}
	}
	else if (argc > 2){
		printf("Too many arguments for cd\n");
		return 1;
	}
}
int
builtIn(char *argv0){
	int i;
	
	for (i = 0; i < NUM_OF_BUILT_INS; ++i){
		if (strcmp(argv0, builtInCommands[i].commandName) == 0){
			return i;
		}
			
	};
        
	return -1;	
}
int emptyLine(char *command){
	int i;

	for (i = 0; i < strlen(command) - 1; ++i){
		if (!isspace(command[i])){
			return 0;
		}
	}

	return 1;
}

int noRedirection(char *command){
	return !(strchr(command, '<') || strchr(command, '>') || strstr(command, ">>"));
}

void execHelper(char *command){
	char *argv[MAX_TOKENS];	
	int argc;				 
	argc = tokenize(command, argv, MAX_TOKENS);
	execvp(argv[0], argv);
	perror("execvp");
	exit(1);
}

char *ioRedirection(char *command, char *operator){
	char *buffer, *buffer2, *argv[MAX_TOKENS];
	int p, j, i;
	int outfd, infd;
	int length, lengthOfOperator;

	length = strlen(command);
	lengthOfOperator = strlen(operator);
	buffer = malloc(length);
	buffer2 = malloc(length);
      
	for (p = 0; p != (strstr(command, operator) - command) ; ++p){
						buffer[p] = command[p];	
	}
         
	buffer[p] = '\0';
       
	for (i = 0, j = p+lengthOfOperator; j < length; ++j){
		if (!isspace(command[j])){
			buffer2[i] = command[j];
			++i;
		}
	}
       
	buffer2[i] = '\0';
      
       	if ((strcmp(operator, ">") == 0)){
		outfd = open(buffer2, O_CREAT | O_WRONLY | O_TRUNC, 0666);
		if (outfd == -1){
			perror("open");
			exit(1);
		}
		close(STDOUT_FILENO);
		if (dup2(outfd, STDOUT_FILENO) == -1){
			perror("dup2");
			exit(1);
		}
	}
	if ((strcmp(operator, ">>") == 0)){
                outfd = open(buffer2, O_CREAT | O_WRONLY | O_APPEND, 0666);
                if (outfd == -1){
                        perror("open");
                        exit(1);
                }
                close(STDOUT_FILENO);
                if (dup2(outfd, STDOUT_FILENO) == -1){
                        perror("dup2");
                        exit(1);
                }
        }

	if ((strcmp(operator, "<") == 0)){
		infd = open(buffer2, O_RDONLY , 0666);
		if (infd == -1){
			perror("open");
			exit(1);
		}
		close(STDIN_FILENO);
		if (dup2(infd, STDIN_FILENO) == -1){
			perror("dup2");
			exit(1);
		}	
	}	
	return buffer;
}

int containsPipe(char *command){
	return strchr(command, '|') != NULL; 
}

void handlePiping(char *command){
	char *buffer, *buffer2, *pipedCommands[MAX_TOKENS];	
	int pid, i, j, k, remainingCharacters, numberOfPipedCommands, listOfPipes[MAX_TOKENS-1][2];
	numberOfPipedCommands = 0;
	
	while(containsPipe(command)){
		buffer = malloc(strlen(command));
		buffer2 = malloc(strlen(command));
		i = 0;
		for (i; i != (strstr(command, "|") - command) && strlen(command); ++i){
			buffer[i] = command[i];
		}
		buffer[i] = '\0';
		pipedCommands[numberOfPipedCommands] = buffer;
		++numberOfPipedCommands;	
		remainingCharacters = strlen(command) - i;
		for (j = 0, i = i + 1; j < remainingCharacters; ++j, ++i){
			buffer2[j] = command[i];
		}	
		buffer2[j - 1] = '\0';
		if (!containsPipe(buffer2)){  //This will be the case at the end of the command	
			buffer2[j - 2] = '\0';
			pipedCommands[numberOfPipedCommands] = buffer2;
			++numberOfPipedCommands;
		}
		command = buffer2;   //Cuts down the command to exclude the first token delineated by a pipe	
	}
	for (i = 0; i < numberOfPipedCommands; ++i){
		
		if (pipe(listOfPipes[i]) == -1){
			perror("pipe");
			exit(1);
		}

		pid = fork();
		if (pid == 0){
			
			dup2(listOfPipes[i-1][0], STDIN_FILENO);
	
			if (i < numberOfPipedCommands - 1){
				dup2(listOfPipes[i][1], STDOUT_FILENO);
			}	
		
			for (j = 0; j < numberOfPipedCommands - 1; ++j){
				close(listOfPipes[j][0]);
				close(listOfPipes[j][1]);	
			}	
			exec(pipedCommands[i]);
			exit(0);	
		}
		
		close(listOfPipes[i-1][0]);
		close(listOfPipes[i-1][1]);	
	}	
	for (i = 0; i < numberOfPipedCommands; ++i){
		wait(NULL);
	}	
}

void exec(char *command){
	int pid;

	pid = fork();
	
	if (pid == 0){
		while(!noRedirection(command)){
			if (strstr(command, ">") != NULL && *(strchr(command, '>')+1) != '>'){
				command = ioRedirection(command, ">");
			}
       
			else if (strstr(command, ">>") != NULL){
				command = ioRedirection(command, ">>");
			}
			else if (strstr(command, "<") != NULL){
				command = ioRedirection(command, "<");
			}
		}
		execHelper(command);
	}	
	else{
		wait(NULL);
	}
}
