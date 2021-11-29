#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

//this global var allows all processes to check if or not in background
int foreG=0;	

struct cmd{		 
		char *cmnd;
		char *argc[513];	//max of 512 arguments, 513 is set to include null character
		char *in;
		char *out;
		int foreG;
};


void expPID(char* str){
	int pid=getpid();
	char pidS[6];
	sprintf(pidS, "%d", pid);

	//if str is $$
	if(strlen(str)==2){
		free(str);
		str=malloc((strlen(pidS)+1) * sizeof(char));
		memset(str, '\0', sizeof(str));
		strcpy(str, pidS);
	}
	//if $$ is in another
	else{	
		char* sHalf=strstr(str, "$$");
		sHalf+=2;
		char* fHalf=strtok(str, "$$");
		free(str);

		int newlen=strlen(pidS)+strlen(str)-1;
		str= malloc(newlen * sizeof(char));
		memset(str, '\0', sizeof(str));

	
		char temp[strlen(sHalf)];
		memset(temp, '\0', sizeof(temp));
		strcpy(temp, sHalf);
		strcat(fHalf, pidS);
		strcpy(str, strcat(fHalf,sHalf));
	}
}

struct cmd *readCmd(char *currLine){
	
	struct cmd *c1= malloc(sizeof(struct cmd));     //allocate space for input
	char *save;						//char pointer for strtok_r
	int idx = 0;						

	char* tok= strtok_r(currLine, " \n",&save);		
	c1->cmnd= calloc(strlen(tok)+1, sizeof(char));	//allocate space for element
	strcpy(c1->cmnd, tok);				

	//while token
	while(tok){
	
		//input
		if(strcmp(tok, "<")== 0){				
			tok= strtok_r(NULL, " \n",&save);	
			c1->in= calloc(strlen(tok) + 1, sizeof(char));	
			strcpy(c1->in, tok);	
        	
			tok= strtok_r(NULL, " \n",&save);	//go to next word 
		}
		//output
		else if(strcmp(tok, ">")==0){		
			tok= strtok_r(NULL, " \n",&save);		
			c1->out= calloc(strlen(tok) + 1, sizeof(char));	
			strcpy(c1->out, tok);	

			tok= strtok_r(NULL, " \n",&save);	//go to next word
         }

		//background mode 
		else if((strcmp(tok, "&")== 0) && (foreG == 0)){ 
			//set background to true if so
			c1->foreG= 1;		
			tok= strtok_r(NULL, "\n",&save);		//go to next word
		}
		
		//rest is args	
		else{
			
			//f $$ is in string expand until no $$
	//		if(strstr(tok, "$$")==0)
	//			expPID(tok);	
					
			c1->argc[idx] = tok;		
			idx++;					
			tok= strtok_r(NULL, " \n",&save);		//go to next word
		}
    }

    return c1;						
}


void changeD(char** commandArray){
	char* directoryName= commandArray[1];	
	
	if(directoryName== NULL)		
		chdir(getenv("HOME"));
	else if(chdir(directoryName)== -1)	
		perror("cd");			//return error statement
}

void printStatus(int childexitmethod){				//based off lecture notes, provided childexitmethod of process	
	if(WIFEXITED(childexitmethod) != 0){			
		int exitStatus= WEXITSTATUS(childexitmethod);
		printf("exit value was %d\n", exitStatus);	//print out exit status
		fflush(stdout);
	}
	else if(WIFSIGNALED(childexitmethod) != 0){		
		int termSignal = WTERMSIG(childexitmethod);
		printf("terminating signal was %d\n", termSignal);	
		fflush(stdout); 
	}

}


void catchSIGTSTP(int signo){
	if(foreG==1){						//if global variable foreG mode is true
		char* message = ("Exiting foreground only mode\n");	//write message
		write(STDOUT_FILENO, message, 28);
		foreG= 0;					//set it to false

	}
	else if(!foreG==0){					//if global variable foreG mode is false
		char* message = ("Entering foreground only mode (& is now ignored)\n");	//write message
		write(STDOUT_FILENO, message,49);
		foreG= 1;					//set it to true
	}
}



