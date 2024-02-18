#include <cstddef>
#include <cstdio>
#include <ctime>

#include <asr/net/MessageSchema>
#include <asr/net/MessageEncoder>
#include <asr/net/Handler>
#include <asr/net/Server>
#include <asr/io/Buffer>

using namespace asr::net;
using namespace asr::io;

// When `true` will attempt to do process forking to handle multiple clients in parallel. If you want
// to check memory leaks first with multiple clients, run server with PARALLEL_MODE set to `false`.
#define PARALLEL_MODE true

// Utility macros for colored output
#define GREY "\x1B[90m"
#define BLUE "\x1B[94m"
#define RED "\x1B[91m"
#define GREEN "\x1B[92m"
#define CYAN "\x1B[96m"
#define MAGENTA "\x1B[95m"
#define DEF "\x1B[0m"

// Message macros
#define M_INFO BLUE "[INFO] " DEF
#define M_ERR RED "[ERROR] " DEF
#define M_IN GREEN "[IN] " DEF
#define M_OUT CYAN "[OUT] " DEF
#define M_DEBUG MAGENTA "[DEBUG] " DEF

struct Message
{
    // Constants
    static constexpr uint32_t HEAD_SIGNATURE = 0x11223344;
    static constexpr uint32_t TAIL_SIGNATURE = 0x55667788;

    // Fields
    uint32_t head_signature;
    uint8_t message_type;
    union {
        struct {
            char *name;
        } hello; // message_type=1

        struct {
            uint16_t year;
            uint8_t month, day;
            uint8_t hour, minute, second;
            char *server_name;
        } server_hello; // message_type=2

        // message_type=3
    }
    req;
    uint32_t tail_signature;

    void clear()
    {
        if (message_type == 1) {
            if (req.hello.name)
                delete req.hello.name;
            req.hello.name = nullptr;
        }
        else if (message_type == 2) {
            if (req.server_hello.server_name)
                delete req.server_hello.server_name;
            req.server_hello.server_name = nullptr;
        }
    }

    void reset(int type) {
        head_signature = HEAD_SIGNATURE;
        message_type = type;
        tail_signature = TAIL_SIGNATURE;
    }

    void dump(const char *prefix)
    {
        printf("%s", prefix);
        if (message_type == 1)
            printf("Hello: %s\n", req.hello.name);
        else if (message_type == 2)
            printf("Server Hello: %s %u-%u-%u %u:%u:%u\n", 
                req.server_hello.server_name,
                req.server_hello.year, req.server_hello.month, req.server_hello.day,
                req.server_hello.hour, req.server_hello.minute, req.server_hello.second);
        else if (message_type == 3)
            printf("Finish\n");
    }

    Message *hello(const char *name) {
        reset(1);
        req.hello.name = (char*)name;
        return this;
    }

    Message *server_hello(const char *server_name) {
        reset(2);
        req.server_hello.server_name = (char*)server_name;

        struct tm *tm;
        time_t t = time(nullptr);
        tm = localtime(&t);
        req.server_hello.year = tm->tm_year + 1900;
        req.server_hello.month = tm->tm_mon + 1;
        req.server_hello.day = tm->tm_mday;
        req.server_hello.hour = tm->tm_hour;
        req.server_hello.minute = tm->tm_min;
        req.server_hello.second = tm->tm_sec;
        return this;
    }

    Message *finish() {
        reset(3);
        return this;
    }
};

MessageSchema *SCHEMA = (new MessageSchema())

    ->uint32(offsetof(Message, head_signature))
        ->throws(1, "invalid head signature")
        ->when(Message::HEAD_SIGNATURE)->end()

    ->int8(offsetof(Message, message_type))
        ->throws(2, "invalid message type")

        ->when(1)
            ->str(offsetof(Message, req.hello.name))
        ->end()

        ->when(2)
            ->uint16(offsetof(Message, req.server_hello.year))
            ->uint8(offsetof(Message, req.server_hello.month))
            ->uint8(offsetof(Message, req.server_hello.day))
            ->uint8(offsetof(Message, req.server_hello.hour))
            ->uint8(offsetof(Message, req.server_hello.minute))
            ->uint8(offsetof(Message, req.server_hello.second))
            ->str(offsetof(Message, req.server_hello.server_name))
        ->end()

        ->when(3)
        ->end()

    ->uint32(offsetof(Message, tail_signature))
        ->throws(1, "invalid tail signature")
        ->when(Message::TAIL_SIGNATURE)
        ->end()
;


class MessageHandler : public Handler
{
    protected:
        MessageEncoder *encoder;
        time_t startTime = 0;
        time_t lastTime = 0;
        int state = 0;

        Message in;
        Message out;

    public:

