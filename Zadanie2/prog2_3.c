#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>

#define MAX_PLAYERS 5
#define MSG_SIZE 16

void initialize_pipes(int pipes[MAX_PLAYERS][2][2], int n) {
    for (int i = 0; i < n; i++) {
        if (pipe(pipes[i][1]) == -1 ||  pipe(pipes[i][0])==-1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }
}



void player_process(int player_id, int read_fd, int write_fd) {
    srand(time(NULL) ^ getpid());
    char message[MSG_SIZE] = {0};

    while (read(read_fd, message, MSG_SIZE) > 0) {
        if (strncmp(message, "quit", 4) == 0) {
            break;
        }
        if (strncmp(message, "new_round", 9) == 0) {
            int card = rand() % 13 + 1;
            printf("Gracz %d (PID %d) wylosował kartę: %d\n", player_id, getpid(), card);
            snprintf(message, MSG_SIZE, "%d", card);
            write(write_fd, message, MSG_SIZE);
        }
    }

    close(read_fd);
    close(write_fd);
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s N\n", argv[0]);
        return EXIT_FAILURE;
    }

    int n = atoi(argv[1]);
    int m = atoi(argv[2]);
    if (n < 2 || n > MAX_PLAYERS) {
        fprintf(stderr, "N powinno być w zakresie 2-5\n");
        return EXIT_FAILURE;
    }
    if(m < 1){
        fprintf(stderr, "M powinno byc wieksze od 0\n");
        return EXIT_FAILURE;
    }

    int pipes[MAX_PLAYERS][2][2];
    initialize_pipes(pipes, n);
    pid_t players[MAX_PLAYERS];

    for (int i = 0; i < n; i++) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            close(pipes[i][1][0]);
            close(pipes[i][0][1]);
            player_process(i, pipes[i][0][0], pipes[i][1][1]);
        } else {
            players[i] = pid;
            close(pipes[i][0][0]);
            close(pipes[i][1][1]);
        }
    }

    for(int round = 1; round <= m; round++){
        printf("NEW ROUND %d\n", round);
        char message[MSG_SIZE] = "new_round";
        for (int i = 0; i < n; i++) {
            if (write(pipes[i][0][1], message, MSG_SIZE) <= 0) {
                fprintf(stderr, "Serwer: Błąd wysyłania do gracza %d\n", i);
            }
        }
        int highest_card = 0;
        int winner = -1;
        for (int i = 0; i < n; i++) {
            char buffer[MSG_SIZE] = {0};
            if (read(pipes[i][1][0], buffer, MSG_SIZE) > 0) {
                int card = atoi(buffer);
                printf("Serwer: Gracz %d wysłał kartę: %d\n", i, card);
                if (card > highest_card) {
                    highest_card = card;
                    winner = i;
                }
            }
            //close(pipes[i][1][0]);
        }
        printf("Serwer: Gracz %d wygal runde %d z karta %d!\n", winner, round, highest_card);
    }

   char quit_message[MSG_SIZE] = "quit";
    for (int i = 0; i < n; i++) {
        write(pipes[i][0][1], quit_message, MSG_SIZE);
        close(pipes[i][0][1]);
        close(pipes[i][1][0]);
    }

    for (int i = 0; i < n; i++) {
        waitpid(players[i], NULL, 0);
    } 

    printf("Gra zakonczona.\n");
    return EXIT_SUCCESS;
}