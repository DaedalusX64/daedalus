#pragma once

#include <algorithm>
#include <cstdint>
#include <cstring>

// Bounded, allocation-free transaction for DSP writes. Reads observe earlier
// writes in command order; commit touches only the bytes the DSP changed.
template<unsigned Records, unsigned Bytes>
class alignas(64) AudioMemoryJournal
{
    struct Entry { std::uint32_t address, offset, size; };
    Entry entries[Records];
    unsigned char data[Bytes];
    unsigned count = 0, used = 0;
public:
    void Reset() { count = used = 0; }
    bool Write(std::uint32_t address, const void *source, unsigned size)
    {
        if (count == Records || size > Bytes - used)
            return false;
        std::memcpy(data + used, source, size);
        entries[count++] = {address, used, size};
        used += size;
        return true;
    }
    void Read(const unsigned char *memory, std::uint32_t address, void *dest, unsigned size) const
    {
        std::memcpy(dest, memory + address, size);
        for (unsigned i = 0; i < count; ++i)
        {
            const auto &e = entries[i];
            const auto first = std::max(address, e.address);
            const auto last = std::min(address + size, e.address + e.size);
            if (first < last)
                std::memcpy(static_cast<unsigned char *>(dest) + first - address,
                    data + e.offset + first - e.address, last - first);
        }
    }
    void Commit(unsigned char *memory) const
    {
        for (unsigned i = 0; i < count; ++i)
        {
            const auto &e = entries[i];
            std::memcpy(memory + e.address, data + e.offset, e.size);
        }
    }
};
