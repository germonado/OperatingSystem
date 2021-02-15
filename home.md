Assignment1_project_shell
=================



제 프로그램은 main, batch, interactive, handlecommandline으로 구성 되어 있습니다.

shell 프로그램을 짜는 데 있어서 크게 순서를 정리하자면

1. 메인문에서 입력을 받고 argument로 batch, interactive mode를 구분합니다.
2. batch mode인경우 파일포인터로 연후 한줄단위로 받아서 넘겨주고, interactive 모드인경우 prompt를 출력하며 두 mode 다 handlecommandline 함수를 통해 명령어를 실행해줍니다.

### HandleCommandLine 함수
핵심은 HandleCommandLine 함수인데, 우선 그 함수에는 interactive 모드 인지 batch 모드 인지 구분하는 식별숫자를 받아서 batch모드를 뜻하는 2가 들어올 경우, 명령어를 출력하고 수행합니다. HandleCommandLine 함수를 편의를 위해 HCL이라고 지칭하겠습니다. 
1. HCL은 최대 101개의 pid 배열과 세미콜론 배열을 가지고 명령어를 수행하는데, 우선 받아온 CommandLine를 strtok 함수를 이용해 세미콜론 단위로 잘라서 포인터를 저장해둡니다. 초반에는 while문 안에서 세미콜론과 빈칸 다 처리하려다가 strtok이 만든 NULL값이 세미콜론과 빈칸 구분이 안되서 처리가 잘 안됐습니다. 
2. 그래서 세미콜론 단위로 파싱해서 포인터 배열에 저장해둔 뒤,  for문을 돌면서 세미콜론 단위로 명령어와 뒤에 들어오는 argument를 구분해 (strtok 함수로 빈칸 파싱) argument 배열에 담습니다.  
3. 그 후 fork함수를 통하여 pid==0 즉 자식 프로세스라면 execvp 함수를 통해(명령어, 명령어담은배열(argument))로 exec을 실행합니다.  
4. 만약 exec에 실패하면 자식 프로세스를 exit 해줍니다.  세미콜론 단위로 받아진 명령어들을 전부 exec한 다음, 포문을 돌면서 핸들한 갯수만큼 pid 배열들(fork된 프로세스들)이 끝날때까지 기다렸다가 HCL함수를 종료해줍니다.


감사합니다.

```
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
    
