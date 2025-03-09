#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

void print_fd(const char *msg, int fds[2]){
    printf("%s: read_fd = %d, write_fd = %d\n", msg, fds[0], fds[1]);
}

void child_process(int read_fd, int write_fd){
    char buffer[10];
    srand(getpid());
    int random_num = rand() % 100;
    snprintf(buffer, sizeof(buffer), "%d", random_num);

    printf("PID %d sending: %s\n", getpid(), buffer);
    int bytes_written = write(write_fd, buffer, strlen(buffer)+1);
    if(bytes_written < 0){
        perror("write");
    }

    if(read_fd != -1){
        int bytes_read = read(read_fd, buffer, sizeof(buffer) -1);
        if(bytes_read > 0){
            buffer[bytes_read] = '\0';
            printf("PID %d recived: %s\n", getpid(), buffer);
        }else if( bytes_read == 0){
            printf("PID %d: Pipe closed, exiting\n", getpid());
        }else{
            perror("read");
        }
    }
    close(read_fd);
    close(write_fd);
    exit(0);
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
    srand(time(NULL));
    
    if((pid1 = fork()) == -1){
        perror("fork()");
        exit(EXIT_FAILURE);
    }

    if(pid1 == 0){
        close(pipe1[1]);
        close(pipe2[0]);
        close(pipe3[0]);
        close(pipe3[1]);
        child_process(pipe1[0], pipe2[1]);
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
        child_process(pipe2[0], pipe3[1]);
    }

    //rodzic:
    close(pipe1[0]);
    close(pipe2[1]);
    close(pipe3[1]);

    int random_num = rand() % 100;
    char msg[10];
    snprintf(msg, sizeof(msg), "%d", random_num);
    printf("Parent sending: %s\n", msg);

    if(write(pipe1[1], msg, strlen(msg)+1) < 0){
        perror("write");
    }
    close(pipe1[1]);
    
    char buffer[10];
    int bytes_read = read(pipe3[0], buffer, sizeof(buffer) -1);
    if(bytes_read > 0){
        buffer[bytes_read] = '\0';
        printf("Parent recived final value: %s\n", buffer);
    }else if(bytes_read == 0){
        printf("Parent: Pipe closed, no data recived\n");
    }else{
        perror("read");
    }

    close(pipe3[0]);

    wait(NULL);
    wait(NULL);

    printf("Parent: all children finished\n");
    return 0;
}