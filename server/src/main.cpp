#include <servertcp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

net::ServerTCP *server;

void handler(int signo, siginfo_t *info, void *context)
{
    int *data = (int*)context;
    printf("\nSIGNAL HANDLER: %d", *data);
    delete server;
    exit(0);
}

int main(int argc, char *argv[])
{
    struct sigaction act = { 0 };
    act.sa_flags = SA_SIGINFO;
    act.sa_sigaction = &handler;
    if (sigaction(SIGINT, &act, NULL) == -1)
    {
        exit(EXIT_FAILURE);
    }
    server = new net::ServerTCP(8000);
    server->start();
    
    return 0;
}
