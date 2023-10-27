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

int verifica_user(char *username) {
    FILE *users;
    char line[100];

    users = fopen("users.txt", "r");

    if(users == NULL) {
        perror("Eroare la deschiderea fisierului");
        return -1;
    }

    while(fgets(line, sizeof(line), users)){
        line[strcspn(line, "\n")] = 0;

        if(strcmp(username, line) == 0) {
            fclose(users);
            return 1;
        }
    }
    fclose(users);
    return 0;
}

int main()
{
    char s[300];
    int num, fd;
    const char *prefix = "login : ";

    int comm_pipe[2];

    if(pipe(comm_pipe) == -1){
        perror("Eroare la crearea pipe-ului");
        exit(1);
    }

    mknod(FIFO_NAME, S_IFIFO | 0666, 0);
    mknod(CLIENT_FIFO, S_IFIFO | 0666, 0);

    printf("Astept sa scrie cineva...\n");
    fd = open(FIFO_NAME, O_RDONLY);
    printf("A venit cineva:\n");

    int fd_client = open(CLIENT_FIFO, O_WRONLY);
    if (fd_client == -1) {
        perror("Eroare la deschiderea FIFO-ului pentru client");
        exit(1);
    }

    do {
        if ((num = read(fd, s, 300)) == -1)
            perror("Eroare la citirea din FIFO!");
        else {
            s[num] = '\0';
            printf("S-au citit din FIFO %d bytes: \"%s\"\n", num, s);
            if(strncmp(s, prefix, strlen(prefix)) == 0) {
                char *username = s + strlen(prefix);
                int pid = fork();
                printf("Valoare pid: %d \n ", pid);
                if(pid == 0) {
                    printf("S-a creat procesul copil pentru comanda login!\n");

                    close(comm_pipe[0]);

                    char *username  = s + strlen(prefix);
                    int rez = verifica_user(username);

                    write(comm_pipe[1], &rez, sizeof(int));

                    close(comm_pipe[1]);
                }
                else if(pid < 0){
                    perror("Eroare la fork");
                    exit(1);
                }
                close(comm_pipe[1]);

                int rez_copil;
                read(comm_pipe[0], &rez_copil, sizeof(int));

                close(comm_pipe[0]);
                if(rez_copil == 1){
                    printf("Success\n");

                    const char *rez
                     = "Success";

                    write(fd_client, rez, strlen(rez));
                }
                else if(rez_copil == 0){
                    printf("Nope\n");
                }
                else {
                    printf("Eroare\n");
                }
            }
        }
    } while (num > 0);

    close(fd_client);
}
