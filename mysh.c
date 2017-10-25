//head file
#include <stdio.h>//feof();
#include <stdlib.h>//exit();getenv();
#include <unistd.h>//fork();getpid();close();execvp();chdir();stat();
#include <string.h>//strlen();strcmp();
#include <sys/types.h>//getuid();opendir();readdir();closedir();
#include <pwd.h>//getpwuid();
#include <dirent.h>
#include <sys/stat.h>

//define
#define MAX_PROMPT 513
#define MAX_NAME 51
#define MAX_PATH 513
#define MAX_CMD 515//512+'\n'
#define MAX_ARG 21
#define MAX_CNT 1024

#define DEBUG


char *myshBuffer;

#define BACKGROUND 			1
#define IN_REDIRECT 		2
#define OUT_REDIRECT 		4
#define OUT_REDIRECT_APPEND	8
#define IS_PIPED 			16

struct CMD_INFO
{
	int flag;
	char* input;
	char* output;
	char* cmd2;
	char** parameters2;
};

void printError() {
	char error_message[30] = "An error has occurred\n";
	write(STDERR_FILENO, error_message, strlen(error_message));   	
    return;
}

int cmp(const void *a, const void *b) {
	return strcmp((char*)a,(char*)b);
}

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
    printf("%s",prompt);
    return;
}

int read_command(char **command,char **parameters,char *prompt) {
    char* cmd = fgets(myshBuffer,MAX_CMD,stdin);
    
    #ifdef db2_0
    printf("cmd_len:%d",(int)strlen(cmd));
    #endif

    if((int)strlen(cmd) > 513) {
    	//scanf("%*[^\n]%*c");
    	//fflush(stdin);
    	rewind(stdin);
    	printError();
    	return -1;
    }

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

            #ifdef db2_1
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

    #ifdef db2_2
    printf("count:%d\n",count);
    printf("pathname:[%s]\ncommand:[%s]\nparameters:\n",*command,parameters[0]);
    int i;
    for(i = 0;i < count - 1; i++)
        printf("[%s]\n", parameters[i]);
    #endif

    return count;
}

/*int cmd_analysis(char **parameters,int para_count,struct CMD_INFO *info) {
	//init
	info->flag = 0;
    info->input = NULL;
    info->output = NULL;
    info->cmd2 = NULL;
    info->parameters2 = NULL;

    if(strcmp(parameters[para_count-1], "&"))

}*/

int built_in_cd(char **parameters) {
	char *cd_path = NULL;
	char *home;
	if(parameters[1] == NULL) {//cd (null)
    	home = getenv("HOME");
    	cd_path = malloc(strlen(home+1));
    	strcpy(cd_path, home);
    }
	else {
		cd_path = malloc(strlen(parameters[1]+1));
		strcpy(cd_path, parameters[1]);//store path
	}
	//printf("%d\n",chdir(cd_path));
	if(chdir(cd_path) != 0)
		printError();
	free(cd_path);
	return 0;
}

int built_in_pwd() {
	char *cwd = NULL;
	cwd = malloc(MAX_PATH);
	getcwd(cwd, MAX_PATH);
	printf("%s\n", cwd);
	return 0;
}

int built_in_ls(char **parameters) {
	DIR *dir;
    char str[MAX_NAME];
    char list[MAX_CNT][MAX_NAME];
    int cnt = 0, i;

    struct dirent *rent;//struct
    
    if(parameters[1] == NULL)//ls (null)
    	parameters[1] = ".";

    dir = opendir(parameters[1]);
    if (dir == NULL) {  
        fprintf(stderr, "Can`t open directory %s\n", parameters[1]);  
        return -1;  
    }

	//char *p = *list;
	while((rent=readdir(dir)) != NULL){//read
    	strcpy(str, rent->d_name);
    	if(str[0] == '.')
        	continue;
    	if(!str)
        	continue;
    	strcpy(list[cnt++], str);
    	printf("%d %s\n", cnt,list[cnt-1]);
    }
    printf("------------\n");
    qsort(list, cnt, sizeof(list[0]), cmp);
    for(i = 0; i < cnt; i++)
        printf("%d %s\n", i,list[i]);
    closedir(dir);
    return 0;
}

int built_in_command(char *command, char **parameters) {
	//exit
	if(strcmp(command,"exit")==0 || strcmp(command,"q")==0)
        exit(0);
    else if(strcmp(command, "cd") == 0)
    	built_in_cd(parameters);
    else if(strcmp(command, "pwd") == 0)
    	built_in_pwd();
    else if(strcmp(command, "ls") == 0)
    	built_in_ls(parameters);
    else
    	return 0;
    return 0;
}

int main() {
	char prompt[MAX_PROMPT];
	char *command = NULL;
	char **parameters;
	int para_count = 0;
	struct CMD_INFO info;

	myshBuffer = malloc(sizeof(char)*MAX_CMD);
	parameters = malloc(sizeof(char*)*MAX_ARG);

	while(1) {	
		type_prompt(prompt);//generate prompt
		para_count = read_command(&command, parameters, prompt);//cmd input
		if(para_count == -1)//wrong input
			continue;
		para_count -= 1;
		//cmd_analysis(parameters,para_count,&info);
		if(built_in_command(command,parameters))
            continue;
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
			execve(command, parameters, 0);//execute command
	*/}

	free(parameters);
	free(myshBuffer);	
    return 0;
}
