#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

#define MAX_PLAYERS 5
#define MAX_MESSAGE_SIZE 16

void initialize_pipes(int pipes[MAX_PLAYERS][2][2], int n){
    for(int i=0; i<n; i++){
        if(pipe(pipes[i][0]) == -1 || pipe(pipes[i][1]) == -1){
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }
}

void close_unused_pipes(int pipes[MAX_PLAYERS][2][2], int n, int process_id, int is_server){
    for(int i=0; i<n; i++){
        if(i != process_id){
            printf("Closing unused pipes for process %d (not %d)\n", i, process_id);
            close(pipes[i][0][0]);
            close(pipes[i][0][1]);
            close(pipes[i][1][0]);
            close(pipes[i][1][1]);
        } else if(is_server){
            printf("Server closing pipes for player %d\n", i);
            close(pipes[i][1][0]);
            close(pipes[i][0][1]);
        }else{
            printf("Player %d closing pipes\n", process_id);
            close(pipes[i][0][0]);
            close(pipes[i][1][1]);
        }
    }
}

void player_process(int player_id, int read_fd, int write_fd, int m){
    srand(time(NULL) ^ (getpid() << 16));
    int cards[m];
    for(int i=0; i<m; i++) 
        cards[i] = i+1;
    for(int round = 0; round < m; round++){
        char message[MAX_MESSAGE_SIZE];

        if(read(read_fd, message, MAX_MESSAGE_SIZE) <= 0){
            fprintf(stderr, "Player %d: Server disconected. Exiting. \n", player_id);
            break;
        }

        printf("Player %d received: %s\n", player_id, message);

        int card_index = rand() % ( m - round );
        int card_value = cards[card_index];

        for(int i=card_index; i<m - round - 1; i++){
            cards[i] = cards[i+1];
        }

        snprintf(message, MAX_MESSAGE_SIZE, "%d", card_value);
        printf("Player %d sends card: %d\n", player_id, card_value);
        if(write(write_fd, message, MAX_MESSAGE_SIZE) <=0){
            fprintf(stderr, "Player %d: Server disconnected. Exiting \n", player_id);
            break;
        }
    }
    close(read_fd);
    close(write_fd);
    exit(0);
}

int main(int argc, char *argv[]){
    if(argc != 3){
        fprintf(stderr, "Usage: %s N M\n", argv[0]);
        return EXIT_FAILURE;
    }

    int n = atoi(argv[1]);
    int m = atoi(argv[2]);

    if( n < 2 || n > 5 || m < 5 || m > 10){
        fprintf(stderr, "N should be 2 <= N <= 5 and M should be 5 <= M <= 10 \n");
        return EXIT_FAILURE;
    }

    int pipes[MAX_PLAYERS][2][2];
    initialize_pipes(pipes, n);
    pid_t players[MAX_PLAYERS];

    int cards[MAX_PLAYERS];
    int winners[MAX_PLAYERS];
    int max_card = -1;
    int num_winners = 0;

    for(int i=0; i<n; i++){
        pid_t pid = fork();
        if(pid == -1){
            perror("fork");
            exit(EXIT_FAILURE);
        }else if( pid == 0){
            close_unused_pipes(pipes, n, i, 0);
            player_process(i, pipes[i][0][0], pipes[i][1][1], m);
        }else{
            players[i] = pid;
        }
    }

    close_unused_pipes(pipes, n, -1, 1);

    for(int round = 0; round < m; round++){
        printf("NEW ROUND\n");

        char message[MAX_MESSAGE_SIZE] = "new_round";

        for(int i = 0; i < n; i++){
            if(write(pipes[i][0][1], message, MAX_MESSAGE_SIZE) <= 0){
                fprintf(stderr, "Server: Failed to send round message to Player %d.\n", i);
            }
        }

        // Get cards from all players and determine the round winner
        for(int i = 0; i < n; i++){
            if(read(pipes[i][1][0], message, MAX_MESSAGE_SIZE) <= 0){
                fprintf(stderr, "Server: Failed to receive card from Player %d.\n", i);
                continue;
            }

            cards[i] = atoi(message); // Convert card from string to integer
            printf("Player %d sent card: %d\n", i, cards[i]);

            if(cards[i] > max_card){
                max_card = cards[i];
                num_winners = 1;
                memset(winners, 0, sizeof(winners));  // Reset winners
                winners[i] = 1;
            } else if(cards[i] == max_card){
                winners[i] = 1;
                num_winners++;
            }
        }

        int points[MAX_PLAYERS] = {0};
        int points_per_winner = (num_winners > 0) ? (n / num_winners): 0;

        for(int i = 0; i < n; i++){
            if(winners[i]) points[i] = points_per_winner;

            snprintf(message, MAX_MESSAGE_SIZE, "%d", points[i]);
            if(write(pipes[i][0][1], message, MAX_MESSAGE_SIZE) <= 0){
                fprintf(stderr, "Server: Failed to send points to Player %d.\n", i);
            }
        }
    }
       
    for (int i = 0; i < n; i++) {
        close(pipes[i][0][1]);
        close(pipes[i][1][0]);
        if(waitpid(players[i], NULL, 0) == -1)
            perror("wairpid");
    }

    printf("Game over.\n");
    return EXIT_SUCCESS;
}