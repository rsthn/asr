#include <asr/socket-addr-ip6>
#include <asr/socket-addr-ip4>
#include <asr/socket-tcp>
#include <iostream>
#include <unistd.h>

using namespace asr;
using namespace std;

bool stop = false;

/**
 */
void test()
{
    SocketTCP socket;
    if (!socket.bind(new SockAddrIP4(1000))) {
        cout << "Error: Unable to bind socket to port 1000" << endl;
        return;
    }

    if (!socket.is_valid()) {
        cout << "Error: Socket number is invalid" << endl;
        return;
    }

    cout << "\e[32m[Listening on " << socket.local << "]\e[0m" << endl;

    signal(SIGINT, [](int) {
        stop = true;
    });

    while (socket.listen() && !stop)
    {
        auto conn = socket.accept();
        if (!conn) continue;

        cout << "Connected to " << conn->remote << endl;
        conn->write("Hello! from a simple and lightweight TCP server.\n");
        usleep(1e3);
        cout << "Closed" << endl;
    }

    cout << "\e[32m[Exiting]\e[0m" << endl;
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
