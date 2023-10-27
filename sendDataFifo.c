/* sendDataFifo.c
Exemplu de comunicare intre procese aflate pe aceeasi masina folosind FIFO-uri. Programul curent trimite date intr-un FIFO.

Autor: Lenuta Alboaie <adria@info.uaic.ro> (c)
*/


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define FIFO_NAME "MyTest_FIFO"
#define CLIENT_FIFO "Client_FIFO"



int main()
{
    char s[300];
    int num, fd;



    printf("Astept cititori...\n");

    fd = open(FIFO_NAME, O_WRONLY);
    int fd_client = open(CLIENT_FIFO, O_RDONLY);


    printf("Am un cititor....introduceti ceva..\n");

    while (gets(s), !feof(stdin)) {
        //Comanda quit
         if (strcmp(s, "quit") == 0) {
            printf("Comanda 'quit' detectată. Închidere...\n");
            break; // Opriți bucla dacă comanda este "quit"
        }
        if ((num = write(fd, s, strlen(s))) == -1)
            perror("Problema la scriere in FIFO!");
        else
        {   printf("S-au scris in FIFO %d bytes\n", num);
            const char *prefix = "login : ";
            if (strncmp(s, prefix, strlen(prefix)) == 0) {
                printf("Am primit comanda : '%s'\n", s);

                char mesaj[100];
                read(fd_client, mesaj, sizeof(mesaj));
                printf("Mesaj de la server: %s\n", mesaj);
            }
        }
    }
}
