#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char *argv[]){
    int chk = fork();
    for(int i=0;i<100;i++){
    if(chk==0){
        printf(1,"child\n");
        yield();
        }   
    else if(chk>0){
        printf(1,"parent\n");
        yield();
        }
    }
    wait();
    exit();

    return 0;
}

