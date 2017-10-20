#include <stdio.h>
#include <stdlib.h>//exit()
#include <unistd.h>//fork();getpid();close();execvp()
#include <string.h>
#include <sys/types.h>//getuid()
#include <pwd.h>//getpwuid();
#define nameLen 50
#define pathLen 1024
#define MAX_CMD 1024
char prompt[MAX_CMD];
//struct passwd *pwd;

void type_prompt(char *prompt) {
	struct passwd *pwd;
	char userName[nameLen],hostName[nameLen],pathName[pathLen];
	int length;

	pwd = getpwuid(getuid());
	getcwd(pathName, pathLen);//get path

	//print hostName and userName 
    if(gethostname(hostName, nameLen) == 0)
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

}

//return value: number of parameters
//0 represents only command without any parameters
//-1 represents wrong input
int read_command()
{

}

int main(int argc, char *argv[]) {
	
	//while(1) {
		type_prompt(prompt);//print mysh>...
		read_command(commad,parameters);//get input

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
