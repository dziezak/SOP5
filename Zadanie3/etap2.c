#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

#define MSG_SIZE 50
#define NUM_STAGES 4

int losuj(int min, int max) {
    return min + rand() % (max - min + 1);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uzycie: %s <liczba_studentow>\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);
    if (n < 3 || n > 20) {
        fprintf(stderr, "Liczba studentow musi byc w zakresie od 3 do 20\n");
        return 1;
    }

    printf("Labolatorium SOP - nauczyciel (PID: %d) tworzy %d studentow\n", getpid(), n);

    int teacher_to_students[n][2];  
    int students_to_teacher[2];  
    pid_t student_pids[n];      

    if (pipe(students_to_teacher) == -1) {
        perror("pipe");
        exit(1);
    }

    for (int i = 0; i < n; i++) {
        if (pipe(teacher_to_students[i]) == -1) {
            perror("pipe");
            exit(1);
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(1);
        } else if (pid == 0) {
            printf("Student %d - PID: %d\n", i + 1, getpid());

            close(teacher_to_students[i][1]);
            close(students_to_teacher[0]); 

            for(int stage = 1; stage <= NUM_STAGES; stage++){
                int t = losuj(100, 500);
                //usleep(t * 1000);  // usleep w mikrosekundach

                int q = losuj(1, 20);
                int k = stage * 100;
                int wynik = k + q;

                char response[MSG_SIZE];
                snprintf(response, MSG_SIZE, "%d %d", getpid(), wynik);
                write(students_to_teacher[1], response, strlen(response)+1);

                char buffer[MSG_SIZE];
                read(teacher_to_students[i][0], buffer, MSG_SIZE);
                printf("Student %d (PID %d): Otrzymał wiadomość -> %s\n", i + 1, getpid(), buffer);
                if(strcmp(buffer, "PASS") == 0){
                    printf("Student %d (PID %d): Stage %d PASSED\n", i + 1, getpid(), stage);
                } else {
                    printf("Student %d (PID %d): Stage %d FAILED\n", i + 1, getpid(), stage);
                    stage--; // Decrement stage, but avoid infinite loop
                }
            }

            printf("Student %d (PID %d): I NAIL IT!!!\n", i + 1, getpid());
            close(teacher_to_students[i][0]);
            close(students_to_teacher[1]);
            exit(0);
        } else {
            student_pids[i] = pid;
            close(teacher_to_students[i][0]);
        }
    }

    close(students_to_teacher[1]);

    // Teacher's part
    for (int i = 0; i < n; i++) {
        char buffer[MSG_SIZE];
        read(students_to_teacher[0], buffer, MSG_SIZE);

        int student_pid, wynik;
        sscanf(buffer, "%d %d", &student_pid, &wynik);

        int d = losuj(1, 20) + (i / n + 1) * 10;
        char message[MSG_SIZE];

        if (wynik >= d) {
            snprintf(message, MSG_SIZE, "PASS");
            printf("Teacher: Student %d finished stage %d\n", student_pid, i / n + 1);
        } else {
            snprintf(message, MSG_SIZE, "FAIL");
            printf("Teacher: Student %d needs to fix stage %d\n", student_pid, i / n + 1);
        }

        for (int j = 0; j < n; j++) {
            if (student_pids[j] == student_pid) {
                write(teacher_to_students[j][1], message, strlen(message) + 1);
                break;
            }
        }
    }

    close(students_to_teacher[0]);

    for (int i = 0; i < n; i++) {
        wait(NULL);
    }

    printf("Wszyscy studenci zakonczyli dzialanie\n");
    return 0;
}
