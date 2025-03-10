#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

#define MSG_SIZE 50

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

            char buffer[MSG_SIZE];
            read(teacher_to_students[i][0], buffer, MSG_SIZE);
            printf("Student %d (PID %d): Otrzymał wiadomość -> %s\n", i + 1, getpid(), buffer);

            char response[MSG_SIZE];
            snprintf(response, MSG_SIZE, "Student %d (PID %d): HERE", i + 1, getpid());
            write(students_to_teacher[1], response, strlen(response) + 1);            


            close(teacher_to_students[i][0]);
            close(students_to_teacher[1]);
            exit(0);
        }else{
            student_pids[i] = pid;
            //dodajemy tutaj
            close(teacher_to_students[i][0]);
        }
    }

    close(students_to_teacher[1]);

    for (int i = 0; i < n; i++) {
        //close(teacher_to_students[i][0]);
        char message[MSG_SIZE];
        snprintf(message, MSG_SIZE, "Teacher: Is student %d (PID %d) here?", i + 1, student_pids[i]);
        write(teacher_to_students[i][1], message, strlen(message) + 1);
        printf("Teacher: Wysłano do studenta %d -> %s\n", i + 1, message); 
        close(teacher_to_students[i][1]);
    }

    for (int i = 0; i < n; i++) {
        char buffer[MSG_SIZE];
        read(students_to_teacher[0], buffer, MSG_SIZE);
        printf("Teacher: Otrzymano od studenta -> %s\n", buffer);
    }

    close(students_to_teacher[0]);

    for (int i = 0; i < n; i++) {
        wait(NULL);
    }

    printf("Wszyscy studenci zakonczyli dzialanie\n");
    return 0;
}
