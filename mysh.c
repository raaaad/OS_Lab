//head file
#include <stdio.h>//feof();fileno();
#include <stdlib.h>//exit();getenv();
#include <unistd.h>//fork();getpid();close();execvp();chdir();stat();
#include <string.h>//strlen();strcmp();
#include <sys/types.h>//getuid();opendir();readdir();closedir();
#include <pwd.h>//getpwuid();
#include <dirent.h>
#include <sys/stat.h>
#include <grp.h>//group
#include <time.h>
#include <fcntl.h>//open();

//define
#define MAX_NAME 51
#define MAX_PATH 513
#define MAX_CMD 515//512+'\n'
#define MAX_ARG 21
#define MAX_CNT 1024
#define MAX_LINE 513

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

void printError();
int cmp(const void *a, const void *b);
void type_prompt(char* prompt);
int read_command(char **command,char **parameters,char *prompt);
int cmd_analysis(char **parameters,int para_count, struct CMD_INFO *info);
int built_in_command(char *command, char **parameters);
int built_in_cd(char **parameters);
int built_in_pwd();
int built_in_ls(char **parameters);
char* uid_str(uid_t uid);
char* gid_str(gid_t gid);

int main() {
	char prompt[MAX_LINE];
	char *command = NULL;
	char **parameters;
	int para_count = 0;
	struct CMD_INFO info;
	pid_t child_pid;

	myshBuffer = malloc(sizeof(char)*MAX_CMD);
	parameters = malloc(sizeof(char*)*MAX_ARG);

	while(1) {	
		
		type_prompt(prompt);//generate prompt
		para_count = read_command(&command, parameters, prompt);//cmd input
		if(para_count == -1)//wrong input
			continue;
		para_count -= 1;
		cmd_analysis(parameters,para_count,&info);
		
		if(built_in_command(command,parameters))
            continue;

        int in_fd,out_fd;//file description
        //int pipe_fd[2];
        /*if(CMD_INFO.flag == IS_PIPED) {//pipe
        	if(pipe(pipe_fd) < 0) {

        	}*/
		
	    if ((child_pid = fork()) < 0) {
	    	//fork failed then exit
	    	char err_fork[] = "fork failed\n";
	    	write(STDERR_FILENO,err_fork, strlen(err_fork));
	        exit(1);
	    }
		else if(child_pid != 0) {
			//parent
			//waitpid(-1, &status, 0);//wait until child exit
			continue;
		}
		else {
			//child
			char err_openfile[] =  "Open file failed\n";
			char err_dup2[] = "Error in dup2\n";
			if (info.flag & OUT_REDIRECT) {//need new a fil
				out_fd = open(info.output, O_CREAT|O_TRUNC|O_WRONLY, 0666);
				if(out_fd < 0) {
					write(STDERR_FILENO, err_openfile, strlen(err_openfile));
					exit(1);
				}
				close(fileno(stdout));//close stdout's file descriper
				if (dup2(out_fd, fileno(stdout)) < 0) {	
					write(STDERR_FILENO, err_dup2, strlen(err_dup2));
				}//copy out_fd to stdout
				close(out_fd);
			}
			if (info.flag & OUT_REDIRECT_APPEND) {
				out_fd = open(info.output, O_CREAT|O_APPEND|O_WRONLY);
				if(out_fd < 0) {
					write(STDERR_FILENO, err_openfile, strlen(err_openfile));
					exit(1);
				}
				close(fileno(stdout));
				if (dup2(out_fd, fileno(stdout)) < 0) {
					write(STDERR_FILENO, err_dup2, strlen(err_dup2));
				}
				close(out_fd);	
			}
			if(info.flag & IN_REDIRECT)
            {
                in_fd = open(info.input, O_CREAT|O_RDONLY, 0666);
                if(in_fd < 0) {
					write(STDERR_FILENO, err_openfile, strlen(err_openfile));
					exit(1);
                }
				close(fileno(stdin)); 
                if (dup2(in_fd, fileno(stdin)) < 0) {
					write(STDERR_FILENO, err_dup2, strlen(err_dup2));
				}
				close(in_fd); 
            }
            printf("im here\n");
			execvp(command, parameters);//execute command
		}
	}
	free(parameters);
	free(myshBuffer);	
    return 0;
}

