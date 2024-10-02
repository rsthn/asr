
#include <asr/socket-addr>
#include <asr/socket-addr-ip4>
#include <asr/socket-addr-ip6>

#include <asr/socket>
#include <asr/socket-tcp>
#include <asr/socket-udp>

#include <iostream>

namespace asr {

    // When using Windows, ensures that Winsocks is properly initialized.
    #if __WIN32__
    int initWinsocks()
    {
        WSADATA wsaData;

        int result = WSAStartup(MAKEWORD(2,2), &wsaData);
        if (result != 0) {
            printf("WSAStartup failed: %d\n", result);
            exit(1);
        }

        return 1;
    }

    static const int __wsaReady = initWinsocks();
    #endif

    // Constructor added here instead of inline in the header file to ensure the Winsocks initialization is performed.
    SockAddrIP4::SockAddrIP4() : SockAddr(sizeof(struct sockaddr_in)) {
        reset();
    }

    SockAddrIP6::SockAddrIP6() : SockAddr(sizeof(struct sockaddr_in6)) {
        reset();
    }

    /* *************************************/
    /* Socket */

    Socket::Socket(SOCKET source) : socket(source) {
    }

    Socket::~Socket() {
        close();
    }

    SOCKET Socket::alloc(int family, int type) {
        socket = ::socket(family, type, 0);
        return socket;
    }

    void Socket::close()
    {
        if (socket == -1) return;

        #if __WIN32__
            if (connected)
                ::shutdown(socket, SD_BOTH);
            ::closesocket(socket);
        #else
            if (connected)
                ::shutdown(socket, SHUT_RDWR);
            ::close(socket);
        #endif

        connected = false;
        socket = -1;
    }

    bool Socket::is_readable(int timeout) const
    {
        fd_set dset;
        FD_ZERO(&dset);

        struct timeval tm;
        tm.tv_sec = timeout / 1000;
        tm.tv_usec = (timeout % 1000) * 1000;
        FD_SET(socket, &dset);

        int cnt = ::select(socket+1, &dset, nullptr, nullptr, &tm);
        return cnt == 1;
    }

    bool Socket::is_writeable(int timeout) const
    {
        fd_set dset;
        FD_ZERO(&dset);

        struct timeval tm;
        tm.tv_sec = timeout / 1000;
        tm.tv_usec = (timeout % 1000) * 1000;
        FD_SET(socket, &dset);

        int cnt = ::select(socket+1, nullptr, &dset, nullptr, &tm);
        return cnt == 1;
    }

    int Socket::get_error() const {
        socklen_t lon = sizeof(int);
        int val = -1;
        ::getsockopt(socket, SOL_SOCKET, SO_ERROR, (char*)&val, &lon);
        return val;
    }

