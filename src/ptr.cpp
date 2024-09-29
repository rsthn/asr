
#include <asr/ptr>

namespace asr
{
    std::unordered_map<void*, int> *refs::memory = nullptr;

    void *refs::add (void *ptr)
    {
        if (!memory)
            memory = new std::unordered_map<void*, int>;

        if (!ptr) return ptr;

        auto it = memory->find(ptr);
        if (it == memory->end())
            memory->insert({ptr, 1});
        else
            it->second++;
        return ptr;
    }

    bool refs::remove (void *ptr)
    {
        if (!memory || !ptr)
            return false;

        auto it = memory->find(ptr);
        if (it == memory->end() || --it->second > 0)
            return false;

        memory->erase(it);
        return true;
    }

    void refs::shutdown()
    {
        if (memory != nullptr) {
            delete memory;
            memory = nullptr;
        }
    }
};
