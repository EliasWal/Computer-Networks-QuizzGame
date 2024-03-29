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


#define DB_FILE "questions.db"
#define PORT 2728
#define MAXTHREADS 1000000
#define MAX_Questions 15
#define MAX_Time 15


int currentQuestion = 1;
int time_remaining;
int client_fds[MAXTHREADS];
int client_scores[MAXTHREADS];

pthread_t threads[MAXTHREADS];
pthread_mutex_t mutex;
pthread_barrier_t barrier;  // Barieră pentru a sincroniza clienții


pthread_t timer_thread;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t timer_mutex = PTHREAD_MUTEX_INITIALIZER;

int timer_started = 0;  
int remaining_time = MAX_Time;  // Initial timer value in seconds
int timer_expired = 0;
int ready_clients=0;
int current_client_fd = -1;

int remainingTime() {
    pthread_mutex_lock(&mutex);
    int current_remaining_time = remaining_time;
    pthread_mutex_unlock(&mutex);

    return current_remaining_time;
}

void waitFor(int seconds) {
    while (seconds > 0) {
        sleep(1);
        seconds--;

        pthread_mutex_lock(&mutex);
        int current_remaining_time = remaining_time;
        pthread_mutex_unlock(&mutex);

        printf("Waiting... Remaining time: %d seconds\n", current_remaining_time);
    }
}

void *timer_function(void *arg) {
    while (1) {
        sleep(1);

        pthread_mutex_lock(&timer_mutex);
        pthread_mutex_lock(&mutex);
        remaining_time--;

        if (remaining_time <= 0) {
            printf("Timer expired!\n");
            timer_expired = 1;
            remaining_time = MAX_Time;

            current_client_fd = -1;
            for (int i = 0; i < MAXTHREADS; ++i) {
                if (client_fds[i] != -1) {
                    current_client_fd = client_fds[i];
                    break;
                }
            }

            pthread_mutex_unlock(&mutex);
            pthread_mutex_unlock(&timer_mutex);

            if (current_client_fd != -1) {
    
                printf("Sending message to client about timer expiration.\n");
                const char *timeout_message = "Timpul a expirat! Te rog raspunde la intrebare.";
                send(current_client_fd, timeout_message, strlen(timeout_message), 0);
    
            }
        } else {
            pthread_mutex_unlock(&mutex);
            pthread_mutex_unlock(&timer_mutex);
        }
    }

    return NULL;
}

int findWinner(int* scores, int numParticipants) {
    int maxScore = -1;
    int winnerIndex = -1;

    for (int i = 0; i < numParticipants; ++i) {
        if (scores[i] > maxScore) {
            maxScore = scores[i];
            winnerIndex = i;
        }
    }

    return winnerIndex;
}



int updateScore(int client_fd, int delta) {
    int current_score = 0;
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < MAXTHREADS; ++i) {
        if (client_fds[i] == client_fd) {
            client_scores[i] += delta;
            current_score = client_scores[i];
            // Update the score in the JSON file
            // updateScoreInJson("clients.json", client_fd, current_score);
            break;
        }
    }
    pthread_mutex_unlock(&mutex);

    return current_score; // Return the updated score
}



