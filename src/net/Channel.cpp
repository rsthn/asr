
#include <asr/net/Handler>
#include <asr/net/Channel>
#include <asr/net/Manager>

namespace asr {
namespace net {

    /**
     * @brief Count of created channels for identification purposes.
     */
    int Channel::numChannels = 0;

    /**
     * @brief Connects to a server and returns a new channel.
     * @param handle Handler to be used by the channel.
     * @param address Address of the server.
     * @param port Port number of the server.
     * @return Channel*
     */
    Channel *Channel::connect (Handler *handler, const char *address, int port)
    {
        Socket *socket = nullptr;

        try {
            Socket *socket = new Socket (SOCK_STREAM);
            if (!socket->valid()) throw Error(1);
            if (!socket->connect (address, port)) throw Error(2);

            Channel *channel = new Channel (socket, handler, new Stream());
            if (!manager->add (channel))
                throw Error(3);

            return channel;
        }
        catch (Error &err)
        {
            if (socket != nullptr)
                delete socket;

            return nullptr;
        }
    }

}};
