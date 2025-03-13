#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>

#define N 3              // Liczba procesów w każdej grupie
#define MAX_HP 100       // Początkowe HP każdego procesu
#define MAX_DAMAGE 20    // Maksymalny damage zadawany przez proces

typedef struct {
    int hp;
    int id;
    bool alive;  // Czy proces żyje
} ProcessData;

void process_fight(int read_fd, int write_fd, int id) {
    ProcessData my_data = {MAX_HP, id, true};
    ProcessData enemy_data;

    while (my_data.hp > 0) {
        // Otrzymanie danych od przeciwnika
        if (read(read_fd, &enemy_data, sizeof(ProcessData)) <= 0) {
            break;  // Jeśli nie ma już danych, kończymy proces
        }

        // Jeśli przeciwnik już nie żyje, kończymy pętlę
        if (!enemy_data.alive) {
            printf("Proces %d widzi, że przeciwnik %d nie żyje. Kończy walkę.\n", my_data.id, enemy_data.id);
            break;
        }

        // Otrzymanie obrażeń od przeciwnika
        int damage = rand() % MAX_DAMAGE + 1;
        my_data.hp -= damage;
        printf("Proces %d otrzymał %d obrażeń! HP: %d\n", my_data.id, damage, my_data.hp);

        // Jeśli zginął, wysyłamy informację o śmierci
        if (my_data.hp <= 0) {
            my_data.alive = false;
            printf("Proces %d został pokonany!\n", my_data.id);
            write(write_fd, &my_data, sizeof(ProcessData));  // Powiadamiamy przeciwnika o śmierci
            break;
        }

        // Atakujemy przeciwnika
        damage = rand() % MAX_DAMAGE + 1;
        enemy_data.hp -= damage;
        printf("Proces %d atakuje %d za %d obrażeń!\n", my_data.id, enemy_data.id, damage);

        // Jeśli przeciwnik zginął, oznaczamy go jako martwego
        if (enemy_data.hp <= 0) {
            enemy_data.alive = false;
            printf("Proces %d pokonał przeciwnika %d!\n", my_data.id, enemy_data.id);
        }

        // Wysyłamy dane do przeciwnika
        write(write_fd, &enemy_data, sizeof(ProcessData));

        sleep(1);
    }

    printf("Proces %d konczy swoje dzialanie.\n", my_data.id);
    close(read_fd);
    close(write_fd);
    exit(0);
}

int main() {
    srand(time(NULL));
    int pipes_A_to_B[N][2], pipes_B_to_A[N][2];
    pid_t pids[2 * N];

    // Tworzenie potoków
    for (int i = 0; i < N; i++) {
        pipe(pipes_A_to_B[i]);
        pipe(pipes_B_to_A[i]);
    }

    // Tworzenie procesów grupy A
    for (int i = 0; i < N; i++) {
        pids[i] = fork();
        if (pids[i] == 0) {  // Proces potomny
            close(pipes_A_to_B[i][0]);  // Zamknięcie odczytu z własnego pipe
            close(pipes_B_to_A[i][1]);  // Zamknięcie zapisu do własnego pipe
            process_fight(pipes_B_to_A[i][0], pipes_A_to_B[i][1], i);
        }
    }

    // Tworzenie procesów grupy B
    for (int i = 0; i < N; i++) {
        pids[N + i] = fork();
        if (pids[N + i] == 0) {  // Proces potomny
            close(pipes_B_to_A[i][0]);  // Zamknięcie odczytu z własnego pipe
            close(pipes_A_to_B[i][1]);  // Zamknięcie zapisu do własnego pipe
            process_fight(pipes_A_to_B[i][0], pipes_B_to_A[i][1], N + i);
        }
    }

    // Inicjalizacja pierwszego ataku dla każdego procesu A -> B
    for (int i = 0; i < N; i++) {
        ProcessData init_data = {MAX_HP, N + i, true};  // Procesy B mają ID zaczynające się od N
        write(pipes_A_to_B[i][1], &init_data, sizeof(ProcessData));
    }

    // Oczekiwanie na zakończenie wszystkich procesów
    for (int i = 0; i < 2 * N; i++) {
        waitpid(pids[i], NULL, 0);
    }

    printf("Walka zakończona! 🔥🔥🔥\n");
    return 0;
}
