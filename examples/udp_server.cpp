#include <cstdio>
#include <cwchar>

#include <asr/net/InAddr>
#include <asr/net/Socket>

/**
 * @brief Runs a test server that listens for UDP packets on port 9999.
 */
int main (int argc, const char *argv[])
{
    asr::net::Socket *socket = new asr::net::Socket (SOCK_DGRAM);
    if (!socket->isValid()) {
        wprintf(L"Error: Unable to allocate UDP socket.");
        return 1;
    }

    if (!socket->bind(9999)) {
        wprintf(L"Error: Unable to bind socket to port 9999.");
        return 1;
    }

    wprintf(L"Waiting for data in port 9999 ...\n");
    while (true) {
        char buffer[1024];
        int bytes = socket->read(buffer, sizeof(buffer)-1);
        buffer[bytes] = '\0';

        wprintf(L"\nRead: %d bytes from %s\nData: ", bytes, socket->remote->getAddrStr());
        wprintf(L"%s\n", buffer);
    }

    return 0;
}
