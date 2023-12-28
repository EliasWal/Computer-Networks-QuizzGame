//gcc -o serv serv.c cJSON/build/libcjson.a -lm -lsqlite3
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include "cJSON/cJSON.h"
#include "JSON_Functions.h"
#include <sqlite3.h>
#include "SQLite_Functions.h"
#include <signal.h>


#define DB_FILE "questions.db"
#define PORT 2728
#define MAXTHREADS 1000000
#define MAX_Questions 15

time_t start_time;
int time_remaining;
int time_expired;

int currentQuestion = 1;

int client_fds[MAXTHREADS];
int client_scores[MAXTHREADS];

pthread_t threads[MAXTHREADS];
pthread_mutex_t mutex;
pthread_barrier_t barrier;  // Barieră pentru a sincroniza clienții

void handler(int signo) {
    printf("Timpul a expirat!\n");
    exit(EXIT_FAILURE);
}

int winnerAnnouncment(int* client_scores) {
    int maximum = -1;
    int winner = -1;

    for (int i = 0; i < MAXTHREADS; ++i) {
        if (client_scores[i] > maximum) {
            maximum = client_scores[i];
        }
    }
    printf("%d",maximum);

    for (int i = 0; i < MAXTHREADS; ++i) {
        if (client_scores[i] == maximum) {
            winner = i;

        }
    }

    return winner;
}



int updateScore(int client_fd, int delta) {
    int current_score = 0;
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < MAXTHREADS; ++i) {
        if (client_fds[i] == client_fd) {
            client_scores[i] += delta;
            current_score = client_scores[i];
            break;
        }
    }

    pthread_mutex_unlock(&mutex);
    return current_score;
}

void askQuestion(int client_fd, int i){
//    time_remaining = 20;
//    time_expired = 0;
//    start_time = time(NULL);

    // Trimite întrebarea către client
    pthread_mutex_lock(&mutex);
    char question[256];
    char* rez = displayQuestion(currentQuestion, 1024);
    send(client_fd, rez, strlen(rez), 0);
    pthread_mutex_unlock(&mutex);

}

void updateTimer(int client_fd,pthread_mutex_t* mutex) {
    while (time_remaining > 0 && !time_expired) {
        // Actualizează timpul rămas pe client
        pthread_mutex_lock(mutex);
        char timer_update[50];
        printf(timer_update, "\rTimp ramas: %d secunde", time_remaining);
//        send(client_fd, timer_update, strlen(timer_update), 0);
        pthread_mutex_unlock(mutex);

        // Așteaptă un moment
        sleep(1);

        // Scade timpul rămas
        time_t current_time = time(NULL);
        time_remaining = 20 - (current_time - start_time);
        printf("%d", time_remaining);

    }

    // Resetarea flag-ului pentru timp expirat
    time_expired = 0;
}

void remove_newline(char *str) {
    size_t len = strlen(str);
    if (len > 0 && str[len - 1] == '\n') {
        str[len - 1] = '\0';  // Șterge caracterul nou
    }
}

