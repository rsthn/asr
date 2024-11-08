#include <asr/socket-addr-ip6>
#include <asr/socket-addr-ip4>
#include <asr/socket-tcp>

#include <asr/data-schema>

using namespace asr;
using namespace std;

class Message
{
    public:

    int version;
};

int main()
{
    DataSchema<Message> *schema = new DataSchema<Message>();
    //schema->uint8(offsetof(Message, version));
    delete schema;

    return 0;
}

#if 0
#include <asr/fcgi>

#include <csignal>
#include <iostream>
#include <unistd.h>
#include <algorithm>

using namespace asr;
using namespace std;

bool stop = false;

void test()
{
    SocketTCP socket;
    if (!socket.bind(new SockAddrIP4("127.0.0.1", 1000))) {
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

    auto reader = FCGIHeader::createReader();
    FCGIHeader hdr;

    char tmp[1024];
    Buffer buff;

    while (socket.listen() && !stop)
    {
        auto conn = socket.accept();
        if (!conn) continue;

        cout << endl << "\e[32m[Connected to " << conn->remote << "]\e[0m" << endl;

        bool _brk = false;
        while (!_brk)
        {
            while (!conn->is_readable());
            usleep(1000);

            int m = conn->recv(tmp, std::min(buff.space_available(), (int)sizeof(tmp)));
            if (!m) continue;

            if (!buff.write(tmp, m)) {
                cout << "Error: Unable to write to buff" << endl;
                break;
            }

            while (true)
            {
                try {
                    if (!reader->read(&buff, &hdr))
                        break;

                    hdr.dump();
                    cout << "\e[34m[skip " << (hdr.content_length + hdr.padding_length) << "]\e[0m" << endl;
                    reader->skip(hdr.content_length + hdr.padding_length, &buff);
                    hdr.clear();
                    reader->reset_state();
                    break;
                }
                catch (const Error &e) {
                    cout << "\e[31mError: " << e.message() << "\e[0m" << endl;
                    buff.read_uint8(); // consume the invalid byte
                    _brk = true;
                    break;
                }
            }
        }

        cout << "\e[33m[Disconnected]\e[0m" << endl;
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
#endif
