//head file
#include <stdio.h>
#include <stdlib.h>//exit()
#include <unistd.h>//fork();getpid();close();execvp()
#include <string.h>
#include <sys/types.h>//getuid()
#include <pwd.h>//getpwuid();

//define
#define MAX_PROMPT 512
#define MAX_NAME 50
#define MAX_PATH 1024
#define MAX_CMD 512

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
    puts(prompt);
    return;
}

//return value: number of parameters
//0 represents only command without any parameters
//-1 represents wrong input
// int read_command(char** command, char** parameters, char* prompt)
// {
// 	#ifdef READLINE_ON
//     	free(buffer);
//     buffer = readline(prompt);
//     if(feof(stdin)) {
//         printf("\n");
//         exit(0);
//     }

// 	#else
// 	    printf("%s",prompt);
// 	    char* Res_fgets = fgets(buffer,MAXLINE,stdin);
// 	    if(Res_fgets == NULL)
// 	    {
// 	        printf("\n");
// 	        exit(0);
// 	    }		
// 	#endif
// }

int main(int argc, char *argv[]) {
	char prompt[MAX_PROMPT];
	//while(1) {
		type_prompt(prompt);//print mysh>
		//read_command(commad, parameters, prompt);//cmd input

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
	*///}
    return 0;
}
