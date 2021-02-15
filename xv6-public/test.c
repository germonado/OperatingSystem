#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char *argv[]){  
    cpu_share(10);
    printf(1, "My pid is %d\n", getpid());
    printf(1, "My ppid is %d\n",getppid());
    exit(); 
    return 0;
}

