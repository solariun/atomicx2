#ifndef ARDUINO

#include <iostream>
#include <unistd.h>
#include <unistd.h>
#include <sys/time.h>
#include <unistd.h>

#include <cstring>
#include <cstdint>
#include <iostream>
#include <string>

#include "atomicx/atomicx.h"

atomicx_time atomicx::getTick (void)
{
#ifndef FAKE_TIMER
    usleep (20000);
    struct timeval tp;
    gettimeofday (&tp, NULL);

    return (atomicx_time)tp.tv_sec * 1000 + tp.tv_usec / 1000;
#else
    nCounter++;

    return nCounter;
#endif
}

void atomicx::sleepTicks(atomicx_time nSleep)
{
#ifndef FAKE_TIMER
    usleep ((useconds_t)nSleep * 1000);
#else
    while (nSleep); usleep(100);
#endif
}

atomicx::RefId tranporVar=0;

/**
 * @brief Test consumer thread
 */
class testThread : public atomicx::thread
{
public:
    testThread(size_t start) : thread(VMEM(vmemory)), start(start)
    {
        id = counterId++;
        std::cout << "Thread " << id << " created" << std::endl;
        setNice(500);
    }

    ~testThread() override
    {
    }

protected:
    bool run() override
    {
        std::cout << "Thread is running" <<  ", CTX:" << &atomicx::ctx << std::endl;
        size_t nCount=start;
        
        setNice(1000 * (id+1));
        atomicx::Tag tag;

        //while(yield())
        while(wait(tranporVar, tag, 0, 1))
        {
            auto metrics = getMetrics();
            std::cout << "Thread " << id << ": Thread is running " << nCount++ << ", size: " << metrics.stackSize << "/" << metrics.maxStackSize << ", Thread: " << sizeof(atomicx::thread) << ", Context: " << sizeof(atomicx::ctx ) << ", Tag: " << tag.param << "/" << tag.value << std::endl;

            notify(tranporVar, atomicx::Notify::ONE, {id, nCount}, 0, 1);
        }

        std::cout << "Thread " << id << " stopped" << std::endl;

        exit(1);

        return true;
    }

    bool StackOverflow() override
    {
        return false;
    }

private:
    size_t vmemory[1024];
    size_t start;
    static size_t counterId;
    size_t id{0};
};

// Initialize the static counter
size_t testThread::counterId = 0;

/** 
 * @brief Test producer thread
*/
class testProducerThread : public atomicx::thread
{
public:
    testProducerThread(size_t start) : thread(VMEM(vmemory)), start(start)
    {
        id = counterId++;
        std::cout << "Thread " << id << " created" << std::endl;
        setNice(100);
    }

    ~testProducerThread() override
    {
    }

protected:
    void printProcess()
    {
        std::cout << "Producer Thread is running" <<  ", CTX:" << &(atomicx::ctx) << std::endl;
        
        for(auto* thread = begin(); thread != nullptr; thread = (*thread)++)
        {
            auto metrics = thread->getMetrics();
            std::cout << "     Thread " << thread << " status: " << (size_t) metrics.state <<  ", size: " << metrics.stackSize << "/" << metrics.maxStackSize << std::endl;
        }

        yield(0, atomicx::state::NOW);
    }       

    bool run() override
    {
        std::cout << "Producer Thread is running" <<  ", CTX:" << &(atomicx::ctx) << std::endl;
        size_t nCount=start;
        
        setNice(1000 * (id+1));

        while(yield())
        {
            printProcess();

            notify(tranporVar, atomicx::Notify::ONE, {id, nCount}, 0, 1);

            auto metrics = getMetrics();
            for(size_t i=0; i<1000; i++)
            {
                auto metrics_ = getMetrics();
                metrics = metrics_;
            }

            std::cout << "Producer Thread " << id << ": Thread is running " << nCount++ << ", size: " << metrics.stackSize << "/" << metrics.maxStackSize << std::endl;
        }

        std::cout << "Thread " << id << " stopped" << std::endl;

        exit(1);

        return true;
    }

    bool StackOverflow() override
    {
        return false;
    }

private:
    size_t vmemory[1024];
    size_t start;
    static size_t counterId;
    size_t id{0};
};

// Initialize the static counter
size_t testProducerThread::counterId = 0;

int main() 
{
    testThread test1(0);

    // Test the thread pool deletion
    {
        testThread test4(4000);
        testThread test5(5000);
        testThread test6(6000);
    }

    testThread test7(7000);
    testThread test8(8000);

    testProducerThread test9(100000);

    for(auto* thread = test1.begin(); thread != nullptr; thread = (*thread)++)
    {
        std::cout << "Thread " << thread << " is in the thread pool" << std::endl;
    }
    
    atomicx::ctx.start();

    return 0;
}

#endif
