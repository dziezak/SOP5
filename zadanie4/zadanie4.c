#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>

#define N 1               // Liczba procesów w każdej grupie
#define MAX_HP 100       // Początkowe HP każdego procesu
#define MAX_DAMAGE 20    // Maksymalny damage zadawany przez proces

typedef struct {
    int hp;
    int id;
    bool alive;
    char group;
} ProcessData;

void process_fight(int read_fd, int write_fd, int id, char group) {
    ProcessData my_data = {MAX_HP, id, true, group};
    ProcessData enemy_data;
    int read_status;

    while (my_data.hp > 0) {
        read_status = read(read_fd, &enemy_data, sizeof(ProcessData));
        if (read_status <= 0) {
            printf("Proces %d (%c) kończy, bo nie ma już przeciwnika.\n", my_data.id, my_data.group);
            break;
        }

        if (!enemy_data.alive) {
            printf("Proces %d (%c) widzi, że przeciwnik %d (%c) nie żyje. Kończy walkę.\n", 
                my_data.id, my_data.group, enemy_data.id, enemy_data.group);
            break;
        }

        if (!my_data.alive || my_data.hp <= 0) {
            printf("Proces %d (%c) nie zyje\n", my_data.id, my_data.group);
            break;
        }

        int damage = rand() % MAX_DAMAGE + 1;
        my_data.hp -= damage;
        printf("Proces %d (%c) otrzymał %d obrażeń! HP: %d\n", my_data.id,my_data.group, damage, my_data.hp);

        if (my_data.hp <= 0) {
            my_data.alive = false;
            printf("Proces %d (%c) został pokonany!\n", my_data.id, my_data.group);
            write(write_fd, &my_data, sizeof(ProcessData));
            break;
        }

        damage = rand() % MAX_DAMAGE + 1;
        enemy_data.hp -= damage;
        printf("Proces %d (%c) atakuje %d za %d obrażeń!\n", my_data.id, my_data.group,enemy_data.id, damage);

        if (enemy_data.hp <= 0) {
            enemy_data.alive = false;
            printf("Proces %d (%c) pokonał przeciwnika %d!\n", my_data.id, my_data.group, enemy_data.id);
        }

        write(write_fd, &enemy_data, sizeof(ProcessData));

        sleep(1);
    }

    close(read_fd);
    close(write_fd);
    printf("Proces %d (%c) kończy swoje działanie.\n", my_data.id, my_data.group);
    exit(0);
}

int main() {
    srand(time(NULL));
    int pipes_A_to_B[N][2], pipes_B_to_A[N][2];
    pid_t pids[2 * N];

    for (int i = 0; i < N; i++) {
        pipe(pipes_A_to_B[i]);
        pipe(pipes_B_to_A[i]);
    }

    for (int i = 0; i < N; i++) {
        pids[i] = fork();
        if (pids[i] == 0) {
            close(pipes_A_to_B[i][0]);
            close(pipes_B_to_A[i][1]);
            process_fight(pipes_B_to_A[i][0], pipes_A_to_B[i][1], i, 'A');
            exit(0);  // 👈 Zapobiega powielaniu procesów!
        }
    }

    for (int i = 0; i < N; i++) {
        pids[N + i] = fork();
        if (pids[N + i] == 0) {
            close(pipes_B_to_A[i][0]);
            close(pipes_A_to_B[i][1]);
            process_fight(pipes_A_to_B[i][0], pipes_B_to_A[i][1], N + i, 'B');
            exit(0);  // 👈 Zapobiega powielaniu procesów!
        }
    }

    for (int i = 0; i < N; i++) {
        ProcessData init_data = {MAX_HP, N + i, true};
        write(pipes_A_to_B[i][1], &init_data, sizeof(ProcessData));
    }

    for (int i = 0; i < N; i++) {
        close(pipes_A_to_B[i][1]);
        close(pipes_B_to_A[i][1]);
    }

    for (int i = 0; i < 2 * N; i++) {
        waitpid(pids[i], NULL, 0);
    }

    printf("Walka zakończona! 🔥🔥🔥\n");
    return 0;
}
