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

    while(1){
        int bytes_read = read(read_fd, buffer, sizeof(buffer) -1);
        if(bytes_read > 0){
            buffer[bytes_read] = '\0';
            int recived_num = atoi(buffer);
            printf("PID %d recived: %d\n", getpid(), recived_num);

            if(recived_num == 0){
                printf("PID %d  stopping due to recived 0\n", getpid());
                break;
            }
            int random_factor = (rand() %21) -10;
            int new_num = (recived_num + random_factor) %20;

            printf("PID %d sending: %d (modified by %d)\n", getpid(), new_num, random_factor);

            snprintf(buffer, sizeof(buffer), "%d", new_num);
            if(write(write_fd, buffer, strlen(buffer)+1) < 0){
                perror("write");
                break;
            }
        }else if(bytes_read == 0){
            printf("PID = %d, Pipe closed, exiting\n", getpid());
            break;
        }else{
            perror("read");
            break;
        }
    }
    close(read_fd);
    close(write_fd);
    exit(0);
}

void sigint_handler(int sig){
    printf("\nProgram interrupted. Exiting gracefully\n");
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
    //srand(time(NULL));
    signal(SIGINT, sigint_handler);
    
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

    //printf("Parent: closed unused pipes/n");
    char msg[10];
    snprintf(msg, sizeof(msg), "%d", 1);
    printf("Parent sending\n");

    if(write(pipe1[1], msg, strlen(msg)+1) < 0){
        perror("write");
    }
    while(1){
        int bytes_read = read(pipe3[0], msg, sizeof(msg) -1);
        if(bytes_read > 0){
            msg[bytes_read] = '\0';
            int recived_num = atoi(msg);
            printf("Parent PID %d recived: %s\n", getpid(),msg);

            if(recived_num == 0){
                printf("Parent PID %d stopping due to recived 0\n", getpid());
                break;
            }

            printf("Parent PID %d sending back: %s\n", getpid(), msg);
            if(write(pipe1[1], msg, strlen(msg)+1) < 0){
                perror("write");
                break;
            }
        }else if(bytes_read == 0){
            printf("Parent: PID %d Pipe closed, no data recived\n", getpid());
            break;
        }else{
            perror("read");
            break;
        }
    }

    close(pipe1[1]);
    close(pipe3[0]);

    wait(NULL);
    wait(NULL);

    printf("Parent: all children finished\n");
    return 0;
}
