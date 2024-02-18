
#include <cstdio>
#include <cwchar>

#include <asr/net/InAddr>
#include <asr/net/Socket>

int main (int argc, const char *argv[])
{
    asr::net::Socket *socket = new asr::net::Socket (SOCK_STREAM);
    if (!socket->isValid()) {
        wprintf(L"Error: Unable to allocate TCP socket.\n");
        return 1;
    }

    wprintf(L"Connecting ...\n");
    if (!socket->connect("127.0.0.1", 9999)) {
        wprintf(L"Error: Unable to connect to server.\n");
        return 1;
    }

    char buffer[1024];
    int n;

    while (socket->isAlive())
    {
        int m = socket->read(&buffer[n], 16, sizeof(buffer)-n-1);
        if (m == 0) {
            if (n > 0) {
                buffer[n] = 0;
                wprintf(L"RECV: %s\n", buffer);
            }

            n = 0;
            break;
        }
        else {
            n += m;
        }
    }

    wprintf(L"Connection closed.\n");
    return 0;
}
