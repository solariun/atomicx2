#include "atomicx.h"

#ifndef ARDUINO
#include <iostream>
#include <memory>
#else
#include <Arduino.h>
#endif

#include <stddef.h>

#include <stdio.h>
#include <unistd.h>

#include <string.h>
#include <stdint.h>
#include <setjmp.h>

#include <stdlib.h>

namespace atomicx {

    void Context::setNextActiveThread()
    {
        Thread* thread = nullptr;

        thread = m_nextThread = m_activeThread;
        
        for(size_t nCount = 0; nCount < threadCount; nCount++)
        {
            thread = (thread == nullptr) ? begin : thread->next;

            if (thread != nullptr)
            {
            switch (thread->threadState)
            {
            case state::READY:
                m_nextThread = thread;
                return;
                
            case state::SLEEPING:
                if (m_nextThread->metrix.nextExecTime >= thread->metrix.nextExecTime)
                    m_nextThread = thread;
                break;

            default:
                break;
            }
            }
        }
    }

    void Context::sleepUntilTick(atomicx_time until)
    {
        atomicx_time now = getTick();

        if (now < until)
        {
            sleepTicks(until - now);
        }
    }

    int Context::start()
    {
        m_running = true;

        m_activeThread = begin;

        while(m_running && threadCount > 0)
        {
            // for debugging
            //m_activeThread = m_activeThread == nullptr ? begin : m_activeThread->next;

            if (m_activeThread != nullptr)
            {
                uint8_t kernelPointer = 0xAA;
                m_activeThread->stack.kernelPointer = &kernelPointer;

                if (setjmp(m_activeThread->kernelRegs) == 0)
                {

                    if (m_activeThread->threadState == state::READY)
                    {
                        m_activeThread->threadState = state::RUNNING;
                        m_activeThread->run();
                        m_activeThread->threadState = state::STOPPED;
                    } else {
                        longjmp(m_activeThread->userRegs, 1);
                    }
                }
            }
            
            setNextActiveThread();
            m_activeThread = m_nextThread;
        }

        return 0;
    }

    void Context::AddThread(Thread* thread)
    {
        if (begin == nullptr)
        {
            begin = thread;
            last = thread;
        } else {
            last->next = thread;
            thread->prev = last;
            last = thread;
        }
        threadCount++;
    }

    void Context::RemoveThread(Thread* thread)
    {
        if (thread == begin)
        {
            begin = thread->next;
        }
        else if (thread == last)
        {
            last = thread->prev;
        }
        else {
            if (thread->prev != nullptr)
                thread->prev->next = thread->next;
            if (thread->next != nullptr)
                thread->next->prev = thread->prev;
        }
        threadCount--;
    }

    bool Context::yield(size_t arg, yieldCmd cmd)
    {
        (void)cmd; (void)arg;
        uint8_t stackPointer = 0xBB;
        
        // Calculate stack size and store are stack.size
        m_activeThread->stack.userPointer = &stackPointer;
        m_activeThread->metrix.stackSize = (size_t)(m_activeThread->stack.kernelPointer - m_activeThread->stack.userPointer);

        if (m_activeThread->metrix.stackSize > m_activeThread->metrix.maxStackSize)
        {
            // Call the user defined StackOverflow function
            m_activeThread->StackOverflow();
            return false;
        }

        if (setjmp(m_activeThread->userRegs) == 0)
        {   
            m_activeThread->metrix.nextExecTime = getTick() + m_activeThread->metrix.nice;
            //setNextActiveThread();

            memcpy(m_activeThread->stack.vmemory, m_activeThread->stack.userPointer, m_activeThread->metrix.stackSize);
            m_activeThread->threadState = state::SLEEPING;
            longjmp(m_activeThread->kernelRegs, 1);
        }

        memcpy(m_activeThread->stack.userPointer, m_activeThread->stack.vmemory, m_activeThread->metrix.stackSize);
        
        m_activeThread->threadState = state::READY;
        sleepUntilTick(m_activeThread->metrix.nextExecTime);        
        m_activeThread->metrix.nextExecTime = getTick();

        return true;
    }

    // Thread Class Implementation

    bool Thread::yield()
    {
        return _ctx.yield();
    }

}; // namespace atomicx 
