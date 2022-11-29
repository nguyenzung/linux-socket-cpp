#include <servertcp.h>
#include <iostream>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/epoll.h>
#include <sys/socket.h>
#include <pthread.h>

using namespace net;

ServerTCP::ServerTCP(int port, int maxConns, int maxEvents)
    :port(port), maxConns(maxConns), maxEvents(maxEvents)
{
    events = new struct epoll_event[maxEvents];
    this->setTimeout(10000);
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

void ServerTCP::setTimeout(int microseconds)
{
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
}

void ServerTCP::makeEPoll()
{
    std::cout << __FUNCTION__ << std::endl;
    epollFd = epoll_create1(0);
	if (epollFd == -1)
    {
        printf("\nFailed to create epoll");
		throw("Failed to create epoll");
    }
}

void ServerTCP::makeSocketListener()
{
    sockFd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockFd < 0)
    {
        printf("\nERROR opening socket");
        throw("ERROR opening socket");
    }
    
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(port);

    if (bind(sockFd, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0)
    {
        printf("\nERROR binding socket");
        throw("ERROR binding socket");
    }
    
    makeSocketNonBlocking(sockFd);
    addSocketConnToEpoll(sockFd);
}

void ServerTCP::makeSocketNonBlocking(int socketFd)
{
    int flags = fcntl(socketFd, F_GETFL);
    if (flags == -1)
    {
        printf("\nCannot get flags info of socketFd");
        throw ("Cannot get flags info of socketFd");
    }
    if (fcntl(socketFd, F_SETFL, flags |= O_NONBLOCK) == -1)
    {
        printf("\nCannot set flag info for socketFd");
        throw ("Cannot set flag info for socketFd");
    }
}

void ServerTCP::setWRTimeout(int socketFd)
{
    if (setsockopt (socketFd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
    {
        printf("Set read timeout failed");
        throw("Set read timeout failed");
    }

    if (setsockopt (socketFd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0)
    {
        printf("Set write timeout failed");
        throw("Set write timeout failed");
    }
}

void ServerTCP::run()
{
    printf("\n MainThreadID: %ld", pthread_self());
    listen(sockFd, maxConns);
    clientSize = sizeof(clientAddress);
    while(true)
    {
        int nfds = epoll_wait(epollFd, events, maxEvents, -1);
        if (nfds == -1)
        {
            printf("\nEPoll Wait failed");
            throw("epoll_wait failed");
        }

        for (int n = 0; n < nfds; ++n)
        {
            if (events[n].data.fd == sockFd) 
            {
                int connFd;
                while(connFd = accept(sockFd, (struct sockaddr *) &clientAddress, (socklen_t*)&clientSize), connFd >= 0)
                {
                    printf("\nAccept new connection");
                    if (connFd == -1)
                    {
                        perror("accept connection error");
                        continue;
                    }
                    setWRTimeout(connFd);
                    makeSocketNonBlocking(connFd);
                    addSocketConnToEpoll(connFd);
                }                
            } else {
                printf("\nAnother event from : %d %d. ThreadID %ld", events[n].data.fd, events[n].events, pthread_self());
                if (events[n].events & EPOLLIN) 
                {
                    char buffer[5];
                    memset(buffer, 0, 5);
                    int m, count = 0;
                    while(m = read(events[n].data.fd, buffer, 5), m >=0)
                    {
                        count += m;
                        printf("\n Message from client %s", buffer);
                        memset(buffer, 0, 5);
                    }
                    int size = snprintf(buffer, 5, "%d", count);
                    write(events[n].data.fd, buffer, 5);
                }

                if (events[n].events & EPOLLERR) 
                {
                    printf("\n Close socket err %d, %d %d ",events[n].events, EPOLLERR, events[n].events & EPOLLERR);
                    close(events[n].data.fd);
                }

                if (events[n].events & EPOLLRDHUP) 
                {
                    printf("\n Close socket shutdown");
                    close(events[n].data.fd);
                }
                
            }
        }
    }
}

void ServerTCP::addSocketConnToEpoll(int socketFd)
{
    printf("\n Add new connection %d. ThreadID: %ld", socketFd, pthread_self());
    event.data.fd = socketFd;
    event.events = EPOLLRDHUP | EPOLLERR | EPOLLIN | EPOLLET;   // Enable Edge-Trigger
    if (epoll_ctl (epollFd, EPOLL_CTL_ADD, socketFd, &event) == -1)
    {
        printf("\nFailed to add socket connection to epoll");
        throw ("Failed to add socket connection to epoll");
    }
}