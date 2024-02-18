
#include <cstdio>
#include <cwchar>

#include <asr/net/InAddr>
#include <asr/net/Socket>

/**
 * @brief Connects to a UDP server on localhost:9999 and sends a plain text message.
 */
int main (int argc, const char *argv[])
{
    asr::net::Socket *socket = new asr::net::Socket (SOCK_DGRAM);
    if (!socket->isValid()) {
        wprintf(L"Error: Unable to allocate UDP socket.");
        return 1;
    }

    socket->target = new asr::net::InAddr("127.0.0.1", 9999);

    if (argc == 2)
        socket->write(argv[1]);
    else
        socket->write("Hello! This is a small test.\n");

    return 0;
}
