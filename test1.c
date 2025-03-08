#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>

int main() {
    int pipefd[2];
    //char buffer[100];
    const char *msg = "To jest dlugi tekst przekraczajacy PIPE_BUF... BRUUH Ciekawe czy to sie kiedykolwiek wywali";


    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) { 
        close(pipefd[1]);
        char buffer[PIPE_BUF];
        size_t bytesRead;

        while((bytesRead = read(pipefd[0], buffer, sizeof(buffer)))>0){
            write(STDOUT_FILENO, buffer, bytesRead);
        }
        printf("Proces potomny otrzymał: %s\n", buffer);

        close(pipefd[0]);
    } else {
        close(pipefd[0]);
        
        size_t msgLength = strlen(msg);
        size_t written = 0;

        while(written < msgLength){
            ssize_t bytes = write(pipefd[1], msg + written, msgLength - written);
            if(bytes == -1){
                perror("write");
                exit(EXIT_FAILURE);
            }
            written += bytes;
        }
        close(pipefd[1]);
    }

    return 0;
}
