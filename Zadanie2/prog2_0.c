#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_PLAYERS 5

void initialize_pipes(int pipes[MAX_PLAYERS][2], int n) {
    for (int i = 0; i < n; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }
}

void close_unused_pipes(int pipes[MAX_PLAYERS][2], int n, int process_id, int is_server) {
    for (int i = 0; i < n; i++) {
        if (i != process_id) {
            close(pipes[i][0]);
            close(pipes[i][1]);
        } else if (is_server) {
            close(pipes[i][1]); // Serwer zamyka zapis do potoku
        } else {
            close(pipes[i][0]); // Gracz zamyka odczyt z potoku
        }
    }
}

void player_process(int player_id, int read_fd) {
    char message[32];

    if (read(read_fd, message, sizeof(message)) > 0) {
        printf("Gracz %d (PID %d) połączony z serwerem\n", player_id, getpid());
    }else{
        fprintf(stderr, "Gracz %d: Błąd połaczenia. \n", player_id);
    }

    close(read_fd);
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s N\n", argv[0]);
        return EXIT_FAILURE;
    }

    int n = atoi(argv[1]);
    if (n < 2 || n > MAX_PLAYERS) {
        fprintf(stderr, "N powinno być w zakresie 2-5\n");
        return EXIT_FAILURE;
    }

    int pipes[MAX_PLAYERS][2];
    initialize_pipes(pipes, n);
    pid_t players[MAX_PLAYERS];

    for (int i = 0; i < n; i++) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            close(pipes[i][1]);
            player_process(i, pipes[i][0]);
        } else {
            players[i] = pid;
            close(pipes[i][0]);
        }
    }


    for (int i = 0; i < n; i++) {
        if (write(pipes[i][1], "connect", 8) <= 0) {
            fprintf(stderr, "Serwer: Błąd wysyłania do gracza %d\n", i);
        }
        close(pipes[i][1]);
    }
    

    for (int i = 0; i < n; i++) {
        waitpid(players[i], NULL, 0);
    }

    printf("Wszyscy gracze połączeni. Koniec.\n");
    return EXIT_SUCCESS;
}
