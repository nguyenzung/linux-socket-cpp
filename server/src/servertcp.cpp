#include <servertcp.h>
#include <iostream>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/epoll.h>
#include <sys/socket.h>

using namespace net;

ServerTCP::ServerTCP(int port, int maxConns, int maxEvents)
    :port(port), maxConns(maxConns), maxEvents(maxEvents)
{
    events = new struct epoll_event[maxEvents];
}

ServerTCP::~ServerTCP()
{
    delete[] events;
    if (close(sockFd) < 0) 
    {
        perror("\nCannot close socket listener");
    }

    if (close(epollFd) < 0)
    {
        perror("\nCannot close epoll");
    }
}

void ServerTCP::start()
{
    std::cout << __FUNCTION__ << std::endl;
    setup();
    run();
}

void ServerTCP::setup()
{
    std::cout << __FUNCTION__ << std::endl;
    this->makeEPoll();
    this->makeSocketListener();
}

void ServerTCP::makeEPoll()
{
    std::cout << __FUNCTION__ << std::endl;
    epollFd = epoll_create1(0);
	if (epollFd == -1)
		throw("Failed to create epoll");
}

void ServerTCP::makeSocketListener()
{
    sockFd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockFd < 0) 
        throw("ERROR opening socket");
    
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
                int connFd = accept(sockFd, (struct sockaddr *) &clientAddress, (socklen_t*)&clientSize);
                printf("Accept new connection\n");
                if (connFd == -1)
                {
                    perror("accept connection error");
                    continue;
                }
                makeSocketNonBlocking(connFd);
                addSocketConnToEpoll(connFd);
            } else {
                printf("\nAnother event from : %d", events[n].data.fd);
                char buffer[256];
                memset(buffer, 0, 256);
                if(read(events[n].data.fd,buffer,255) < 0)
                {
                    perror("ERROR reading from socket");
                    close(events[n].data.fd);
                    continue;
                }
                printf("\n Message from client %s", buffer);
                close(events[n].data.fd);
            }
        }
    }
}

void ServerTCP::addSocketConnToEpoll(int socketFd)
{
    printf("\n Add new connection %d", socketFd);
    event.data.fd = socketFd;
    event.events = EPOLLIN | EPOLLET;   // Enable Edge-Trigger
    if (epoll_ctl (epollFd, EPOLL_CTL_ADD, socketFd, &event) == -1)
        throw ("Failed to add socket connection to epoll");
}