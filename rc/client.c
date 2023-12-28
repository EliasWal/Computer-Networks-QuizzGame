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


/* codul de eroare returnat de anumite apeluri */
extern int errno;

/* portul de conectare la server*/
int port;

void receiveAndDisplayTimer(int sd) {
    char timer[50];
    int bytes_received;

    while (1) {
        // Primește timpul rămas de la server
        bytes_received = recv(sd, timer, sizeof(timer), 0);
        if (bytes_received <= 0) {
            perror("[client] Eroare la recv() de la server.\n");
            exit(errno);
        }

        // Afișează timpul rămas pe aceeași linie
        printf("%s", timer);
        fflush(stdout);

        // Ieși din buclă dacă timpul rămas este 0
        if (strstr(timer, "0 secunde") != NULL) {
            break;
        }

        // Așteaptă un moment pentru a nu bloca prea mult consola
        sleep(1);
        // Șterge linia curentă pentru a afișa timpul rămas actualizat
        printf("\r");
        fflush(stdout);
    }
}


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
/* citirea raspunsului dat de server (apel blocant pana cand serverul raspunde) */
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


    /* citirea raspunsului dat de server (apel blocant pana cand serverul raspunde) */
//      system("clear");




}
void remove_newline(char *str) {
    size_t len = strlen(str);
    if (len > 0 && str[len - 1] == '\n') {
        str[len - 1] = '\0';  // Șterge caracterul nou
    }
}



void answer(int sd) {
    char msg[1024];
    char fullMsg[1024];
    // citirea raspunsului dat de server
    bzero(msg, sizeof(msg));
    if (read(sd, msg, sizeof(msg)) < 0) {
        perror("[client] Eroare la read() de la server.\n");
        exit(errno);
    }
    printf("[client] Mesajul primit este: %s\n", msg);

    while (1) {
        // citirea mesajului
        bzero(msg, sizeof(msg));
        printf("[client] Raspunde cu a, b sau c: ");
        fflush(stdout);
        fgets(msg, sizeof(msg), stdin);
        remove_newline(msg);

        snprintf(fullMsg, sizeof(fullMsg), "raspuns: %s", msg);

        // trimiterea mesajului la server
        if (write(sd, fullMsg, sizeof(fullMsg)) <= 0) {
            perror("[client] Eroare la write() spre server.\n");
            exit(errno);
        }

        // citirea raspunsului dat de server
        bzero(msg, sizeof(msg));
        if (read(sd, msg, sizeof(msg)) < 0) {
            perror("[client] Eroare la read() de la server.\n");
            exit(errno);
        }

        // afisam mesajul primit
        printf("[client] Mesajul primit este: %s\n", msg);

        if (write(sd, "next", 4) <= 0) {
            perror("[client] Eroare la write() spre server.\n");
            exit(errno);
        }

        // citirea raspunsului dat de server
        bzero(msg, sizeof(msg));
        if (read(sd, msg, sizeof(msg)) < 0) {
            perror("[client] Eroare la read() de la server.\n");
            exit(errno);
        }

        printf("--------------------------------------------------\n");
        printf("[client] Mesajul primit este: %s\n", msg);
    }
}

//
//void answer(int sd) {
//  char msg[1024];
//    if (read(sd, msg, 1024) < 0)
//        {
//          perror("[client] Eroare la read() de la server.\n");
//          exit(errno);
//        }
//
//
//    /* afisam mesajul primit */
//    printf("[client] Mesajul primit este: %s\n", msg);
//
//    while(1){
//
//        /* citirea mesajului */
//        bzero(msg, 100);
//        printf("[client] Raspunde cu a, b sau c: ");
//        fflush(stdout);
//        read(0, msg, 1024);
//        char fullMsg[1024];
//
//        snprintf(fullMsg, sizeof(fullMsg), "raspuns: %s", msg);
//
//        /* trimiterea mesajului la sserver */
//        if (write(sd, fullMsg, 100) <= 0)
//        {
//          perror("[client] Eroare la write() spre server.\n");
//          exit(errno);
//        }
//
//        /* citirea raspunsului dat de server (apel blocant pana cand serverul raspunde) */
//        if (read(sd, msg, 100) < 0)
//        {
//          perror("[client] Eroare la read() de la server.\n");
//          exit(errno);
//        }
//        /* afisam mesajul primit */
//        printf("[client] Mesajul primit este: %s\n", msg);
//
//        if (write(sd, "next", 100) <= 0)
//        {
//          perror("[client] Eroare la write() spre server.\n");
//          exit(errno);
//        }
//
//        if (read(sd, msg, 100) < 0)
//        {
//          perror("[client] Eroare la read() de la server.\n");
//          exit(errno);
//
//        }
//        printf("--------------------------------------------------\n");
//        printf("[client] Mesajul primit este: %s\n", msg);
//
//    }
//}

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
