#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <netdb.h>

#define SERVER_PORT 8080
#define MAXLINE 4096
#define SA struct sockaddr

void err(const char *fmt, ...){
    int errno_save;
    va_list ap;

    //all system calls can set arrno, so we need to save it now
    errno_save = errno;

    //print out the fmt + args to standard out
    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    fprintf(stdout, "\n");
    fflush(stdout);

    //print out error message is errno was set
    if(errno_save != 0){
        fprintf(stdout, "(errno = %d) : %s\n", errno_save, strerror(errno_save));
        fprintf(stdout, "\n");
        fflush(stdout);
    }
}

char *bintohex(const unsigned char *input, size_t len){
    char *result;
    char *hexits = "0123456789ABCDEF";
    if(input == NULL || len <= 0)
        return NULL;

    int resultlength = (len*3)+1;
    result = malloc(resultlength);

    for(int i=0; i<len; i++){
        result[i*3] = hexits[input[i] >> 4];
        result[(i*3)+1] = hexits[input[i] & 0x0F];
        result[(i*3)+2] = ' ';
    }

    return result;
}

char* get_page(char* buffer){
    int sockfd;
    printf("%s\n", "Creating a socket");
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        err("Socket Error.");
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    addr.sin_addr.s_addr = 0x22d8b85d;
     printf("%s\n", "Connection the web service");
    if (connect(sockfd, (struct sockaddr *) &addr, sizeof (addr)) < 0)
        err("Connect Issue");
    char *header = "GET / HTTP/1.1\r\nHost: www.example.com\r\n\r\n";
    if((send(sockfd,header,strlen(header),0)) < 0)
        err("Write Error");
    printf("%s\n", "Reading the index.html page of the web server");
    if((read(sockfd, buffer, 2048)) < 0)
        err("Read Error");
    return buffer;
}

int main(int argc, char **argv){
    //allocating resources
    int listenfd, connfd, n;
    struct sockaddr_in servaddr;
    uint8_t buff[MAXLINE+1];
    uint8_t recvline[MAXLINE+1];

    //creating a socket
    //AF_INET -> internet socket, SOCK_STREAM -> TCP
    printf("%s\n", "Creating a Socket");
    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        err("Socket Error.");

    //Set up address
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;                  //internet address
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);   //responds to any internet address
    servaddr.sin_port = htons(SERVER_PORT);         //port listening on

    //listen to socket and bind to address
    printf("%s\n", "Binding the Socket to the port and the address");
    if((bind(listenfd, (SA *) &servaddr, sizeof(servaddr))) < 0)
        err("Bind Error.");
    if((listen(listenfd, 10)) < 0)
        err("Listen Error.");

    //infinite loop to accept blocks until an incoming connection arrives
     printf("%s\n", "Listening for incoming messages");
    for( ; ; ){
        struct sockaddr_in addr;
        socklen_t addr_len;

        //it returns a file descriptor to the connection
        printf("Waiting for a connection of port %d\n", SERVER_PORT);
        fflush(stdout);
        connfd = accept(listenfd, (SA*) NULL, NULL);   //socket that talks to client once connected

        //zero out recieve buffer to make sure it ends up null terminated
        memset(recvline, 0, MAXLINE);

        //read client's message
        while((n = read(connfd, recvline, MAXLINE-1)) > 0){
            //print out binary and hex of message
           // fprintf(stdout, "\n%s\n\n%s", bintohex(recvline, n), recvline);
            printf("%s\n", "Recieving Client's Message to create a connection");
            
            //detect end of message
            if(recvline[n-1] == '\n'){
                break;
            }
            printf("%s\n", "Recieved OK message of connection from from client");
            memset(recvline, 0, MAXLINE);
        }

        if(n < 0){
            err("Read Error");
        }

        //connect to web server
        char page[2048];
        get_page(page);

        //send a response
        printf("%s\n", "Sending response to the client with OK message recieved");
        char* respond = "HTTP/1.0 200 OK\r\nContent-Type:text/html\r\n";
        send(connfd, respond, strlen(respond), 0);
        printf("%s\n", "Sending response to the client with message content");
        send(connfd, page, strlen(page), 0);

        write(connfd, (char*) buff, strlen((char*) buff));
        close(connfd);
    }
}