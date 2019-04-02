
/**
 * Simple shell interface program.
 *
 * Operating System Concepts - Tenth Edition
 * Author: Michael Dmitry
*/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#define MAX_LINE		80 /* 80 chars per line, per command */
#define BUFFER_SIZE 		50

char history[10][BUFFER_SIZE]; //Assuming the history holds only the last 10
int count;	//count for thr number of command sin history
int redirection; //1 -> standard child process  2-> output/input redirection  3->piping
char *FileRed[BUFFER_SIZE]; //for the redirection I/O, saves file names
int in=0; //check if input redirection
int out=0;
int should_run;

int process(char inputBuffer[], char *args[],char *args2[], int *flag){
	int length,i,start,ct,ct2;
	redirection=1;
	ct=0; //index for args
	ct2=0; //index for args2
	int fileCounter=0;
	
	length=read(STDIN_FILENO,inputBuffer,MAX_LINE);
	
	//Check if history feature is needed
	if(inputBuffer[0]=='!'&& inputBuffer[1]=='!'){
		if(count<=0){
			printf("No commands in history\n");
			count=-1;
			inputBuffer="";
		}
		else{
			strcpy(inputBuffer,history[0]);
			printf("%s\n",inputBuffer);
		}
	}
	
	//empty space for new command to enter the history 
	for(i=9;i>=0;i--)
		strcpy(history[i],history[i-1]);
	
	//copy the entered cmmand to the history array
	strcpy(history[0],inputBuffer);

	count++;
	
	if(count>10)
		count=10;

	//Parsing command and tekonizing arguments
	length=strlen(inputBuffer);
	start=-1;

	if(length==0)
		return 0;

	//Go through each character in the command
	for(i=0;i<length;i++){
		switch(inputBuffer[i]){
			case ' ':
			case '\t':
				//if its a piping command, process right hand side of pipe
				if(start!=-1 && redirection ==3){
					args2[ct2]=&inputBuffer[start];
					ct2++;
				}
				// if its a redirection command
				else if(start!=-1 && redirection==2){
					FileRed[fileCounter]=&inputBuffer[start];
					fileCounter++;
				}
				else if(start!=-1){
					args[ct]=&inputBuffer[start];
					ct++;
				}
				inputBuffer[i]='\0'; //null terminate each word
				start=-1;
				break;
			case '\n':
				if(start!=-1 && redirection==3){
					args2[ct2]=&inputBuffer[start];
					ct2++;
				}
				else if(start!=-1 && redirection==2){
					FileRed[fileCounter]=&inputBuffer[start];
					fileCounter++;	
				}
				else if(start!=-1){
					args[ct]=&inputBuffer[start];
					ct++;	
				}
				inputBuffer[i]='\0';
				args[ct]=NULL;
				break;
			default:
				
				if((redirection==2 || redirection==3) &&start==-1){
					start=i;
				}
				if(inputBuffer[i]=='|'){
					redirection=3;
				}
				else if(inputBuffer[i]=='>' || inputBuffer[i]=='<'){
					redirection=2;
					if(inputBuffer[i]=='>')
						out=1;
					else
						in=1;
				}
				else if(start==-1)
					start=i;
				else if(inputBuffer[i]=='&'){
					*flag=1;
					inputBuffer[i-1]='\0'; //don't take & as an argument
				}
		}


	}
	args[ct]=NULL;
	args2[ct2]=NULL;

	//handle 'exit' command
	if(args[0][0]=='e' && args[0][1]=='x' && args[0][2]=='i' && args[0][3]=='t'){
		should_run=0;
		args[0]="";
		return 0;
	}
	return redirection;
}


int main(void)
{
	char inputBuffer[MAX_LINE];
	int flag;
	char *args2[MAX_LINE/2 +1];
	char *args[MAX_LINE/2 + 1];	/* command line (of 80) has max of 40 arguments */
    	should_run = 1;
	pid_t pid;
	int i;
	int redirect;
	int newfd_out,newfd_in;
	int fd[2];
	count=0;
		
    	while (should_run){
		flag=0;   
        	printf("osh>");
        	fflush(stdout);
        
		//read the command and process it
        	redirect=process(inputBuffer,args,args2,&flag);
		
		//If it is a redirection command
		if(redirect==2){

			//if it is a '>'
			if(in==1){
				in=0;
				newfd_in=open(FileRed[0],O_RDONLY);
				dup2(newfd_in,STDIN_FILENO);
				close(newfd_in);
			}
			//if its a '<'
			else if(out==1){
				out=0;
				//if there isn't such file, create one
				newfd_out=open(FileRed[0],O_WRONLY | O_TRUNC | O_CREAT, 0644);
				dup2(newfd_out,STDOUT_FILENO);
				close(newfd_out);
		
			}
			
			execvp(args[0],args);
			
		}
		//Then it is a piping command
		else if(redirect==3){

			if(pipe(fd)<0)
				printf("pipe failed\n");

			pid_t pid1=fork(); //Fork 1st child
			
			if(pid1==-1)
				printf("fork failed");

			else if(pid1==0){ //child to run first command
				dup2(fd[1],STDOUT_FILENO);
				close(fd[0]);
				close(fd[1]);

				if(execvp(args[0],args)==-1)
					printf("Error executing the command\n");
			}
			else{  //else it is the parent
				pid_t pid2=fork(); //fork 2nd child
				
				if(pid2==-1)
					printf("fork failed");

				else if(pid2==0){ //second child to run 2nd command
					dup2(fd[0],STDIN_FILENO);
					close(fd[1]);
					close(fd[0]);

					if(execvp(args2[0],args2)==-1)
						printf("Error executing the command\n");
				}

			}
		}
		//Then it is a normal command
		else if(redirect==1){

			pid=fork(); //fork child

			if(pid<0){
				printf("fork failed.\n");
			}

			else if(pid==0){

				if(execvp(args[0],args)==-1){
					printf("Error executing the command\n");
				}
			}
			else{
				if(flag==0){
					wait(NULL);
				}
			}
		}

    	}
    
	return 0;
}
