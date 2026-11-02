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

ax::time ax::getTick (void)
{
#ifndef FAKE_TIMER
    usleep (10000); // 10ms slow dow to simulate a real system
    struct timeval tp;
    gettimeofday (&tp, NULL);

    return (time)tp.tv_sec * 1000 + tp.tv_usec / 1000;
#else
    nCounter++;

    return nCounter;
#endif
}

void ax::sleepTicks(ax::time  nSleep)
{
#ifndef FAKE_TIMER
    usleep ((useconds_t)nSleep * 1000);
#else
    while (nSleep); usleep(100);
#endif
}

class MyThreadItem : public ax::thread_item
{
    public:
        MyThreadItem(size_t stackSize, ax::time nice, size_t counter) : thread_item(stackSize), _counter(counter) 
        { 
            setNice(nice);
            setName("MyThreadItem");
        }

        void run() override
        {
            while (true) 
            {
                (void)_counter;
                
                std::cout << "Thread item at address: " << this << " with counter: " << _counter++ << " Nice: " << getNice() << std::endl;
                yield();
            }
        }
    private:
        size_t _counter;
};

class MySecThreadItem : public ax::thread_item
{
    public:
        MySecThreadItem(size_t stackSize, ax::time nice, size_t counter) : thread_item(stackSize), _counter(counter) 
        { 
            setNice(nice); 
            setName("MySecThreadItem");
        }


        void run() override
        {
            while (true) 
            {
                dump();
            }
        }
    private:

        void dump()
        {
            std::cout << "Thread item at address: " << this << " with counter: " << _counter++ << " Nice: " << getNice() << std::endl << std::flush;
            yield();
        }

        size_t _counter;
};

ax::ref someReference;

class producer : public ax::thread_item
{
    public:
        producer(size_t stackSize, ax::time nice, size_t counter) : thread_item(stackSize), _counter(counter) 
        { 
            setNice(nice); 
            setName("ProducerThread");
        }

    private:  

        void run() override
        {
            while (true)
            {
                std::cout << "Producer " << getName() << " is about to notify with counter: " << _counter << std::endl << std::flush;
                notify(someReference, {1, _counter++});
                std::cout << "Producer " << getName() << " notified with counter: " << _counter << std::endl << std::flush;
                yield();
            }
        }
    private:
        size_t _counter;
};

class consumer : public ax::thread_item
{
    public:
        consumer(size_t stackSize, ax::time nice, size_t counter) : thread_item(stackSize), _counter(counter) 
        { 
            setNice(nice);
            setName("ConsumerThread");
        }

        void run() override
        {
            while (true)
            {
                std::cout << "[CONSUMER] " << getName() << " is about to wait with counter: " << _counter << std::endl << std::flush;
                auto res = wait(someReference, _someMessage, 1); // Example usage

                std::cout << "[CONSUMER] thread at address: " << this \
                << " with counter: " << _counter++ \
                << ", ret: " << res() \
                << ", message: " << _someMessage.attribute << "/" << _someMessage.value \
                << std::endl;

                _counter++;

                yield();
            }
        }
    private:
        size_t _counter;
        ax::message _someMessage;
};

int main() 
{
    MyThreadItem myThreadItem_1(500, 500, 1000);
    // MyThreadItem myThreadItem_2(500, 1000, 2000);
    // MyThreadItem myThreadItem_3(500, 2000, 3000);
    // MyThreadItem myThreadItem_4(500, 5000, 4000);

    MySecThreadItem mySecThreadItem_1(500, 200, 5000);
    // MySecThreadItem mySecThreadItem_2(500, 1000, 6000);
    // MySecThreadItem mySecThreadItem_3(500, 2000, 7000);
    // MySecThreadItem mySecThreadItem_4(500, 5000, 8000);

    // consumer myConsumer(1000, 100, 0);

    // producer myProducer(1000, 100, 0);
    // producer myProducer2(500, 100, 1000);
    // producer myProducer3(500, 100, 2000);
    // producer myProducer4(500, 100, 3000);

    for(ax::auto_obj_list::item* current = ax::context.getHeadItem(); current; current = current->getNextItem(current))
    {
        std::cout << "Thread item at address: " << current << std::endl;
    }

    ax::context.start();
    
    std::cout << "Main thread finished." << std::endl;
    
    return 0;
}

#endif
