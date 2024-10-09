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

ax::Time ax::getTick (void)
{
#ifndef FAKE_TIMER
    usleep (10000); // 10ms slow dow to simulate a real system
    struct timeval tp;
    gettimeofday (&tp, NULL);

    return (Time)tp.tv_sec * 1000 + tp.tv_usec / 1000;
#else
    nCounter++;

    return nCounter;
#endif
}

void ax::sleepTicks(ax::Time nSleep)
{
#ifndef FAKE_TIMER
    usleep ((useconds_t)nSleep * 1000);
#else
    while (nSleep); usleep(100);
#endif
}

ax::RefId tranporVar=0;

/**
 * @brief Test consumer thread
 */
class testThread : public ax::thread
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
        std::cout << "Thread is running" <<  ", CTX:" << &ax::ctx << std::endl;
        size_t nCount=start;
        
        setNice(1000 * (id+1));
        ax::Tag tag;

        //while(yield())
        while(yield(10))
        {
            std::cout << "Thread " << id << ": Thread is WAITING " << std::endl;
            while (!wait(tranporVar, tag, 1000, 1)) std::cout << id << ": wait timedout..." << std::endl;

            auto metrics = getMetrics();
            std::cout << "Thread " << id << ": Thread is running " << nCount++ << ", size: " << metrics.stackSize << "/" << metrics.maxStackSize << ", Thread: " << sizeof(ax::thread) << ", Context: " << sizeof(ax::ctx ) << ", Tag: " << tag.param << "/" << tag.value << ", state: " << (size_t) metrics.state << std::endl;
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
class testProducerThread : public ax::thread
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
        std::cout << "Producer Thread is running" <<  ", CTX:" << &(ax::ctx) << std::endl;
        
        for(auto* thread = begin(); thread != nullptr; thread = (*thread)++)
        {
            auto metrics = thread->getMetrics();
            std::cout << "     Thread " << thread << " status: " << (size_t) metrics.state <<  ", size: " << metrics.stackSize << "/" << metrics.maxStackSize << std::endl;
        }

        yield(0, ax::STATE::NOW);
    }       

    bool run() override
    {
        std::cout << "Producer Thread is running" <<  ", CTX:" << &(ax::ctx) << std::endl;
        size_t nCount=start;
        size_t ret=0;

        (void) start; (void) nCount; (void) ret;

        setNice(1000 * (id+1));

        while(true)
        {
            printProcess();

            if(!(ret = notify(tranporVar, ax::Notify::ONE, {600, nCount}, ax::TIME::UNDERFINED, 1)))
            {
                std::cout << "PRODUCER Thread " << id << ": notify failed" << std::endl;
            }

            auto metrics = getMetrics();
            for(size_t i=0; i<1000; i++)
            {
                auto metrics_ = getMetrics();
                metrics = metrics_;
            }

            std::cout << "Producer Thread " << id << ": Thread is running " << nCount++ << ", size: " << metrics.stackSize << "/" << metrics.maxStackSize << ", ret: " << ret << std::endl;
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
    
    ax::ctx.start();

    return 0;
}

#endif
