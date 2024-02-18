
#include <asr/events/EventBus>

namespace asr {
namespace events {

    /**
     * @brief Next free event code.
     */
    int EventBus::availableEventCode = 1;

    /**
    */
    EventBus::EventBus()
    {
        for (int i = 0; i < ASR_MAX_EVENT_CODES; i++)
            this->listeners[i] = nullptr;

        this->queue = new List<Event*>();
    }

    /**
    */
    EventBus::~EventBus()
    {
        for (int i = 0; i < ASR_MAX_EVENT_CODES; i++)
            if (this->listeners[i] != nullptr) delete this->listeners[i];

        delete this->queue;
    }


    /**
     * @brief Resets the event bus by removing all listeners and empying the queue.
     */
    void EventBus::reset()
    {
        for (int i = 0; i < ASR_MAX_EVENT_CODES; i++)
        {
            if (this->listeners[i] != nullptr) {
                delete this->listeners[i];
                this->listeners[i] = nullptr;
            }
        }

        this->queue->clear();
    }


    /**
     * @brief Adds an event listener for a specified event.
     * 
     * @param eventGroup 
     * @param eventCode
     * @param handler
     * @return bool
     */
    bool EventBus::on (int eventGroup, int eventCode, void (*handler)(Event*))
    {
        eventGroup = EVTGRP(eventGroup);
        eventCode &= ASR_EVENT_CODE_MASK;
        if (eventCode < 0 || eventCode >= ASR_MAX_EVENT_CODES)
            return false;

        if (this->listeners[eventCode] == nullptr)
            this->listeners[eventCode] = new List<EventListener*>();

        EventListener *evl = new EventListener (eventGroup, handler);
        this->listeners[eventCode]->push(evl);
        return true;
    }


    /**
     * @brief Removes an event listener from the bus. If only the event code or group is provided all the
     * handlers attached to that event or group will be removed.
     * 
     * @param eventGroup 
     * @param eventCode 
     * @param handler 
     * @return bool 
     */
    bool EventBus::off (int eventGroup, int eventCode, void (*handler)(Event*))
    {
        eventGroup = EVTGRP(eventGroup);
        eventCode &= ASR_EVENT_CODE_MASK;
        if (eventCode < 0 || eventCode >= ASR_MAX_EVENT_CODES)
            return false;

        if (eventCode == 0)
        {
            for (int j = 0; j < ASR_MAX_EVENT_CODES; j++)
            {
                List<EventListener*> *list = this->listeners[j];
                if (list == nullptr) continue;

                Linkable<EventListener*> *ni = nullptr;

                for (Linkable<EventListener*> *i = list->head(); i != nullptr; i = ni)
                {
                    bool k = true;
                    ni = i->next();

                    if (handler != nullptr)
                        k = k && i->value->handler == handler;

                    if (eventGroup != 0)
                        k = k && i->value->eventGroup == eventGroup;

                    if (k == true)
                        delete list->remove(i);
                }
            }
        }
        else
        {
            List<EventListener*> *list = this->listeners[eventCode];
            if (list == nullptr) return true;

            Linkable<EventListener*> *ni = nullptr;

            for (Linkable<EventListener*> *i = list->head(); i != nullptr; i = ni)
            {
                bool k = true;
                ni = i->next();

                if (handler != nullptr)
                    k = k && i->value->handler == handler;

                if (eventGroup != 0)
                    k = k && i->value->eventGroup == eventGroup;

                if (k == true)
                    delete list->remove(i);
            }
        }

        return true;
    }


    /**
     * @brief Silences or unsilences all handlers attached to an event. If the event fires, silenced handler
     * will not be called.
     * 
     * @param eventGroup 
     * @param eventCode 
     * @param value 
     * @return bool 
     */
    bool EventBus::silence (int eventGroup, int eventCode, bool value)
    {
        eventGroup = EVTGRP(eventGroup);
        eventCode &= ASR_EVENT_CODE_MASK;
        if (eventCode < 0 || eventCode >= ASR_MAX_EVENT_CODES)
            return false;

        int silent = value ? 1 : -1;

        if (eventCode == 0)
        {
            for (int j = 0; j < ASR_MAX_EVENT_CODES; j++)
            {
                List<EventListener*> *list = this->listeners[j];
                if (list == nullptr) continue;

                for (Linkable<EventListener*> *i = list->head(); i != nullptr; i = i->next())
                {
                    bool k = true;

                    if (eventGroup != 0)
                        k = k && i->value->eventGroup == eventGroup;

                    if (k == true)
                        i->value->silent += silent;
                }
            }
        }
        else
        {
            List<EventListener*> *list = this->listeners[eventCode];
            if (list == nullptr) return true;

            for (Linkable<EventListener*> *i = list->head(); i != nullptr; i = i->next())
            {
                bool k = true;

                if (eventGroup != 0)
                    k = k && i->value->eventGroup == eventGroup;

                if (k == true)
                    i->value->silent += silent;
            }
        }

        return true;
    }


    /**
     * @brief Prepares an event for its later use. The event is started by calling the `resume` method. Note that
     * NULL is returned if there is an error with the eventCode or if there are no listeners for the event.
     * 
     * @param event 
     * @return Event* 
     */
    Event *EventBus::prepare (Event *event)
    {
        int eventGroup = event->eventCode & ASR_EVENT_GRP_MASK;
        int eventCode = event->eventCode & ASR_EVENT_CODE_MASK;
        if (eventCode < 1 || eventCode >= ASR_MAX_EVENT_CODES)
            return nullptr;

        List<EventListener*> *list = new List<EventListener*> ();

        if (this->listeners[eventCode] != nullptr)
        {
            for (Linkable<EventListener*> *i = this->listeners[eventCode]->head(); i; i = i->next())
            {
                if (i->value->silent > 0 || (eventGroup != 0 && i->value->eventGroup != eventGroup))
                    continue;

                list->push(i->value);
            }
        }

        if (this->listeners[0] != nullptr)
        {
            for (Linkable<EventListener*> *i = this->listeners[0]->head(); i; i = i->next())
            {
                if (i->value->silent > 0 || (eventGroup != 0 && i->value->eventGroup != eventGroup))
                    continue;

                list->push(i->value);
            }
        }

        if (list->length() == 0) {
            delete list;
            return nullptr;
        }

        event->reset(list);
        return event;
    }


    /**
     * @brief Dispatches an event to the respective listeners.
     * @param event 
     */
    void EventBus::dispatch (Event *event)
    {
        Event *evt = this->prepare(event);
        if (evt == nullptr) return;
        evt->resume()->destroy();
    }


    /**
     * @brief Adds an event to the event bus's queue. Delayed events are dispatched by calling `dispatch`. If there
     * is no matching listener for the desired event no handler will be enqueued.
     * 
     * @param event 
     */
    void EventBus::enqueue (Event *event)
    {
        Event *evt = this->prepare(event);
        if (evt == nullptr) return;
        this->queue->push(evt);
    }


    /**
     * @brief Dispatches all the delayed events added to the event bus's queue by using `enqueue`.
     */
    void EventBus::dispatch()
    {
        Event *evt;
        while ((evt = this->queue->shift()) != nullptr) {
            evt->resume()->destroy();
        }
    }

}};
