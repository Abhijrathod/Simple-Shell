#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

#define MAX_LINE 80
#define MAX_ARGS (MAX_LINE/2 + 1)
#define REDIRECT_OUT_OP '>'
#define REDIRECT_IN_OP '<'
#define PIPE_OP '|'
#define BG_OP '&'

/* Holds a single command. */
typedef struct Cmd {
	/* The command as input by the user. */
	char line[MAX_LINE + 1];
	/* The command as null terminated tokens. */
	char tokenLine[MAX_LINE + 1];
	/* Pointers to each argument in tokenLine, non-arguments are NULL. */
	char* args[MAX_ARGS];
	/* Pointers to each symbol in tokenLine, non-symbols are NULL. */
	char* symbols[MAX_ARGS];
	/* The process id of the executing command. */
	pid_t pid;

	/* These variables help keep track of whether a process is stopped, printed or, done. If It's not stopped, it's assumed to be running */
	int done;
	int printed;
	int stopped;
} Cmd;

Cmd** jobs; //global list of jobs 

/* The process of the currently executing foreground command, or 0. */
Cmd* foreground_job;

/* stores PID of currently running foreground porcess*/
pid_t foregroundPid = 0;

/* keeps track of the number of jobs  */ 
int global_job_num = 0; 


/* Parses the command string contained in cmd->line.
 * * Assumes all fields in cmd (except cmd->line) are initailized to zero.
 * * On return, all fields of cmd are appropriatly populated. */
void parseCmd(Cmd* cmd) {
	char* token;
	int i=0;
	strcpy(cmd->tokenLine, cmd->line);
	strtok(cmd->tokenLine, "\n");
	token = strtok(cmd->tokenLine, " ");
	while (token != NULL) {
		if (*token == '\n') {
			cmd->args[i] = NULL;
		} else if (*token == REDIRECT_OUT_OP || *token == REDIRECT_IN_OP
				|| *token == PIPE_OP || *token == BG_OP) {
			cmd->symbols[i] = token;
			cmd->args[i] = NULL;
		} else {
			cmd->args[i] = token;
		}
		token = strtok(NULL, " ");
		i++;
	}
	cmd->args[i] = NULL;
}

/* Finds the index of the first occurance of symbol in cmd->symbols.
 * * Returns -1 if not found. */
int findSymbol(Cmd* cmd, char symbol) {
	for (int i = 0; i < MAX_ARGS; i++) {
		if (cmd->symbols[i] && *cmd->symbols[i] == symbol) {
			return i;
		}
	}
	return -1;
}

/* Signal handler for SIGTSTP (SIGnal - Terminal SToP),
 * which is caused by the user pressing control+z. */
void sigtstpHandler(int sig_num) {
	/* Reset handler to catch next SIGTSTP. */
	signal(SIGTSTP, sigtstpHandler);
	if (foregroundPid > 0) {
		/* Foward SIGTSTP to the currently running foreground process. */
		kill(foregroundPid, SIGTSTP);
		/* TODO: Add foreground command to the list of jobs. */
		jobs[global_job_num] = foreground_job;
		jobs[global_job_num]->stopped = 1; 
		global_job_num++;
	}
}

/* this method handles redirection and piping, as well as all regular command execution*/
void executeCommand(Cmd *cmd){
	//child process calls execvp() to run cmd
	//checks for redirects first
	
	// <

	if(findSymbol(cmd, REDIRECT_IN_OP) != -1){
		int in = findSymbol(cmd, REDIRECT_IN_OP);
		int fdIn = open(cmd->args[in+1], O_RDONLY); //gets file to read from 
		dup2(fdIn, 0); //sets file as stdin

	}

	// >
	if(findSymbol(cmd, REDIRECT_OUT_OP) != -1){
		int out = findSymbol(cmd, REDIRECT_OUT_OP);
		int fdOut = open(cmd->args[out+1], O_WRONLY|O_CREAT); //gets file to write to
		dup2(fdOut, fileno(stdout)); //sets file as stdout 
	}

	//then checks for pipes
	if(findSymbol(cmd, PIPE_OP) != -1){
		int pipePos = findSymbol(cmd, PIPE_OP); //ls | grep txt >> pipePos = 1
		int num = pipePos + 1;
		while(cmd->args[num] != NULL){num++;}
		num = num - pipePos - 1; // ls | grep txt >> num = 2
		
		//populating argsLeft
		char* argsLeft[pipePos * sizeof(char*)];
		for(int i = 0; i < pipePos; i++){argsLeft[i] = cmd->args[i];}

		//populating argsRight
		char* argsRight[num * sizeof(char*)];
		int parser = pipePos + 1;
		for(int i = 0; i < num; i++){
			argsRight[i] = cmd->args[parser];
			parser++;
		}

		//making pipes
		int fd[2];
		pipe(fd);
		pid_t pid = fork();
		if(pid < 0) return;
		if(pid == 0){ //child
			dup2(fd[0], 0);
			close(fd[1]);
			execvp(argsRight[0], argsRight); //does not return 
			perror("child exe failed\n");
			exit(1); //error if returned 
		}
		else { //parent
			dup2(fd[1], 1);
			close(fd[0]);
			execvp(argsLeft[0], argsLeft); //does not return 
			perror("parent exe failed\n");
			exit(1); //error if returned 
		}
		return; //return here because we don't want to execute it twice
	}

	//Execute the command
	execvp(cmd->args[0], cmd->args);
	
	
	
}

