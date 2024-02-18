#include <asr/events/EventBus>
#include <asr/events/Event>
#include <iostream>

using namespace asr::events;

const int EventA = ALLOCEVTCODE();
const int EventB = ALLOCEVTCODE();
const int GroupA = 1;
const int GroupB = 2;

int main (int argc, const char *argv[])
{
    EventBus *bus = new EventBus();

    bus->on(EventA, [](Event *e) {
        std::cout << "EventA" << std::endl;
    });

    bus->on(EventB, [](Event *e) {
        std::cout << "EventB" << std::endl;
    });

    bus->on(GroupA, EventA, [](Event *e) {
        std::cout << "GroupA::EventA" << std::endl;
    });

    bus->on(GroupB, EventA, [](Event *e) {
        std::cout << "GroupB::EventA" << std::endl;
    });

    Event *evt = (new Event(EventA))->keep();

    // Trigger EventA everywhere
    bus->dispatch(evt->setGroup(0));

    // Trigger on GroupB only
    bus->dispatch(evt->setGroup(GroupB));

    // Trigger on GroupA only
    bus->dispatch(evt->setGroup(GroupA));

    // Trigger EventB
    bus->dispatch(new Event(EventB));

    delete evt;
    delete bus;

    return 0;
}
