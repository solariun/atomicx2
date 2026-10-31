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

ax::thread_context myThreadContext;

class MyThreadItem : public ax::thread_item
{
    public:
        MyThreadItem(ax::thread_context& parent, size_t stackSize, ax::time nice, size_t counter) : thread_item(parent, stackSize), _counter(counter) { setNice(nice); }

        void run() override
        {
            while (true) 
            {
                (void)_counter;
                
                //std::cout << "Thread item at address: " << this << " with counter: " << _counter++ << " Nice: " << getNice() << std::endl;
                yield();
            }
        }
    private:
        size_t _counter;
};

class MySecThreadItem : public ax::thread_item
{
    public:
        MySecThreadItem(ax::thread_context& parent, size_t stackSize, ax::time nice, size_t counter) : thread_item(parent, stackSize), _counter(counter) { setNice(nice); }


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
            std::cout << "Thread item at address: " << this << " with counter: " << _counter++ << " Nice: " << getNice() << std::endl;
            yield();
        }

        size_t _counter;
};


int main() 
{
    MyThreadItem myThreadItem_1(myThreadContext, 500, 500, 1000);
    MyThreadItem myThreadItem_2(myThreadContext, 500, 1000, 2000);
    MyThreadItem myThreadItem_3(myThreadContext, 500, 2000, 3000);
    MyThreadItem myThreadItem_4(myThreadContext, 500, 5000, 4000);

    MySecThreadItem mySecThreadItem_1(myThreadContext, 500, 500, 5000);
    MySecThreadItem mySecThreadItem_2(myThreadContext, 500, 1000, 6000);
    MySecThreadItem mySecThreadItem_3(myThreadContext, 500, 2000, 7000);
    MySecThreadItem mySecThreadItem_4(myThreadContext, 500, 5000, 8000);

    for(ax::auto_obj_list::item* current = myThreadContext.getHeadItem(); current; current = current->getNextItem(current))
    {
        std::cout << "Thread item at address: " << current << std::endl;
    }

    myThreadContext.start();
    
    std::cout << "Main thread finished." << std::endl;
    
    return 0;
}

#endif