void* handle_client(void* arg) {
    int client_fd = *((int*)arg);
    int total_questions = 0;

    int i = currentQuestion;
    char buffer[1024];
    int bytes_received;

    while(total_questions<MAX_Questions){
        // Primeste date de la client
        bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            // Eroare sau clientul s-a deconectat
            printf("Clientul cu descriptorul %d s-a deconectat.\n", client_fd);
            shutdown(client_fd, SHUT_RDWR);
            // Excludere client din vectorul de descriptori
            pthread_mutex_lock(&mutex);
            close(client_fd);
            for (int i = 0; i < MAXTHREADS; ++i) {
                if (client_fds[i] == client_fd) {
                    client_fds[i] = -1;
                    client_scores[i] = 0;

                    break;
                }
            }
            pthread_mutex_unlock(&mutex);
//            const char* filename = "clients.json";
//            removeClientByFd(filename, client_fd);
//            printf("Client %d: ", client_fd);
//            printf("Client with fd %d removed from JSON.\n", client_fd);
//
            break;  // Iesire din bucla la deconectare
        } else
        {
            // Procesează datele de la client
            buffer[bytes_received] = '\0'; // Adauga terminatorul la sfarsitul datelor primite
            printf("De la clientul %d: %s\n", client_fd, buffer);

            const char *filename = "clients.json";
            if (strstr(buffer, "quit") != NULL){
                printf("Clientul cu descriptorul %d s-a deconectat.\n", client_fd);
                pthread_mutex_lock(&mutex);
                for (int i = 0; i < MAXTHREADS; ++i) {
                    if (client_fds[i] == client_fd) {
                        client_fds[i] = -1;
                        break;
                    }
                }
                pthread_mutex_unlock(&mutex);
                break;
            }
            else
            if (strstr(buffer, "nume") != NULL) {
                addClient(filename, client_fd, buffer);
                pthread_mutex_lock(&mutex);
                const char* response = "Te-ai conectat cu succes! Incepem jocul?";
                send(client_fd, response, strlen(response), 0);
                pthread_mutex_unlock(&mutex);
            }
            else
            if (strstr(buffer, "ready") != NULL){
                // Trimite un mesaj de la server către client
                char response[100];
                // Procesează răspunsul de la client
                buffer[bytes_received] = '\0'; // Adauga terminatorul la sfarsitul datelor primite
                remove_newline(buffer);

                printf("Clientul %d a raspuns: %s\n", client_fd, buffer);
                if (strstr(buffer, "DA") !=NULL) {
//                    sprintf(response, "Ready to play!\n");
//                    printf("%s", response);
//                    send(client_fd, response, strlen(response), 0);
                    printf("Clientul %d a inceput jocul.\n", client_fd);
                    currentQuestion = i;

                    askQuestion(client_fd, i);

//                    updateTimer(client_fd,&mutex);
                }
            }
            else
            if (strstr(buffer, "raspuns") != NULL) {


                char response[100];
                // Căutați începutul șirului "raspuns"
                const char *start = strstr(buffer, "raspuns: ");
                if (start != NULL) {
                    // Extrageți litera care urmează după "raspuns"
                    char answer = start[strlen("raspuns: ")];

                    printf("Clientul %d a raspuns cu litera '%c' la intrebarea %d. \n", client_fd, answer, i);

                    char* right_answer = getCorrectAnswer(i);
                    int current_score = 0;

                    if (strncmp(&answer, right_answer, 1) != 0) {
                        current_score = updateScore(client_fd, 0); // Scorul rămâne neschimbat la răspuns greșit
                        sprintf(response, "Raspuns gresit! Scorul tau actual: %d\n", current_score);
                        printf("Raspuns gresit! Scorul tau actual: %d\n\n", current_score);
                        printf("--------------------------------------------------\n\n");
                        send(client_fd, response, strlen(response), 0);
                    } else {
                        current_score = updateScore(client_fd, 1); // Actualizează scorul cu 1 punct la răspuns corect
                        sprintf(response, "Raspuns corect! Scorul tau actual: %d\n", current_score);
                        printf("Raspuns corect!Scorul tau actual: %d\n\n", current_score);
                        send(client_fd, response, strlen(response), 0);
                    }
                } else {
                    printf("Nu s-a gasit inceputul răspunsului.\n");
                }
            }

            else
            if (strstr(buffer, "next") != NULL){
                i++;
                                char response[100];
                if (i > MAX_Questions) {
                    int winner = winnerAnnouncment(client_scores);
                    sprintf(response,"Clientul cu descriptorul %d a castigat! Si tu ai pierdut ca prostu :D", winner);
//                    strcpy(response, "Ati pierdut!");

                    send(client_fd, response, strlen(response), 0);
                    shutdown(client_fd, SHUT_RDWR);
                    close(client_fd);

                    break;
                }
                currentQuestion = i;

                askQuestion(client_fd, i);

//                updateTimer(client_fd,&mutex);
            }
        }
    }
//        printf("Clientul %d a terminat toate cele %d intrebari. Scor final: %d\n", client_fd, MAX_Questions, client_scores[i]);

}

void initialize_server() {
    int server_fd, client_fd, i;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;

    // Creare socket pentru server
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Eroare la crearea socket-ului");
        exit(EXIT_FAILURE);
    }

    // Inițializare structura server_addr
    bzero((char*)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Legare socket la adresa și port
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Eroare la legarea socket-ului");
        exit(EXIT_FAILURE);
    }

    // Ascultare pentru conexiuni
    listen(server_fd, 5);

    // Inițializare vector de descriptori de clienti
    pthread_mutex_init(&mutex, NULL);
    pthread_barrier_init(&barrier, NULL, MAXTHREADS);
    for (i = 0; i < MAXTHREADS; ++i) {
        client_fds[i] = -1;
        client_scores[i] = 0;
    }

    printf("Serverul asteapta conexiuni la portul %d...\n", PORT);

    while (1) {
        // Accepta conexiunea de la un client
        client_len = sizeof(client_addr);
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("Eroare la acceptarea conexiunii");
            exit(EXIT_FAILURE);
        }

        // Adauga noul client la vectorul de descriptori
        pthread_mutex_lock(&mutex);
        for (i = 0; i < MAXTHREADS; ++i) {
            if (client_fds[i] == -1) {
                client_fds[i] = client_fd;
                pthread_create(&threads[i], NULL, handle_client, (void*)&client_fds[i]);
                printf("Clientul cu descriptorul %d s-a conectat.\n", client_fd);

                break;
            }
        }
        pthread_mutex_unlock(&mutex);
    }

    pthread_barrier_destroy(&barrier);
    pthread_mutex_destroy(&mutex);
    close(server_fd);
}



int main() {
    const char *filename = "clients.json";
    deleteFileContent(filename);
    initializeDatabase();
//    addQuestions();
//    deleteDatabaseContent();
//    resetAutoIncrement();
//    displayAllQuestions(5024);
    initialize_server();


    return 0;
}
