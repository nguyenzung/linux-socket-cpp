#include <server.h>
#include <iostream>

using namespace net;

Server::Server(int port)
    :port(port)
{

}

Server::~Server()
{
    
}

void Server::start()
{
    std::cout << __FUNCTION__ << std::endl;
}

void Server::setup()
{
    this->makeEPoll();
    this->makeListener();
}

void Server::makeListener()
{

}

void Server::makeEPoll()
{

}