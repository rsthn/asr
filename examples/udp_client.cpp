#include <asr/socket-addr-ip6>
#include <asr/socket-addr-ip4>
#include <asr/socket-udp>
#include <iostream>
#include <unistd.h>

using namespace asr;
using namespace std;

/**
 */
void test()
{
    SocketUDP socket;
    if (!socket.bind(new SockAddrIP4())) {
        cout << "Error: Unable to bind socket" << endl;
        return;
    }

    socket.remote = new SockAddrIP4(2000, "127.0.0.1");
    socket.send("Good day from a UDP client!");

    char buffer[1024];

    const char *messages[] = {
        "Ping", "Hello", "Привет", "stop"
    };

    const int num_messages = sizeof(messages) / sizeof(messages[0]);
    int next_message = 0;

    while (true)
    {
        buffer[socket.recv(buffer, sizeof(buffer)-1)] = 0;
        if (!buffer[0]) {
            usleep(1000);
            continue;
        }

        cout << "\e[90m" << socket.remote << "\e[0m: " << buffer << endl;

        if (!strcmp(buffer, "stop"))
            break;

        socket.send(messages[next_message]);
        next_message = (next_message + 1) % num_messages;
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
