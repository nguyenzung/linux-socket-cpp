#include <servertcp.h>

int main(int argc, char *argv[])
{
    net::ServerTCP server(8000);
    server.start();
    return 0;
}
