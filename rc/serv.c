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
#define MAX_Questions 20

time_t start_time;
int time_remaining;
int time_expired;

int client_fds[MAXTHREADS];
pthread_t threads[MAXTHREADS];
pthread_mutex_t mutex;
pthread_barrier_t barrier;  // Barieră pentru a sincroniza clienții

void handler(int signo) {
    printf("Timpul a expirat!\n");
    exit(EXIT_FAILURE);
}

void askQuestion(int client_fd, int i){
//    time_remaining = 20;
//    time_expired = 0;
//    start_time = time(NULL);

    char rasp_corect[MAX_Questions] = {'a', 'b', 'c', 'd', 'a', 'a', 'a', 'c', 'a', 'b', 'a', 'b', 'c', 'd', 'a', 'a', 'a', 'c', 'a', 'b'};

    // Trimite întrebarea către client
    pthread_mutex_lock(&mutex);
    char question[256];
//    sprintf(question, "Intrebarea %d \n Variante de raspuns: \n a)... \n b) ... \n c) ... \n d) ... \n", i);
    char *rez = displayQuestion(i,1024);
//    printf("Intrebarea curenta: %d\n", i);
//    printf("Raspuns corect: %c\n", rasp_corect[i]);
    send(client_fd, rez, strlen(rez), 0);
//    send(client_fd, question, strlen(question), 0);
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
    int i=1;
    char buffer[1024];
    int bytes_received;
    char rasp_corect[MAX_Questions] = {'a', 'b', 'c', 'd', 'a', 'a', 'a', 'c', 'a', 'b', 'a', 'b', 'c', 'd', 'a', 'a', 'a', 'c', 'a', 'b'};
    while(1){
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


                    if (strncmp(&answer, right_answer, 1) != 0) {
                        sprintf(response, "Raspuns gresit!\n");
                        printf("Raspuns gresit!\n\n");
                        printf("--------------------------------------------------\n\n");
                        send(client_fd, response, strlen(response), 0);
                    } else {
                        sprintf(response, "Raspuns corect!\n");
                        printf("Raspuns corect!\n\n");
                        printf("--------------------------------------------------\n\n");
                        send(client_fd, response, strlen(response), 0);
                    }
                } else {
                    printf("Nu s-a gasit inceputul răspunsului.\n");
                }
            }

            else
            if (strstr(buffer, "next") != NULL){
                i++;
askQuestion(client_fd, i);

//                updateTimer(client_fd,&mutex);
            }
        }
    }
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
//    deleteFileContent(filename);
    initializeDatabase();
//    addQuestions();
//    deleteDatabaseContent();
//    resetAutoIncrement();
    displayAllQuestions(5024);
    initialize_server();

    return 0;
}
