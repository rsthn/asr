
#include <asr/events/EventBus>
#include <iostream>

namespace asr {
namespace events {

    /**
     * Next free event code.
     */
    int EventBus::availableEventCode = 1;

    /**
     * Removes all listeners and empties the queue.
     */
    void EventBus::reset()
    {
        for (int i = 0; i < ASR_MAX_EVENT_CODES; i++)
            listeners[i].clear();

        queue.clear();
    }


    /**
     * Adds an event listener for a specified event.
     * 
     * @param eventGroup 
     * @param eventCode
     * @param handler
     * @return bool
     */
    bool EventBus::on (int eventGroup, int eventCode, EventHandler *handler)
    {
        eventGroup = EVTGRP(eventGroup);
        eventCode &= ASR_EVENT_CODE_MASK;
        if (eventCode < 0 || eventCode >= ASR_MAX_EVENT_CODES)
            return false;

        listeners[eventCode].push_back(new EventListener (eventGroup, handler));
        return true;
    }


    /**
     * Removes an event listener from the bus. If only the event code or group is provided all the
     * handlers attached to that event or group will be removed.
     * 
     * @param eventGroup 
     * @param eventCode 
     * @param handler 
     * @return bool 
     */
    bool EventBus::off (int eventGroup, int eventCode, EventHandler *handler)
    {
        eventGroup = EVTGRP(eventGroup);
        eventCode &= ASR_EVENT_CODE_MASK;
        if (eventCode < 0 || eventCode >= ASR_MAX_EVENT_CODES)
            return false;

        if (eventCode == 0)
        {
            for (int j = 0; j < ASR_MAX_EVENT_CODES; j++)
            {
                std::list<ptr<EventListener>>& list = listeners[j];

                for (std::list<ptr<EventListener>>::iterator i = list.begin(); i != list.end(); i++)
                {
                    bool k = true;

                    if (handler != nullptr)
                        k = k && (*i)->handler == handler;

                    if (eventGroup != 0)
                        k = k && (*i)->eventGroup == eventGroup;

                    if (k == true)
                        list.erase(i);
                }
            }
        }
        else
        {
            std::list<ptr<EventListener>>& list = listeners[eventCode];

            for (std::list<ptr<EventListener>>::iterator i = list.begin(); i != list.end(); i++)
            {
                bool k = true;

                if (handler != nullptr)
                    k = k && (*i)->handler == handler;

                if (eventGroup != 0)
                    k = k && (*i)->eventGroup == eventGroup;

                if (k == true)
                    list.erase(i);
            }
        }

        return true;
    }


    /**
     * Silences or unsilences all handlers attached to an event. If the event fires, silenced handler
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
                std::list<ptr<EventListener>>& list = this->listeners[j];

                for (std::list<ptr<EventListener>>::iterator i = list.begin(); i != list.end(); i++)
                {
                    bool k = true;

                    if (eventGroup != 0)
                        k = k && (*i)->eventGroup == eventGroup;

                    if (k == true)
                        (*i)->silent += silent;
                }
            }
        }
        else
        {
            std::list<ptr<EventListener>>& list = this->listeners[eventCode];

            for (std::list<ptr<EventListener>>::iterator i = list.begin(); i != list.end(); i++)
            {
                bool k = true;

                if (eventGroup != 0)
                    k = k && (*i)->eventGroup == eventGroup;

                if (k == true)
                    (*i)->silent += silent;
            }
        }

        return true;
    }


    /**
     * Prepares an event for its later use. The event is started by calling the `resume` method. Note that
     * NULL is returned if there is an error with the eventCode or if there are no listeners for the event.
     * 
     * @param event 
     * @return bool
     */
    bool EventBus::prepare (ptr<Event> &event)
    {
        int eventGroup = event->eventCode & ASR_EVENT_GRP_MASK;
        int eventCode = event->eventCode & ASR_EVENT_CODE_MASK;
        if (eventCode < 1 || eventCode >= ASR_MAX_EVENT_CODES)
            return false;

        std::list<ptr<EventListener>> list;

        for (std::list<ptr<EventListener>>::iterator i = this->listeners[eventCode].begin(); i != this->listeners[eventCode].end(); i++)
        {
            if ((*i)->silent > 0 || (eventGroup != 0 && (*i)->eventGroup != eventGroup))
                continue;

            list.push_back(*i);
        }

        for (std::list<ptr<EventListener>>::iterator i = this->listeners[0].begin(); i != this->listeners[0].end(); i++)
        {
            if ((*i)->silent > 0 || (eventGroup != 0 && (*i)->eventGroup != eventGroup))
                continue;

            list.push_back(*i);
        }

        if (list.size() == 0)
            return false;

        event->prepare(this, std::move(list));
        return true;
    }


    /**
     * Dispatches an event to the respective listeners.
     * @param event 
     */
    void EventBus::dispatch (ptr<Event> event)
    {
        if (prepare(event))
            event->resume();
    }

    /**
     * Adds an event to the event bus's queue. Delayed events are dispatched by calling `dispatch`. If there
     * is no matching listener for the desired event no handler will be queued.
     * 
     * @param event 
     */
    void EventBus::enqueue (ptr<Event> event)
    {
        if (prepare(event))
            queue.push_back(event);
    }

    /**
     * Dispatches all the delayed events added to the event bus.
     */
    void EventBus::dispatch()
    {
        while (queue.size() != 0) {
            ptr<Event> evt = queue.front();
            queue.pop_front();
            evt->resume();
        }
    }

}};
