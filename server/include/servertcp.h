#ifndef SERVER_TCP_H
#define SERVER_TCP_H

#include <netinet/in.h>
#include <sys/epoll.h>
#include <unordered_set>

namespace net
{

#define MAX_SERVER_CONNECTIONS 8192
#define MAX_EPOLL_EVENTS (MAX_SERVER_CONNECTIONS << 1)

class ServerTCP
{
protected:
    struct sockaddr_in serverAddress, clientAddress;
    socklen_t clientSize;
    int port;

    int sockFd;
    int epollFd;

    struct epoll_event event;
    struct epoll_event *events;

    int maxConns;
    int maxEvents;

    std::unordered_set<int> openedFd;

public:
    ServerTCP(int port, int maxConns = MAX_SERVER_CONNECTIONS, int maxEvents = MAX_EPOLL_EVENTS);
    ~ServerTCP();

    void start();

protected:
    void setup();

    void makeEPoll();
    void makeSocketListener();
    void makeSocketNonBlocking(int socketFd);
    void run();

    void addSocketConnToEpoll(int socketFd);
    void removeSocketConnFromEpoll(int socketFd);
};

}

#endif