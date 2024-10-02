#include <asr/socket-addr-ip6>
#include <asr/socket-addr-ip4>
#include <asr/socket-udp>
#include <csignal>
#include <iostream>
#include <unistd.h>

using namespace asr;
using namespace std;

bool stop = false;

/**
 */
void test()
{
    SocketUDP socket;
    if (!socket.bind(new SockAddrIP4(2000))) {
        cout << "Error: Unable to bind socket to port 2000" << endl;
        return;
    }

    if (!socket.is_valid()) {
        cout << "Error: Socket number is invalid" << endl;
        return;
    }

    cout << "\e[32m[Waiting on " << socket.local << "]\e[0m" << endl;

    signal(SIGINT, [](int) {
        stop = true;
    });

    socket.set_nonblocking(true);

    char buffer[1024];
    while (!stop)
    {
        buffer[socket.read(buffer, sizeof(buffer)-1)] = 0;
        if (!buffer[0]) {
            usleep(1000);
            continue;
        }

        cout << "\e[90m" << socket.remote << "\e[0m: " << buffer << endl;

        if (strcmp(buffer, "stop"))
            strcat(buffer, " - ACK");

        socket.write(buffer);
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
