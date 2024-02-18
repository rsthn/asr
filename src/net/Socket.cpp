
#include <asr/net/Socket>

namespace asr {
namespace net {

    /**
     * @brief Allocates a network socket.
     *
     * @param type Socket type (SOCK_STREAM, SOCK_DGRAM, SOCK_RAW, SOCK_SEQPACKET).
     * @param source When provided, this socket number will be used instead of allocating a new one.
     */
    Socket::Socket (int type, SOCKET source)
    {
        this->remote = new InAddr();
        this->local = nullptr;
        this->target = nullptr;

        this->socket = -1;
        this->persistent = 0;
        this->type = type;
        this->_connected = false;

        // Create the socket if source socket was not specified.
        if (source == -1)
        {
            this->socket = ::socket (AF_INET, type, 0);
            if (this->socket == -1) {
                //trace (slog ("Socket: Constructor: ERROR: Unable to allocate socket.\n"));
                return;
            }
        }
        else
            this->socket = source;

        // Attempt to set SO_REUSEADDR to avoid listen() stalls after closing the socket.
        int reuse_addr = 1;
        ::setsockopt (this->socket, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse_addr, sizeof(reuse_addr));
    }


    /**
     * @brief Releases resources, closes and shuts down the socket.
     */
    Socket::~Socket ()
    {
        if (this->socket != -1) {
            this->close();
            this->socket = -1;
        }

        if (this->remote) {
            delete this->remote;
            this->remote = nullptr;
        }

        if (this->local) {
            delete this->local;
            this->local = nullptr;
        }
    }

}};
