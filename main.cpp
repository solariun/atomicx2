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


//User defined thread context
static atomicx::Context localCtx;

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

class contextThread : public atomicx::Thread
{
public:
    template<size_t stackSize>
    contextThread(size_t (&vmemory)[stackSize]) : Thread(vmemory, localCtx)
    {
    }

    ~contextThread() override
    {
    }

protected:
    virtual bool run() override = 0;
    virtual bool StackOverflow() override = 0;
};

class testThread : public atomicx::Thread
{
public:
    testThread(size_t start) : Thread(VMEM(vmemory), localCtx), start(start)
    {
        id = counterId++;
        std::cout << "Thread " << id << " created" << std::endl;
        setNice(500);
    }

    ~testThread()
    {
    }

protected:
    bool run() override
    {
        std::cout << "Thread is running" <<  ", CTX:" << &localCtx << std::endl;
        size_t nCount=start;
        
        setNice(1000 * (id+1));

        while(yieldUntil(100))
        {
            auto metrics = getMetrix();
            std::cout << "Thread " << id << ": Thread is running " << nCount++ << ", size: " << metrics.stackSize << "/" << metrics.maxStackSize << ", Thread: " << sizeof(atomicx::Thread) << ", Context: " << sizeof(localCtx) << std::endl;
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

int main() 
{
    testThread test1(0);
    testThread test2(2000);
    testThread test3(3000);

    // Test the thread pool deletion
    {
        testThread test4(4000);
        testThread test5(5000);
        testThread test6(6000);
    }

    testThread test7(7000);
    testThread test8(8000);


    for(atomicx::Thread* thread = test1.begin(); thread != nullptr; thread = (*thread)++)
    {
        std::cout << "Thread " << thread << " is in the thread pool" << std::endl;
    }
    
    localCtx.start();
    return 0;
}

#endif