void printError() {
	char error_message[30] = "An error has occurred\n";
	write(STDERR_FILENO, error_message, strlen(error_message));   	
    return;
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
    write(STDOUT_FILENO, prompt, length);
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

int cmd_analysis(char **parameters,int para_count, struct CMD_INFO *info) {
	int i;
	//init
	info->flag = 0;
    info->input = NULL;
    info->output = NULL;
    info->cmd2 = NULL;
    info->parameters2 = NULL;

    if(strcmp(parameters[para_count-1],"&") ==0) {//wait
        info->flag |= BACKGROUND;
        parameters[--para_count] = NULL;
    }

    for(i = 0; i < para_count;) {//redirect
        if(strcmp(parameters[i], "<<") == 0 || strcmp(parameters[i], "<") == 0) {
            info->flag |= IN_REDIRECT;
            info->input = parameters[i+1];
            parameters[i] = NULL;
            i+=2;
        }
        else if(strcmp(parameters[i], ">") == 0) {
            info->flag |= OUT_REDIRECT;
            info->output = parameters[i+1];
            parameters[i] = NULL;
            i+=2;
        }
        else if(strcmp(parameters[i], ">>") == 0) {
            info->flag |= OUT_REDIRECT_APPEND;
            info->output = parameters[i+1];
            parameters[i] = NULL;
            i+=2;
        }
        else if(strcmp(parameters[i], "|") == 0) {//pipe
            char* para2;
            info->flag |= IS_PIPED;
            parameters[i] = NULL;
            info->cmd2 = parameters[i+1];
            info->parameters2 = &parameters[i+1];
            for(para2 = info->parameters2[0] + strlen(info->parameters2[0]); para2 != &(info->parameters2[0][0]) && *para2!='/'; para2--);
            //point para2 at the second list of parameters
            if(*para2 == '/')
                para2++;
            info->parameters2[0] = para2;
            break;
        }
        else
            i++;
    }
    #ifdef DEBUG_3
    printf("\nbackground:%s\n", info->flag&BACKGROUND ? "yes" : "no");
	printf("in redirect:");
	info->flag&IN_REDIRECT ? printf("yes,file:%s\n", info->input) : printf("no\n");
	printf("out redirect:");
	info->flag&OUT_REDIRECT ? printf("yes,file:%s\n",info->output) : printf("no\n");
	printf("pipe:");
	info->flag&IS_PIPED ? printf("yes,command:%s %s %s\n", info->cmd2, info->parameters2[0], info->parameters2[1]) : printf("no\n");
	#endif
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
    else//wait
    	return 0;
    return 0;
}

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
	int j;
	char buf_pwd[MAX_PATH];
	j = sprintf(buf_pwd, "%s\n", cwd);
	write(STDOUT_FILENO, buf_pwd, strlen(buf_pwd));
	//printf("%s\n", cwd);
	return 0;
}

int cmp(const void *a, const void *b) {
	return strcmp((char*)a,(char*)b);
}

