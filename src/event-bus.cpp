
#include <asr/event-bus>
#include <iostream>

namespace asr {

    std::unordered_map<std::string, int> EventBus::event_codes;
    std::unordered_map<std::string, int> EventBus::group_codes;

    int EventBus::getCode(std::string name)
    {
        auto it = event_codes.find(name);
        if (it != event_codes.end())
            return it->second;

        int code = 1+event_codes.size();
        event_codes.insert({ name, code });
        return code;
    }

    int EventBus::getGroup(std::string name)
    {
        auto it = group_codes.find(name);
        if (it != group_codes.end())
            return it->second;

        int code = 1+group_codes.size();
        group_codes.insert({ name, code });
        return code;
    }

    void EventBus::reset() {
        queue.clear();
    }


    void EventBus::on(int group_code, int event_code, EventHandler *handler)
    {
        if (!listeners.count(event_code))
            listeners[event_code] = std::list<ptr<EventListener>>();

        listeners[event_code].push_back(new EventListener (group_code, handler));
    }


    void EventBus::off(int group_code, int event_code, EventHandler *handler)
    {
        if (event_code == 0)
        {
            for (auto j = listeners.begin(); j != listeners.end(); j++)
            {
                auto& list = j->second;
                for (auto i = list.begin(); i != list.end(); i++)
                {
                    bool k = true;
                    if (handler != nullptr)
                        k = k && (*i)->handler == handler;
                    if (group_code != 0)
                        k = k && (*i)->group_code == group_code;

                    if (k) list.erase(i);
                }
            }
        }
        else
        {
            auto it = listeners.find(event_code);
            if (it == listeners.end()) return;

            auto& list = it->second;
            for (auto i = list.begin(); i != list.end(); i++)
            {
                bool k = true;
                if (handler != nullptr)
                    k = k && (*i)->handler == handler;
                if (group_code != 0)
                    k = k && (*i)->group_code == group_code;

                if (k) list.erase(i);
            }
        }
    }


    void EventBus::silence(int group_code, int event_code, bool value)
    {
        int silent = value ? 1 : -1;
        if (event_code == 0)
        {
            for (auto j = listeners.begin(); j != listeners.end(); j++)
            {
                auto& list = j->second;
                for (auto i = list.begin(); i != list.end(); i++)
                {
                    bool k = true;
                    if (group_code != 0)
                        k = k && (*i)->group_code == group_code;

                    if (k) (*i)->silent += silent;
                }
            }
        }
        else
        {
            auto it = listeners.find(event_code);
            if (it == listeners.end()) return;

            auto& list = it->second;
            for (auto i = list.begin(); i != list.end(); i++)
            {
                bool k = true;
                if (group_code != 0)
                    k = k && (*i)->group_code == group_code;

                if (k) (*i)->silent += silent;
            }
        }
    }


    bool EventBus::prepare(ptr<Event> &event)
    {
        int group_code = event->group_code;
        int event_code = event->event_code;

        std::list<ptr<EventListener>> list;

        auto it = listeners.find(event_code);
        if (it != listeners.end())
        {
            auto& listeners = it->second;
            for (auto i = listeners.begin(); i != listeners.end(); i++) {
                if ((*i)->silent > 0 || (group_code != 0 && (*i)->group_code != group_code))
                    continue;
                list.push_back(*i);
            }
        }

        it = listeners.find(0);
        if (it != listeners.end())
        {
            auto& listeners = it->second;
            for (auto i = listeners.begin(); i != listeners.end(); i++) {
                if ((*i)->silent > 0 || (group_code != 0 && (*i)->group_code != group_code))
                    continue;
                list.push_back(*i);
            }
        }

        if (list.size() == 0)
            return false;

        event->prepare(this, std::move(list));
        return true;
    }


    void EventBus::dispatch(ptr<Event> event) {
        if (prepare(event))
            event->resume();
    }

    void EventBus::enqueue(ptr<Event> event) {
        if (prepare(event))
            queue.push_back(event);
    }

    void EventBus::dispatch()
    {
        while (queue.size() != 0) {
            ptr<Event> evt = queue.front();
            queue.pop_front();
            evt->resume();
        }
    }

};
