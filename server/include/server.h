#ifndef SERVER_H
#define SERVER_H

#include <netinet/in.h>

namespace net
{

class Server
{
protected:
    struct sockaddr_in server_address;
    int port;

    int sockFd;
    int epollFd;        

public:
    Server(int port);
    ~Server();

    void start();

protected:
    void setup();
    void makeListener();
    void makeEPoll();
};

}

#endif