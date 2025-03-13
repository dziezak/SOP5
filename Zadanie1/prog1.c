#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

void print_fd(const char *msg, int fds[2]){
    printf("%s: read_fd = %d, write_fd = %d\n", msg, fds[0], fds[1]);
}

int main(){
    int pipe1[2], pipe2[2], pipe3[2];

    if(pipe(pipe1) == -1 || pipe(pipe2) == -1 || pipe(pipe3) == -1){
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    print_fd("Pipe1", pipe1);
    print_fd("Pipe2", pipe2);
    print_fd("Pipe3", pipe3);

    pid_t pid1, pid2;
    if((pid1 = fork()) == -1){
        perror("fork()");
        exit(EXIT_FAILURE);
    }

    if(pid1 == 0){
        close(pipe1[0]);
        close(pipe2[0]);
        close(pipe3[0]);
        close(pipe3[1]);

        printf("Child 1: closed unused pipes\n");

        exit(0);
    }

    if((pid2 = fork()) == -1){
        perror("fork()");
        exit(EXIT_FAILURE);
    }

    if(pid2 == 0){
        close(pipe2[1]);
        close(pipe3[0]);
        close(pipe1[0]);
        close(pipe1[1]);
        
        printf("Child 2: closed unused pipes\n");

        exit(0);
    }

    //rodzic:
    close(pipe1[0]);
    close(pipe2[1]);
    close(pipe3[1]);

    printf("Parent: closed unused pipes/n");
    
    wait(NULL);
    wait(NULL);
    printf("Parent: all children finished\n");
    return 0;
}