void askQuestion(int client_fd, int i){
    // Trimite întrebarea către client
    pthread_mutex_lock(&mutex);
    char question[256];
    char* rez = displayQuestion(currentQuestion, 1024);
    send(client_fd, rez, strlen(rez), 0);
    pthread_mutex_unlock(&mutex);
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
            const char* filename = "clients.json";
            removeClientByFd(filename, client_fd);
            printf("Clientul cu descriptorul %d eliminat din fisierul JSON.\n", client_fd);
            break;
        } else
        {
            // Procesează datele de la client
            buffer[bytes_received] = '\0';
            printf("De la clientul %d: %s\n", client_fd, buffer);

            if (timer_expired && client_fd == current_client_fd) {
                // Trimite un mesaj către client că timpul a expirat
                // printf("Sending message to client about timer expiration.\n");
                // const char *timeout_message = "Timpul a expirat! Te rog raspunde la intrebare.";
                // send(client_fd, timeout_message, strlen(timeout_message), 0);

                // Resetează variabilele pentru următorul ciclu
                timer_expired = 0;
                current_client_fd = -1;
            }

            const char *filename = "clients.json";
            if (strstr(buffer, "quit") != NULL){
                printf("Clientul cu descriptorul %d s-a deconectat.\n", client_fd);
                pthread_mutex_lock(&mutex);
                if(ready_clients!=0){
                    ready_clients--;
                    currentQuestion = 1;
                }
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
                ready_clients++;
                // Trimite un mesaj de la server către client
                char response[100];
                // Procesează răspunsul de la client
                buffer[bytes_received] = '\0';
                remove_newline(buffer);

                printf("Clientul %d a raspuns: %s\n", client_fd, buffer);
                if (strstr(buffer, "DA") !=NULL || strstr(buffer, "Da") !=NULL || strstr(buffer, "da") !=NULL ) {
                    printf("Clientul %d a inceput jocul.\n", client_fd);
                    
                    pthread_mutex_lock(&mutex);
                    current_client_fd = client_fd;
                    pthread_mutex_unlock(&mutex);
                    
                    currentQuestion = i;
                    askQuestion(client_fd, i);
                    
                    
                    if (ready_clients == 1) {
                        pthread_mutex_lock(&timer_mutex);
                        if (!timer_started) {
                            timer_started = 1;  
                            pthread_create(&timer_thread, NULL, timer_function, NULL);
                        }
                        pthread_mutex_unlock(&timer_mutex);
                        while (!timer_expired) {
                            sleep(1);
                            pthread_mutex_lock(&mutex);
                            int current_remaining_time = remaining_time;
                            pthread_mutex_unlock(&mutex);
                            printf("Timer thread: Remaining time: %d seconds\n", current_remaining_time);
                        }

                        // timer_expired = 0;
                    }
                    else {
                        int rem_time = remainingTime();
                        waitFor(rem_time);
                    }
                                        
                }
                else {
                    printf("Clientul %d nu a inceput jocul. Acesta va fi eliminat :(\n", client_fd);
                    printf("Clientul cu descriptorul %d a fost deconectat.\n", client_fd);
                    pthread_mutex_lock(&mutex);
                    if(ready_clients!=0){
                        ready_clients--;
                        currentQuestion = 1;
                    }
                    for (int i = 0; i < MAXTHREADS; ++i) {
                        if (client_fds[i] == client_fd) {
                            client_fds[i] = -1;
                            break;
                        }
                    }
                    pthread_mutex_unlock(&mutex);
                    break;
                    }
            }
            else
            if (strstr(buffer, "raspuns") != NULL) {


                char response[100];
                const char *start = strstr(buffer, "raspuns: ");
                if (start != NULL) {
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
                timer_expired=0;
            }

            else
            if (strstr(buffer, "next") != NULL){
                i++;

                printf("Primita comanda 'next' de la client.\n");

                char response[100];
                if (i > MAX_Questions) {
                    int winnerIndex = findWinner(client_scores, MAXTHREADS);
                    char* winnerName= extractNameByDescriptor(client_fds[winnerIndex], "clients.json");
                    if (winnerIndex != -1) {
                        printf("Câștigătorul este %s\n.", winnerName);
                        sprintf(response,"Câștigătorul este %s\n.", winnerName);
                    } else {
                        sprintf(response,"Niciun câștigător. Scoruri egale sau niciun participant.\n");
                    }

                    currentQuestion = 1;
                    send(client_fd, response, strlen(response), 0);
//                    shutdown(client_fd, SHUT_RDWR);
//                    close(client_fd);

                    break;
                }
                currentQuestion = i;

                askQuestion(client_fd, i);
                if (ready_clients == 1) {
                        printf("Timer started?: %d\n", timer_started);
                        pthread_mutex_lock(&timer_mutex);
                        if (!timer_started) {
                            timer_started = 1; 
                            pthread_create(&timer_thread, NULL, timer_function, NULL);
                        }
                        pthread_mutex_unlock(&timer_mutex);
                        while (!timer_expired) {
                            sleep(1);
                            pthread_mutex_lock(&mutex);
                            int current_remaining_time = remaining_time;
                            pthread_mutex_unlock(&mutex);
                            printf("Timer thread: Remaining time: %d seconds\n", current_remaining_time);
                        }
                    }
                    else {
                        int rem_time = remainingTime();
                        waitFor(rem_time);
                    }
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
    pthread_join(timer_thread, NULL);
    deleteFileContent(filename);
    initializeDatabase();
//    addQuestions();
//    deleteDatabaseContent();
//    resetAutoIncrement();
//    displayAllQuestions(5024);
//    timer(10);
    

    initialize_server();


    return 0;
}
