#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

int main(int argc, char *argv[])
{
    int sockfd, port;
    struct sockaddr_in serverAddress;
    struct hostent *server;

    char buffer[256], response[256];
    if (argc < 3) {
       printf("Usage %s ip port\n", argv[0]);
       return 1;
    }
    port = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        printf("\nCannot open socket");
        return 1;
    }
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        printf("\n No host");
        return 1;
    }

    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    memcpy(server->h_addr, &serverAddress.sin_addr.s_addr, server->h_length);
    serverAddress.sin_port = htons(port);
    if (connect(sockfd,(struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0)
    {
        printf("\nCannot connect to host");
    }
    do
    {
        printf("\nEnter your message: ");
        memset(buffer, 0, 255);
        read(0, buffer, 255);
        buffer[strlen(buffer) - 1] = 0;
        printf("\n<---[%s]--->", buffer);
        if(write(sockfd, buffer, strlen(buffer)) < 0) 
        {
            printf("\nCannot send data");
            return 1;
        }
        memset(response, 0, 255);
        if(read(sockfd, response, 255) < 0)
        {
            printf("\nCannot read data");
            return 1;
        }
        printf("\nMessage from server: %s", response);
    } while (strcmp(buffer, "q") != 0);
    close(sockfd);
}