//head file
#include <stdio.h>//feof();
#include <stdlib.h>//exit()
#include <unistd.h>//fork();getpid();close();execvp()
#include <string.h>//strlen();strcmp();
#include <sys/types.h>//getuid()
#include <pwd.h>//getpwuid();

//define
#define MAX_PROMPT 512
#define MAX_NAME 50
#define MAX_PATH 512
#define MAX_CMD 513//512+'\n'
#define MAX_ARG 20

#define DEBUG

char *myshBuffer;

char error_message[30] = "An error has occurred\n";

void type_prompt(char* prompt) {
	struct passwd *pwd;
	char userName[MAX_NAME],hostName[MAX_NAME],pathName[MAX_PATH];
	int length;

	pwd = getpwuid(getuid());
	getcwd(pathName, MAX_PATH);//get path

	//print hostName and userName 
    if(gethostname(hostName, MAX_NAME) == 0)
        length = sprintf(prompt, "%s@%s:",pwd->pw_name,hostName);
    else
        length = sprintf(prompt, "%s@unknown:",pwd->pw_name);
    
    //printf("pathname: %s,length:%d\npw_dir:%s,length:%d\n",pathName,(int)strlen(pathName),pwd->pw_dir,(int)strlen(pwd->pw_dir));
    
    if(strlen(pathName) < strlen(pwd->pw_dir) || strncmp(pathName, pwd->pw_dir, strlen(pwd->pw_dir)) != 0)
        length += sprintf(prompt+length, "%s", pathName);
    else
        length += sprintf(prompt+length, "~%s", pathName + strlen(pwd->pw_dir));

    length += sprintf(prompt+length, " mysh> ");
    //puts(prompt);
    return;
}



int read_command(char **command,char **parameters,char *prompt) {
	printf("%s",prompt);
    
    char* cmd = fgets(myshBuffer,MAX_CMD,stdin);
    
    //printf("\ncmd_len:%d\n",(int)strlen(cmd));
    
    if((int)strlen(cmd) == 512 && feof(stdin) == 0) {
    	scanf("%*[^\n]%*c");
    	write(STDERR_FILENO, error_message, strlen(error_message));   	
    	return -1;
    }
    
    //if (cmd == NULL) {
      //  printf("\n");
        //exit(0);
    //}

    if(myshBuffer[0] == '\0')
        return -1;

    char *cmdHead,*cmdTail;
    int count = 0;
    int isFinished = 0;
    
    cmdHead = cmdTail = myshBuffer;//point to command's head
    
    while(isFinished == 0) {
    	//deal with command's head
        while((*cmdTail == ' ' && *cmdHead == ' ') || (*cmdTail == '\t' && *cmdHead == '\t')) {
        	//leapfrog space and tab at command's head
            cmdHead++;
            cmdTail++;
        }

        if(*cmdTail == '\0' || *cmdTail == '\n') {
            if(count == 0)//have a line break at command's head,exit
                return -1;
            break;
        }

        //move the cmdTail
        while(*cmdTail != ' ' && *cmdTail != '\0' && *cmdTail != '\n')
            cmdTail++;//pointers select a word without space

        if(count == 0) { //to handle cd
            char *p = cmdTail;
            *command = cmdHead;
            while(p != cmdHead && *p !='/')
                p--;
            if(*p == '/')
                p++;//ignore '/''
            parameters[0] = p;
            count += 2;

            #ifdef DEBU
            printf("\nHead:%s\nTail:%s\n",cmdHead,cmdTail);
        	printf("\ncount==%d\ncommand:%s\n",count,*command);
        	#endif
        }

        else if(count <= MAX_ARG) {
            parameters[count-1] = cmdHead;
            count++;
        }
        else
        {
            break;
        }

        //ready for next parameter
        if(*cmdTail == '\0' || *cmdTail == '\n')
        {
            *cmdTail = '\0';
            isFinished = 1;//to break
        }
        else {
            *cmdTail = '\0';
            cmdTail++;
			cmdHead = cmdTail;//to continue
        }

    }

    parameters[count-1] = NULL;

    #ifdef DEBUG
    printf("count:%d\n",count);
    printf("pathname:[%s]\ncommand:[%s]\nparameters:\n",*command,parameters[0]);
    int i;
    for(i = 0;i < count - 1; i++)
        printf("[%s]\n", parameters[i]);
    #endif
    if(strcmp(parameters[0],"exit") != 0)
	    return count;
	else
		return -2;
}

int main() {
	char prompt[MAX_PROMPT];
	char *command = NULL;
	char **parameters;
	int para_count = 0;

	myshBuffer = malloc(sizeof(char)*MAX_CMD);
	parameters = malloc(sizeof(char*)*MAX_ARG);
	
	while(1) {
		type_prompt(prompt);//generate prompt
		para_count = read_command(&command, parameters, prompt);//cmd input
		if(para_count == -1)//wrong input
			continue;
		if (para_count == -2)
			break;
		/*int rc = fork();
	    if (rc < 0) {
	    	//fork failed then exit
			fprintf(stderr, "fork failed\n");
	        exit(1);
	    }
		else if(rc != 0) {
			//parent
			waitpid(-1, &status, 0);//wait until child exit
		}
		else {
			//child
			execve(command, parameters, 0);//execute commad
	*/}

	free(parameters);
	free(myshBuffer);	
    return 0;
}
