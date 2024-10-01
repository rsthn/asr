#include <string>
#include <iostream>

#include <asr/event-bus>

using namespace asr;
using namespace std;


/**
 * Allocate event codes and groups.
 */
const int EventA = EventBus::getCode("EventA");
const int EventB = EventBus::getCode("EventB");

const int GroupA = EventBus::getGroup("GroupA");
const int GroupB = EventBus::getGroup("GroupB");
const int GroupC = EventBus::getGroup("GroupC");


/**
 * Custom event.
 */
class EventX : public Event
{
    public:

    static int code;
    string value;

    EventX() : Event(EventX::code), value("dummy") { }
    EventX(const char *value) : Event(EventX::code), value(value) { }

    void dump() {
        printf("  EventX: %s\n", value.c_str());
    }
};

int EventX::code = EventBus::getCode("EventX");


/**
 * Test entry point.
 */
void test()
{
    EventBus bus;

    // Add listeners.
    bus.on(EventA, [](Event* e) { cout << "  EventA" << endl; });
    bus.on(EventB, [](Event* e) { cout << "  EventB" << endl; });

    bus.on(GroupA, EventA, [](Event* e) {
        cout << "  GroupA::EventA" << endl;
    });

    bus.on(GroupB, EventA, [](Event* e) {
        cout << "  GroupB::EventA" << endl;
        e->enqueue(new EventX("triggered from group_b"));
    });

    bus.on(GroupC, EventA, [](Event* e) {
        cout << "  GroupC::EventA" << endl;
        e->enqueue(new EventX("triggered from group_c"));
    });

    bus.on(EventX::code, [](Event* e) {
        dynamic_cast<EventX*>(e)->dump();
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