/* runs command in background */
int backgroundCmd(Cmd *cmd){
	pid_t forkPID = fork();
	if(forkPID == 0){
		executeCommand(cmd); 
		exit(0);
	}
	else{
		jobs[global_job_num] = cmd;
		jobs[global_job_num]->pid = forkPID;
		jobs[global_job_num]->done = 0;
		jobs[global_job_num]->printed = 0;
		jobs[global_job_num]->stopped = 0;
		printf("[%d]	%d\n", global_job_num, forkPID);
		global_job_num++; 

	}
	return 0;
}

/* runs command in foreground */
int foreGroundCmd(Cmd *cmd){ 
	pid_t forkPID = fork();
	if(forkPID >= 0){  //fork succesful 
		if(forkPID == 0){ //child process
			executeCommand(cmd); //execvp(cmd->args[0], cmd->args);
			exit(0); 
		}
		
		else //parent process, should wait for child 
		{
			foregroundPid = forkPID; 
			foreground_job = (Cmd*) malloc(1 * sizeof(Cmd));
			foreground_job = cmd;
			waitpid(forkPID, NULL, WUNTRACED);
		}
	}
	else{ //fork failed 
		printf("fork failed");
	}
	return 0; 
}

/* This method checks the status of jobs*/
int processCheck(int exitCode){ 
	for (int i = 0; i < global_job_num; i++){
		int status; 
		int amp = findSymbol(jobs[i], BG_OP);
		pid_t checker = waitpid(jobs[i]->pid, &status, WNOHANG);

		if(checker > 0){ //this specific job is terminated
			if(WIFEXITED(status)){
				status = WEXITSTATUS(status);
				jobs[i]->done = 1;
			}
			else if(WIFSIGNALED(status)){
				status = WTERMSIG(status);
				jobs[i]->done = 1;
				printf("[%d]\tTerminated\t", i);
				int n = 0; 
				if(jobs[i]->printed == 0){
					while(1){
						if(amp == n){break;}
						if(jobs[i]->args[n] == NULL && jobs[i]->symbols[n] == NULL){break;}
                        else if(jobs[i]->args[n] == NULL){printf("%s ", jobs[i]->symbols[n]);}
                        else{ printf("%s ", jobs[i]->args[n]);}
                        n++;
                    }
					jobs[i]->printed = 1;
					printf("\n");
				}
				return 0;
			}

			if(status != 0){printf("[%d]\tExit\t%d ", i, status);}
			else {printf("[%d]\tDone\t\t", i); }
			//loop to print args
			int n = 0;
			if(jobs[i]->printed == 0){
				while(1){
					if(amp == n){break;}
					if(jobs[i]->args[n] == NULL && jobs[i]->symbols[n] == NULL){break;}
					else if(jobs[i]->args[n] == NULL){printf("%s ", jobs[i]->symbols[n]);}
					else{ printf("%s ", jobs[i]->args[n]);}
					n++;
				}
				jobs[i]->printed = 1;
				printf("\n");
			}
		}




	}
}

int main(void) {
	/* Listen for control+z (suspend process). */
	signal(SIGTSTP, sigtstpHandler);
	jobs = malloc(sizeof(Cmd)*100); //limits number of jobs to 100 at any given time 
	int bg = 0;
	while (1) {
		printf("352> ");
		fflush(stdout);
		Cmd *cmd = (Cmd*) calloc(1, sizeof(Cmd));
		fgets(cmd->line, MAX_LINE, stdin);
		parseCmd(cmd);

		int processcheck = processCheck(bg);

		if (!cmd->args[0]) {
			free(cmd);
		} else if (strcmp(cmd->args[0], "exit") == 0) {
			free(cmd);
			exit(0);
		}
		else if(strcmp(cmd->args[0], "jobs") == 0){ //displays a list of all current jobs
			for(int i = 0; i < global_job_num; i++){
				if(jobs[i]->done == 0 && jobs[i]->printed == 0 && jobs[i]->stopped != 1){printf("[%d]\tRunning\t\t%s", i, jobs[i]->line);}
				else if(jobs[i]->done == 1 && jobs[i]->printed == 0 && jobs[i]->stopped != 1){
					printf("[%d]\tDone\t\t%s", i, jobs[i]->line);
					jobs[i]->printed = 1;
				}
				else if(jobs[i]->stopped == 1){printf("[%d]\tStopped\t\t%s", i, jobs[i]->line);}
			}
		}
		
		else if(strcmp(cmd->args[0], "bg") == 0){ //puts it back into running from stopped state
			int ind = atoi(cmd->args[1]);
			int jobPid = jobs[ind]->pid;
			kill(jobPid, SIGCONT);
			jobs[ind]->stopped = 0;
		}
		else {
			
			if (findSymbol(cmd, BG_OP) != -1) {	 //bacground symbol &
				bg = backgroundCmd(cmd); 
			} 
			else {
				int fg = foreGroundCmd(cmd); //put it in the foreground 
			}
		}

		
	
	}
	
	//frees jobs
	for(int i = 0; i < global_job_num; i++){
		free(jobs[i]);
	}
	free(jobs);

	return 0;
}
