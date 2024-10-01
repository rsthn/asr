#include <asr/socket-addr-ip6>
#include <asr/socket-addr-ip4>
#include <asr/socket-tcp>
#include <iostream>
#include <unistd.h>

using namespace asr;
using namespace std;

/**
 */
void test()
{
    SocketTCP socket;

    cout << "Connecting ..." << endl;
    if (!socket.connect(new SockAddrIP4(1000, "127.0.0.1"))) {
        cout << "Error: Unable to connect to port 1000" << endl;
        return;
    }

    cout << "Connected to " << socket.remote << endl;

    char buffer[1024];
    int n = 0;

    while (true)
    {
        while (!socket.is_readable());

        int m = socket.read(&buffer[n], sizeof(buffer)-n-1);
        if (m != 0) {
            n += m;
            continue;
        }

        buffer[n] = 0;
        cout << "\e[94m" << buffer << "\e[0m";
        break;
    }

    cout << "Closed" << endl;
}

/**
 */
int main (int argc, const char *argv[])
{
    auto n = asr::memblocks;

    test();

    asr::refs::shutdown();
    if (asr::memblocks != n)
        cout << "\e[31mMemory leak detected: \e[91m" << asr::memsize << " bytes\e[0m\n";

    return 0;
}
