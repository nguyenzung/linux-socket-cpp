#include <servertcp.h>
#include <iostream>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/epoll.h>
#include <sys/socket.h>
#include <pthread.h>

#define READ_BUFFER_SIZE 1024

using namespace net;

ServerTCP::ServerTCP(int port, int maxConns, int maxEvents)
    :port(port), maxConns(maxConns), maxEvents(maxEvents)
{
    events = new struct epoll_event[maxEvents];
}

ServerTCP::~ServerTCP()
{
    printf("\n Exit program");
    delete[] events;

    if (close(sockFd) < 0) 
        perror("\nCannot close socket listener");

    if (close(epollFd) < 0)
        perror("\nCannot close epoll");

    for (auto it = openedFd.begin(); it != openedFd.end(); ++it)
    {
        printf("\n Close %d", *it);
        removeSocketConnFromEpoll(*it);
    }
}

void ServerTCP::start()
{
    setup();
    run();
}

void ServerTCP::setup()
{
    this->makeEPoll();
    this->makeSocketListener();
}

void ServerTCP::makeEPoll()
{
    epollFd = epoll_create1(0);
	if (epollFd == -1)
		throw("Failed to create epoll");
}

void ServerTCP::makeSocketListener()
{
    sockFd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockFd < 0)
        throw("ERROR opening socket");
    int reuseAddr = 1;
    setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR, &reuseAddr, sizeof(reuseAddr));

    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(port);

    if (bind(sockFd, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0)
        throw("ERROR binding socket");

    makeSocketNonBlocking(sockFd);
    addSocketConnToEpoll(sockFd);
}

void ServerTCP::makeSocketNonBlocking(int socketFd)
{
    int flags = fcntl(socketFd, F_GETFL);
    if (flags == -1)
        throw ("Cannot get flags info of socketFd");

    if (fcntl(socketFd, F_SETFL, flags |= O_NONBLOCK) == -1)
        throw ("Cannot set flag info for socketFd");
}

void ServerTCP::run()
{
    listen(sockFd, maxConns);
    clientSize = sizeof(clientAddress);
    while(true)
    {
        int nfds = epoll_wait(epollFd, events, maxEvents, -1);
        if (nfds == -1)
            throw("epoll_wait failed");

        for (int n = 0; n < nfds; ++n)
        {
            if (events[n].data.fd == sockFd)
            {
                int connFd;
                while(connFd = accept(sockFd, (struct sockaddr *) &clientAddress, (socklen_t*)&clientSize), connFd > 0)
                {
                    printf("\n Add %d", connFd);
                    openedFd.insert(connFd);
                    makeSocketNonBlocking(connFd);
                    addSocketConnToEpoll(connFd);
                }
            } else {
                if (events[n].events & EPOLLIN) 
                {
                    char buffer[READ_BUFFER_SIZE];
                    memset(buffer, 0, READ_BUFFER_SIZE);
                    int m = 0;
                    int size;
                    while(size = read(events[n].data.fd, buffer, READ_BUFFER_SIZE), size > 0)
                    {
                        m += size;
                        write(1, buffer, size);
                    }
                    snprintf(buffer, 5, "%d", m);
                    write(events[n].data.fd, buffer, 5);
                }

                if (events[n].events & EPOLLERR) 
                    removeSocketConnFromEpoll(events[n].data.fd);

                if (events[n].events & EPOLLRDHUP)
                    removeSocketConnFromEpoll(events[n].data.fd);
            }
        }
    }
}

void ServerTCP::addSocketConnToEpoll(int socketFd)
{
    event.data.fd = socketFd;
    event.events = EPOLLRDHUP | EPOLLERR | EPOLLIN | EPOLLET;   // Enable Edge-Trigger
    if (epoll_ctl (epollFd, EPOLL_CTL_ADD, socketFd, &event) == -1)
        throw ("Failed to add socket connection to epoll");
}

void ServerTCP::removeSocketConnFromEpoll(int socketFd)
{
    printf("\n [Close: %d] \n", socketFd);
    openedFd.erase(socketFd);
    close(socketFd);
    epoll_ctl (epollFd, EPOLL_CTL_DEL, socketFd, 0);
}