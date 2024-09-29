#include <string>
#include <iostream>

#include <asr/events/EventBus>
#include <asr/events/Event>

using namespace asr::events;
using namespace std;

const int EventA = ALLOCEVTCODE();
const int EventB = ALLOCEVTCODE();
const int GroupA = 1;
const int GroupB = 2;
const int GroupC = 3;

class EventX : public Event
{
    public:

    static int code;
    string value;

    EventX() : Event(EventX::code), value("dummy")
    { }

    EventX(const char *value) : Event(EventX::code), value(value)
    { }

    EventX& setValue (string& other_value) {
        value = other_value;
        return *this;
    }

    void dump() {
        printf("EventX: %s\n", value.c_str());
    }
};

int EventX::code = ALLOCEVTCODE();

void test()
{
    EventBus bus;

    bus.on(EventA, [](Event* e) {
        cout << "  EventA" << endl;
    });

    bus.on(EventB, [](Event* e) {
        cout << "  EventB" << endl;
    });

    bus.on(EventX::code, [](Event* e) {
        cout << "  " << dynamic_cast<EventX*>(e)->value << endl;
    });

    bus.on(GroupA, EventA, [](Event* e) {
        cout << "  GroupA::EventA" << endl;
    });

    bus.on(GroupB, EventA, [](Event* e) {
        cout << "  GroupB::EventA" << endl;
        e->enqueue(new EventX("event_x from group_b"));
    });

    bus.on(GroupC, EventA, [](Event* e) {
        cout << "  GroupC::EventA" << endl;
        e->enqueue(new EventX("event_x from group_c"));
    });

    // Trigger EventA global.
    cout << "EventA (global):" << endl;
    bus.dispatch(new Event(EventA));
    cout << "\n";

    // Trigger on GroupB only
    cout << "GroupB only:" << endl;
    bus.dispatch(new Event(EventA, GroupB));
    cout << "\n";

    // Trigger on GroupA only
    cout << "GroupA only:" << endl;
    bus.dispatch(new Event(EventA, GroupA));
    cout << "\n";

    // Trigger EventB global.
    cout << "EventB (global):" << endl;
    bus.dispatch(new Event(EventB));
    cout << "\n";
}

int main (int argc, const char *argv[])
{
    auto n = asr::memblocks;

    test();

    asr::refs::shutdown();
    if (asr::memblocks != n)
        cout << "\e[31mMemory leak detected: \e[91m" << asr::memsize << " bytes\e[0m\n";

    return 0;
}