    void Socket::set_reuse_addr(bool value) {
        int val = value ? 1 : 0;
        ::setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (const char *)&val, sizeof(val));
    }

    void Socket::set_broadcast(bool value) {
        int val = value ? 1 : 0;
        ::setsockopt(socket, SOL_SOCKET, SO_BROADCAST, (const char *)&val, sizeof(val));
    }

    void Socket::set_nonblocking(bool value)
    {
        if (socket == -1) return;

        if (!value) {
            #if __WIN32__
                unsigned long val = 0;
                ::ioctlsocket(socket, FIONBIO, &val);
            #else
                ::fcntl(socket, F_SETFL, fcntl(socket, F_GETFL) & ~O_NONBLOCK);
            #endif
        }
        else {
            #if __WIN32__
                unsigned long val = 1;
                ::ioctlsocket(socket, FIONBIO, &val);
            #else
                ::fcntl(socket, F_SETFL, fcntl(socket, F_GETFL) | O_NONBLOCK);
            #endif
        }
    }


    /* *************************************/
    /* SocketTCP */

    SocketTCP::SocketTCP(SOCKET source) : Socket(source) {
        local = nullptr;
        remote = nullptr;
        connected = false;
    }

    bool SocketTCP::bind(ptr<SockAddr> addr)
    {
        if (socket == -1 && alloc(addr->get_family(), SOCK_STREAM) == -1)
            return false;

        local = addr;
        if (::bind(socket, &addr->data, addr->length) == -1)
            return false;

        ::getsockname(socket, &addr->data, &addr->length);
        return true;
    }

    bool SocketTCP::listen(int backlog)
    {
        if (socket == -1)
            return false;

        set_reuse_addr(true);
        set_nonblocking(true);

        return ::listen(socket, backlog) != -1;
    }

    ptr<SocketTCP> SocketTCP::accept()
    {
        if (socket == -1) return nullptr;

        ptr<SockAddr> remote = local->alloc();
        int nsocket = ::accept(socket, &remote->data, &remote->length);
        if (nsocket == -1) return nullptr;

        SocketTCP *client = new SocketTCP(nsocket);
        client->remote = remote;
        client->connected = true;
        return client;
    }

    bool SocketTCP::connect(ptr<SockAddr> addr, int timeout)
    {
        struct timeval tv;
        fd_set tmpset; 

        connected = false;

        if (socket == -1 && alloc(addr->get_family(), SOCK_STREAM) == -1)
            return false;

        remote = addr;
        set_nonblocking(true);

        int res = ::connect(socket, &remote->data, remote->length);
        if (res < 0)
        {
            #if __WIN32__
                if (WSAGetLastError() != WSAEWOULDBLOCK)
                    return false;
            #else
                if (errno != EINPROGRESS && errno != EWOULDBLOCK)
                    return false;
            #endif

            tv.tv_sec = 10;
            tv.tv_usec = 0;

            FD_ZERO(&tmpset); 
            FD_SET(socket, &tmpset);

            if (::select(socket+1, nullptr, &tmpset, nullptr, &tv) > 0) {
                if (!get_error())
                    return connected = true;
            }

            return false;
        }

        return connected = true;
    }

    int SocketTCP::read(char *buffer, int num_bytes, int buffer_space)
    {
        if (socket == -1)
            return 0;

        if (buffer_space == -1)
            buffer_space = num_bytes;

        num_bytes = num_bytes > buffer_space ? buffer_space : num_bytes;
        int n = ::recv(socket, buffer, num_bytes, 0);
        return n < 1 ? 0 : n;
    }

    int SocketTCP::write(const char *buffer, int num_bytes)
    {
        if (socket == -1 || !is_writeable(0))
            return 0;

        if (num_bytes == -1)
            num_bytes = ::strlen(buffer);

        int n = ::send(socket, buffer, num_bytes, 0);
        return n < 1 ? 0 : n;
    }


    /* *************************************/
    /* SocketUDP */

    SocketUDP::SocketUDP(SOCKET source) : Socket(source) {
        local = nullptr;
        remote = nullptr;
        connected = false;
    }

    bool SocketUDP::bind(ptr<SockAddr> addr)
    {
        if (socket == -1 && alloc(addr->get_family(), SOCK_DGRAM) == -1)
            return false;

        remote = addr->alloc();
        local = addr;

        if (::bind(socket, &addr->data, addr->length) == -1)
            return false;

        ::getsockname(socket, &addr->data, &addr->length);
        return true;
    }

    int SocketUDP::read(char *buffer, int num_bytes, int buffer_space)
    {
        if (socket == -1)
            return 0;

        if (buffer_space == -1)
            buffer_space = num_bytes;

        num_bytes = num_bytes > buffer_space ? buffer_space : num_bytes;
        int n = ::recvfrom(socket, buffer, num_bytes, 0, &remote->data, &remote->length);
        return n < 1 ? 0 : n;
    }

    int SocketUDP::write(const char *buffer, int num_bytes)
    {
        if (socket == -1 || !is_writeable(0))
            return 0;

        if (num_bytes == -1)
            num_bytes = ::strlen(buffer);

        int n = ::sendto(socket, buffer, num_bytes, 0, &remote->data, remote->length);
        return n < 1 ? 0 : n;
    }

};
