#include <sys/ioctl.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include "salausmoduuli_macros.h"

char buffer_rcv[MSG_LEN];
char buffer_snd[MSG_LEN] = "(Nothing sent)";
char key[MAX_KEY_LEN];


int read_from(const int fd)
{
    memset(buffer_rcv, 0, MSG_LEN);
    int check;

    check = read(fd, buffer_rcv, MSG_LEN-1);

    if(check < 0) {
        printf("Read failed \n");
        return errno;
    }
    printf("\nMessage of %i chars received : %s \n\n",check, buffer_rcv);
    //printf("Previous message sent : %s  \n\n", buffer_snd);

    return 0;

}

int write_to(const int fd)
{
    memset(buffer_snd, 0, MSG_LEN);
    int check;

    printf("give the secret message: \n");
    scanf("%255[^\n]s", buffer_snd);
    //lets clear extra stuff from stdin
    int c;
    while((c = getchar()) != '\n' && c != EOF);

    check = write(fd, buffer_snd, strlen(buffer_snd));


    if(check < 0) {
        printf("Write failed \n");
        memset(buffer_snd, 0, MSG_LEN);
        return errno;
    }

    return 0;
}

int encrypt(const int fd)
{
    int check;
    memset(key, 0, MAX_KEY_LEN);

    printf("give a key for encryption (max size %i)\n", MAX_KEY_LEN );
    scanf("%25[^\n]s", key);

    //lets clear extra stuff from stdin
    int c;
    while((c = getchar()) != '\n' && c != EOF);

    check = ioctl(fd, SET_LEN, strlen(key));
	if(check < 0)
        return errno;

    check = ioctl(fd, ENCRYPT, key);

    if(check < 0)
        return errno;

    return 0;

}


int decrypt(const int fd)
{
    int check;

    memset(key, 0, MAX_KEY_LEN);
    printf("give a key for decryption (max size %i)\n", MAX_KEY_LEN );
    scanf("%25[^\n]s", key);

    //lets clear extra stuff from stdin
    int c;
    while((c = getchar()) != '\n' && c != EOF);

    check = ioctl(fd, SET_LEN, strlen(key));
	if(check < 0)
        return errno;
    check = ioctl(fd, DECRYPT, key);
    if(check < 0)
        return errno;

    return 0;

}

void help_print()
{
    printf("q = quit, h = help, r = read, w = write, e = encrypt, d = decrypt\n");
}

int main (int argc, char** argp)
{
    int fd;
    int check = 0;
    char* key;
    char cmd;
    //works only for modules named salausmoduuli
    fd = open("/dev/salausmoduuli", O_RDWR);

    if(fd < 0) {

        printf("Did you insmod the module and use sudo? \n");
        return errno;

    }


    help_print();

    while (1) {
        printf("give command\n" );
        cmd = getchar();

        //lets clear extra stuff from stdin
        int c;
        while((c = getchar()) != '\n' && c != EOF);

        switch (cmd) {

        case 'r':
            check = read_from(fd);
            break;

        case 'w':
            check = write_to(fd);
            break;

        case 'e':
            check = encrypt(fd);
            break;

        case 'd':
            check = decrypt(fd);
            break;

        case 'q':
            return 0;

        case 'h':
            help_print();
            break;

        default:
            printf("Try 'h' for help\n\n");
            break;

        if(check < 0)
            return errno;

        }

    }
    close(fd);
    return 0;
}
