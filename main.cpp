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

void ax::sleepTicks(ax::time nSleep)
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
        MyThreadItem(ax::thread_context& parent, size_t stackSize) : thread_item(parent, stackSize) {}

        void run() override
        {
            std::cout << "Running thread item at address: " << this << std::endl;
        }
};

int main() 
{
    MyThreadItem myThreadItem_1(myThreadContext, 250);
    MyThreadItem myThreadItem_2(myThreadContext, 250);
    MyThreadItem myThreadItem_3(myThreadContext, 250);
    MyThreadItem myThreadItem_4(myThreadContext, 250);

    for(ax::auto_obj_list::item* current = myThreadContext.getHeadItem(); current; current = current->getNextItem(current))
    {
        std::cout << "Thread item at address: " << current << std::endl;
    }

    return 0;
}

#endif
