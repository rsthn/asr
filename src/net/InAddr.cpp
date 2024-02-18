
#include <asr/net/InAddr>

namespace asr {
namespace net {

    /**
     * When using Windows, ensures that Winsocks is properly initialized.
     */
    #if __WIN32__
    int initWinsocks()
    {
        WSADATA wsaData;

        int result = WSAStartup (MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            printf("WSAStartup failed: %d\n", result);
            exit(1);
        }

        return 1;
    }

    static const int __wsaReady = initWinsocks();
    #endif

    /**
     * Constructor added here instead of inline in the header file to ensure the Winsocks initialization is performed.
     */
    InAddr::InAddr() {
        this->length = sizeof(struct sockaddr_in);
        this->clear();
    }

    /**
    **	Returns the address of a given host name or nullptr if unable to resolve. The resulting string should be copied by the caller for further usage.
    */
    /*const char *InAddr::getHostByName (const char *name)
    {
        struct hostent *h = ::gethostbyname(name);
        if (h == nullptr) return nullptr;

        return strcpy (tempbuff(), ::inet_ntoa (*((in_addr *)h->h_addr)));
    }*/

}};
