#pragma once

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstring>

// SC publishes copied stereo PCM packets. ME consumes/resamples them without
// retaining pointers into RDRAM that the game can subsequently reuse.
template<unsigned Slots, unsigned Frames>
struct alignas(64) AudioPacketStream
{
    static_assert(std::atomic<std::uint32_t>::is_always_lock_free);
    struct Packet { std::uint32_t count, step; std::uint32_t samples[Frames]; };
    std::atomic<std::uint32_t> written{0}, read{0};
    Packet packets[Slots];

    void Reset() { written.store(0); read.store(0); }
    bool Push(const void *source, unsigned count, unsigned frequency, unsigned output_frequency)
    {
        if (!count || count > Frames || !frequency || frequency > 192000 || !output_frequency)
            return false;
        const std::uint32_t writer = written.load(std::memory_order_relaxed);
        if (writer - read.load(std::memory_order_acquire) == Slots)
            return false;
        auto &p = packets[writer % Slots];
        p.count = count;
        p.step = std::max<std::uint32_t>(1, (frequency << 12) / output_frequency);
        std::memcpy(p.samples, source, count * sizeof(std::uint32_t));
        written.store(writer + 1, std::memory_order_release);
        return true;
    }
    bool Next(std::uint32_t &phase, std::uint32_t &sample)
    {
        std::uint32_t reader = read.load(std::memory_order_relaxed);
        // A high resampling step can cross packets published between calls.
        while (reader != written.load(std::memory_order_acquire)
            && (phase >> 12) >= packets[reader % Slots].count)
        {
            phase -= packets[reader % Slots].count << 12;
            ++reader;
        }
        read.store(reader, std::memory_order_release);
        if (reader == written.load(std::memory_order_acquire))
        {
            phase = 0;
            sample = 0;
            return false;
        }
        const auto &p = packets[reader % Slots];
        const unsigned index = phase >> 12;
        const std::uint32_t a = p.samples[index];
        std::uint32_t b = a;
        if (index + 1 < p.count)
            b = p.samples[index + 1];
        else if (reader + 1 != written.load(std::memory_order_acquire))
            b = packets[(reader + 1) % Slots].samples[0];
        const std::int32_t left = static_cast<std::int16_t>(a);
        const std::int32_t right = static_cast<std::int16_t>(a >> 16);
        const std::int32_t fraction = phase & 4095;
        const auto l = left + ((static_cast<std::int16_t>(b) - left) * fraction >> 12);
        const auto r = right + ((static_cast<std::int16_t>(b >> 16) - right) * fraction >> 12);
        sample = static_cast<std::uint16_t>(l) | (std::uint32_t(static_cast<std::uint16_t>(r)) << 16);
        phase += p.step;
        // High input rates can skip more than one short packet.
        while (reader != written.load(std::memory_order_acquire) && (phase >> 12) >= packets[reader % Slots].count)
        {
            phase -= packets[reader % Slots].count << 12;
            ++reader;
        }
        read.store(reader, std::memory_order_release);
        return true;
    }
};
