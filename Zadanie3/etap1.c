#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char* argv[]){
    if(argc != 2){
        fprintf(stderr, "Uzycie: %s <liczba_studentow>\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);
    if( n < 3 || n > 20){
        fprintf(stderr, "Liczba studentow musi byc w zakresie od 3 do 20\n");
        return 1;
    }

    printf("Labolatorium SOP - nauczyciel (PID: %d) tworzy %d studentow\n", getpid(), n);

    for(int i=0; i<n; i++){
        pid_t pid;
        if((pid = fork())<0){
            perror("fork");
            exit(1);
        }else if(pid == 0){
            printf("Studnet %d - PID: %d\n", i+1, getpid());
            exit(0);
        }
    }

    for(int i=0; i<n; i++){
        wait(NULL);
    }
    printf("Wszyscy studenci zakonczyli dzialanie\n");
    return 0;
}