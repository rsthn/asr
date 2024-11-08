
#include <asr/ptr>

namespace asr
{
    std::unordered_map<void*, int> *refs_memory = nullptr;

    void *refs::add (void *ptr)
    {
        if (!refs_memory)
            refs_memory = new std::unordered_map<void*, int>;

        if (!ptr) return ptr;

        auto it = refs_memory->find(ptr);
        if (it == refs_memory->end())
            refs_memory->insert({ptr, 1});
        else
            it->second++;
        return ptr;
    }

    bool refs::remove (void *ptr)
    {
        if (!refs_memory || !ptr)
            return false;

        auto it = refs_memory->find(ptr);
        if (it == refs_memory->end() || --it->second > 0)
            return false;

        refs_memory->erase(it);
        return true;
    }

    void refs::shutdown()
    {
        if (refs_memory != nullptr) {
            delete refs_memory;
            refs_memory = nullptr;
        }
    }
};
