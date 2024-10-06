#include <thread>
#include <chrono>
#include <memory>
#include <cassert>

#include "Base/Types.h"
#include "System/Thread.h"



namespace
{

std::thread::id HandleToStdThread(ThreadHandle& handle)
{
    return handle ? handle->get_id() : std::thread::id();
}

ThreadHandle StdThreadToHandle(std::thread&& thread)
{
    return std::make_unique<std::thread>(std::move(thread));  // Return unique_ptr to std::thread
}

struct SDaedThreadDetails
{
    SDaedThreadDetails(DaedThread function, void* argument)
        : ThreadFunction(function), Argument(argument)
    {
    }

    DaedThread ThreadFunction;
    void* Argument;
};

// The real thread is passed in as an argument. We call it and return the result.
void StartThreadFunc(SDaedThreadDetails* thread_details)
{
    int result = thread_details->ThreadFunction(thread_details->Argument);

    delete thread_details;
}

} // anonymous namespace

ThreadHandle CreateThread(const char* name [[maybe_unused]], DaedThread function, void* argument)
{
    auto thread_details = new SDaedThreadDetails(function, argument);

    try
    {
        std::thread thread(StartThreadFunc, thread_details);
        return StdThreadToHandle(std::move(thread));
    }
    catch (...)
    {
        delete thread_details;
        return nullptr;
    }
}

bool JoinThread(ThreadHandle& handle, s32 timeout [[maybe_unused]])
{
    if (handle && handle->joinable())
    {
        handle->join();
        return true;
    }
    return false;
}

void ThreadSleepMs(u32 ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void ThreadSleepTicks(u32 ticks)
{
    std::this_thread::sleep_for(std::chrono::microseconds(ticks));
}

void ThreadYield()
{
    std::this_thread::yield();
}