int main(){
	char* buffer= malloc(2048);		//maximum of 2048 characters in user input
	size_t len= 0;				

	//check file i/o
	int fin;				
	int fout;				
	int status= 0;				
			 
	pid_t spawnpid= -5;	
	int pidC= 0;			
	pid_t procPids[30];			

	struct sigaction SIGINT_action= {0};	//sigaction struct to handle (Control c) press
	struct sigaction SIGTSTP_action= {0};	//sigaction struct to handle (control z) press


	SIGINT_action.sa_handler= SIG_IGN;	//ignore control c signal handler
	sigfillset(&SIGINT_action.sa_mask);	//block other signals
	SIGINT_action.sa_flags= SA_RESTART;	//configures system calls to restart if interrupted
	sigaction(SIGINT,&SIGINT_action, NULL);	//call the sigint_action struct if sigint is detected

	
	SIGTSTP_action.sa_handler= catchSIGTSTP;       //call signal handler function if control z is pressed
	sigfillset(&SIGTSTP_action.sa_mask);	       //block other signals
	SIGTSTP_action.sa_flags= SA_RESTART;	      //configures system calls to restart if needed
	sigaction(SIGTSTP, &SIGTSTP_action,NULL);    //calls the sigtstp_action struct if sigtstp is detected


	while(1){
		printf(": ");
		fflush(stdout); 
 		getline(&buffer, &len, stdin);	//getline to get user input


		while(buffer[0]=='\n'){			//if user inputs nothing, ignore and reprompt
			printf(": ");
			fflush(stdout);
			getline(&buffer,&len,stdin);
		}

			
		struct cmd *input1= readCmd(buffer);	

		if(strncmp(input1->cmnd,"#",1)==0){}
			
		if(strcmp(input1->cmnd, "exit")==0){	
			for (int i=0; i<pidC; i++)
				kill(procPids[i],SIGKILL);	
			exit(0);
		}
		else if(strcmp(input1->cmnd, "cd")==0){	
			changeD(input1->argc);	

		}
		else if (strcmp(input1->cmnd, "status")==0){	
			printStatus(status);		
		
		}
		else{
	
			spawnpid = fork();				//create child process using fork
			switch(spawnpid)
			{
				case -1://if there was an error forking the process
					perror("Error Forking!");	
					status = 1;
					exit(1);
					break;

				case 0:	//child process
					SIGTSTP_action.sa_handler= SIG_IGN;		//signal handler for sigtstp, which is control Z. Ignore in child processes
       				sigfillset(&SIGTSTP_action.sa_mask);		
       				SIGTSTP_action.sa_flags= SA_RESTART;		
        			sigaction(SIGTSTP,&SIGTSTP_action, NULL);		 

					if (input1->foreG==0){			
						SIGINT_action.sa_handler= SIG_DFL;	
						sigaction(SIGINT, &SIGINT_action, NULL);				
					}
					else if (input1->foreG==1){			//if this is a background process do the following
						
						if (input1->in==NULL){				
							fin = open("/dev/null",O_RDONLY);		
                            if(fin==-1){				
								printf("Failed to use /dev/null for input\n");	//print error statement
								fflush(stdout);					//fflush stdout
              					exit(1);
                            }
							int redirSTDIN= dup2(fin,0);
                                                	
							if(redirSTDIN==-1){						
								printf("Failed input redirection\n");	//print error message
                          		fflush(stdout);
								exit(1);
							}
							close(fin);				
						}
						if(input1->out==NULL){			
							fout= open("/dev/null",O_WRONLY);
							if (fout==-1){
								printf("Failed to use /dev/null for output\n");
								fflush(stdout);
								exit(1);
							}
							int redirSTDOUT= dup2(fout,1);	//second parameter is changed from 0 to 1, as 0 is stdin and 1 is stdout
							
							if (redirSTDOUT== -1){
								printf("Failed output redirection\n");
								fflush(stdout);
								exit(1);
							}
							close(fout);
						}
					}
				
					if(input1->in != NULL){					//if inputfile is provided, open it using the file descriptor
						fin= open(input1->in,O_RDONLY);		
						if(fin== -1){					//if it failed print error message
							printf("Failed to open input file %s\n",input1->in);
							fflush(stdout);
							exit(1);
						}

						int redirectSTDIN= dup2(fin,0);			
						
						if(redirectSTDIN== -1){
							printf("Failed input redirection with file: %s\n",input1->in);
							fflush(stdout);
							exit(1);
						}
						close(fin);					
					}

					if(input1->out != NULL){							
						fout= open(input1->out, O_WRONLY|O_CREAT|O_TRUNC,0644);	
				
						if(fout== - 1){
							printf("Failed to open output file %s\n",input1->out);
							fflush(stdout);
							exit(1);
						}

						int redirectSTDOUT= dup2(fout,1);
						
						if(redirectSTDOUT== -1){
								printf("Failed output redirection with file: %s\n",input1->out);
								fflush(stdout);
								exit(1);
						}
					close(fout);			
					}
					
					int exec= execvp(input1->argc[0],input1->argc);	//call exec to execute non built in command

					if ((exec < 0) && (strncmp(input1->cmnd,"#",1) != 0)) { 
						printf("unknown command %s cannot be executed\n",input1->argc[0]);		//print error message
						fflush(stdout);							
						exit(1);
					}
					exit(1);
					break;
	
				default:
					if(input1->foreG== 0){	
						spawnpid= waitpid(spawnpid, &status, 0);
					}
					else if(input1->foreG== 1){	//if this is a background process, print out the pid of the child process
						printf("Background PID is: %d\n", spawnpid);	
						fflush(stdout);

						procPids[pidC]= spawnpid;	
						pidC= pidC + 1;						
					}
					break;
			}

		}			
			spawnpid= waitpid(-1,&status,WNOHANG); //taken from lecture.
				while (spawnpid > 0){		
					printf("Background PID is: %d\n", spawnpid);	
					fflush(stdout);
					printStatus(status);				
					spawnpid= waitpid(-1,&status,WNOHANG);		//check for other processes
				}
  	}

  	return 0;
}

