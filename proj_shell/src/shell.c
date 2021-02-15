#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

//max input line char number
char CommandLine[1025];
//This function is handler for input command line
void HandleCommandLine(int mode){
    //This is distinguish batch, interactive mode
    //if batch mode, print command line
    if(mode == 2){
    printf("%s\n",CommandLine);
    }
    //Make pid table to handle processes
    pid_t pid[101];
    //This variable checks process numbers
    int handlenum = 0;
    //This is for division semicolon and put semicolon pointers
    char *semiptrs[101];
    char *semiptr = strtok(CommandLine, ";");
    //This is for division blank and put blank pointers
    char *blankptr = NULL;
    //This operation is parsing semicolon
    while(semiptr != NULL){
        semiptrs[handlenum++] = semiptr;
        semiptr = strtok(NULL,";");
    }
    //This operation is parsing blank from each parsed semicolon line
    for(int i=0;i<handlenum;i++){
        blankptr = strtok(semiptrs[i]," ");
            //This is making argument array for exec function
            while(blankptr != NULL){
            char *exec_argu[100];
            int argunum = 0;
                while(blankptr != NULL){
                    exec_argu[argunum++] = blankptr;
                    blankptr = strtok(NULL, " ");
                }
            exec_argu[argunum] = blankptr;
            if(exec_argu != NULL){
            //if fork return value is lower than 0, that means fork error.
                if((pid[i]=fork()) < 0){
                    printf("Fork error");
                    exit(0);
                }
                //if pid==0 which means child process, exec input command
                else if(pid[i] == 0){
                    if(execvp(exec_argu[0],exec_argu) < 0){
                        printf("Command Error, Please re-check your command\n");
                        //if exec failed, child process(forked) should be exited
                        exit(1);
                    }
                }
                //if it is not child process, continue and check another command
                else{
                    continue;
                    }
                }
            }
    }
    //This is waiting forked process
    for(int i = 0; i<handlenum; i++){
        waitpid(pid[i],NULL,0);
    }
    return;
}
//This function runs Interactive mode
void Interactive(){
    while(1){
        printf("prompt> ");
        /* the NULL return value of fgets is EOF or Error, Thus shell should
           be exited */
        if(fgets(CommandLine, sizeof(CommandLine), stdin) == NULL){
            return;
        }
        // This removes end line of CommandLine
        CommandLine[strlen(CommandLine)-1] = '\0';
        //if input CommandLine is quit than shell should be killed
        if(strcmp(CommandLine, "quit") == 0){
            return;
        }
        //and then handle
        HandleCommandLine(1);
    }
    return;
}
//This function runs Batch mode
void Batch(char *input){
    //Open input file with read mode
    FILE *inputf = fopen(input,"r");
    while(1){
        // fgets reads input file stream
        if(fgets(CommandLine,sizeof(CommandLine), inputf) == NULL){
            return;
        }
        // This part is same with Interactive function
        CommandLine[strlen(CommandLine)-1] = '\0';
        if(strcmp(CommandLine, "quit") == 0){
            printf("quit\n");
            return;
        }
        HandleCommandLine(2);
    }
    return;
}

int main(int argc, char* argv[]){
    //if argc is 1, run Interactive function
    if(argc == 1) {
         Interactive();
    }
    //if argc is 2, run Batch function with input file
    else if(argc == 2) {
         Batch(argv[1]);
    }
    //else skip the command line and return
    else {
         printf("Too many arguments, please re-check command line\n");
         return -1;
    }
    return 0;
}
    
