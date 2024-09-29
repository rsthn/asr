
#include <asr/events/EventBus>
#include <asr/events/Event>

namespace asr {
namespace events {

    /**
     * Resumes event propagation. Should be called if an event handler had previously called `wait`.
     */
    void Event::resume()
    {
        isAsync = false;
        while (!isAsync && curr != list.end())
        {
            if (!(*curr)->silent) (*curr)->handler(this);
            curr++;
        }

        if (curr == list.end() && next != nullptr)
            next->resume();
    }

    /**
     * Adds the specified event to the bottom of the event chain.
     * @param event 
     */
    void Event::enqueue (ptr<Event> event)
    {
        if (!bus->prepare(event))
            return;

        Event *evt = this;
        while (evt->next != nullptr)
            evt = evt->next.get();

        evt->next = event;
    }

}};
