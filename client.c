#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h>
#include <sys/select.h>

/* codul de eroare returnat de anumite apeluri */
extern int errno;

volatile sig_atomic_t timer_expired = 0;

/* portul de conectare la server*/
int port;

void registerClient(int sd) {
  char msg[100];

  /* citirea mesajului */
  bzero(msg, 100);
  printf("[client] Introduceti un nume: ");
  fflush(stdout);
  read(0, msg, 100);
  char fullMsg[200];
  snprintf(fullMsg, sizeof(fullMsg), "nume: %s", msg);

  /* trimiterea mesajului la server */
  if (write(sd, fullMsg, sizeof(fullMsg)) <= 0)
  {
    perror("[client] Eroare la write() spre server.\n");
    exit(errno);
  }

}

void readyOrNot(int sd) {

    char msg[100];

    if (read(sd, msg, 100) < 0)
    {
    perror("[client] Eroare la read() de la server.\n");
    exit(errno);
    }

    /* afisam mesajul primit */
    printf("[client] Mesajul primit este: %s\n", msg);



    /* citirea mesajului */
    bzero(msg, 100);
    printf("[client] Raspunde cu DA cand esti gata sa incepi! : ");
    fflush(stdout);
    read(0, msg, 100);
    char fullMsg[200];

    snprintf(fullMsg, sizeof(fullMsg), "ready: %s", msg);

    /* trimiterea mesajului la server */
    if (write(sd, fullMsg, 100) <= 0)
    {
      perror("[client] Eroare la write() spre server.\n");
      exit(errno);
    }
}
void remove_newline(char *str) {
    size_t len = strlen(str);
    if (len > 0 && str[len - 1] == '\n') {
        str[len - 1] = '\0';  // Șterge caracterul nou
    }
}




void answer(int sd) {
    char msg[1024];
    char fullMsg[1033];

    fd_set read_fds;
    FD_ZERO(&read_fds);

    while (1) {
        FD_SET(0, &read_fds); // Descriptorul pentru citire de la tastatură
        FD_SET(sd, &read_fds); // Descriptorul pentru citire de la server

        // Setăm timeout la 1 secundă
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int ready_fds = select(sd + 1, &read_fds, NULL, NULL, &timeout);

        if (ready_fds < 0) {
            perror("[client] Eroare la select().\n");
            exit(errno);
        } else if (ready_fds > 0) {
            if (FD_ISSET(0, &read_fds)) {
                // Citire de la tastatură
                bzero(msg, sizeof(msg));
                fflush(stdout);
                fgets(msg, sizeof(msg), stdin);
                remove_newline(msg);

                snprintf(fullMsg, sizeof(fullMsg), "raspuns: %s", msg);

                // Trimitere mesaj la server
                if (write(sd, fullMsg, sizeof(fullMsg)) <= 0) {
                    perror("[client] Eroare la write() spre server.\n");
                    exit(errno);
                }
            }

            if (FD_ISSET(sd, &read_fds)) {
                // Citire de la server
                bzero(msg, sizeof(msg));
                if (read(sd, msg, sizeof(msg)) < 0) {
                    perror("[client] Eroare la read() de la server.\n");
                    exit(errno);
                }

                if (strstr(msg, "expirat") != NULL) {
                    timer_expired = 1;
                }

                if (timer_expired) {
                    printf("Handle timer expiration logic here.\n");

                    timer_expired = 0;

                }

                printf("[client] Mesajul primit este: %s\n", msg);

                // Trimitere comandă "next" la server
                if (write(sd, "next", strlen("next") + 1) <= 0) {
                    perror("[client] Eroare la write() spre server.\n");
                    exit(errno);
                }

                // Citire de la server pentru comanda "next"
                bzero(msg, sizeof(msg));
                if (read(sd, msg, sizeof(msg)) < 0) {
                    perror("[client] Eroare la read() de la server.\n");
                    exit(errno);
                }

                printf("--------------------------------------------------\n");
                printf("[client] Mesajul primit este: %s\n", msg);
            }
        }
    }
}


int main(int argc, char *argv[])
{
  int sd;                      // descriptorul de socket
  struct sockaddr_in server;   // structura folosita pentru conectare

  /* exista toate argumentele in linia de comanda? */
  if (argc != 3)
  {
    printf("[client] Sintaxa: %s <adresa_server> <port>\n", argv[0]);
    return -1;
  }

  /* stabilim portul */
  port = atoi(argv[2]);

  /* cream socketul */
  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("[client] Eroare la socket().\n");
    return errno;
  }

  /* umplem structura folosita pentru realizarea conexiunii cu serverul */
  /* familia socket-ului */
  server.sin_family = AF_INET;
  /* adresa IP a serverului */
  server.sin_addr.s_addr = inet_addr(argv[1]);
  /* portul de conectare */
  server.sin_port = htons(port);

  /* ne conectam la server */
  if (connect(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
  {
    perror("[client] Eroare la connect().\n");
    return errno;
  }

  // Register the client with the server
  registerClient(sd);

  // Ready or not communication
  readyOrNot(sd);
    answer(sd);
  /* inchidem conexiunea, am terminat */
  close(sd);

  return 0;
}
