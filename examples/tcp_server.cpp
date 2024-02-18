#include <cstdio>
#include <cwchar>
#include <unistd.h>

#include <asr/net/InAddr>
#include <asr/net/Socket>

int main (int argc, const char *argv[])
{
    asr::net::Socket *socket = new asr::net::Socket (SOCK_STREAM);
    if (!socket->isValid()) {
        wprintf(L"Error: Unable to allocate TCP socket.\n");
        return 1;
    }

    if (!socket->bind(9999)) {
        wprintf(L"Error: Unable to bind socket to port 9999.\n");
        return 1;
    }

    wprintf(L"Listening on port 9999 ...\n");
    while (socket->listen()) {
        asr::net::Socket *conn = socket->accept();
        wprintf(L"\nConnecting received from: %s\n", conn->remote->getAddrStr());

        wprintf(L"Sending data ...\n");
        conn->write("Hello! from a simple and tiny TCP server.\n");
        usleep(2e6);
        wprintf(L"Closing connection.\n");
        delete conn;
    }

    return 0;
}
