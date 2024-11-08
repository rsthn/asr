
#include <asr/data-schema-reader>
#include <iostream>
#include <chrono>

using namespace asr;
using namespace std;

class SubMessageType1
{
    public:

    int val1, val2, val3;

    void dump() {
        cout << "  SubMessageType1: " << val1 << " " << val2 << " " << val3 << endl;
    }
};

class SubMessageType2
{
    public:

    int val1, val2;

    void dump() {
        cout << "  SubMessageType2: " << val1 << " " << val2 << endl;
    }
};

class Message : public IDataSchema
{
    protected:

    void init_schema(DataSchema *schema) const
    {
        signature = uint32be()
        version = int8().values(1, 2)
        checksum = uint16be()

        xasdasd

        Message
            signature uint32be

        schema
        ->uint32be(&Message::signature)
        ->int8(&Message::version)
            ->throws(1, "invalid version")
            ->when(1)->end()
            ->when(2)->end()
        ->uint16be(&Message::checksum)
        ->call(&Message::check)
        ;
    }

    public:

    unsigned int signature = 0;
    int version = 0;
    unsigned checksum = 0;

    int dump() {
        cout << "signature: " << signature << endl;
        cout << "version: " << version << endl;
        cout << "checksum: " << uppercase << hex << "0x" << checksum << endl;
        return 0;
    }

    int check() {
        cout << "prepare: " << version << endl;
        return 0;
    }
};

void test()
{
    unsigned char buffer[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0xAA, 0x55 };
    Buffer input_buffer;
    input_buffer.write((char*)buffer, sizeof(buffer));

    DataSchemaReader<Message> reader(schema, &input_buffer);

    try {
        auto message = reader.feed();
        if (!message) {
            cout << "\e[31m**** Message Incomplete ****\e[0m" << endl;
        }
        else {
            cout << "\e[32m**** Message Complete ****\e[0m" << endl;
            message->dump();
        }
    }
    catch (std::exception& e) {
        cout << "\e[91mError: " << e.what() << "\e[0m" << endl;
    }
    catch (Error& err) {
        cout << "\e[91mError: " << err.message() << "\e[0m" << endl;
    }
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