        MessageHandler() { encoder = new MessageEncoder(SCHEMA); }
        virtual ~MessageHandler() { delete encoder; }

        virtual bool connected() {
            printf(M_INFO "Connected: %s:%u\n", channel->socket->remote->getAddrStr(), channel->socket->remote->getPort());
            return true;
        }

        virtual void disconnected() {
            printf(M_INFO "Disconnected: %s:%u\n", channel->socket->remote->getAddrStr(), channel->socket->remote->getPort());
        }

        bool update (time_t time, Buffer *output)
        {
            time -= startTime;
            if (lastTime == time) return true;
            lastTime = time;

            handleClock(time);
            return true;
        }

        void send (Message *msg = nullptr) {
            msg = msg ? msg : &out;
            if (!encoder->write(channel->stream->output, msg))
                printf(M_ERR "Failed to send message\n");
            msg->dump(M_OUT);
        }

        virtual void handleClock (time_t time) = 0;
        virtual bool handleMessage() = 0;

        bool dataAvailable (Buffer *input, Buffer *output)
        {
            printf(GREY "Data available: %u\n" DEF, input->bytesAvailable());

            while (true)
            {
                try {
                    if (!encoder->read(input, &in)) break;
                    in.dump(M_IN);

                    bool result = handleMessage();
                    in.clear();

                    if (!result)
                        return false;
                }
                catch (const Error &e) {
                    printf(M_ERR "%s\n", e.message());
                    input->readUInt8(); // consume the invalid byte
                }
            }

            return true;
        }

};


class ServerHandler : public MessageHandler
{
    public:

        ServerHandler() : MessageHandler() {
            printf(M_DEBUG "ServerHandler created\n");
        }
        virtual ~ServerHandler() {
            printf(M_DEBUG "ServerHandler destroyed\n");
        }

        void disconnected() {
            if (PARALLEL_MODE)
                manager->exit();
        }

        virtual void handleClock(time_t time)
        {
            if (time < 700) return;
            startTime = time;

            switch (state)
            {
                case 0:
                    send(out.server_hello("Server Name is Selfo"));
                    state++;
                    break;
            }
        }

        bool handleMessage()
        {
            switch (in.message_type)
            {
                case 1: // hello
                    send(out.server_hello("I am Selfo"));
                    break;

                case 2: // server_hello
                    break;

                case 3: // finish
                    send(out.finish());
                    return false;
            }

            return true;
        }
};


class ClientHandler : public MessageHandler
{
    public:

        ClientHandler() : MessageHandler() {
            printf(M_DEBUG "ClientHandler created\n");
        }
        virtual ~ClientHandler() {
            printf(M_DEBUG "ClientHandler destroyed\n");
        }

        void disconnected() {
            manager->exit();
        }

        bool handleMessage()
        {
            switch (in.message_type)
            {
                case 1: // hello
                    break;

                case 2: // server_hello
                    break;

                case 3: // finish
                    return false;
            }

            return true;
        }

        virtual void handleClock(time_t time)
        {
            if (time < 500) return;
            startTime = time;

            switch (state)
            {
                case 0:
                    send(out.hello("John Doe"));
                    state++;
                    break;

                case 1:
                    send(out.hello("Jane Doe"));
                    state++;
                    break;

                case 2:
                    send(out.finish());
                    state++;
                    break;
            }
        }

};

int main (int argc, const char *argv[])
{
    if (argc < 2) {
        printf(M_ERR "Usage: %s <server|client>\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "server") == 0)
    {
        printf(M_INFO "Starting server on port 1000...\n");
        auto server = new Server([]() { return (Handler*)new ServerHandler(); }, 1000);
        if (!server->ready()) {
            printf(M_ERR "Failed to start server\n");
            return 1;
        }

        server->enableParallel(PARALLEL_MODE);
        printf(GREY "[Server ready, press Ctrl+C to exit]\n" DEF);
    }
    else
    {
        printf(M_INFO "Connecting to server...\n");
        Channel *channel = Channel::connect (new ClientHandler(), "127.0.0.1", 1000);
    }

    struct : IUpdate {
        time_t lastTime = 0;
        void update (time_t time) {
            if (lastTime == 0) lastTime = time - 5000;
            time_t delta = time - lastTime;
            if (delta < 5000) return;
            lastTime = time;
            printf(GREY "[MEM] blocks=%u, size=%.1f KB, peak=%.1f KB, channels=%u, handled=%u\n" DEF, 
                asr::memblocks, 
                asr::memsize / 1024.0, 
                asr::peak_memsize / 1024.0,
                manager->activeChannels(),
                manager->totalChannels()
            );
        }
    } mem_reporter;

    manager->main(&mem_reporter);
    return 0;
}
