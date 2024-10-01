#include <iostream>

#include <asr/socket-addr-ipv4>
#include <asr/socket-tcp>

using namespace asr;
using namespace std;

void test()
{
    SocketTCP socket;
    if (!socket.bind(new SockAddrIPv4(1000))) {
        cout << "Error: Unable to bind socket to port 1000" << endl;
        return;
    }

    cout << "\e[90m[Listening on " << socket.local << "]\e[0m" << endl;
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
