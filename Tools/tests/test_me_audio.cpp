#include "SysPSP/ME/AudioMemoryJournal.h"
#include "SysPSP/ME/AudioPacketStream.h"
#include <array>
#include <cassert>
#include <thread>
#include <iostream>

static std::uint32_t Stereo(std::int16_t left, std::int16_t right)
{
    return std::uint16_t(left) | (std::uint32_t(std::uint16_t(right)) << 16);
}

static void TestJournal()
{
    std::array<unsigned char, 128> ram;
    ram.fill(0xab);
    AudioMemoryJournal<8, 64> journal;
    unsigned char first[]{1, 2, 3, 4}, second[]{9, 10};
    assert(journal.Write(31, first, sizeof(first)));
    assert(journal.Write(32, second, sizeof(second)));
    first[0] = 77; // Requests retain values, never pointers into the ME stack.
    std::array<unsigned char, 8> view;
    journal.Read(ram.data(), 29, view.data(), view.size());
    assert((view == std::array<unsigned char, 8>{0xab, 0xab, 1, 9, 10, 4, 0xab, 0xab}));
    assert(ram[31] == 0xab); // DSP writes remain private until SP completion.
    ram[30] = 0x55;
    ram[35] = 0x66; // CPU changes on the same cache line survive publication.
    journal.Commit(ram.data());
    assert(ram[30] == 0x55 && ram[31] == 1 && ram[32] == 9 && ram[33] == 10);
    assert(ram[34] == 4 && ram[35] == 0x66);
    journal.Reset();
    journal.Read(ram.data(), 31, view.data(), 4);
    assert(view[0] == 1 && view[1] == 9 && view[2] == 10 && view[3] == 4);

    AudioMemoryJournal<2, 4> bounded;
    assert(bounded.Write(0, second, 2));
    assert(!bounded.Write(4, first, 4)); // Failed append does not erase prior work.
    assert(bounded.Write(4, second, 2));
    assert(!bounded.Write(6, second, 1));
    bounded.Commit(ram.data());
    assert(ram[0] == 9 && ram[1] == 10 && ram[4] == 9 && ram[5] == 10);
}

static void TestStream()
{
    AudioPacketStream<2, 2> stream;
    const std::uint32_t a[]{Stereo(-1000, 1000), Stereo(0, 0)};
    const std::uint32_t b[]{Stereo(1000, -1000), Stereo(2000, -2000)};
    assert(!stream.Push(a, 0, 22050, 44100));
    assert(!stream.Push(a, 2, 0, 44100));
    assert(!stream.Push(a, 3, 22050, 44100));
    assert(stream.Push(a, 2, 22050, 44100));
    assert(stream.Push(b, 2, 22050, 44100));
    assert(!stream.Push(a, 2, 22050, 44100));
    std::uint32_t phase = 0, sample;
    const std::int16_t expected[]{-1000, -500, 0, 500, 1000, 1500, 2000, 2000};
    for (auto left : expected)
    {
        assert(stream.Next(phase, sample));
        assert(sample == Stereo(left, -left));
    }
    assert(!stream.Next(phase, sample) && sample == 0 && phase == 0);
    for (int i = 0; i < 20; ++i)
    {
        assert(stream.Push(a, 2, 44100, 44100));
        assert(stream.Next(phase, sample) && sample == a[0]);
        assert(stream.Next(phase, sample) && sample == a[1]);
    }
    // High rates with short packets must not index past a newly queued packet.
    stream.Reset();
    assert(stream.Push(a, 1, 192000, 44100));
    assert(stream.Next(phase, sample) && sample == a[0]);
    assert(stream.Push(b, 1, 192000, 44100));
    assert(!stream.Next(phase, sample) && sample == 0);
    assert(stream.Push(a, 1, 44100, 44100));
    assert(stream.Next(phase, sample) && sample == a[0]);
}

static void TestConcurrentPackets()
{
    AudioPacketStream<4, 1> stream;
    constexpr unsigned count = 50000;
    auto value = [](unsigned i) {
        const auto left = static_cast<std::int16_t>(int(i % 30000) - 16000);
        return Stereo(left, -left);
    };
    std::thread producer([&] {
        for (unsigned i = 0; i < count; ++i)
        {
            const auto sample = value(i);
            while (!stream.Push(&sample, 1, 44100, 44100))
                std::this_thread::yield();
        }
    });
    std::uint32_t phase = 0, sample;
    for (unsigned i = 0; i < count; ++i)
    {
        while (!stream.Next(phase, sample))
            std::this_thread::yield();
        assert(sample == value(i));
    }
    producer.join();
}

int main()
{
    TestJournal();
    TestStream();
    TestConcurrentPackets();
    std::cout << "ME audio journal and PCM stream tests passed\n";
}
