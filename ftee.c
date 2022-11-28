/* ftee - clone stdin to stdout and to a named pipe 
original: https://stackoverflow.com/questions/7360473/linux-non-blocking-fifo-on-demand-logging
(c) racic@stackoverflow
bdvstg modified at 2022-11-28
WTFPL Licence

compile:
gcc ftee.c -o /usr/bin/ftee
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>

#define PERROR(...) fprintf(stderr, __VA_ARGS__)

#define MAX_NUMS_FILE (5)
int fd[MAX_NUMS_FILE];
int nums_fd = 0;


int add_fd(char *filename) {
    int readfd, writefd;
    struct stat status;
    readfd = open(filename, O_RDONLY | O_NONBLOCK);
    if(-1==readfd)
    {
        PERROR("ftee: can not open %s\n", filename);
        exit(EXIT_FAILURE);
    }

    if(-1==fstat(readfd, &status))
    {
        PERROR("ftee: fstat to %s fail\n", filename);
        close(readfd);
        exit(EXIT_FAILURE);
    }

    if(!S_ISFIFO(status.st_mode))
    {
        PERROR("ftee: %s in not a fifo!\n", filename);
        close(readfd);
        exit(EXIT_FAILURE);
    }

    writefd = open(filename, O_WRONLY | O_NONBLOCK);
    if(-1==writefd)
    {
        PERROR("ftee: no write permission to %s\n", filename);
        close(readfd);
        exit(EXIT_FAILURE);
    }

    close(readfd);

    return writefd;
}


int main(int argc, char *argv[])
{
    char buffer[BUFSIZ];

    signal(SIGPIPE, SIG_IGN);

    if(argc < 2 || argc > (MAX_NUMS_FILE+1))
    {
        printf("Usage:\n someprog 2>&1 | %s FIFO1 FIFO2\n FIFO - path to a"
            " named pipe, required argument, allowed MAX FIFO files are %d\n", argv[0], MAX_NUMS_FILE);
        printf("\nusage example:\n");
        printf("    terminal 1:\n");
        printf("        mkfifo /dev/ttyUSB0-tee1 /dev/ttyUSB0-tee2\n");
        printf("        cat /dev/ttyUSB0 | ftee /dev/ttyUSB0-tee1 /dev/ttyUSB0-tee2 >/dev/null &\n");
        printf("    terminal 2:\n");
        printf("        cat /dev/ttyUSB0-tee1\n");
        printf("    terminal 3:\n");
        printf("        cat /dev/ttyUSB0-tee2\n");
        exit(EXIT_FAILURE);
    }

    for(int i = 1; i < argc ; i++) {
        fd[nums_fd] = add_fd(argv[i]);
        nums_fd++;
    }

    //printf("nums_fd = %d\n", nums_fd);

    while(1)
    {
        int n_read = read(STDIN_FILENO, buffer, sizeof(buffer));
        if (n_read < 0 && errno == EINTR)
            continue;
        if (n_read <= 0)
            break;

        int n_stdout = write(STDOUT_FILENO, buffer, n_read);
        if(-1==n_stdout)
            PERROR("ftee: error when writing to stdout");
        for(int i = 0 ; i < nums_fd ; i++) {
            int n = write(fd[i], buffer, n_read);
            if(-1==n);//Ignoring the errors
            //printf("%d:%d, ", i, n);fflush(stdout);
        }
    }
    for(int i = 0 ; i < nums_fd ; i++)
        close(fd[i]);
    return(0);
}


