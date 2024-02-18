
#include <asr/events/Event>

namespace asr {
namespace events {

    /**
     * @brief Destroy the event and all its chained events.
     */
    Event::~Event ()
    {
        delete this->list->reset();
        if (this->next != nullptr)
            delete this->next;
    }


    /**
     * @brief Resets the event to its initial state. An event object can be reused by resetting it and later
     * calling the resume method.
     */
    void Event::reset (List<EventListener*> *list)
    {
        if (list != nullptr)
        {
            if (this->list != nullptr)
                delete this->list->reset();
            this->list = list;
        }

        this->isAsync = false;
        this->i = this->list->head();

        if (this->next != nullptr)
            this->next->reset();
    }


    /**
     * @brief Resumes event propagation. Should be called if an event handler had previously called `wait`.
     * @return Event*
     */
    Event *Event::resume()
    {
        this->isAsync = false;

        while (!this->isAsync && this->i != nullptr)
        {
            if (this->i->value->silent) {
                this->i = this->i->next();
                continue;
            }

            this->i->value->handler (this);
            this->i = this->i->next();
        }

        if (this->i == nullptr && this->next != nullptr)
            this->next->resume();

        return this;
    }

    /**
     * @brief Adds the specified event to the bottom of the event chain.
     * @param event 
     */
    void Event::enqueue (Event *event)
    {
        if (event == nullptr) return;

        Event *evt = this;
        while (evt->next != nullptr) evt = evt->next;

        evt->next = event;
        event->root = this;
    }

}};
