#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
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

void comanda_quit(int *pid_array, int num_pids) {
    // Oprirea proceselor copil
    for (int i = 0; i < num_pids; i++) {
        kill(pid_array[i], SIGTERM); // Trimite semnalul de terminare (SIGTERM) către procesul copil
    }

    // Așteaptă ca toate procesele copil să se termine
    for (int i = 0; i < num_pids; i++) {
        int status;
        waitpid(pid_array[i], &status, 0);
    }

    exit(0);
}
int main()
{
    char s[300];
    int num, fd;
    const char *prefix = "login : ";
    pid_t pid_array[100];
    int num_pids = 0;

    int comm_pipe[2];

    int logged_in = 0;

    if(pipe(comm_pipe) == -1){
        perror("Eroare la crearea pipe-ului");
        exit(1);
    }

    int sock_pair[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sock_pair) == -1) {
        perror("Eroare la crearea socket pair-ului");
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


            //Comanda Login
            if(strncmp(s, prefix, strlen(prefix)) == 0) {
                char *username = s + strlen(prefix);
                int pid_login = fork();
                printf("Valoare pid: %d \n ", pid_login);
                if(pid_login == 0) {
                    printf("S-a creat procesul copil pentru comanda login!\n");

                    char *username  = s + strlen(prefix);
                    int rez = verifica_user(username);
                    if (rez) {
                        logged_in = 1;
                    } else {
                        logged_in = 0;
                    }
                    close(comm_pipe[0]);

                    write(comm_pipe[1], &logged_in, sizeof(int));
                    printf("logged_in : (pc), %d\n",logged_in);

                    close(comm_pipe[1]);
                    exit(0);

                }
                else if (pid_login > 0) {
                    close(comm_pipe[1]);
                    read(comm_pipe[0], &logged_in, sizeof(int));

                }else if(pid_login < 0){
                    perror("Eroare la fork");
                    exit(1);
                }
                int rez_copil;

                printf("logged_in: %d\n", logged_in);


                if(logged_in== 1){
                    printf("Success\n");

                    const char *rez = "Success";
                    write(fd_client, rez, strlen(rez) + 1);
                }
                else if(logged_in== 0){
                    printf("Nope\n");
                    const char *rez0 = "Username not in the file.";
                    write(fd_client, rez0, strlen(rez0) + 1);
                }
                else {
                    printf("Eroare\n");
                }
                  close(comm_pipe[0]);
                  printf("logged_in (login) %d\n", logged_in);
            }


            //Comanda logout
            else if(strcmp(s, "logout") == 0){
                const char *rez = "";

                printf("logged_in (pp) %d\n", logged_in);
                int pid_logout = fork();

                if (pid_logout == 0) {
                    printf("S-a creat procesul copil pentru comanda logout!\n");

                    printf("logged_in (pc) %d\n", logged_in);
                    if(logged_in == 1){
                        logged_in = 0;
                        printf("User logged out succesfully\n");
                        close(comm_pipe[0]);
                        write(comm_pipe[1], &logged_in, sizeof(int));
                        printf("logged_in : (pc), %d\n",logged_in);
                        close(comm_pipe[1]);
                        rez = "User logged out succesfully\n";
                        write(fd_client, rez, strlen(rez));
                        exit(0);
                    }
                    else{
                        printf("No user is logged in\n");
                        rez = "No user is logged in\n";
                        write(fd_client, rez, strlen(rez));
                    }
                }
                else if (pid_logout< 0) {
                    perror("Eroare la fork");
                    exit(1);
                }
    }

//                close(comm_pipe[1]);
//                read(comm_pipe[0], &logged_in, sizeof(int));
//                close(comm_pipe[0]);
//                }
//                else{
//                    printf("No user is logged in\n");
//                    rez = "No user is logged in\n";
//                    write(fd_client, rez, strlen(rez));
//                }
//            }
            //Comanda quit
            else if (strcmp(s, "quit") == 0) {
                int pid_quit = fork();
                if (pid_quit == 0) {

                    printf("S-a creat procesul copil pentru comanda quit!\n");


                    close(comm_pipe[1]);


                    char quit_msg[10];
                    read(comm_pipe[0], quit_msg, sizeof(quit_msg));


                    write(fd_client, quit_msg, strlen(quit_msg) + 1);


                    for (int i = 0; i < num_pids; i++) {
                        kill(pid_array[i], SIGTERM);
                    }


                    for (int i = 0; i < num_pids; i++) {
                        int status;
                        waitpid(pid_array[i], &status, 0);
                    }


                    close(fd);
                    close(fd_client);
                    close(comm_pipe[0]);
                    exit(0);
                }
                else if (pid_quit < 0) {
                    pid_array[num_pids++] = pid_quit;
                }
                else {

                    perror("Eroare la fork");
                    exit(1);
                }
                close(comm_pipe[1]);
            }
            //Comanda invalida
            else {
                const char *invalid_msg = "Comanda invalida! Introduceti alta comanda : ";

                write(fd_client, invalid_msg, strlen(invalid_msg) + 1);
            }

        }
    } while (num > 0);

    close(fd_client);
}