int built_in_ls(char **parameters) {
	DIR *dir;
    struct dirent *rent;//struct
    struct stat dir_stat;

    char list[MAX_CNT][MAX_NAME];
    char str[MAX_NAME];
    char dir_path[MAX_PATH];
    //str = malloc(sizeof(char)*MAX_NAME);
    //dir_path = malloc(sizeof(char)*MAX_PATH);
    //list = malloc(sizeof(char)*MAX_NAME*MAX_CNT);

    int cnt = 0, i;
    int option_a = 0, option_l = 0;
    
    for(i = 1;parameters[i] != NULL; i++) {
    	//printf("%s\n",parameters[i] );
    	if(parameters[i][0] == '-') {
    		int j = 1;
    		while(parameters[i][j] != '\0') {
    			if(parameters[i][j] == 'a')
    				option_a = 1;
    			else if(parameters[i][j] == 'l')
    				option_l = 1;
    			j++;
    		}
    	}
    	else
    		break;
    }
    //printf("a:%d l:%d\n",option_a,option_l );
    if(parameters[i] == NULL)//ls (null
    	strcpy(dir_path, ".");	
	else {
		//printf("%s\n",parameters[i]);
		strcpy(dir_path, parameters[i]);
	}
	//printf("%s\n", dir_path);
    dir = opendir(dir_path);
    if (dir == NULL) {
    	int j;
    	char err_path[MAX_PATH];
    	j = sprintf(err_path, "Can`t open directory %s\n", dir_path); 
    	write(STDERR_FILENO, err_path, strlen(err_path));
        return -1;
    }

	while((rent=readdir(dir)) != NULL){//read
    	strcpy(str, rent->d_name);
    	if(str[0] == '.' && option_a == 0)
        	continue;
    	if(!str)
        	continue;     
        //printf("%d %s\n", cnt,list[cnt-1]);
	    strcpy(list[cnt++], str);
    	
    }
    qsort(list, cnt, sizeof(list[0]), cmp);

    //usual output
    if(option_l == 0) {
	    for(i = 0; i < cnt; i++) {
	    	int j = 0;
	    	char list_buf[MAX_PATH];
	    	j = sprintf(list_buf, "%s", list[i]);
	    	if(i == cnt-1)
	        	j += sprintf(list_buf + j, "\n");
	        else
	        	j += sprintf(list_buf + j, "  ");
	        write(STDOUT_FILENO, list_buf, strlen(list_buf));
	        
	    }
	}//-l(long output)
    else {
    	char mode_info[MAX_CNT][12];
    	int nlink_info[MAX_CNT];
    	char uid_info[MAX_CNT][MAX_NAME];
	    char gid_info[MAX_CNT][MAX_NAME];
	    long sz_info[MAX_CNT];
	    char time_info[MAX_CNT][MAX_NAME];

    	for (i = 0; i < cnt; i++)
    	{
    		if(stat(list[i], &dir_stat) == -1){ // cannot stat
    			int j = 0;
    			char err_getinfo[MAX_LINE];
    			j = sprintf(err_getinfo, "Can't get infomation of file ");
    			j += sprintf(err_getinfo + j, "%s\n", list[i]);
    			write(STDERR_FILENO, err_getinfo, strlen(err_getinfo));
    			return -1;
    		}

    		int mode = dir_stat.st_mode;
    		strcpy(mode_info[i], "----------"); // default = no perms 
	        if(S_ISDIR(mode)) mode_info[i][0] = 'd'; //directory
	        if(S_ISCHR(mode)) mode_info[i][0] = 'c'; // char device 
	        if(S_ISBLK(mode)) mode_info[i][0] = 'b'; // block device 
	        if(S_ISLNK(mode)) mode_info[i][0] = 'l';

	        if(mode & S_IRUSR) mode_info[i][1] = 'r'; //user 
	        if(mode & S_IWUSR) mode_info[i][2] = 'w';
	        if(mode & S_IXUSR) mode_info[i][3] = 'x';

	        if(mode & S_IRGRP) mode_info[i][4] = 'r'; //group 
	        if(mode & S_IWGRP) mode_info[i][5] = 'w';
	        if(mode & S_IXGRP) mode_info[i][6] = 'x';

	        if(mode & S_IROTH) mode_info[i][7] = 'r'; //other
	        if(mode & S_IWOTH) mode_info[i][8] = 'w';
	        if(mode & S_IXOTH) mode_info[i][9] = 'x';

	        nlink_info[i] = (int)dir_stat.st_nlink;
	        strcpy(uid_info[i] , uid_str(dir_stat.st_uid));
	        strcpy(gid_info[i] , gid_str(dir_stat.st_gid));
	        sz_info[i] = (long)dir_stat.st_size;
	        strcpy(time_info[i] , 4 + ctime(&(dir_stat.st_mtime)));
    	}
    	//print
    	char buf_totalinfo[MAX_LINE];
    	int t = sprintf(buf_totalinfo, "total %d\n", cnt);
    	write(STDOUT_FILENO, buf_totalinfo, strlen(buf_totalinfo));
    	for(i = 0; i < cnt; i++) {
    		int j = 0;
    		char buf_print[MAX_LINE];
    		stat(list[i], &dir_stat);
    		j = sprintf(buf_print, "%s", mode_info[i]);
	        j += sprintf(buf_print + j, "%3d ", nlink_info[i]);
	        j += sprintf(buf_print + j, "%-10s",uid_info[i]);
	        j += sprintf(buf_print + j, "%-10s", gid_info[i]);
	        j += sprintf(buf_print + j, "%6ld ", sz_info[i]);
	        j += sprintf(buf_print + j, "%.12s ", time_info[i]);
	        j += sprintf(buf_print + j, "%s\n", list[i]);
	        write(STDOUT_FILENO, buf_print, strlen(buf_print));
	    }
    }
    closedir(dir);
    //free(list);
    //free(str);
    return 0;
}

char* uid_str(uid_t uid) {
        static char userstr[10];
        struct passwd *pw_ptr;
        if((pw_ptr = getpwuid(uid)) == NULL){
                sprintf(userstr, "%d", uid);
                return userstr;
        }else
                return pw_ptr->pw_name;
}

char* gid_str(gid_t gid) {
        static char grpstr[10];
        struct group *grp_ptr;
        if((grp_ptr = getgrgid(gid)) == NULL){
                sprintf(grpstr, "%d", gid);
                return grpstr;
        }else
                return grp_ptr->gr_name;
}