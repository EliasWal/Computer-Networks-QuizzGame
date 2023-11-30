#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>

#define PORT 2728
#define MAXTHREADS 1000000
#define MAX_Questions 20


int client_fds[MAXTHREADS];
pthread_t threads[MAXTHREADS];
pthread_mutex_t mutex;
pthread_barrier_t barrier;  // Barieră pentru a sincroniza clienții

void remove_newline(char *str) {
    size_t len = strlen(str);
    if (len > 0 && str[len - 1] == '\n') {
        str[len - 1] = '\0';  // Șterge caracterul nou
    }
}


void* handle_client(void* arg) {
    int client_fd = *((int*)arg);
    char buffer[256];
    int bytes_received;
    char rasp_corect[MAX_Questions] = {'a', 'b', 'c', 'd', 'a', 'a', 'a', 'c', 'a', 'b', 'a', 'b', 'c', 'd', 'a', 'a', 'a', 'c', 'a', 'b'};

    // Primeste date de la client
    bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);
    if (bytes_received <= 0) {
        // Eroare sau clientul s-a deconectat
        printf("Clientul cu descriptorul %d s-a deconectat.\n", client_fd);

        // Excludere client din vectorul de descriptori
        pthread_mutex_lock(&mutex);
        for (int i = 0; i < MAXTHREADS; ++i) {
            if (client_fds[i] == client_fd) {
                client_fds[i] = -1;
                break;
            }
        }
        pthread_mutex_unlock(&mutex);

//        break;  // Iesire din bucla la deconectare
    } else {
        // Procesează datele de la client
        buffer[bytes_received] = '\0'; // Adauga terminatorul la sfarsitul datelor primite
        printf("De la clientul %d: %s\n", client_fd, buffer);

        // Trimite un mesaj de la server către client
        pthread_mutex_lock(&mutex);
        const char* response = "Te-ai conectat cu succes! Incepem jocul?";
        send(client_fd, response, strlen(response), 0);
        pthread_mutex_unlock(&mutex);
        bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
                // Eroare sau clientul s-a deconectat
                printf("Clientul cu descriptorul %d s-a deconectat.\n", client_fd);

            } else {
        buffer[bytes_received] = '\0'; // Adauga terminatorul la sfarsitul datelor primite
        printf(buffer);
        if (strcmp(buffer, "DA\0") == 0) {
    printf("Da ma da\n");
}}

        printf("Clientul %d a inceput jocul.\n", client_fd);
    int i = 0;
         while (i < MAX_Questions) {
            i++;

            // Trimite întrebarea către client
            pthread_mutex_lock(&mutex);
            char question[256];
            sprintf(question, "Intrebarea %d \n Variante de raspuns: \n a)... \n b) ... \n c) ... \n d) ... \n", i);
            printf("Intrebarea curenta: %d\n", i);
            printf("Raspuns corect: %c\n", rasp_corect[i]);
            send(client_fd, question, strlen(question), 0);
            pthread_mutex_unlock(&mutex);

            // Așteaptă răspunsul de la client
            bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);
            if (bytes_received <= 0) {
                // Eroare sau clientul s-a deconectat
                printf("Clientul cu descriptorul %d s-a deconectat.\n", client_fd);
                break;
            } else {
                char response[100];
                // Procesează răspunsul de la client
                buffer[bytes_received] = '\0'; // Adauga terminatorul la sfarsitul datelor primite
                remove_newline(buffer);
                printf("Clientul %d a raspuns %s la intrebarea %d. \n", client_fd, buffer, i);
                if (buffer[0] != rasp_corect[i]){
                    sprintf(response,"Raspuns gresit!\n");
                    printf("Raspuns gresit!\n");
                    send(client_fd, response, strlen(response), 0);
                }
                else {
                    sprintf(response,"Raspuns corect!\n");
                    printf("Raspuns corect!\n");
                    send(client_fd, response, strlen(response), 0);
                }
                // Așteaptă mesajul de la server
                bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);
                if (bytes_received <= 0) {
                    // Eroare sau clientul s-a deconectat
                    printf("Clientul cu descriptorul %d s-a deconectat.\n", client_fd);
                    break;
                }
                buffer[bytes_received] = '\0'; // Adauga terminatorul la sfarsitul datelor primite
                printf("Mesaj de la client: %s\n", buffer);

                char wait_message[100];
                sprintf(wait_message, "Asteapta urmatoarea intrebare...\n");
                send(client_fd, wait_message, strlen(wait_message), 0);
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
    initialize_server();
    return 0;
}
