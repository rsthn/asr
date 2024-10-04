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
    if (!socket.bind(new SockAddrIP4(2020))) {
        cout << "Error: Unable to bind socket to port 2020" << endl;
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
    socket.set_broadcast(true);

    ptr<SockAddr> target = new SockAddrIP4("255.255.255.255", 2020);
    int id = rand();

    char send_buff[1024];
    sprintf(send_buff, "HELLO:ID=%u", id);

    char buffer[1024];
    while (!stop)
    {
        buffer[socket.recv(buffer, sizeof(buffer)-1)] = 0;
        if (!buffer[0])
        {
            socket.send(target, send_buff);
            usleep(1e6);
            continue;
        }

        cout << "\e[90m" << socket.remote << "\e[0m: " << buffer << endl;
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
